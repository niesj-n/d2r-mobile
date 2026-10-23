#include "diagnostics_android.h"
#include "perf_android.h"
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include <math.h>

/* Each category retains evidence independently. Producers never wait for an exporter
 * or another producer. No allocation, file access, Vulkan call or guest read here. */
enum { PIPE_LINE = 768, PIPE_ROWS = 64 };
typedef struct pipeline_ring {
    char lines[PIPE_ROWS][PIPE_LINE];
    uint64_t sequence[PIPE_ROWS], written;
} pipeline_ring;
static pipeline_ring pipeline_rings[3];
/* Namespace and startup acceptance must survive an arbitrary number of saves. */
static char pipeline_identity[3][PIPE_LINE];
static uint64_t pipeline_identity_sequence[3];
static atomic_flag pipeline_lock = ATOMIC_FLAG_INIT;
static atomic_uint pipeline_dropped;
static uint64_t pipeline_sequence;
static int pipeline_category(const char *s) {
    if (!s) return -1;
    if (!strncmp(s, "NVN pipeline cache namespace=", 29)) return 3;
    if (!strncmp(s, "NVN pipeline cache load ", 24)) return 4;
    if (!strncmp(s, "NVN warm list=", 14)) return 5;
    if (!strncmp(s, "NVN pipeline cache ", 19) || !strncmp(s, "NVN warm list", 13)) return 0;
    if (!strncmp(s, "NVN shader ", 11))
        for (size_t i = 11; i < 300 && s[i]; ++i) if (!strncmp(s + i, " loaded words=", 14)) return 1;
    if (!strncmp(s, "NVN slow-pso ", 13)) return 2;
    return -1;
}
void dm_android_diagnostics_pipeline_log(const char *text) {
    int category = pipeline_category(text);
    if (category < 0) return;
    if (atomic_flag_test_and_set_explicit(&pipeline_lock, memory_order_acquire)) {
        atomic_fetch_add_explicit(&pipeline_dropped, 1, memory_order_relaxed); return;
    }
    char *line; uint64_t *sequence;
    if (category >= 3) { line = pipeline_identity[category - 3]; sequence = &pipeline_identity_sequence[category - 3]; }
    else {
        pipeline_ring *r = &pipeline_rings[category]; size_t slot = r->written++ % PIPE_ROWS;
        line = r->lines[slot]; sequence = &r->sequence[slot];
    }
    /* Exported evidence is deliberately ASCII: this also prevents split UTF-8,
     * control-line injection and untrusted paths/strings becoming binary output. */
    size_t i = 0;
    for (; i + 1 < PIPE_LINE && text[i]; ++i) {
        unsigned char c = (unsigned char)text[i]; line[i] = c >= 32 && c < 127 ? (char)c : '_';
    }
    line[i] = 0; *sequence = ++pipeline_sequence;
    atomic_flag_clear_explicit(&pipeline_lock, memory_order_release);
}
size_t dm_android_diagnostics_pipeline_copy(char *out, size_t cap) {
    if (!out || !cap) return 0;
    size_t used = 0; out[0] = 0;
    while (atomic_flag_test_and_set_explicit(&pipeline_lock, memory_order_acquire)) {}
    int n = snprintf(out, cap, "pipeline_diagnostics=1\nsource=bounded process-memory dm-vulkan evidence; grouped categories; seq is process order\n"
        "slow_policy=create >=20ms; max128 events/process; warm snapshot before synchronous create; diagnostics are not a performance fix\n"
        "warm_listed=-1 means unknown/not applicable; startup index capped4096; pending is not proof of building\n"
        "limits=64 rows/category plus3 pinned startup rows,767 ASCII bytes/row; no shader/cache binary or save data\n"
        "contention_dropped=%u\n", atomic_load_explicit(&pipeline_dropped, memory_order_relaxed));
    used = n < 0 ? 0 : (size_t)n < cap ? (size_t)n : cap - 1;
    for (unsigned i = 0; i < 3 && used + 1 < cap; ++i) if (pipeline_identity_sequence[i]) {
        n = snprintf(out + used, cap - used, "seq=%llu pinned=1 %s\n", (unsigned long long)pipeline_identity_sequence[i], pipeline_identity[i]);
        used += n < 0 ? 0 : (size_t)n < cap - used ? (size_t)n : cap - used - 1;
    }
    for (unsigned c = 0; c < 3 && used + 1 < cap; ++c) {
        pipeline_ring *r = &pipeline_rings[c];
        n = snprintf(out + used, cap - used, "category=%s total=%llu overwritten=%llu\n", c == 0 ? "cache" : c == 1 ? "shader" : "slow-pso",
            (unsigned long long)r->written, (unsigned long long)(r->written > PIPE_ROWS ? r->written - PIPE_ROWS : 0));
        used += n < 0 ? 0 : (size_t)n < cap - used ? (size_t)n : cap - used - 1;
        uint64_t first = r->written > PIPE_ROWS ? r->written - PIPE_ROWS : 0;
        for (uint64_t i = first; i < r->written && used + 1 < cap; ++i) {
            size_t slot = i % PIPE_ROWS;
            n = snprintf(out + used, cap - used, "seq=%llu %s\n", (unsigned long long)r->sequence[slot], r->lines[slot]);
            used += n < 0 ? 0 : (size_t)n < cap - used ? (size_t)n : cap - used - 1;
        }
    }
    atomic_flag_clear_explicit(&pipeline_lock, memory_order_release);
    return used;
}

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
