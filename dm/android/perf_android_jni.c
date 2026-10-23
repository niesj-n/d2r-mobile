#include "perf_android.h"
#include <jni.h>
#include <time.h>

static uint64_t monotonic_ns(void) {
    struct timespec t;
    return clock_gettime(CLOCK_MONOTONIC, &t) ? 0 : (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec;
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_PerformanceOverlay_nativeSetMonitoringEnabled
  (JNIEnv *env, jclass cls, jboolean enabled) {
    (void)env; (void)cls;
    dm_android_perf_set_enabled(enabled);
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_PerformanceOverlay_nativeReadSnapshot
  (JNIEnv *env, jclass cls, jfloatArray values, jintArray dimensions) {
    (void)cls;
    if (!values || !dimensions || (*env)->GetArrayLength(env, values) < 4 || (*env)->GetArrayLength(env, dimensions) < 4) return;
    dm_android_perf_snapshot s;
    dm_android_perf_read(monotonic_ns(), &s);
    const jfloat floats[4] = {(jfloat)s.fps, (jfloat)s.frame_ms, (jfloat)s.gpu_ms, (jfloat)s.scale};
    const jint ints[4] = {(jint)s.target_width, (jint)s.target_height, (jint)s.output_width, (jint)s.output_height};
    (*env)->SetFloatArrayRegion(env, values, 0, 4, floats);
    (*env)->SetIntArrayRegion(env, dimensions, 0, 4, ints);
}
