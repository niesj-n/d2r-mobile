#include "../map_goal_android.h"
#include "../../host/dm_world.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#ifdef NDEBUG
#error assertions required
#endif
static int reads, mode=1, goals=2, old_reads, have_mini=1, width=2000;
static float cell=4;
static pthread_mutex_t gate_lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_cond=PTHREAD_COND_INITIALIZER;
static int gated,entered,released;
static void *once(void *unused){(void)unused;dm_android_map_goal_publish(1);return NULL;}
int dm_ui_map_geom(float *c) { reads++; *c=cell; return mode; }
int dm_map_goals(dm_map_goal_view *v) { reads++;
 pthread_mutex_lock(&gate_lock);
 if(gated){entered=1;pthread_cond_broadcast(&gate_cond);while(!released)pthread_cond_wait(&gate_cond,&gate_lock);}
 pthread_mutex_unlock(&gate_lock);
 memset(v,0,sizeof *v); v->n=goals; v->px=100;v->py=200;v->sx=.4f;v->sy=.6f; for(int i=0;i<12;i++){v->x[i]=101+i;v->y[i]=201+i;v->art[i]=i?70:2;} return goals; }
int dm_screen_real(int *w,int *h) { reads++; *w=width;*h=1000;return 1; }
int dm_ui_minimap_rect(float r[4]) { reads++; r[0]=.7f;r[1]=.1f;r[2]=.98f;r[3]=.4f;return have_mini; }
int dm_lang_group(void) { reads++;return 1; }
int dm_lang_index(void) { reads++;return 8; }
int dm_custom_get(const char *key) { assert(!strcmp(key,"item_notes"));reads++;old_reads++;return 1; }
static void *writer(void *unused) { (void)unused; for(int i=0;i<20000;i++){dm_android_map_goal_publish(1);dm_android_map_goal_publish(0);}return NULL; }
int main(void) {
 float f[DM_ANDROID_MAP_GOAL_FLOATS];
 dm_android_map_goal_publish(0);dm_android_map_goal_snapshot(f); assert(reads==0 && f[0]==0);
 dm_android_map_goal_publish(1);int n=reads;dm_android_map_goal_snapshot(f);assert(reads==n);
 assert(f[0]==1 && f[1]==2 && f[2]==4 && f[3]==2000 && f[4]==1000);
 assert(f[5]==100 && f[6]==200 && f[7]==.4f && f[8]==.6f);
 assert(f[13]==1 && f[14]==1 && f[16]==101 && f[18]==2 && f[21]==70);
 dm_android_map_goal_publish(0);dm_android_map_goal_snapshot(f);for(int i=0;i<DM_ANDROID_MAP_GOAL_FLOATS;i++)assert(f[i]==0);assert(reads==n);
 goals=0;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==0);
 goals=13;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==0);
 goals=2;mode=0;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==0);mode=1;
 have_mini=0;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==0);
 mode=2;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==2);mode=1;have_mini=1;
 width=0;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==0);width=2000;
 goals=2;cell=NAN;dm_android_map_goal_publish(1);dm_android_map_goal_snapshot(f);assert(f[0]==0);cell=4;
 pthread_t t;assert(!pthread_create(&t,NULL,writer,NULL));
 for(int i=0;i<20000;i++){dm_android_map_goal_snapshot(f);if(f[0])assert(f[1]==2 && f[3]==2000 && f[18]==2 && f[21]==70);else for(int j=0;j<DM_ANDROID_MAP_GOAL_FLOATS;j++)assert(f[j]==0);}
 assert(!pthread_join(t,NULL));assert(old_reads==1);
 gated=1;assert(!pthread_create(&t,NULL,once,NULL));
 pthread_mutex_lock(&gate_lock);while(!entered)pthread_cond_wait(&gate_cond,&gate_lock);pthread_mutex_unlock(&gate_lock);
 n=reads;dm_android_map_goal_publish(0);assert(reads==n);
 pthread_mutex_lock(&gate_lock);released=1;pthread_cond_broadcast(&gate_cond);pthread_mutex_unlock(&gate_lock);
 assert(!pthread_join(t,NULL));dm_android_map_goal_snapshot(f);for(int i=0;i<DM_ANDROID_MAP_GOAL_FLOATS;i++)assert(f[i]==0);
 puts("PASS lifecycle cancellation wins in-flight sampling; inactive no reads, host-only sampling, invalid/empty data and coherent concurrent snapshots");
}
