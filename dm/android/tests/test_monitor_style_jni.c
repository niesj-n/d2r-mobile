/* Execute the real read-only JNI boundary with fake Java arrays. */
#include <jni.h>
#undef JNIEXPORT
#define JNIEXPORT /* Host harness does not export unrelated Android entry points. */
#include "../touch_android_jni.c"
#include <assert.h>
#include <stddef.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions required
#endif
typedef struct { jsize length; jint data[4]; } array_t;
static int ready,reads,writes;
void dm_android_touch_event(int p,int s,float x,float y) { (void)p;(void)s;(void)x;(void)y;assert(0); }
void dm_android_touch_tablet(int t) { (void)t;assert(0); }
void dm_android_touch_resize(float w,float h,float l,float r,float b) { (void)w;(void)h;(void)l;(void)r;(void)b;assert(0); }
void dm_android_touch_margins(float l,float r) { (void)l;(void)r;assert(0); }
void dm_android_touch_obstructions(float s,float l,float t,float r,float b) { (void)s;(void)l;(void)t;(void)r;(void)b;assert(0); }
void dm_android_touch_action(int a) { (void)a;assert(0); }
void dm_android_touch_cancel_pending(void) { assert(0); }
int dm_android_touch_gameplay(void) { assert(0);return 0; }
int dm_android_touch_sort_context(void) { assert(0);return 0; }
void dm_android_touch_layout(float out[DM_ANDROID_TOUCH_DRAW_FLOATS]) { (void)out;assert(0); }
void dm_custom_android_seed_legacy(int l,int r,int m) { (void)l;(void)r;(void)m;assert(0); }
int dm_custom_android_read_monitor_style(int out[2]) { ++reads;if(!ready)return 0;out[0]=9;out[1]=45;return 1; }
int dm_custom_android_read_applied(int out[3]) { out[0]=96;out[1]=37;out[2]=1;return 1; }
static jsize array_length(JNIEnv *env,jarray array) { (void)env;return ((array_t *)array)->length; }
static void set_region(JNIEnv *env,jintArray array,jsize start,jsize length,const jint *values) {
    (void)env;array_t *a=(array_t *)array;assert(start==0 && length<=a->length);memcpy(a->data,values,length*sizeof(jint));++writes;
}
int main(void) {
    static const struct JNINativeInterface table={.GetArrayLength=array_length,.SetIntArrayRegion=set_region};JNIEnv env=&table;
    array_t a={.length=1,.data={-7,-7,-7,-7}};
    assert(!Java_com_xiaoboluo_d2r_TouchOverlay_nativeReadMonitorStyle(&env,NULL,NULL));
    assert(!Java_com_xiaoboluo_d2r_TouchOverlay_nativeReadMonitorStyle(&env,NULL,(jintArray)&a) && reads==0 && writes==0);
    a.length=4;assert(!Java_com_xiaoboluo_d2r_TouchOverlay_nativeReadMonitorStyle(&env,NULL,(jintArray)&a) && writes==0 && a.data[0]==-7);
    ready=1;assert(Java_com_xiaoboluo_d2r_TouchOverlay_nativeReadMonitorStyle(&env,NULL,(jintArray)&a));assert(a.data[0]==9 && a.data[1]==45 && a.data[2]==-7 && writes==1);
    assert(Java_com_xiaoboluo_d2r_TouchOverlay_nativeReadSettings(&env,NULL,(jintArray)&a));assert(a.data[0]==96 && a.data[1]==37 && a.data[2]==1 && a.data[3]==-7);
    return 0;
}
