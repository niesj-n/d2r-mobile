#include "map_goal_android.h"
#include "../host/dm_world.h"
#include "../host/dm_cursor.h"
#include "../host/dm_uiscreen.h"
#include "../host/dm_i18n.h"
#include "../host/dm_custom.h"
#include <math.h>
#include <pthread.h>
#include <string.h>

static pthread_mutex_t map_lock=PTHREAD_MUTEX_INITIALIZER;
static float published[DM_ANDROID_MAP_GOAL_FLOATS];
static unsigned long generation;
void dm_android_map_goal_publish(int active) {
    float out[DM_ANDROID_MAP_GOAL_FLOATS]={0};
    pthread_mutex_lock(&map_lock);
    if (!active) {
        ++generation;
        memset(published,0,sizeof published);
        pthread_mutex_unlock(&map_lock);
        return;
    }
    unsigned long observed=generation;
    pthread_mutex_unlock(&map_lock);
    if (active) {
        /* Host world publication is bounded (four seqlock attempts), geometry
         * and minimap rectangle are atomics. No guest pointer escapes here. */
        dm_map_goal_view v={0}; float cell=0, mini[4]={0}; int rw=0,rh=0;
        int mode=dm_ui_map_geom(&cell), n=dm_map_goals(&v);
        dm_screen_real(&rw,&rh);
        int have_mini=dm_ui_minimap_rect(mini);
        if ((mode==1 || mode==2) && n>0 && n<=12 && v.n==n && rw>0 && rh>0 &&
            isfinite(cell) && cell>0 && isfinite(v.sx) && isfinite(v.sy) &&
            (mode==2 || (have_mini && isfinite(mini[0]) && isfinite(mini[1]) &&
             isfinite(mini[2]) && isfinite(mini[3]) && mini[2]>mini[0] && mini[3]>mini[1]))) {
            static int old=-1; /* Original old-name setting is restart-scoped. */
            if (old<0) old=dm_custom_get("item_notes")!=0;
            out[0]=mode;out[1]=n;out[2]=cell;out[3]=rw;out[4]=rh;
            out[5]=v.px;out[6]=v.py;out[7]=v.sx;out[8]=v.sy;
            for(int i=0;i<4;i++) out[9+i]=mini[i];
            out[13]=dm_lang_group();out[14]=old && dm_lang_index()==8;
            for(int i=0;i<n;i++) {out[16+i*3]=v.x[i];out[17+i*3]=v.y[i];out[18+i*3]=v.art[i];}
        }
    }
    pthread_mutex_lock(&map_lock);
    if (generation==observed) memcpy(published,out,sizeof published);
    pthread_mutex_unlock(&map_lock);
}
void dm_android_map_goal_snapshot(float out[DM_ANDROID_MAP_GOAL_FLOATS]) {
    pthread_mutex_lock(&map_lock);
    memcpy(out,published,sizeof published);
    pthread_mutex_unlock(&map_lock);
}
