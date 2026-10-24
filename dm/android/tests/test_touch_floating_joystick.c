/* Real Android queue and shared state machine; game/renderer hosts are fake. */
#define main original_snapshot_main
#include "test_touch_snapshot.c"
#undef main
static void play(void) { fixture(0);scene.game=1;labels_done=1; }
static void test_new_right_and_upper_start_points(void) {
    const float points[][2]={{350,220},{100,160}};
    for(unsigned i=0;i<2;i++) {
        play();float x=points[i][0],y=points[i][1],s[235];
        dm_android_touch_event(0,0,x,y);queue_sample();
        REQUIRE(tc.joy_slot==0 && !tc.lx && !tc.ly);
        read_twice(s);REQUIRE(s[4+12*5+4]==1 && s[4+12*5]==x && s[4+12*5+1]==y);
        dm_android_touch_event(1,0,x+36,y-12);queue_sample();
        REQUIRE(tc.lx>0 && tc.ly>0 && tc.joy_origin_x==x && tc.joy_origin_y==y);
        dm_android_touch_event(2,0,x+36,y-12);queue_sample();read_twice(s);
        REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly && s[4+12*5+4]==0);
    }
}
static void test_sticky_drag_and_full_map(void) {
    play();map_full=1;
    dm_android_touch_event(0,0,350,220);queue_sample();REQUIRE(tc.joy_slot==0);
    dm_android_touch_event(1,0,600,80);queue_sample();
    REQUIRE(tc.joy_slot==0 && tc.lx>0 && tc.ly>0 && tc.joy_origin_x==350 && tc.joy_origin_y==220);
    REQUIRE(map_full_calls==0 && map_absolute==-1);
    dm_android_touch_event(3,0,600,80);queue_sample();REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly);
}
static void test_outside_and_panel_ownership(void) {
    play();dm_android_touch_event(0,0,370,220);queue_sample();REQUIRE(tc.joy_slot<0);
    dm_android_touch_event(3,0,370,220);queue_sample();
    play();dm_android_touch_event(0,0,100,140);queue_sample();REQUIRE(tc.joy_slot<0);
    dm_android_touch_event(3,0,100,140);queue_sample();
    panel_fixture();scene.game=1;
    dm_android_touch_event(0,0,350,220);queue_sample();REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly);
    dm_android_touch_event(3,0,350,220);queue_sample();
    play();dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y);REQUIRE(queue_sample()&DM_BTN_A);REQUIRE(tc.joy_slot<0);
    dm_android_touch_event(3,0,main.x,main.y);queue_sample();
}
static void test_margins_caps_and_relayout(void) {
    for(int margin=0;margin<=96;margin+=24) {
        play();dm_android_touch_resize(869.333f,400,24,12,0);dm_android_touch_margins(margin,margin);queue_sample();
        REQUIRE(tc.layout.joy_zone_r<=tc.layout.joy_max_x && tc.layout.joy_zone_l<=tc.layout.joy_zone_r);
        REQUIRE(tc.layout.joy_zone_t==160);
        float right=tc.layout.joy_zone_r;
        dm_android_touch_resize(869.333f,400,24,12,0);queue_sample();REQUIRE(tc.layout.joy_zone_r==right);
        float x=(tc.layout.joy_zone_l+right)*.5f;
        dm_android_touch_event(0,0,x,170);queue_sample();REQUIRE(tc.joy_slot==0);
        dm_android_touch_focus(0);queue_sample();REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly);
    }
}
int main(void) {
    run_case("floating_expanded_start_points",test_new_right_and_upper_start_points);
    run_case("floating_sticky_drag_full_map",test_sticky_drag_and_full_map);
    run_case("floating_outside_panel_skill_ownership",test_outside_and_panel_ownership);
    run_case("floating_margins_caps_relayout",test_margins_caps_and_relayout);
    printf("RESULT PASS floating joystick cases=%u assertions=%u; real queue, fake guest; Canvas reviewed separately\n",cases,checks);
}
