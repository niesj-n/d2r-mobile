/* Android's selected 3D ratio remains fixed under every former fallback input. */
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
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 10.0, 1000.0/60, &good, &bad) == 0);  /* CPU-only slow frame */
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 16.0, 1000.0/60, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 10.0, 1000.0/60, &good, &bad) == 0 && bad == 0); /* one upload spike */
    for (int k = 0; k < 4; ++k) assert(dm_android_render_scale_gpu_next(1.25f, 0, 31.0, 1000.0/30, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(1.25f, 0, 31.0, 1000.0/30, &good, &bad) == 1); /* 10 s sustained */
    for (int k = 0; k < 4; ++k) assert(dm_android_render_scale_gpu_next(1.25f, 1, 10.0, 1000.0/30, &good, &bad) == 1);
    assert(dm_android_render_scale_gpu_next(1.25f, 1, 10.0, 1000.0/30, &good, &bad) == 0);
    assert(dm_android_render_scale_gpu_next(.50f, 0, 31.0, 1000.0/30, &good, &bad) == 0);
    puts("PASS fixed Android render-scale values, invalid inputs and fallback resistance");
    return 0;
}
