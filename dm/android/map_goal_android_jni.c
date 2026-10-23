#include <jni.h>
#include "map_goal_android.h"
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_MapGoalOverlay_nativeSnapshot(JNIEnv *env,jclass cls,jfloatArray dst) {
    (void)cls;
    if(!dst || (*env)->GetArrayLength(env,dst)<DM_ANDROID_MAP_GOAL_FLOATS) return;
    float out[DM_ANDROID_MAP_GOAL_FLOATS];
    dm_android_map_goal_snapshot(out);
    (*env)->SetFloatArrayRegion(env,dst,0,DM_ANDROID_MAP_GOAL_FLOATS,out);
}
