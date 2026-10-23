/* Actual Android queue and shared controls, renderer/guest host boundaries fake. */
#define main original_snapshot_main
#include "test_touch_snapshot.c"
#undef main
#ifndef DM_ANDROID_TOUCH_GHOST_AVAILABLE
static void dm_android_touch_ghost(float out[5]) { memset(out,0,5*sizeof(float)); }
#endif
static void ghost_fixture(int kind) {
    panel_fixture(); scene.game=1; observed_item_kind=kind;
    dm_android_touch_event(0,0,400,120); queue_sample();
    REQUIRE(!ghost_request_on && !tc.finger[0].shop_drag);
    dm_android_touch_event(1,0,460,145); test_clock_ns+=UINT64_C(20000000); queue_sample();
}
static void read_dim(float out[5]) {
    struct { uint32_t a;float f[5];uint32_t b; } s={.a=0x12345678,.b=0x87654321};
    int calls=shelf_grid_calls,rects=ghost_rect_calls,requests=ghost_request_calls;
    dm_android_touch_ghost(s.f);
    REQUIRE(s.a==0x12345678 && s.b==0x87654321);
    REQUIRE(calls==shelf_grid_calls && rects==ghost_rect_calls && requests==ghost_request_calls);
    for (int i=0;i<5;i++) REQUIRE(isfinite(s.f[i]));
    memcpy(out,s.f,sizeof s.f);
}
static void test_origin_centre_and_read_only_dim(void) {
    ghost_fixture(7);
    REQUIRE(tc.finger[0].shop_drag && ghost_request_on);
    REQUIRE(fabsf(ghost_origin[0]-.5f)<.00001f && fabsf(ghost_origin[1]-.32f)<.00001f);
    REQUIRE(fabsf(ghost_centre[0]-460/800.f)<.00001f);
    float lift=DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(width,height);
    REQUIRE(fabsf(ghost_centre[1]-(145-lift)/height)<.00001f);
    REQUIRE(ghost_grid[2]==.04f && ghost_grid[3]==.08f);
    float dim[5],again[5];read_dim(dim);REQUIRE(dim[0]==0);
    ghost_rect_available=1;queue_sample();read_dim(dim);read_dim(again);
    REQUIRE(dim[0]==1 && memcmp(dim,again,sizeof dim)==0);
    REQUIRE(memcmp(dim+1,ghost_rect_source,sizeof ghost_rect_source)==0);
    dm_android_touch_event(1,0,520,160);queue_sample();
    REQUIRE(fabsf(ghost_centre[0]-.65f)<.00001f && ghost_origin[0]==.5f);
    dm_android_touch_event(2,0,520,160);queue_sample();
    REQUIRE(!ghost_request_on);read_dim(dim);REQUIRE(dim[0]==0);
}
static void test_advanced_stack_dim_and_other_items(void) {
    ghost_fixture(8);REQUIRE(ghost_request_on && tc.pointer_stack);
    ghost_rect_available=1;queue_sample();float dim[5];read_dim(dim);REQUIRE(dim[0]==0);
    stack_left_value=1;queue_sample();read_dim(dim);REQUIRE(dim[0]==1);
    stack_left_value=-1;queue_sample();read_dim(dim);REQUIRE(dim[0]==0);
    dm_android_touch_event(3,0,460,145);queue_sample();REQUIRE(!ghost_request_on);
    ghost_fixture(1);REQUIRE(!ghost_request_on && !tc.finger[0].shop_drag);
    REQUIRE(shelf_grid_calls==0);dm_android_touch_cancel_pending();queue_sample();
}
static void test_immediate_cancel_focus_and_loading(void) {
    ghost_fixture(7);ghost_rect_available=1;queue_sample();
    dm_android_touch_cancel_pending();REQUIRE(!ghost_request_on);float dim[5];read_dim(dim);REQUIRE(!dim[0]);
    ghost_fixture(7);dm_android_touch_focus(0);REQUIRE(!ghost_request_on);read_dim(dim);REQUIRE(!dim[0]);
    ghost_fixture(7);observed_ingame=0;queue_sample();REQUIRE(!ghost_request_on);read_dim(dim);REQUIRE(!dim[0]);
    ghost_fixture(7);frontend=1;queue_sample();REQUIRE(!ghost_request_on);read_dim(dim);REQUIRE(!dim[0]);
    ghost_fixture(7);dm_android_touch_resize(900,400,0,0,0);queue_sample();
    REQUIRE(!ghost_request_on);read_dim(dim);REQUIRE(!dim[0]);
}
static void test_pointer_ownership_and_lifted_edge(void) {
    ghost_fixture(7);REQUIRE(tc.pointer_slot==0 && ghost_request_on);
    dm_android_touch_event(0,1,500,150);queue_sample();
    dm_android_touch_event(1,1,550,100);queue_sample();
    REQUIRE(tc.pointer_slot==0 && ghost_request_on && ghost_centre[0]==460/800.f);
    dm_android_touch_event(2,1,550,100);queue_sample();REQUIRE(ghost_request_on);
    dm_android_touch_event(1,0,500,10);queue_sample();
    REQUIRE(ghost_centre[1]<0 && ghost_centre[1]==(10-DM_TC_POINTER_LIFT_PT*dm_tc_scale_for_size(width,height))/height);
    dm_android_touch_event(3,0,500,10);queue_sample();REQUIRE(!ghost_request_on);
}
static void test_grid_unavailable_clears_and_recovers(void) {
    ghost_fixture(7);ghost_rect_available=1;queue_sample();
    shelf_grid_have=0;queue_sample();REQUIRE(!ghost_request_on);float dim[5];read_dim(dim);REQUIRE(!dim[0]);
    shelf_grid_have=1;queue_sample();REQUIRE(ghost_request_on);read_dim(dim);REQUIRE(!dim[0]);
    dm_android_touch_event(3,0,460,145);queue_sample();REQUIRE(!ghost_request_on);
}
int main(void) {
    run_case("ghost_origin_centre_and_read_only_dim",test_origin_centre_and_read_only_dim);
    run_case("ghost_stack_dim_and_other_items",test_advanced_stack_dim_and_other_items);
    run_case("ghost_immediate_cancel_focus_loading",test_immediate_cancel_focus_and_loading);
    run_case("ghost_grid_unavailable",test_grid_unavailable_clears_and_recovers);
    run_case("ghost_pointer_ownership_and_lifted_edge",test_pointer_ownership_and_lifted_edge);
    printf("RESULT PASS ghost cases=%u assertions=%u; real_queue_fake_guest_renderer\n",cases,checks);
}
