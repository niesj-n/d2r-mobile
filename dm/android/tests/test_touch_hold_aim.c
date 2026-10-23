/* Real Android input queue and shared hold state machine; no guest or phone. */
#define main original_snapshot_main
#include "test_touch_snapshot.c"
#undef main

static void gameplay_fixture(void) { fixture(0); scene.game=1; labels_done=1; }
static uint64_t hold_frame(double seconds) { test_clock_ns+=(uint64_t)(seconds*1e9); return queue_sample(); }
static void test_hold_visual_snapshot(void) {
    gameplay_fixture(); dm_tc_target k=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,k.x,k.y); REQUIRE(hold_frame(.02)==DM_BTN_A);
    dm_android_touch_event(1,0,k.x+60,k.y-30); REQUIRE(hold_frame(.2)==DM_BTN_A);
    float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s);
    REQUIRE(s[DM_ANDROID_TOUCH_AIM]==1);
    REQUIRE(s[DM_ANDROID_TOUCH_AIM+1]>0 && s[DM_ANDROID_TOUCH_AIM+2]<0);
    REQUIRE(s[DM_ANDROID_TOUCH_CANCEL+3]==1);
    dm_android_touch_event(2,0,k.x+60,k.y-30); REQUIRE(hold_frame(.2)==0);
    REQUIRE(!tc.pulse_button);
}
static void test_five_slots_keep_shared_hold_timing(void) {
    for(int target=DM_TC_MAIN;target<=DM_TC_SKILL4;++target) {
        gameplay_fixture(); dm_tc_target k=tc.layout.target[target];
        dm_android_touch_event(0,0,k.x,k.y);
        uint64_t first=hold_frame(0);
        REQUIRE(first==(target==DM_TC_MAIN?k.button:0));
        REQUIRE(hold_frame(.1)==k.button);
        dm_android_touch_event(1,0,k.x-40,k.y-20); REQUIRE(hold_frame(.1)==k.button);
        for(int n=0;n<8;++n) REQUIRE(hold_frame(.2)==k.button);
        float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s); int p=DM_ANDROID_TOUCH_AIM+target*6;
        REQUIRE(s[p]==1 && s[p+1]<0 && s[p+2]<0 && s[p+5]==0);
        float kscale=dm_tc_scale_for_size(width,height);
        REQUIRE(fabsf(hypotf(s[p+1],s[p+2])-18*kscale)<.001f);
        REQUIRE(fabsf(hypotf(s[p+3],s[p+4])-1)<.001f);
        dm_android_touch_event(2,0,k.x-40,k.y-20); REQUIRE(hold_frame(.1)==0);
        REQUIRE(!tc.pulse_button && !tc.defer_button); read_twice(s);
        REQUIRE(s[p]==0 && s[DM_ANDROID_TOUCH_CANCEL+3]==0);
    }
    gameplay_fixture(); tc.skill_set=1; dm_tc_target t=tc.layout.target[DM_TC_SKILL2];
    dm_android_touch_event(0,0,t.x,t.y); REQUIRE(hold_frame(0)==DM_BTN_ZL);
    REQUIRE(hold_frame(.1)==DM_BTN_ZL);
    REQUIRE(hold_frame(.08)==(DM_BTN_ZL|t.button));
    REQUIRE(hold_frame(.5)==(DM_BTN_ZL|t.button));
    dm_android_touch_event(2,0,t.x,t.y); REQUIRE(hold_frame(.1)==0 && !tc.pulse_button);
}
static void test_deadzone_and_cancel_circle_exact_hit(void) {
    gameplay_fixture(); dm_tc_target k=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,k.x,k.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    dm_android_touch_event(1,0,k.x+6,k.y-4); REQUIRE(hold_frame(.1)==DM_BTN_A);
    float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s);
    REQUIRE(s[201]==6 && s[202]==-4 && s[203]==0 && s[204]==0);
    float x=s[230],y=s[231],r=s[232];
    dm_android_touch_event(1,0,x+r*.9f,y+r*.9f); REQUIRE(hold_frame(.1)==DM_BTN_A);
    REQUIRE(tc.finger[0].active); /* Bounding-box corner is outside the circle. */
    dm_android_touch_event(1,0,x+r,y); REQUIRE(hold_frame(.01)==0);
    REQUIRE(!tc.finger[0].active && !tc.pulse_button && !tc.defer_button);
    REQUIRE(observed_idle==0); /* The cancelled physical finger is still held. */
    read_twice(s); REQUIRE(s[235-2]==1 && s[235-1]==1 && s[205]==1);
    dm_android_touch_event(1,0,k.x,k.y); REQUIRE(hold_frame(.2)==0); /* Sticky until UP. */
    dm_android_touch_event(2,0,k.x,k.y); REQUIRE(hold_frame(.2)==0);
    REQUIRE(!tc.pulse_button && !tc.world_tap_seq && !tc.mouse_click_seq);
    read_twice(s); REQUIRE(s[200]==0 && s[233]==0 && s[234]==0);
    REQUIRE(observed_idle==1);
}
static void test_feedback_direction_matches_saturated_input(void) {
    gameplay_fixture(); dm_tc_target k=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,k.x,k.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    const float deltas[][2]={{120,60},{-120,-60},{30,-120},{6,4}};
    for(unsigned i=0;i<sizeof deltas/sizeof deltas[0];++i) {
        dm_android_touch_event(1,0,k.x+deltas[i][0],k.y+deltas[i][1]); REQUIRE(hold_frame(.1)==DM_BTN_A);
        float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s);
        float n=hypotf((float)tc.rx,(float)tc.ry);
        if(n>0) { REQUIRE(fabsf(s[203]-tc.rx/n)<.0001f); REQUIRE(fabsf(s[204]+tc.ry/n)<.0001f); }
        else REQUIRE(s[203]==0 && s[204]==0);
    }
}
static void test_same_batch_cancel_only_its_finger(void) {
    gameplay_fixture(); dm_tc_target a=tc.layout.target[DM_TC_MAIN],b=tc.layout.target[DM_TC_SKILL1];
    dm_tc_target c=aim_feedback_circle();
    dm_android_touch_event(0,0,a.x,a.y);
    dm_android_touch_event(0,1,b.x,b.y);
    dm_android_touch_event(1,1,b.x+30,b.y);
    dm_android_touch_event(1,1,c.x,c.y);
    dm_android_touch_event(2,1,c.x,c.y);
    REQUIRE(hold_frame(.1)==DM_BTN_A); /* B was briefly latched in this same batch; it must be revoked. */
    REQUIRE(tc.finger[0].active && !tc.finger[1].active && !tc.pulse_button);
    REQUIRE(hold_frame(.2)==DM_BTN_A);
    dm_android_touch_event(3,0,a.x,a.y); REQUIRE(hold_frame(.1)==0);
    /* A cancelled direction finger cannot erase a different finger's quick tap. */
    gameplay_fixture(); a=tc.layout.target[DM_TC_MAIN]; b=tc.layout.target[DM_TC_SKILL1]; c=aim_feedback_circle();
    dm_android_touch_event(0,0,a.x,a.y); dm_android_touch_event(2,0,a.x,a.y);
    dm_android_touch_event(0,1,b.x,b.y); dm_android_touch_event(2,1,c.x,c.y);
    REQUIRE(hold_frame(.1)==DM_BTN_A && !tc.pulse_button);
}
static void test_joystick_and_other_skill_survive_cancel(void) {
    gameplay_fixture(); dm_tc_target main=tc.layout.target[DM_TC_MAIN],b=tc.layout.target[DM_TC_SKILL1];
    dm_tc_target c=aim_feedback_circle();
    dm_android_touch_event(0,0,90,275); dm_android_touch_event(1,0,150,275);
    dm_android_touch_event(0,1,main.x,main.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    dm_android_touch_event(0,2,b.x,b.y); REQUIRE(hold_frame(.1)==(DM_BTN_A|DM_BTN_B));
    dm_android_touch_event(1,2,c.x,c.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    REQUIRE(tc.lx==32767 && tc.ly==0 && tc.joy_slot==0 && tc.finger[1].active);
    dm_android_touch_event(2,2,c.x,c.y); REQUIRE(hold_frame(.1)==DM_BTN_A && tc.lx==32767);
    dm_android_touch_event(3,1,main.x,main.y); REQUIRE(hold_frame(.1)==0 && tc.lx==32767);
    dm_android_touch_event(3,0,150,275); REQUIRE(hold_frame(.1)==0 && tc.lx==0);
}
static void test_legacy_hidden_cancel_removed_android_only(void) {
    gameplay_fixture(); dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    REQUIRE(tc.layout.target[DM_TC_MAP].r==0);
    /* The visible cancel now lives at the top right. Probe a corner of the
     * removed legacy square that is outside the actual visible circle. */
    dm_tc_target c=aim_feedback_circle();
    REQUIRE(hypotf(width-52-c.x,8-c.y)>c.r);
    dm_android_touch_event(1,0,width-52,8); REQUIRE(hold_frame(.1)==DM_BTN_A);
    REQUIRE(tc.finger[0].active && !tc.finger[0].aim_cancelled);
    dm_android_touch_cancel_pending(); REQUIRE(hold_frame(.1)==0);
    dm_tc_set_navigation_mode(&tc,1); REQUIRE(hold_frame(.1)==0);
    REQUIRE(tc.layout.target[DM_TC_MAP].r>0);
    float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s); REQUIRE(s[233]==0);
}
static void test_feedback_cancel_lifecycle(void) {
    for(int reason=0;reason<6;++reason) {
        gameplay_fixture(); dm_tc_target k=tc.layout.target[DM_TC_SKILL1];
        dm_android_touch_event(0,0,k.x,k.y); REQUIRE(hold_frame(0)==0);
        REQUIRE(hold_frame(.1)==DM_BTN_B);
        dm_tc_target c=aim_feedback_circle();
        dm_android_touch_event(1,0,c.x,c.y); REQUIRE(hold_frame(.1)==0);
        if(reason==0) dm_android_touch_event(3,0,c.x,c.y);
        if(reason==1) dm_android_touch_focus(0);
        if(reason==2) dm_android_touch_resize(960,440,20,24,16);
        if(reason==3) dm_android_touch_margins(24,48);
        if(reason==4) dm_android_touch_cancel_pending();
        if(reason==5) dm_android_touch_uimsg("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
        REQUIRE(hold_frame(.1)==0);
        float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s); REQUIRE(s[233]==0 && s[234]==0);
        if(reason==5) allow_empty_binding_hit=1; /* Normal read-only panel hit query on the stale UP. */
        dm_android_touch_event(2,0,c.x,c.y); REQUIRE(hold_frame(.1)==0 && !tc.pulse_button && !tc.defer_button);
    }
}
static void test_circle_clear_mainstream_layouts(void) {
    const float sizes[][2]={{667,375},{800,375},{2608.f/3,400},{960,440},{1024,768}};
    for(unsigned n=0;n<sizeof sizes/sizeof sizes[0];++n) for(int above=0;above<2;++above) for(int margin=0;margin<2;++margin) {
        gameplay_fixture(); observed_swap_above=above;
        dm_android_touch_tablet(n==4); dm_android_touch_resize(sizes[n][0],sizes[n][1],20,24,16);
        dm_android_touch_margins(margin?96:0,margin?96:0); REQUIRE(hold_frame(.1)==0);
        dm_tc_target c=aim_feedback_circle(); float k=dm_tc_scale_for_size(width,height);
        REQUIRE(c.x>width*.75f && c.y<height*.25f);
        REQUIRE(tc.layout.target[DM_TC_SKILL4].x-tc.layout.target[DM_TC_SKILL3].x>=79.9f*k);
        REQUIRE(hypotf(tc.layout.target[DM_TC_SKILL2].x-tc.layout.target[DM_TC_SKILL1].x,
                       tc.layout.target[DM_TC_SKILL2].y-tc.layout.target[DM_TC_SKILL1].y)>79*k);
        for(int i=DM_TC_MAIN;i<=DM_TC_SKILL4;++i) {
            dm_tc_target t=tc.layout.target[i];
            /* Actual enlarged ring + rotating arrow's outer point. */
            REQUIRE(t.x+t.r+25*k<=width-safe_right);
            REQUIRE(t.y+t.r+25*k<=height-safe_bottom);
        }
        if(!aim_feedback_circle_clear(c.x,c.y,c.r,k)) fprintf(stderr,"circle_clear size=%u above=%d margin=%d circle=%.1f,%.1f,r%.1f\n",n,above,margin,c.x,c.y,c.r);
        REQUIRE(aim_feedback_circle_clear(c.x,c.y,c.r,k));
        REQUIRE(c.x-c.r>=safe_left && c.x+c.r<=width-safe_right);
        REQUIRE(c.y-c.r>=safe_top && c.y+c.r<=height-safe_bottom);
        float u=tc.layout.height/1320.f;
        for(int row=1;row<4;++row) for(int col=0;col<4;++col) {
            dm_tc_target b=tc.layout.target[DM_TC_BELT1+col];
            float dx=fmaxf(0,fabsf(c.x-b.x)-60*u),dy=fmaxf(0,fabsf(c.y-(b.y-(165+162*(row-1))*u))-53.75f*u);
            REQUIRE(hypotf(dx,dy)>c.r+1);
        }
        dm_tc_target a=tc.layout.target[DM_TC_MAIN];
        dm_android_touch_event(0,0,a.x,a.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
        float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s);
        REQUIRE(s[230]==c.x && s[231]==c.y && s[232]==c.r && s[233]==1);
        dm_android_touch_event(2,0,c.x,c.y); REQUIRE(hold_frame(.1)==0 && !tc.pulse_button);
    }
}
static void test_circle_cache_and_hidden_pending_snapshot(void) {
    gameplay_fixture(); dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s);
    REQUIRE(aim_circle_cached); dm_tc_target before=aim_feedback_circle();
    read_twice(s); REQUIRE(aim_feedback_circle().x==before.x && aim_feedback_circle().y==before.y);
    dm_android_touch_obstructions(14,30,54,314,200);
    read_twice(s); REQUIRE(s[233]==0 && s[200]==0); /* Pending layout cancellation is hidden before guest sample. */
    REQUIRE(hold_frame(.1)==0);
    REQUIRE(!aim_circle_cached);
    main=tc.layout.target[DM_TC_MAIN]; dm_android_touch_event(0,0,main.x,main.y); REQUIRE(hold_frame(.1)==DM_BTN_A);
    read_twice(s); REQUIRE(aim_circle_cached && s[233]==1);
    dm_tc_target c=aim_feedback_circle();
    REQUIRE(aim_feedback_circle_clear(c.x,c.y,c.r,dm_tc_scale_for_size(width,height)));
}
int main(void) {
    run_case("hold_feedback_snapshot_and_release_stop",test_hold_visual_snapshot);
    run_case("all_five_slots_keep_original_hold_and_II_lead",test_five_slots_keep_shared_hold_timing);
    run_case("deadzone_circular_hit_and_sticky_cancel_no_pulse",test_deadzone_and_cancel_circle_exact_hit);
    run_case("visual_direction_matches_actual_saturated_right_axis",test_feedback_direction_matches_saturated_input);
    run_case("same_batch_cancel_retracts_only_its_latch",test_same_batch_cancel_only_its_finger);
    run_case("movement_and_other_skill_survive_local_cancel",test_joystick_and_other_skill_survive_cancel);
    run_case("android_hidden_map_cancel_removed_nav_preserved",test_legacy_hidden_cancel_removed_android_only);
    run_case("cancelled_feedback_clears_on_lifecycle",test_feedback_cancel_lifecycle);
    run_case("cancel_circle_clear_mainstream_sizes_margins_profiles",test_circle_clear_mainstream_layouts);
    run_case("circle_cached_until_actual_layout_cancel",test_circle_cache_and_hidden_pending_snapshot);
    printf("RESULT PASS hold cases=%u assertions=%u\n",cases,checks);
    return 0;
}
