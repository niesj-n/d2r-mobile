/* Real Android queue and shared state machine; game/renderer hosts are fake. */
#define main original_snapshot_main
#include "test_touch_snapshot.c"
#undef main
static void play(void) { fixture(0);scene.game=1;labels_done=1; }
static void test_left_edge_start_ignores_left_margin(void) {
    for(int left=0;left<=96;left+=24) {
        play();dm_android_touch_resize(869.333f,400,24,12,0);dm_android_touch_margins(left,72);queue_sample();
        float x=1,y=240,s[235];
        dm_android_touch_event(0,0,x,y);queue_sample();
        REQUIRE(tc.joy_slot==0 && !tc.lx && !tc.ly);
        REQUIRE(tc.layout.joy_zone_l==0);
        read_twice(s);float ox=s[4+12*5],oy=s[4+12*5+1];
        REQUIRE(s[4+12*5+4]==1 && ox==tc.layout.joy_min_x && oy==y);
        dm_android_touch_event(1,0,x+36,y-12);queue_sample();
        REQUIRE(tc.lx>0 && tc.ly>0 && tc.joy_origin_x==ox && tc.joy_origin_y==oy);
        dm_android_touch_event(2,0,x+36,y-12);queue_sample();read_twice(s);
        REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly && s[4+12*5+4]==0);
    }
}
static void test_sticky_drag_and_full_map(void) {
    play();map_full=1;
    dm_android_touch_event(0,0,100,240);queue_sample();REQUIRE(tc.joy_slot==0);
    dm_android_touch_event(1,0,600,80);queue_sample();
    REQUIRE(tc.joy_slot==0 && tc.lx>0 && tc.ly>0 && tc.joy_origin_x==100 && tc.joy_origin_y==240);
    REQUIRE(map_full_calls==0 && map_absolute==-1);
    dm_android_touch_event(3,0,600,80);queue_sample();REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly);
}
static void test_outside_and_panel_ownership(void) {
    play();dm_android_touch_event(0,0,350,220);queue_sample();REQUIRE(tc.joy_slot<0);
    dm_android_touch_event(3,0,350,220);queue_sample();
    play();dm_android_touch_event(0,0,100,160);queue_sample();REQUIRE(tc.joy_slot<0);
    dm_android_touch_event(3,0,100,160);queue_sample();
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
        REQUIRE(tc.layout.joy_zone_t==180);
        float right=tc.layout.joy_zone_r;
        dm_android_touch_resize(869.333f,400,24,12,0);queue_sample();REQUIRE(tc.layout.joy_zone_r==right);
        float x=(tc.layout.joy_zone_l+right)*.5f;
        dm_android_touch_event(0,0,x,200);queue_sample();REQUIRE(tc.joy_slot==0);
        dm_android_touch_focus(0);queue_sample();REQUIRE(tc.joy_slot<0 && !tc.lx && !tc.ly);
    }
}
static void joy_geometry(float out[9]) {
    dm_tc_layout *l=&tc.layout;
    const float values[]={l->joy_x,l->joy_y,l->joy_radius,l->joy_zone_l,l->joy_zone_r,l->joy_zone_t,l->joy_min_x,l->joy_max_x,l->joy_max_y};
    memcpy(out,values,sizeof values);
}
static void test_left_slider_changes_map_key_not_joystick(void) {
    const float sizes[][2]={{869.333f,400},{560,280}};
    for(unsigned size=0;size<2;size++)for(int right=0;right<=96;right+=48) {
        play();dm_android_touch_resize(sizes[size][0],sizes[size][1],24,12,0);dm_android_touch_margins(0,right);queue_sample();
        float baseline[9],actual[9],before[235],after[235];joy_geometry(baseline);read_twice(before);
        for(int left=24;left<=96;left+=24) {
            dm_android_touch_margins(left,right);queue_sample();joy_geometry(actual);read_twice(after);
            REQUIRE(!memcmp(baseline,actual,sizeof baseline));
            REQUIRE(after[185]>before[185]); /* Existing left-side map button still follows the slider. */
            for(int i=DM_TC_MAIN;i<=DM_TC_PORTAL;i++)for(int c=0;c<3;c++)
                REQUIRE(after[field(i,c)]==before[field(i,c)]);
        }
    }
}
int main(void) {
    run_case("floating_left_edge_without_margin",test_left_edge_start_ignores_left_margin);
    run_case("floating_sticky_drag_full_map",test_sticky_drag_and_full_map);
    run_case("floating_outside_panel_skill_ownership",test_outside_and_panel_ownership);
    run_case("floating_margins_caps_relayout",test_margins_caps_and_relayout);
    run_case("floating_left_slider_decoupled",test_left_slider_changes_map_key_not_joystick);
    printf("RESULT PASS floating joystick cases=%u assertions=%u; real queue, fake guest; Canvas reviewed separately\n",cases,checks);
}
