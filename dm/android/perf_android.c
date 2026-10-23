#include "perf_android.h"
#include <math.h>
#include <stdatomic.h>

/* The present thread owns the window. UI/JNI only reads atomic published values. */
static _Atomic int monitoring;
static _Atomic unsigned session;
static _Atomic unsigned publish_seq;
static _Atomic unsigned published_session;
static _Atomic uint64_t published_at;
static _Atomic int published_fps_100, published_frame_us;
static _Atomic int published_gpu_us, published_scale_1000;
static _Atomic unsigned dimensions_seq;
static _Atomic uint64_t target_dimensions, output_dimensions;

extern double xbg_recent_gpu_ms(void);
extern float dm_3d_scale_cur(void);

void dm_android_perf_set_enabled(int enabled) {
    enabled = !!enabled;
    if (atomic_exchange(&monitoring, enabled) != enabled)
        atomic_fetch_add(&session, 1);
}

int dm_android_perf_enabled(void) { return atomic_load(&monitoring); }

void dm_android_perf_feed(double iv_ms, uint64_t now_ns) {
    static unsigned local_session, count;
    static uint64_t begin = UINT64_MAX;
    static double interval_sum;
    if (!dm_android_perf_enabled()) return;
    unsigned current = atomic_load(&session);
    if (current != local_session) {
        local_session = current;
        begin = UINT64_MAX;
        count = 0;
        interval_sum = 0;
    }
    int positive = isfinite(iv_ms) && iv_ms > 0;
    if (begin == UINT64_MAX) {
        uint64_t iv_ns = positive && iv_ms < 1000.0 ? (uint64_t)(iv_ms * 1000000.0) : 0;
        begin = now_ns >= iv_ns ? now_ns - iv_ns : now_ns;
    }
    if (positive) { ++count; interval_sum += iv_ms; }
    if (now_ns <= begin || now_ns - begin < 500000000ull) return;
    double seconds = (now_ns - begin) / 1e9;
    double gpu = count ? xbg_recent_gpu_ms() : -1;
    double scale = count ? dm_3d_scale_cur() : -1;
    atomic_fetch_add(&publish_seq, 1);
    atomic_store(&published_session, current);
    atomic_store(&published_fps_100, count ? (int)lround(count / seconds * 100.0) : -1);
    atomic_store(&published_frame_us, count ? (int)lround(interval_sum / count * 1000.0) : -1);
    atomic_store(&published_gpu_us, isfinite(gpu) && gpu > 0 ? (int)lround(gpu * 1000.0) : -1);
    atomic_store(&published_scale_1000, isfinite(scale) && scale > 0 ? (int)lround(scale * 1000.0) : -1);
    atomic_store(&published_at, now_ns);
    atomic_fetch_add(&publish_seq, 1);
    begin = now_ns;
    count = 0;
    interval_sum = 0;
}

void dm_android_perf_dimensions(unsigned tw, unsigned th, unsigned ow, unsigned oh) {
    atomic_fetch_add(&dimensions_seq, 1);
    atomic_store(&target_dimensions, ((uint64_t)tw << 32) | th);
    atomic_store(&output_dimensions, ((uint64_t)ow << 32) | oh);
    atomic_fetch_add(&dimensions_seq, 1);
}

void dm_android_perf_read(uint64_t now_ns, dm_android_perf_snapshot *out) {
    if (!out) return;
    out->valid = 0; out->fps = out->frame_ms = out->gpu_ms = out->scale = -1;
    uint64_t target, output;
    unsigned a, b;
    for (;;) {
        a = atomic_load(&dimensions_seq);
        if (a & 1) continue;
        target = atomic_load(&target_dimensions);
        output = atomic_load(&output_dimensions);
        b = atomic_load(&dimensions_seq);
        if (a == b) break;
    }
    out->target_width = (unsigned)(target >> 32); out->target_height = (unsigned)target;
    out->output_width = (unsigned)(output >> 32); out->output_height = (unsigned)output;
    if (!dm_android_perf_enabled()) return;
    unsigned published;
    uint64_t at;
    int fps, frame, gpu, scale;
    for (;;) {
        a = atomic_load(&publish_seq);
        if (a & 1) continue;
        published = atomic_load(&published_session);
        at = atomic_load(&published_at);
        fps = atomic_load(&published_fps_100);
        frame = atomic_load(&published_frame_us);
        gpu = atomic_load(&published_gpu_us);
        scale = atomic_load(&published_scale_1000);
        b = atomic_load(&publish_seq);
        if (a == b) break;
    }
    if (!at || published != atomic_load(&session) || now_ns < at || now_ns - at > 2000000000ull || fps < 0) return;
    out->valid = 1;
    out->fps = fps / 100.0;
    out->frame_ms = frame / 1000.0;
    out->gpu_ms = gpu < 0 ? -1 : gpu / 1000.0;
    out->scale = scale < 0 ? -1 : scale / 1000.0;
}
