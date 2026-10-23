#include <jni.h>
#include "touch_android.h"
#include "../host/dm_custom.h"

/* 使用命名JNI入口，与宿主已有JNI_OnLoad兼容。 | Named JNI entry points coexist with the host's JNI_OnLoad. */
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeEvent(JNIEnv *env, jclass cls, jint phase, jint slot, jfloat x, jfloat y) {
    (void)env; (void)cls;
    dm_android_touch_event(phase,slot,x,y);
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeResize(JNIEnv *env, jclass cls, jfloat width, jfloat height, jfloat left, jfloat right, jfloat bottom, jboolean tablet) {
    (void)env; (void)cls;
    dm_android_touch_tablet(tablet);
    dm_android_touch_resize(width,height,left,right,bottom);
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeMargins(JNIEnv *env, jclass cls, jfloat left, jfloat right) {
    (void)env; (void)cls;
    dm_android_touch_margins(left, right);
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeAction(JNIEnv *env, jclass cls, jint action) {
    (void)env; (void)cls;
    dm_android_touch_action(action);
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeCancelPending(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    dm_android_touch_cancel_pending();
}
JNIEXPORT jboolean JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeGameplay(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return dm_android_touch_gameplay();
}
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeSortContext(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return dm_android_touch_sort_context();
}
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeLayout(JNIEnv *env, jclass cls, jfloatArray dst) {
    (void)cls;
    if (!dst || (*env)->GetArrayLength(env,dst)<DM_ANDROID_TOUCH_DRAW_FLOATS) return;
    float data[DM_ANDROID_TOUCH_DRAW_FLOATS];
    dm_android_touch_layout(data);
    (*env)->SetFloatArrayRegion(env,dst,0,DM_ANDROID_TOUCH_DRAW_FLOATS,data);
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeSeedLegacySettings(JNIEnv *env, jclass cls, jint left, jint right, jboolean monitor) {
    (void)env; (void)cls;
    dm_custom_android_seed_legacy(left, right, monitor);
}
JNIEXPORT jboolean JNICALL Java_com_xiaoboluo_d2r_TouchOverlay_nativeReadSettings(JNIEnv *env, jclass cls, jintArray dst) {
    (void)cls;
    if (!dst || (*env)->GetArrayLength(env, dst) < 3) return JNI_FALSE;
    int values[3];
    if (!dm_custom_android_read_applied(values)) return JNI_FALSE;
    jint data[3] = {values[0], values[1], values[2]};
    (*env)->SetIntArrayRegion(env, dst, 0, 3, data);
    return JNI_TRUE;
}
