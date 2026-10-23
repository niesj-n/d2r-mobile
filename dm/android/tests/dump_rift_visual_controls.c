/* Public geometry fixtures from the ACTUAL Android queue/235-float bridge.
 * The existing harness replaces guest boundaries; no control layout is copied.
 * Run this host-only generator and pass stdout to RiftInfoVisualBoundsTest.
 * The reused harness has an unused static map_pump on this baseline; compile
 * with -O2 -UNDEBUG -Wno-unused-function and the repository root on -I. */
#define main touch_existing_test_main
#include "dm/android/tests/test_touch_snapshot.c"
#undef main

int main(void) {
    const float sizes[][2]={{560,280},{667,375},{2608.f/3,400}};
    for (unsigned size=0;size<sizeof sizes/sizeof *sizes;size++)
        for (int margin=0;margin<2;margin++) for (int monitor=0;monitor<2;monitor++) {
            fixture(0); scene.game=1; labels_done=1;
            dm_android_touch_resize(sizes[size][0],sizes[size][1],20,24,16); REQUIRE(queue_sample()==0);
            dm_android_touch_margins(margin?96:0,margin?96:0); REQUIRE(queue_sample()==0);
            dm_android_touch_obstructions(14,monitor?30:0,monitor?62:0,monitor?314:0,monitor?226:0);
            REQUIRE(queue_sample()==0);
            for (int skill=-1;skill<5;skill++) for (int drag=0;drag<(skill<0?1:2);drag++) {
                if (skill>=0) {
                    dm_tc_target t=tc.layout.target[skill];
                    dm_android_touch_event(0,0,t.x,t.y); queue_sample();
                    test_clock_ns+=200000000; queue_sample();
                    if (drag) { dm_android_touch_event(1,0,t.x+100,t.y-100); queue_sample(); }
                }
                float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s);
                if (skill>=0) REQUIRE(s[DM_ANDROID_TOUCH_AIM+skill*6]==1 && s[DM_ANDROID_TOUCH_CANCEL+3]==1);
                printf("%u %d %d %d %d",size,margin,monitor,skill,drag);
                for (int i=0;i<DM_ANDROID_TOUCH_DRAW_FLOATS;i++) printf(" %.9g",s[i]);
                puts("");
                if (skill>=0) { dm_android_touch_event(3,0,0,0); queue_sample(); }
            }
        }
    return 0;
}
