#ifdef NDEBUG
#error Performance monitor tests require active assertions
#endif
#include "../perf_android.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static double gpu = -1;
static float scale = 0.75f;
double xbg_recent_gpu_ms(void) { return gpu; }
float dm_3d_scale_cur(void) { return scale; }

static int near(double a, double b) { return fabs(a - b) < 0.05; }

int main(void) {
    dm_android_perf_snapshot s;
    dm_android_perf_dimensions(1440, 810, 2400, 1080);
    dm_android_perf_read(0, &s);
    assert(!s.valid && s.fps < 0 && s.gpu_ms < 0);

    dm_android_perf_set_enabled(1);
    dm_android_perf_read(0, &s);
    assert(!s.valid && s.target_width == 1440 && s.output_width == 2400);
    gpu = 10.5;
    for (int i = 1; i <= 30; ++i)
        dm_android_perf_feed(16.667, (uint64_t)i * 16667000ull);
    dm_android_perf_read(500010000ull, &s);
    assert(s.valid && near(s.fps, 60.0) && near(s.frame_ms, 16.667));
    assert(near(s.gpu_ms, 10.5) && near(s.scale, 0.75));
    assert(s.target_width == 1440 && s.target_height == 810);
    assert(s.output_width == 2400 && s.output_height == 1080);

    dm_android_perf_read(3000000000ull, &s);
    assert(!s.valid && s.fps < 0 && s.frame_ms < 0 && s.gpu_ms < 0);
    dm_android_perf_set_enabled(0);
    dm_android_perf_read(500010000ull, &s);
    assert(!s.valid);

    dm_android_perf_set_enabled(1);
    dm_android_perf_read(500010000ull, &s);
    assert(!s.valid); /* old enabled session is never shown */
    for (int i = 1; i <= 30; ++i)
        dm_android_perf_feed(-1, 3000000000ull + (uint64_t)i * 16667000ull);
    dm_android_perf_read(3500010000ull, &s);
    assert(!s.valid && s.fps < 0); /* dropped frames do not become FPS */
    dm_android_perf_set_enabled(0);
    dm_android_perf_set_enabled(1);
    gpu = -1;
    for (int i = 1; i <= 30; ++i)
        dm_android_perf_feed(16.667, 4000000000ull + (uint64_t)i * 16667000ull);
    dm_android_perf_read(4500010000ull, &s);
    assert(s.valid && near(s.fps, 60.0) && s.gpu_ms < 0);
    dm_android_perf_dimensions(0, 0, 0, 0);
    dm_android_perf_read(4500010000ull, &s);
    assert(s.target_width == 0 && s.output_width == 0);
    puts("PASS performance sampling, stale data, session reset, dimensions and missing GPU");
    return 0;
}
