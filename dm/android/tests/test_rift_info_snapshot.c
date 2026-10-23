/* Actual publisher/read implementation; only the existing guest HUD and clock
 * are replaced. No game, saves, Android runtime or input is opened. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <jni.h>
#undef JNIEXPORT
#define JNIEXPORT
static uint64_t test_now = UINT64_C(10000000000);
static int test_clock_gettime(clockid_t id, struct timespec *out) {
    assert(id == CLOCK_MONOTONIC);
    out->tv_sec = test_now / 1000000000; out->tv_nsec = test_now % 1000000000;
    return 0;
}
#define clock_gettime test_clock_gettime
#include "dm/android/rift_info_android.c"
#include "dm/android/rift_info_android_jni.c"
#undef clock_gettime

static dm_rift_hud_t guest;
static int guest_visible, guest_reads, revoke_during_read;
int dm_rift_hud(dm_rift_hud_t *out) {
    ++guest_reads; *out = guest;
    if (revoke_during_read) {
        revoke_during_read=0;
        dm_android_rift_info_set_active(0);
        dm_android_rift_info_set_active(1);
    }
    return guest_visible;
}
static float snapshot[DM_ANDROID_RIFT_INFO_FLOATS];
static void read_snapshot(void) { dm_android_rift_info_read(test_now, snapshot); }
static _Atomic int finished;
static void *publisher(void *unused) {
    (void)unused;
    for (int i=0;i<40000;i++) {
        int tier=i%100+1;
        guest=(dm_rift_hud_t){.tier=tier,.progress=.25f,.secs_left=1000+tier,.next=123,.nwave=3};
        dm_android_rift_info_publish();
    }
    atomic_store(&finished,1); return NULL;
}
static void *reader(void *unused) {
    (void)unused;
    do {
        float copy[DM_ANDROID_RIFT_INFO_FLOATS];
        dm_android_rift_info_read(test_now,copy);
        if (copy[0]) assert(copy[3]==1000+copy[1] && copy[2]==.25f && copy[7]==123 && copy[13]==3);
    } while (!atomic_load(&finished));
    return NULL;
}
typedef struct { jsize length; jfloat data[20]; } java_array;
static int jni_writes;
static jsize array_length(JNIEnv *env,jarray array) { (void)env; return ((java_array *)array)->length; }
static void set_region(JNIEnv *env,jfloatArray array,jsize start,jsize length,const jfloat *values) {
    (void)env; java_array *a=(java_array *)array;
    assert(start==0 && length==DM_ANDROID_RIFT_INFO_FLOATS && length<=a->length);
    memcpy(a->data,values,length*sizeof *values); ++jni_writes;
}
static void jni_test(void) {
    static const struct JNINativeInterface_ table={.GetArrayLength=array_length,.SetFloatArrayRegion=set_region};
    JNIEnv env=&table;
    java_array a={.length=17};
    Java_com_xiaoboluo_d2r_RiftInfoOverlay_nativeReadSnapshot(&env,NULL,NULL);
    Java_com_xiaoboluo_d2r_RiftInfoOverlay_nativeReadSnapshot(&env,NULL,(jfloatArray)&a);
    assert(!jni_writes);
    a.length=20; a.data[18]=-71;
    int reads=guest_reads;
    Java_com_xiaoboluo_d2r_RiftInfoOverlay_nativeReadSnapshot(&env,NULL,(jfloatArray)&a);
    assert(jni_writes==1 && a.data[18]==-71 && a.data[0]==1 && guest_reads==reads);
}
int main(void) {
    dm_android_rift_info_set_active(0); dm_android_rift_info_publish(); read_snapshot();
    assert(!snapshot[0] && guest_reads == 0);
    guest_visible = 1;
    dm_android_rift_info_set_active(1);
    guest = (dm_rift_hud_t){.tier=95,.progress=.625f,.secs_left=389,.guardian=1,
        .inc=3,.next=98,.overtime=0,.goal=2,.gdead=1,.gtotal=2,.wave=2,.nwave=3};
    dm_android_rift_info_publish(); read_snapshot();
    assert(snapshot[0] == 1 && snapshot[1] == 95 && snapshot[2] == .625f);
    assert(snapshot[3] == 389 && snapshot[4] == 0 && snapshot[5] == 1);
    assert(snapshot[6] == 3 && snapshot[7] == 98 && snapshot[8] == 0);
    assert(snapshot[9] == 2 && snapshot[10] == 1 && snapshot[11] == 2);
    assert(snapshot[12] == 2 && snapshot[13] == 3);
    float previous[DM_ANDROID_RIFT_INFO_FLOATS]; memcpy(previous,snapshot,sizeof previous);
    read_snapshot(); assert(!memcmp(previous,snapshot,sizeof previous) && guest_reads == 1);
    const float mini[4] = {.1f,.2f,.35f,.45f};
    dm_android_rift_info_set_minimap(mini); read_snapshot();
    assert(snapshot[14] == .1f && snapshot[17] == .45f);
    dm_android_rift_info_set_minimap(NULL); read_snapshot(); assert(snapshot[14] == 0 && snapshot[17] == 0);
    const float top_lifted[4] = {.1f,-.2f,.4f,.3f};
    dm_android_rift_info_set_minimap(top_lifted); read_snapshot();
    assert(snapshot[14] == .1f && snapshot[15] == 0 && snapshot[17] == .3f);
    const float bad[4] = {0,0,NAN,1};
    dm_android_rift_info_set_minimap(bad); read_snapshot(); assert(snapshot[16] == 0);
    guest.secs_left = -9; guest.overtime=1; guest.goal=5;
    dm_android_rift_info_publish(); read_snapshot();
    assert(snapshot[3] == -9 && snapshot[8] == 1 && snapshot[9] == 5);
    guest.done=1; guest.inc=0; guest.next=95; guest.goal=0;
    dm_android_rift_info_publish(); read_snapshot();
    assert(snapshot[4] == 1 && snapshot[6] == 0 && snapshot[7] == 95);
    guest.tier=100; guest.inc=3; guest.next=100;
    dm_android_rift_info_publish(); read_snapshot();
    assert(snapshot[1] == 100 && snapshot[6] == 3 && snapshot[7] == 100);
    int reads=guest_reads;
    test_now += DM_ANDROID_RIFT_INFO_MAX_AGE_NS + 1; read_snapshot();
    assert(snapshot[0] == 0 && guest_reads == reads);
    dm_android_rift_info_publish();
    dm_android_rift_info_set_active(0); read_snapshot();
    assert(snapshot[0] == 0 && guest_reads == reads + 1);
    dm_android_rift_info_set_active(1); read_snapshot(); assert(snapshot[0] == 0);
    guest_visible=0; dm_android_rift_info_publish(); read_snapshot(); assert(snapshot[0] == 0);
    guest_visible=1; revoke_during_read=1;
    dm_android_rift_info_publish(); read_snapshot(); assert(snapshot[0]==0);
    dm_android_rift_info_publish(); read_snapshot(); assert(snapshot[0]==1);
    jni_test();
    dm_android_rift_info_set_active(0); dm_android_rift_info_set_active(1);
    pthread_t writer_thread,reader_thread;
    assert(!pthread_create(&writer_thread,NULL,publisher,NULL));
    assert(!pthread_create(&reader_thread,NULL,reader,NULL));
    assert(!pthread_join(writer_thread,NULL)); assert(!pthread_join(reader_thread,NULL));
    puts("rift native: host fields, inactive/revoked/stale, map, repeat JNI, 40000 concurrent publications passed");
    return 0;
}
