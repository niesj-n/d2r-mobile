#include "rift_info_android.h"
#include <jni.h>
#include <time.h>

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_RiftInfoOverlay_nativeReadSnapshot
  (JNIEnv *env, jclass cls, jfloatArray values) {
    (void)cls;
    if (!values || (*env)->GetArrayLength(env,values)<DM_ANDROID_RIFT_INFO_FLOATS) return;
    struct timespec t = {0};
    clock_gettime(CLOCK_MONOTONIC,&t);
    float snapshot[DM_ANDROID_RIFT_INFO_FLOATS];
    dm_android_rift_info_read((uint64_t)t.tv_sec*1000000000ull+t.tv_nsec,snapshot);
    (*env)->SetFloatArrayRegion(env,values,0,DM_ANDROID_RIFT_INFO_FLOATS,snapshot);
}
