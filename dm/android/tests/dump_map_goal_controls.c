/* Source-backed visual-obstruction fixture. Includes the production Android
 * queue and shared controls through their existing neutral-host harness. */
#define main original_snapshot_main
#include "test_touch_snapshot.c"
#undef main
static void emit(const char *name) {
    float s[235];dm_android_touch_layout(s);printf("%s",name);
    for(int i=0;i<235;i++)printf(",%.9g",s[i]);puts("");
}
int main(void) {
    fixture(0);scene.game=1;labels_done=1;emit("idle");
    for(int i=0;i<5;i++) {
        fixture(0);scene.game=1;labels_done=1;dm_tc_target t=tc.layout.target[i];
        dm_android_touch_event(0,0,t.x,t.y);queue_sample();
        dm_android_touch_event(1,0,t.x,t.y-12);queue_sample();
        char name[32];snprintf(name,sizeof name,"held%d",i);emit(name);
        dm_android_touch_event(2,0,t.x,t.y-12);queue_sample();
        snprintf(name,sizeof name,"released%d",i);emit(name);
    }
}
