#include "rift_info_android.h"
#include "../host/dm_rift.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t rift_info_lock = PTHREAD_MUTEX_INITIALIZER;
static float rift_info[DM_ANDROID_RIFT_INFO_FLOATS];
static uint64_t rift_info_published_ns;
/* Odd = active. Every gate transition revokes in-flight owner reads. */
static _Atomic uint64_t rift_info_epoch;

void dm_android_rift_info_set_active(int active) {
    pthread_mutex_lock(&rift_info_lock);
    uint64_t epoch = atomic_load(&rift_info_epoch);
    if ((epoch&1) != !!active) atomic_store(&rift_info_epoch,epoch+1);
    if (!active) {
        memset(rift_info,0,sizeof rift_info);
        rift_info_published_ns=0;
    }
    pthread_mutex_unlock(&rift_info_lock);
}

void dm_android_rift_info_publish(void) {
    uint64_t epoch = atomic_load(&rift_info_epoch);
    if (!(epoch&1)) return;
    dm_rift_hud_t h = {0};
    /* Do not put dm_rift_hud behind this lock and then call it from Java: the
     * lock protects only our copy. The server caller owns the plain K fields. */
    int visible = dm_rift_hud(&h);
    struct timespec t = {0};
    if (visible && clock_gettime(CLOCK_MONOTONIC, &t)) visible = 0;
    pthread_mutex_lock(&rift_info_lock);
    if (atomic_load(&rift_info_epoch) != epoch) {
        pthread_mutex_unlock(&rift_info_lock);
        return;
    }
    memset(rift_info, 0, sizeof(float)*14);
    rift_info_published_ns = visible ? (uint64_t)t.tv_sec*1000000000ull + t.tv_nsec : 0;
    if (visible) {
        const float fields[14] = {1,h.tier,h.progress,h.secs_left,h.done,h.guardian,
            h.inc,h.next,h.overtime,h.goal,h.gdead,h.gtotal,h.wave,h.nwave};
        memcpy(rift_info, fields, sizeof fields);
    } else memset(rift_info+14, 0, sizeof(float)*4);
    pthread_mutex_unlock(&rift_info_lock);
}

void dm_android_rift_info_set_minimap(const float normalized[4]) {
    int valid = normalized != NULL;
    float clipped[4] = {0};
    if (valid) {
        for (int i=0;i<4;i++) {
            if (!isfinite(normalized[i])) valid=0;
            clipped[i]=fminf(1,fmaxf(0,normalized[i]));
        }
        /* Cached original small-map bounds may have negative top after the
         * existing top-lift. Avoid its visible part, never reject that map. */
        if (clipped[2]<=clipped[0] || clipped[3]<=clipped[1]) valid=0;
    }
    pthread_mutex_lock(&rift_info_lock);
    if (valid) memcpy(rift_info+14, clipped, sizeof clipped);
    else memset(rift_info+14, 0, sizeof(float)*4);
    pthread_mutex_unlock(&rift_info_lock);
}

void dm_android_rift_info_read(uint64_t now_ns, float out[DM_ANDROID_RIFT_INFO_FLOATS]) {
    if (!out) return;
    pthread_mutex_lock(&rift_info_lock);
    memcpy(out, rift_info, sizeof rift_info);
    if (!rift_info_published_ns || now_ns<rift_info_published_ns ||
            now_ns-rift_info_published_ns>DM_ANDROID_RIFT_INFO_MAX_AGE_NS)
        memset(out,0,sizeof rift_info);
    pthread_mutex_unlock(&rift_info_lock);
}
