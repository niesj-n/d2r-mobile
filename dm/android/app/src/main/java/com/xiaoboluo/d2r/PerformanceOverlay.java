package com.xiaoboluo.d2r;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.BatteryManager;
import android.view.MotionEvent;
import android.view.View;
import java.util.Locale;

/** A passive, optional display of cached host metrics. It never handles game touches. */
public final class PerformanceOverlay extends View {
    private static native void nativeSetMonitoringEnabled(boolean enabled);
    private static native void nativeReadSnapshot(float[] values, int[] dimensions);
    private static final String MISSING = "—";
    private static final long REFRESH_MS = 500;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF panel = new RectF();
    private final float[] values = {-1, -1, -1, -1};
    private final int[] dimensions = new int[4];
    private final float density;
    private final BroadcastReceiver batteryReceiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            if (!Intent.ACTION_BATTERY_CHANGED.equals(intent.getAction())) return;
            int t = intent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, Integer.MIN_VALUE);
            batteryTenths = t == Integer.MIN_VALUE ? Integer.MIN_VALUE : t;
            invalidate();
        }
    };
    private final Runnable refresh = new Runnable() {
        @Override public void run() {
            if (!running) return;
            nativeReadSnapshot(values, dimensions);
            invalidate();
            postDelayed(this, REFRESH_MS);
        }
    };
    private boolean enabled, running;
    private int batteryTenths = Integer.MIN_VALUE;
    private int safeLeft, safeTop, safeRight, safeBottom;

    public PerformanceOverlay(Context context) {
        super(context);
        density = getResources().getDisplayMetrics().density;
        setVisibility(GONE);
        setClickable(false);
        setFocusable(false);
    }

    public void setMonitoringEnabled(boolean enabled) {
        if (this.enabled == enabled) return;
        this.enabled = enabled;
        setVisibility(enabled ? VISIBLE : GONE);
        if (enabled && isAttachedToWindow()) start();
        else if (!enabled) stop();
    }

    public void setSafeInsets(int left, int top, int right, int bottom) {
        safeLeft = Math.max(0, left);
        safeTop = Math.max(0, top);
        safeRight = Math.max(0, right);
        safeBottom = Math.max(0, bottom);
        invalidate();
    }

    @Override protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (enabled) start();
    }

    @Override protected void onDetachedFromWindow() {
        stop();
        super.onDetachedFromWindow();
    }

    private void start() {
        if (running) return;
        running = true;
        nativeSetMonitoringEnabled(true);
        getContext().registerReceiver(batteryReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        post(refresh);
    }

    private void stop() {
        if (!running) return;
        running = false;
        removeCallbacks(refresh);
        getContext().unregisterReceiver(batteryReceiver);
        nativeSetMonitoringEnabled(false);
        for (int i = 0; i < values.length; ++i) values[i] = -1;
    }

    private float dp(float amount) { return amount * density; }

    private String number(float value, String unit, int decimals) {
        if (!Float.isFinite(value) || value < 0) return MISSING;
        return String.format(Locale.getDefault(), "%1$." + decimals + "f%2$s", value, unit);
    }

    private void row(Canvas canvas, float x, float right, float baseline, int label, String value) {
        paint.setTypeface(null);
        paint.setTextSize(dp(11));
        paint.setColor(0xffbdad91);
        canvas.drawText(getContext().getString(label), x, baseline, paint);
        paint.setColor(0xfff3e3be);
        paint.setTextAlign(Paint.Align.RIGHT);
        canvas.drawText(value, right, baseline, paint);
        paint.setTextAlign(Paint.Align.LEFT);
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (!enabled) return;
        float width = dp(284), height = dp(164);
        float right = getWidth() - safeRight - dp(10);
        float top = safeTop + dp(48);
        if (right - width < safeLeft + dp(6)) right = Math.min(getWidth() - dp(4), safeLeft + width + dp(6));
        if (top + height > getHeight() - safeBottom) top = Math.max(safeTop + dp(4), getHeight() - safeBottom - height - dp(4));
        panel.set(right - width, top, right, top + height);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0xe515171b);
        canvas.drawRoundRect(panel, dp(8), dp(8), paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(dp(1));
        paint.setColor(0xb6947649);
        canvas.drawRoundRect(panel, dp(8), dp(8), paint);
        paint.setStyle(Paint.Style.FILL);
        float x = panel.left + dp(12), valueRight = panel.right - dp(12);
        paint.setColor(0xffcaa96b);
        paint.setTextSize(dp(11));
        paint.setFakeBoldText(true);
        canvas.drawText(getContext().getString(R.string.performance_title), x, top + dp(19), paint);
        paint.setFakeBoldText(false);
        float y = top + dp(39), gap = dp(19);
        row(canvas, x, valueRight, y, R.string.performance_fps, number(values[0], "", 1));
        row(canvas, x, valueRight, y += gap, R.string.performance_frame, number(values[1], " ms", 1));
        row(canvas, x, valueRight, y += gap, R.string.performance_gpu, number(values[2], " ms", 1));
        row(canvas, x, valueRight, y += gap, R.string.performance_scale, number(values[3] * 100, "%", 0));
        String size = dimensions[0] > 0 && dimensions[1] > 0 && dimensions[2] > 0 && dimensions[3] > 0
                ? dimensions[0] + "×" + dimensions[1] + " / " + dimensions[2] + "×" + dimensions[3] : MISSING;
        row(canvas, x, valueRight, y += gap, R.string.performance_dimensions, size);
        String battery = batteryTenths == Integer.MIN_VALUE ? MISSING
                : String.format(Locale.getDefault(), "%.1f °C", batteryTenths / 10.0);
        row(canvas, x, valueRight, y += gap, R.string.performance_battery, battery);
    }

    @Override public boolean onTouchEvent(MotionEvent event) { return false; }
}
