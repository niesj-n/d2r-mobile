/* The Android product offers discrete, fixed 3D target ratios. */
#pragma once
#include <math.h>

static const float dm_android_render_scales[] = {
    .50f, .60f, .70f, .75f, .80f, .85f, .90f, 1.00f, 1.25f
};
#define DM_ANDROID_RENDER_SCALE_COUNT 9
#define DM_ANDROID_RENDER_SCALE_DEFAULT 5
static const float dm_android_render_gpu_factors[] = {
    .92f, .85f, .78f, .72f, .66f, .60f, .55f, .50f, .45f, .40f
};

static inline float dm_android_render_scale_value(int index) {
    if (index < 0 || index >= DM_ANDROID_RENDER_SCALE_COUNT)
        index = DM_ANDROID_RENDER_SCALE_DEFAULT;
    return dm_android_render_scales[index];
}

static inline int dm_android_render_scale_index(float value) {
    if (!isfinite(value)) return -1;
    for (int i = 0; i < DM_ANDROID_RENDER_SCALE_COUNT; ++i)
        if (fabsf(value - dm_android_render_scales[i]) < .0001f) return i;
    return -1;
}

static inline int dm_android_render_scale_step(int index, int direction) {
    if (index < 0 || index >= DM_ANDROID_RENDER_SCALE_COUNT)
        index = DM_ANDROID_RENDER_SCALE_DEFAULT;
    if (direction < 0 && index > 0) --index;
    if (direction > 0 && index + 1 < DM_ANDROID_RENDER_SCALE_COUNT) ++index;
    return index;
}

/* Android never consumes interval, GPU, thermal or ruler fallback levels. */
static inline float dm_android_render_scale_fixed(float selected, int interval_level,
                                                   float gpu_scale, int ruler_truth) {
    (void)interval_level; (void)gpu_scale; (void)ruler_truth;
    return selected;
}

static inline int dm_android_render_scale_gpu_max_level(float ceiling) {
    int max_level = 0;
    for (int i = 0; i < (int)(sizeof dm_android_render_gpu_factors / sizeof dm_android_render_gpu_factors[0]); ++i)
        if (ceiling * dm_android_render_gpu_factors[i] >= .50f - .0001f) max_level = i + 1;
    return max_level;
}

static inline float dm_android_render_scale_gpu_value(float ceiling, int level) {
    int maximum = dm_android_render_scale_gpu_max_level(ceiling);
    if (level <= 0) return ceiling;
    if (level > maximum) level = maximum;
    return level ? ceiling * dm_android_render_gpu_factors[level - 1] : ceiling;
}

/* One completed two-second GPU sample. The selected FPS determines budget. */
static inline int dm_android_render_scale_gpu_next(float ceiling, int level,
                                                   double gpu_ms, double budget_ms,
                                                   int *good_windows, int *bad_windows) {
    int maximum = dm_android_render_scale_gpu_max_level(ceiling);
    if (level > maximum) level = maximum;
    if (!(gpu_ms > 0 && budget_ms > 0)) return level;
    if (gpu_ms >= .92 * budget_ms && level < maximum) {
        *good_windows = 0;
        if (++*bad_windows >= 5) { *bad_windows = 0; return level + 1; }
        return level;
    }
    *bad_windows = 0;
    if (level > 0 && gpu_ms < .70 * budget_ms) {
        if (++*good_windows >= 5) { *good_windows = 0; return level - 1; }
    } else *good_windows = 0;
    return level;
}
