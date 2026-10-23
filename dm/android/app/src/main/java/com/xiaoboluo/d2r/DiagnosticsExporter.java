package com.xiaoboluo.d2r;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.provider.DocumentsContract;
import android.widget.Toast;
import java.io.File;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/** Local save only. Menu/JNI/UI never reads logs or writes a provider stream. */
final class DiagnosticsExporter {
    private static final int CREATE_DIAGNOSTICS = 0xD205;
    private final Activity activity;
    private boolean busy;
    private String pendingMetadata;
    private String pendingPipeline;
    private File pendingRuntime;
    private static native String nativePipelineDiagnostics();

    DiagnosticsExporter(Activity activity) { this.activity = activity; }

    void begin(String runtime, String nativeMetadata) {
        if (busy) { Toast.makeText(activity, R.string.dmAndroidDiagnosticsBusy, Toast.LENGTH_SHORT).show(); return; }
        busy = true;
        File files = activity.getFilesDir();
        new Thread(() -> {
            String pipeline = nativePipelineDiagnostics();
            DriverPackageStore.DriverInfo selected = DriverPackageStore.selected(files);
            String metadata = "D2R Mobile Android Diagnostics\nexport_time=" + new Date() +
                "\napplication_id=" + BuildConfig.APPLICATION_ID + "\nversion=" + BuildConfig.VERSION_NAME +
                "\nversion_code=" + BuildConfig.VERSION_CODE + "\nbuild_type=" + BuildConfig.BUILD_TYPE +
                "\nproduct=" + BuildConfig.DM_PRODUCT + "\nmanufacturer=" + Build.MANUFACTURER +
                "\nmodel=" + Build.MODEL + "\nandroid_release=" + Build.VERSION.RELEASE +
                "\nandroid_sdk=" + Build.VERSION.SDK_INT +
                "\nselected_driver=" + (selected == null ? "system" : "custom") +
                "\nselected_driver_digest=" + (selected == null ? "none" : selected.digest) +
                "\nselected_driver_library=" + (selected == null ? "system" : selected.libraryName) +
                "\nselection_note=selection applies on next restart; loaded_driver is current process\n" + nativeMetadata;
            activity.runOnUiThread(() -> {
                if (activity.isFinishing() || activity.isDestroyed()) { busy = false; return; }
                pendingRuntime = new File(runtime); pendingMetadata = metadata; pendingPipeline = pipeline;
                Intent picker = new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE)
                    .setType("application/zip").putExtra(Intent.EXTRA_TITLE, "D2RMobile_diagnostics_" +
                        new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.ROOT).format(new Date()) + ".zip");
                try { activity.startActivityForResult(picker, CREATE_DIAGNOSTICS); }
                catch (RuntimeException e) { clear(); showFailure(e); }
            });
        }, "dm-diagnostics-prepare").start();
    }

    private void clear() { busy = false; pendingMetadata = null; pendingPipeline = null; pendingRuntime = null; }

    boolean onActivityResult(int request, int result, Intent data) {
        if (request != CREATE_DIAGNOSTICS) return false;
        if (result != Activity.RESULT_OK || data == null || data.getData() == null) {
            clear(); Toast.makeText(activity, R.string.dmAndroidDiagnosticsCancelled, Toast.LENGTH_SHORT).show(); return true;
        }
        Uri destination = data.getData();
        String metadata = pendingMetadata; String pipeline = pendingPipeline; File runtime = pendingRuntime;
        if (metadata == null || runtime == null) {
            clear(); showFailure(new IllegalStateException(activity.getString(R.string.dmAndroidDiagnosticsRequestLost))); return true;
        }
        File files = activity.getFilesDir();
        new Thread(() -> {
            try {
                OutputStream stream = activity.getContentResolver().openOutputStream(destination, "wt");
                DiagnosticsArchive.write(files, runtime, metadata, pipeline, stream);
                activity.runOnUiThread(() -> {
                    clear();
                    if (!activity.isFinishing() && !activity.isDestroyed()) new AlertDialog.Builder(activity)
                        .setTitle(R.string.dmAndroidDiagnosticsTitle)
                        .setMessage(activity.getString(R.string.dmAndroidDiagnosticsSaved, destination.toString()))
                        .setPositiveButton(android.R.string.ok, null).show();
                });
            } catch (Exception e) {
                // Some providers leave a partial document after failure. Best-effort remove only this chosen document.
                try { DocumentsContract.deleteDocument(activity.getContentResolver(), destination); } catch (Exception ignored) { }
                activity.runOnUiThread(() -> { clear(); showFailure(e); });
            }
        }, "dm-diagnostics-save").start();
        return true;
    }

    private void showFailure(Exception e) {
        if (activity.isFinishing() || activity.isDestroyed()) return;
        String reason = e.getMessage(); if (reason == null || reason.isEmpty()) reason = e.getClass().getSimpleName();
        new AlertDialog.Builder(activity).setTitle(R.string.dmAndroidDiagnosticsTitle)
            .setMessage(activity.getString(R.string.dmAndroidDiagnosticsFailed, reason))
            .setPositiveButton(android.R.string.ok, null).show();
    }
}
