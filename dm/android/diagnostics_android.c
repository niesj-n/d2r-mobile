#include "diagnostics_android.h"
#include "perf_android.h"
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <math.h>

/* Startup writes only. Snapshot copies while locked, formatting and perf reads after unlock. */
static atomic_flag cache_lock = ATOMIC_FLAG_INIT;
static char runtime[1200], overlay[80] = "unknown", revision[100] = "unknown";
static struct gpu_cache { char name[256], digest[65], library[81]; uint32_t api, driver; int known, custom; } gpu;
static void lock(void) { while (atomic_flag_test_and_set_explicit(&cache_lock, memory_order_acquire)) {} }
static void unlock(void) { atomic_flag_clear_explicit(&cache_lock, memory_order_release); }
static void copy(char *out, size_t cap, const char *in) {
    snprintf(out, cap, "%s", in && *in ? in : "unknown");
    for (char *p = out; *p; ++p) if (*p == '\n' || *p == '\r') *p = ' ';
}
void dm_android_diagnostics_configure(const char *r, const char *o, const char *v) {
    /* Runtime directory is immutable before any guest/export request can run. */
    copy(runtime, sizeof runtime, r);
    lock(); copy(overlay, sizeof overlay, o); copy(revision, sizeof revision, v); unlock();
}
const char *dm_android_diagnostics_runtime(void) { return runtime; }
void dm_android_diagnostics_gpu(const char *n, uint32_t a, uint32_t d, int c, const char *s, const char *l) {
    lock(); copy(gpu.name, sizeof gpu.name, n); copy(gpu.digest, sizeof gpu.digest, c ? s : "none");
    copy(gpu.library, sizeof gpu.library, c ? l : "system");
    gpu.api = a; gpu.driver = d; gpu.custom = !!c; gpu.known = 1; unlock();
}
static void metric(char *out, size_t cap, int valid, double value) {
    if (valid && isfinite(value) && value >= 0) snprintf(out, cap, "%.3f", value);
    else snprintf(out, cap, "unknown");
}
void dm_android_diagnostics_snapshot(char *out, size_t cap, uint64_t now) {
    struct gpu_cache cached; char fp[80], rev[100];
    lock(); cached = gpu; memcpy(fp, overlay, sizeof fp); memcpy(rev, revision, sizeof rev); unlock();
    dm_android_perf_snapshot sample; dm_android_perf_read(now, &sample);
    char fps[40], frame[40], gpu_ms[40], scale[40], api[40], driver[40], target[40], output[40];
    metric(fps, sizeof fps, sample.valid, sample.fps); metric(frame, sizeof frame, sample.valid, sample.frame_ms);
    metric(gpu_ms, sizeof gpu_ms, sample.valid, sample.gpu_ms); metric(scale, sizeof scale, sample.valid, sample.scale);
    if (cached.known) {
        snprintf(api, sizeof api, "%u.%u.%u", cached.api >> 22, (cached.api >> 12) & 1023, cached.api & 4095);
        snprintf(driver, sizeof driver, "0x%08x", cached.driver);
    } else { snprintf(api, sizeof api, "unknown"); snprintf(driver, sizeof driver, "unknown"); }
    if (sample.target_width && sample.target_height) snprintf(target, sizeof target, "%ux%u", sample.target_width, sample.target_height);
    else snprintf(target, sizeof target, "unknown");
    if (sample.output_width && sample.output_height) snprintf(output, sizeof output, "%ux%u", sample.output_width, sample.output_height);
    else snprintf(output, sizeof output, "unknown");
    snprintf(out, cap,
        "overlay_source_sha256=%s\nsource_revision=%s\n"
        "loaded_driver=%s\nloaded_driver_digest=%s\nloaded_driver_library=%s\n"
        "gpu_name=%s\nvulkan_api=%s\nvulkan_driver_raw=%s\n"
        "performance_monitoring=%s\nperformance_snapshot=%s\n"
        "fps=%s\nframe_ms=%s\ngpu_ms=%s\nscale=%s\n"
        "target_dimensions=%s\noutput_dimensions=%s\n"
        "source_revision_note=git HEAD plus dirty marker; overlay fingerprint describes overlay inputs only\n"
        "metrics_source=existing cached Android perf snapshot; disabled/stale/unavailable=unknown\n"
        "present_interval_source=CPU monotonic time after xbg_present_texture; not measured display scanout\n"
        "gpu_time_source=existing xbg_recent_gpu_ms window aggregate; unavailable if no GPU timestamp samples\n"
        "perf_log_source=existing dm_perf_present interval/submit CPU/scale records; no extra GPU queries\n",
        fp, rev, cached.known ? (cached.custom ? "custom" : "system") : "unknown",
        cached.known ? cached.digest : "unknown", cached.known ? cached.library : "unknown",
        cached.known ? cached.name : "unknown", api, driver,
        dm_android_perf_enabled() ? "enabled" : "disabled", sample.valid ? "valid" : "unavailable",
        fps, frame, gpu_ms, scale, target, output);
}
