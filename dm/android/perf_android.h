#ifndef DM_ANDROID_PERF_H
#define DM_ANDROID_PERF_H
#include <stdint.h>

typedef struct {
    int valid;
    double fps;          /* positive present samples / monotonic window */
    double frame_ms;     /* mean positive present interval in the window */
    double gpu_ms;       /* renderer's existing GPU timing, or -1 */
    double scale;        /* current 3D ratio, or -1 */
    unsigned target_width, target_height;
    unsigned output_width, output_height;
} dm_android_perf_snapshot;

void dm_android_perf_set_enabled(int enabled);
int dm_android_perf_enabled(void);
void dm_android_perf_feed(double present_interval_ms, uint64_t now_ns);
void dm_android_perf_read(uint64_t now_ns, dm_android_perf_snapshot *out);
void dm_android_perf_dimensions(unsigned tw, unsigned th, unsigned ow, unsigned oh);
#endif
