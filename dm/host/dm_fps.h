/* Android FPS setting. Stored 0/1 retain the v1 60/30 FPS meanings. */
#pragma once

static inline int dm_android_fps_valid_index(int index) {
    return index >= 0 && index <= 5;
}

static inline int dm_android_fps_value(int index) {
    static const int fps[] = {60, 30, 40, 45, 90, 120};
    return fps[dm_android_fps_valid_index(index) ? index : 0];
}

static inline int dm_android_fps_step(int index, int direction) {
    static const int ordered[] = {1, 2, 3, 0, 4, 5};
    int position = 3;
    for (int i = 0; i < 6; ++i) if (ordered[i] == index) { position = i; break; }
    if (direction < 0 && position > 0) --position;
    if (direction > 0 && position < 5) ++position;
    return ordered[position];
}
