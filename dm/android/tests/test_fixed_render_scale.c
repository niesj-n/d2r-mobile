/* Android's selected 3D ratio remains fixed under every former fallback input. */
#ifdef NDEBUG
#error Render-scale regression requires active assertions
#endif
#include "../../host/dm_render_scale.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    const float expected[] = {.50f, .60f, .70f, .75f, .80f, .85f, .90f, 1.00f, 1.25f};
    for (int i = 0; i < 9; ++i) {
        assert(fabsf(dm_android_render_scale_value(i) - expected[i]) < .0001f);
        assert(dm_android_render_scale_index(expected[i]) == i);
        assert(dm_android_render_scale_fixed(expected[i], 9, .40f, 1) == expected[i]);
    }
    assert(dm_android_render_scale_value(5) == .85f);
    assert(dm_android_render_scale_value(-1) == .85f);
    assert(dm_android_render_scale_value(9) == .85f);
    assert(dm_android_render_scale_index(NAN) == -1);
    assert(dm_android_render_scale_index(INFINITY) == -1);
    assert(dm_android_render_scale_index(1.01f) == -1);
    assert(dm_android_render_scale_index(.49f) == -1);
    assert(dm_android_render_scale_step(5, -1) == 4);
    assert(dm_android_render_scale_step(5, +1) == 6);
    assert(dm_android_render_scale_step(6, +1) == 7 && dm_android_render_scale_value(7) == 1.00f);
    assert(dm_android_render_scale_step(0, -1) == 0);
    assert(dm_android_render_scale_step(8, +1) == 8);
    int good = 0, bad = 0;
    /* 60 FPS: GPU exceeds its budget but accounts for only 45% of a 40 ms CPU-bound frame. */
    for (int k = 0; k < 5; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 18.0, 40.0, 1000.0/60, &good, &bad) == 0);
    assert(bad == 0);
    for (int k = 0; k < 5; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 16.0, 16.6, 1000.0/60, &good, &bad) == 0);
    assert(bad == 0);  /* already meeting 60 FPS: preserve quality */
    /* A single CPU-bound or missing-data window breaks a partial GPU-bound streak. */
    for (int k = 0; k < 4; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 18.0, 20.0, 1000.0/60, &good, &bad) == 0);
    assert(bad == 4);
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 18.0, 40.0, 1000.0/60, &good, &bad) == 0 && bad == 0);
    for (int k = 0; k < 4; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 18.0, 20.0, 1000.0/60, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(1.25f, 0, -1.0, 20.0, 1000.0/60, &good, &bad) == 0 && bad == 0);
    for (int k = 0; k < 4; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 18.0, 20.0, 1000.0/60, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 18.0, 20.0, 1000.0/60, &good, &bad) == 1);
    /* 30 FPS: 31 ms GPU is over its 30.67 ms threshold, but 40 ms frames remain CPU-bound. */
    bad = good = 0;
    for (int k = 0; k < 5; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 31.0, 40.0, 1000.0/30, &good, &bad) == 0);
    for (int k = 0; k < 4; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 0, 31.0, 35.0, 1000.0/30, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 31.0, 35.0, 1000.0/30, &good, &bad) == 1);
    for (int k = 0; k < 4; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 1, 10.0, 35.0, 1000.0/30, &good, &bad) == 1);
    assert(good == 4);
    assert(dm_android_render_scale_gpu_next(1.25f, 1, 10.0, 0.0, 1000.0/30, &good, &bad) == 1 && good == 0);
    for (int k = 0; k < 4; ++k)
        assert(dm_android_render_scale_gpu_next(1.25f, 1, 10.0, 35.0, 1000.0/30, &good, &bad) == 1);
    assert(dm_android_render_scale_gpu_next(1.25f, 1, 10.0, 35.0, 1000.0/30, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(.50f, 0, 31.0, 35.0, 1000.0/30, &good, &bad) == 0);
    puts("PASS fixed choices, invalid inputs, CPU/GPU-bound windows, 30/60 budgets and counter reset");
    return 0;
}
