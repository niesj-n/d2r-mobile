/* Standalone regression harness. Do not link it into the product library.
 * Includes the REAL Android bridge and shared touch state machine. No duplicate
 * implementation of layout serialization, hit testing, or key latching.
 * Compile with the repository root on -I. NDK: --target=aarch64-linux-android31 -O2 -ffunction-sections -fdata-sections
 * -I <repo> -Wl,--gc-sections -Wl,--no-undefined -lm.
 * This executable never opens saves, starts the guest, or sends Android input.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dm/android/touch_android.c"
#include "dm/ios/Sources/dm_touch_controls.c"
#define dm_product_ui_touch test_product_ui_real_touch
#include "dm/host/dm_product_ui.c"
#undef dm_product_ui_touch
#ifndef DM_ANDROID_TOUCH_MARGINS_AVAILABLE
#define dm_tc_set_android_edge_margins(l,r) ((void)0)
#define dm_android_touch_margins(l,r) ((void)0)
#endif
#ifndef DM_ANDROID_TOUCH_ACTIONS_AVAILABLE
enum { DM_ANDROID_ACTION_MAP_VISIBILITY=2, DM_ANDROID_ACTION_MAP_POSITION=3,
       DM_ANDROID_ACTION_ITEM_NAMES=4, DM_ANDROID_ACTION_SORT=5 };
#define dm_android_touch_action(action) ((void)(action))
#endif
#ifndef DM_ANDROID_TOUCH_GAMEPLAY_GATE_AVAILABLE
#define dm_android_touch_gameplay() (0)
#endif
#ifndef DM_ANDROID_TOUCH_CANCEL_PENDING_AVAILABLE
#define dm_android_touch_cancel_pending() ((void)0)
#endif

#ifndef DM_ANDROID_TOUCH_OBSTRUCTIONS_AVAILABLE
#define dm_android_touch_obstructions(t,l,y,r,b) ((void)0)
#endif

_Static_assert(DM_ANDROID_TOUCH_GLYPHS==119 && DM_ANDROID_TOUCH_HUD==167 && DM_ANDROID_TOUCH_QUICK==185 && DM_ANDROID_TOUCH_QUICK_COUNT==3, "The legacy prefix offsets must stay stable");
_Static_assert(DM_ANDROID_TOUCH_DRAW_FLOATS == 200, "Review the Java/native ABI before changing this test");
enum { DRAW_JOY = 12, DRAW_SWAP = 13, DRAW_DIFF = 14, DRAW_PICK0 = 15 };
static const char *case_name = "startup";
static unsigned checks, cases;
static uint64_t test_clock_ns = UINT64_C(100000000000), test_sample;
static int observed_panel;
static int product_down_calls, product_cancel_calls;
void dm_product_ui_touch(int phase) { if (phase==0) ++product_down_calls; else if (phase==3) ++product_cancel_calls; else abort(); test_product_ui_real_touch(phase); }

static unsigned panel_calls;
static int map_full, map_mode=1, map_full_calls, map_toggle_calls, map_center_calls, map_position_calls;
static int map_absolute=-1, map_toggles_pending;
static int observed_idle, observed_swap_above, allow_synthetic_touch, synthetic_touch_calls;
static int observed_ingame=1, observed_cursor_pending;
static int sort_context=-1, sort_inventory_calls, sort_personal_calls, sort_cancel_calls, sort_last_token=-1;

#define REQUIRE(expression) do { \
    ++checks; \
    if (!(expression)) { \
        fprintf(stderr, "FAIL %s:%d case=%s expression=%s\n", __FILE__, __LINE__, case_name, #expression); \
        exit(1); \
    } \
} while (0)

static void unexpected_host_call(const char *name) {
    fprintf(stderr, "FAIL case=%s unexpected host path=%s\n", case_name, name);
    exit(2);
}

/* The Android queue test retains dm_android_touch_step(), so its host boundary
 * must link. Neutral read-only state is deliberate. Unrelated mutations fail
 * loudly rather than silently pretending to execute game behavior.
 */
uint64_t now_ns(void) { return test_clock_ns; }
uint64_t dm_input_samples(void) { return test_sample; }
void dm_cursor_clear(void) {}
int dm_cursor_pending(void) { return observed_cursor_pending; }
void dm_ui_hand_idle(int idle) { observed_idle=idle; }
int dm_ui_panel_state(void) { return 0; }
int dm_hud_stack_msg(const char *act, const char *arg) {
    if (!strcmp(act, "OpenPanel") && !strcmp(arg, "PlayerInventoryExpansionLayout")) return 1;
    if (!strcmp(act, "OpenPanel") && !strcmp(arg, "NpcDialogPanel")) return -1;
    return 0;
}
int dm_ui_tap_box(void) { return 0; }
uint32_t dm_ui_cur_stack_id(void) { return 0; }
uint32_t dm_ui_hover_unid(void) { return 0; }
int dm_ui_targeting(void) { return 0; }
#ifndef DM_ANDROID_REAL_FRONTEND_DRAG
int dm_ui_touch_active(void) { return 0; }
#endif
int dm_ui_bind_active(void) { return 0; }
int dm_belt_rows(int *expanded) { if (expanded) *expanded = 0; return 1; }
void dm_hud_get(dm_hud_info *out) { memset(out, 0, sizeof *out); }
int dm_custom_get(const char *key) { return !strcmp(key,"touch_swap_above") ? observed_swap_above : 0; }
void dm_ui_safe_left(int units) { (void)units; }
void dm_ui_safe_right(int units) { (void)units; }
int dm_touch_game_snapshot(dm_tg_snapshot *out) { memset(out,0,sizeof *out); return 0; }
dm_ta_glyph dm_ta_skill(uint32_t id) { (void)id; return (dm_ta_glyph){.status=DM_TA_UNKNOWN,.atlas=-1}; }
dm_ta_glyph dm_ta_current_set(const dm_tg_snapshot *s,uint64_t key,uint64_t now,int set) { (void)s; (void)key; (void)now; (void)set; return dm_ta_skill(0); }
int dm_ta_frame_at(int atlas,int frame,dm_ta_frame *out) { (void)atlas; (void)frame; (void)out; return 0; }
int dm_swapskill_slot(int slot,int *charges) { (void)slot; *charges=0; return 0; }
int dm_ui_minimap_rect(float r[4]) { (void)r; return 0; }
int dm_ui_map_is_full(void) { return map_full; }
void dm_ui_map_full(int on) { map_absolute=on; ++map_full_calls; }
void dm_ui_map_toggle(void) { ++map_toggles_pending; ++map_toggle_calls; }
void dm_ui_map_recenter(void) { ++map_center_calls; }
void dm_ui_map_cycle_position(void) { ++map_position_calls; }
int dm_ui_sort_context(void) { return sort_context; }
void dm_inv_sort_touch(int page) {
    sort_last_token=page;
    if (page==0) ++sort_inventory_calls;
    else if (page>=4 && page<=8) ++sort_personal_calls;
    else unexpected_host_call(__func__);
}
void dm_inv_sort_touch_cancel(void) { ++sort_cancel_calls; }
int dm_ui_map_geom(float *cell) { *cell=1; return map_mode; }
static void map_pump(void) {
    if (map_absolute>=0) map_full=map_absolute;
    if (map_toggles_pending&1) map_full=!map_full;
    map_absolute=-1; map_toggles_pending=0;
}
unsigned dm_world_tap(float x,float y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_world_tap_result(unsigned seq) { (void)seq; return -1; }
uint32_t dm_ui_hit_id(void) { return 0; }
void dm_points_get(dm_points_info *out) { memset(out,0,sizeof *out); }
void dm_stat_add(int stat,int n) { (void)stat; (void)n; unexpected_host_call(__func__); }
void dm_skill_add_selected(void) { unexpected_host_call(__func__); }

void dm_cursor_set_screen(float x, float y, float w, float h) { (void)x; (void)y; (void)w; (void)h; unexpected_host_call(__func__); }
#ifdef DM_ANDROID_REAL_FRONTEND_DRAG
/* Generated directly from dm_cursor.c by test_frontend_drag.py. */
#include "frontend_cursor_drag.inc"
#else
void dm_ui_touch(int phase, float x, float y) { (void)x; (void)y; if (!allow_synthetic_touch) unexpected_host_call(__func__); REQUIRE(phase==0 || phase==2); ++synthetic_touch_calls; }
int dm_ui_fe_drag_ok(void) { unexpected_host_call(__func__); return 0; }
void dm_ui_drag_begin(int kind, float x, float y, float tolerance) { (void)kind; (void)x; (void)y; (void)tolerance; unexpected_host_call(__func__); }
void dm_ui_drag_move(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_drag_end(void) { unexpected_host_call(__func__); }
void dm_ui_drag_lift(float y) { (void)y; unexpected_host_call(__func__); }
#endif
int dm_ui_item_at_now(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_ui_drop_at_now(float x, float lifted, float finger) { (void)x; (void)lifted; (void)finger; unexpected_host_call(__func__); return 0; }
int dm_ui_drop_bag(void) { unexpected_host_call(__func__); return 0; }
int dm_ui_drop_point(float *x, float *y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_ui_origin_point(float *x, float *y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
int dm_ui_bind_skill_hit(float *x, float *y) { (void)x; (void)y; unexpected_host_call(__func__); return 0; }
void dm_ui_bind_begin(uint64_t key) { (void)key; unexpected_host_call(__func__); }
void dm_ui_world_tap(float ux, float uy, float wx, float wy) { (void)ux; (void)uy; (void)wx; (void)wy; unexpected_host_call(__func__); }
void dm_ui_stack_take_at(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_stack_cube_at(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_shop_buy_at(float x, float y) { (void)x; (void)y; unexpected_host_call(__func__); }
void dm_ui_tab_go(void) { unexpected_host_call(__func__); }
void dm_ui_use_item(void) { unexpected_host_call(__func__); }
void dm_ui_identify_hovered(void) { unexpected_host_call(__func__); }
void dm_ui_long_press(void) { unexpected_host_call(__func__); }
void dm_ui_item_tap(void) { unexpected_host_call(__func__); }
void dm_stash_take(int n) { (void)n; unexpected_host_call(__func__); }
void dm_hud_request_players(int n) { (void)n; unexpected_host_call(__func__); }
void dm_belt_expand(int on) { (void)on; unexpected_host_call(__func__); }
void dm_belt_use(int col, int row) { (void)col; (void)row; unexpected_host_call(__func__); }
void dm_shop_sell_cursor(void) { unexpected_host_call(__func__); }
void dm_tp_request(void) { unexpected_host_call(__func__); }

/* 观测真实桥发出的面板模式；不依赖链接器删除未用代码，不链接产品宿主。
 * Observe the real bridge's panel mode without relying on linker garbage collection or linking the product host. */
void dm_uiscreen_set_panel(int panel) {
    REQUIRE(panel==0 || panel==1);
    observed_panel=panel; ++panel_calls;
}
int dm_uiscreen_ingame(void) { return observed_ingame; }
static void fixture(int navigation) {
    dm_android_touch_obstructions(0,0,0,0,0);
    memset(&tc, 0, sizeof tc);
    memset(&scene, 0, sizeof scene);
    memset(queue, 0, sizeof queue);
    head = tail = 0; ready = 0; focused = 1; cancel = resizing = ui_drag = 0;
    action_head = action_tail = 0; action_release = 0;
    memset(quick,0,sizeof quick); memset(quick_flash,0,sizeof quick_flash);
    product_down_calls=product_cancel_calls=0;
    margin_left=margin_right=0;
    width = 800; height = 375; safe_left = safe_right = safe_bottom = 0; tablet_layout = swap_above = 0;
    epoch = world = 1; pulse_epoch = pulse_sample = resume_sample = 0; pulse_hold_start = 0;
    seen_world = seen_mouse = seen_tab = seen_use = seen_ident = seen_long = seen_item = seen_stack = 0;
    frontend = 0; fe_slot = -1; fe_drag = 0; fe_x0 = fe_y0 = 0;
    test_clock_ns = UINT64_C(100000000000); test_sample = 0;
    observed_panel=-1; panel_calls=0;
    observed_idle=-1; observed_swap_above=0; allow_synthetic_touch=synthetic_touch_calls=0;
    dm_product_ui_scene(-1,0); test_product_ui_real_touch(3);
    observed_ingame=1; observed_cursor_pending=0;
    sort_context=-1; sort_inventory_calls=sort_personal_calls=sort_cancel_calls=0; sort_last_token=-1;
    map_full=map_full_calls=map_toggle_calls=map_center_calls=map_position_calls=map_toggles_pending=0;
    map_absolute=-1; map_mode=1;
    dm_tc_set_safe_insets(0, 0); dm_tc_set_edge_bottom(0); dm_tc_set_safe_bottom(0); dm_tc_set_swap_above(0);
    dm_tc_set_android_edge_margins(0, 0);
    dm_tc_set_panel_state(0); dm_tc_set_tap_box(0); dm_tc_set_cur_stack(0); dm_tc_set_hover_unid(0);
    init_locked();
    dm_tc_set_navigation_mode(&tc, navigation);
    dm_tc_enable_targets(&tc, navigation ? dm_tc_nav_mask(1) : ((1u << DM_TC_COUNT) - 1u));
}

static int field(int record, int component) { return 4 + record * 5 + component; }
static int lit(const float *s, int record) { return s[field(record, 4)] != 0; }
static int visible(const float *s, int record) { return s[field(record, 3)] != 0; }

/* Read as #19 does: probe, then actual draw. Both must see the same data and
 * neither read may consume pulse/pressed state. Guard words detect ABI overrun.
 */
static void read_twice(float result[DM_ANDROID_TOUCH_DRAW_FLOATS]) {
    struct guarded { uint32_t left; float values[DM_ANDROID_TOUCH_DRAW_FLOATS]; uint32_t right; } a, b;
    unsigned char before[sizeof tc];
    a.left = b.left = UINT32_C(0x13579bdf); a.right = b.right = UINT32_C(0x2468ace0);
    memset(a.values, 0xa5, sizeof a.values); memset(b.values, 0x5a, sizeof b.values);
    memcpy(before, &tc, sizeof tc);
    dm_android_touch_layout(a.values);
    dm_android_touch_layout(b.values);
    REQUIRE(a.left == UINT32_C(0x13579bdf) && b.left == a.left);
    REQUIRE(a.right == UINT32_C(0x2468ace0) && b.right == a.right);
    REQUIRE(memcmp(a.values, b.values, sizeof a.values) == 0);
    REQUIRE(memcmp(before, &tc, sizeof tc) == 0);
    for (int i = 0; i < DM_ANDROID_TOUCH_DRAW_FLOATS; ++i) REQUIRE(isfinite(a.values[i]));
    memcpy(result, a.values, sizeof a.values);
}

static void unchanged_geometry(const float *a, const float *b) {
    REQUIRE(a[0] == b[0] && a[1] == b[1]);
    for (int i = 0; i < 23; ++i)
        for (int c = 0; c < 3; ++c) REQUIRE(a[field(i, c)] == b[field(i, c)]);
}

static void down_target(int slot, int target, double at) {
    dm_tc_target t = tc.layout.target[target];
    dm_tc_begin_at(&tc, slot, t.x, t.y, at);
    REQUIRE(tc.finger[slot].active && tc.finger[slot].target == target);
}

static void test_idle_and_abi(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a); read_twice(b);
    REQUIRE(memcmp(a, b, sizeof a) == 0);
    REQUIRE(a[0] == 800 && a[1] == 375 && a[2] == 0 && a[3] == 0);
    REQUIRE(visible(a, DRAW_JOY) && visible(a, DRAW_SWAP) && visible(a, DRAW_DIFF));
    for (int i = 0; i < 23; ++i) REQUIRE(!lit(a, i));
}

static void test_press_release_and_multitouch(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    down_target(0, DM_TC_MAIN, 100.0); read_twice(b);
    REQUIRE(lit(b, DM_TC_MAIN)); REQUIRE(memcmp(a, b, sizeof a) != 0);
    unchanged_geometry(a, b);
    down_target(1, DM_TC_SKILL1, 100.01); read_twice(b);
    REQUIRE(lit(b, DM_TC_MAIN) && lit(b, DM_TC_SKILL1));
    dm_tc_cancel_finger(&tc, 0); read_twice(b);
    REQUIRE(!lit(b, DM_TC_MAIN) && lit(b, DM_TC_SKILL1));
    dm_tc_cancel_all(&tc); read_twice(b);
    for (int i = 0; i < 23; ++i) REQUIRE(!lit(b, i));
    down_target(0, DM_TC_MAIN, 101.0);
    dm_tc_end_at(&tc, 0, 101.1); read_twice(b);
    REQUIRE(!lit(b, DM_TC_MAIN) && tc.buttons == 0);
}

static void test_slide_cancel(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; down_target(0, DM_TC_SKILL1, 100.0);
    dm_tc_target map = tc.layout.target[DM_TC_MAP];
    dm_tc_move_at(&tc, 0, map.x, map.y, 100.1);
    REQUIRE(tc.finger[0].active && tc.finger[0].aim_cancelled);
    REQUIRE((tc.buttons & tc.layout.target[DM_TC_SKILL1].button) == 0);
    read_twice(a); REQUIRE(!lit(a, DM_TC_SKILL1));
    dm_tc_end_at(&tc, 0, 100.11); read_twice(a);
    REQUIRE(!lit(a, DM_TC_SKILL1) && tc.pulse_button == 0);
}

static void test_release_pulse(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; down_target(0, DM_TC_SKILL1, 100.0);
    dm_tc_end_at(&tc, 0, 100.01);
    REQUIRE(!tc.finger[0].active && tc.pulse_button == tc.layout.target[DM_TC_SKILL1].button);
    read_twice(a); REQUIRE(lit(a, DM_TC_SKILL1));
    REQUIRE(dm_tc_pulse_end(&tc, tc.pulse_epoch));
    read_twice(b); REQUIRE(!lit(b, DM_TC_SKILL1));
    REQUIRE(memcmp(a, b, sizeof a) != 0); unchanged_geometry(a, b);
}

static void test_swap_and_delayed_pulse(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    dm_tc_target swap = tc.layout.swap;
    dm_tc_begin_at(&tc, 0, swap.x, swap.y, 100.0); read_twice(b);
    REQUIRE(tc.finger[0].target == DM_TC_SWAP && b[3] == 0 && lit(b, DRAW_SWAP));
    REQUIRE(memcmp(a, b, sizeof a) != 0); unchanged_geometry(a, b);
    dm_tc_end_at(&tc, 0, 100.01); read_twice(b);
    REQUIRE(b[3] == 1 && !lit(b, DRAW_SWAP));
    down_target(0, DM_TC_SKILL1, 101.0); dm_tc_end_at(&tc, 0, 101.01);
    REQUIRE(!tc.finger[0].active && tc.defer_button != 0 && tc.pulse_button == 0);
    read_twice(a); REQUIRE(a[3] == 1 && !lit(a, DM_TC_SKILL1));
    dm_tc_tick(&tc, 101.0 + DM_TC_ZL_LEAD_S + 0.01); read_twice(b);
    REQUIRE(tc.defer_button == 0 && tc.pulse_button != 0 && lit(b, DM_TC_SKILL1));
    REQUIRE(memcmp(a, b, sizeof a) != 0); unchanged_geometry(a, b);
    REQUIRE(dm_tc_pulse_end(&tc, tc.pulse_epoch)); read_twice(b); REQUIRE(!lit(b, DM_TC_SKILL1));
    dm_tc_begin_at(&tc, 0, swap.x, swap.y, 102.0); dm_tc_end_at(&tc, 0, 102.01);
    read_twice(b); REQUIRE(b[3] == 0);
}

static void test_difficulty_hit_and_close(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    for (int i = 0; i < 8; ++i) REQUIRE(!visible(a, DRAW_PICK0 + i));
    dm_tc_target diff = tc.layout.diff;
    dm_tc_begin_at(&tc, 0, diff.x, diff.y, 100.0); read_twice(b);
    REQUIRE(tc.diff_open && tc.finger[0].target == DM_TC_DIFF && lit(b, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) { REQUIRE(visible(b, DRAW_PICK0 + i)); REQUIRE(!lit(b, DRAW_PICK0 + i)); }
    REQUIRE(memcmp(a, b, sizeof a) != 0);
    dm_tc_cancel_finger(&tc, 0); REQUIRE(tc.diff_open);
    dm_tc_target pick = tc.layout.pick[3];
    dm_tc_begin_at(&tc, 0, pick.x, pick.y, 100.1); read_twice(b);
    REQUIRE(tc.diff_request == 4 && !tc.diff_open && tc.finger[0].target == DM_TC_DIFF);
    REQUIRE(!lit(b, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) {
        REQUIRE(!visible(b, DRAW_PICK0 + i));
        if (i != 3) REQUIRE(!lit(b, DRAW_PICK0 + i));
    }
    /* Picking immediately collapses the strip. The selected hidden cell's
     * pressed bit may be retained or gated by visibility; no rendered state is
     * asserted for an invisible cell. Incorrectly highlighting DIFF is caught.
     */
    dm_tc_cancel_finger(&tc, 0);
    dm_tc_begin_at(&tc, 0, diff.x, diff.y, 101.0); dm_tc_end_at(&tc, 0, 101.01);
    REQUIRE(tc.diff_open);
    dm_tc_begin_at(&tc, 0, diff.x, diff.y, 101.1); read_twice(b);
    REQUIRE(!tc.diff_open && lit(b, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) REQUIRE(!visible(b, DRAW_PICK0 + i));
}

static void test_belt_diff_target_does_not_light_difficulty(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; tc.belt_rows = 2; tc.belt_open = 1;
    dm_tc_target belt = tc.layout.target[DM_TC_BELT1];
    float row_y = belt.y - 165.0f * tc.layout.height / 1320.0f;
    dm_tc_begin_at(&tc, 0, belt.x, row_y, 100.0);
    REQUIRE(tc.finger[0].active && tc.finger[0].target == DM_TC_DIFF);
    REQUIRE(tc.belt_request != 0); read_twice(a);
    REQUIRE(!lit(a, DRAW_DIFF));
    for (int i = 0; i < 8; ++i) REQUIRE(!visible(a, DRAW_PICK0 + i));
}

static void test_navigation_and_resize(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
    dm_tc_set_navigation_mode(&tc, 1); dm_tc_enable_targets(&tc, dm_tc_nav_mask(1)); read_twice(b);
    REQUIRE(b[2] == 1 && memcmp(a, b, sizeof a) != 0);
    REQUIRE(!visible(b, DRAW_JOY) && !visible(b, DRAW_SWAP) && !visible(b, DRAW_DIFF));
    dm_tc_resize(&tc, 960, 440); read_twice(b);
    REQUIRE(b[0] == 960 && b[1] == 440 && b[2] == 1);
}

static uint64_t queue_sample(void) {
    uint64_t buttons = 0; int32_t lx = 0, ly = 0, rx = 0, ry = 0;
    ++test_sample;
    dm_android_touch_step(test_clock_ns, &buttons, &lx, &ly, &rx, &ry);
    return buttons;
}

static void test_android_queue_ten_short_taps(void) {
    fixture(0); dm_tc_target main = tc.layout.target[DM_TC_MAIN]; unsigned observed = 0;
    for (unsigned i = 0; i < 10; ++i) {
        test_clock_ns += UINT64_C(100000000);
        dm_android_touch_event(0, 0, main.x, main.y);
        test_clock_ns += UINT64_C(5000000);
        dm_android_touch_event(2, 0, main.x, main.y);
        REQUIRE(tail - head == 2);
        uint64_t delivered = queue_sample();
        REQUIRE((delivered & DM_BTN_A) != 0);
        REQUIRE(!tc.finger[0].active && (tc.buttons & DM_BTN_A) == 0);
        ++observed;
        test_clock_ns += UINT64_C(10000000);
        REQUIRE((queue_sample() & DM_BTN_A) == 0);
    }
    REQUIRE(observed == 10);
    printf("DETAIL native_queue_short_taps=%u/10 scope=bridge_samples_not_game_actions\n", observed);
}

static void test_android_focus_cancel(void) {
    fixture(0); float a[DM_ANDROID_TOUCH_DRAW_FLOATS]; dm_tc_target main = tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0, 0, main.x, main.y);
    REQUIRE((queue_sample() & DM_BTN_A) != 0); read_twice(a); REQUIRE(lit(a, DM_TC_MAIN));
    dm_android_touch_focus(0); REQUIRE(queue_sample() == 0); read_twice(a);
    REQUIRE(!lit(a, DM_TC_MAIN) && tc.buttons == 0 && head == tail);
    dm_android_touch_focus(1); REQUIRE(queue_sample() == 0); read_twice(a); REQUIRE(!lit(a, DM_TC_MAIN));
}

/* 同一批内取消不能把旧按下沿带给游戏；另一手指的快按仍有效。
   Cancellation in a batch must revoke the old edge without discarding another finger's quick tap. */
static void test_android_cancelled_latch(void) {
    fixture(0);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN], map=tc.layout.target[DM_TC_MAP];
    dm_android_touch_event(0,0,main.x,main.y);
    dm_android_touch_event(1,0,map.x,map.y);
    dm_android_touch_event(2,0,map.x,map.y);
    REQUIRE((queue_sample() & DM_BTN_A)==0);
    REQUIRE(tc.buttons==0);
    fixture(0); main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y);
    dm_android_touch_event(3,0,main.x,main.y);
    dm_android_touch_event(0,1,main.x,main.y);
    dm_android_touch_event(2,1,main.x,main.y);
    REQUIRE((queue_sample() & DM_BTN_A)!=0);
    REQUIRE((queue_sample() & DM_BTN_A)==0);
}

/* 换组只在原位松手时触发；滑出去再回来的手势仍取消。
   Switching commits on release inside the original target; leaving and returning still cancels. */
static void test_android_swap_release_cancel(void) {
    fixture(0); dm_tc_target swap=tc.layout.swap;
    dm_android_touch_event(0,0,swap.x,swap.y); queue_sample(); REQUIRE(tc.skill_set==0);
    dm_android_touch_event(2,0,swap.x,swap.y); queue_sample(); REQUIRE(tc.skill_set==1);
    dm_android_touch_event(0,0,swap.x,swap.y);
    dm_android_touch_event(1,0,swap.x-100,swap.y);
    dm_android_touch_event(2,0,swap.x,swap.y);
    queue_sample(); REQUIRE(tc.skill_set==1);
}

/* 从加载进入游戏、开关背包、只开NPC对话时，宿主面板模式必须各自正确。
 * Host panel mode must distinguish entering gameplay, opening/closing inventory and an NPC-dialog-only stack. */
static void test_android_panel_observation(void) {
    fixture(0);
    dm_android_touch_uimsg(NULL,"UnloadPanel","LoadScreenPanel");
    REQUIRE(panel_calls==0 && observed_panel==-1);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","LoadScreenPanel");
    REQUIRE(scene.game==1 && tc.navigation_mode==0 && observed_panel==0 && panel_calls==1);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y);
    REQUIRE((queue_sample()&DM_BTN_A)!=0);
    dm_android_touch_event(2,0,main.x,main.y);
    REQUIRE(head!=tail);
    dm_android_touch_uimsg("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
    REQUIRE(tc.navigation_mode!=0 && observed_panel==1 && panel_calls==2);
    REQUIRE(tc.buttons==0 && head==tail && !tc.finger[0].active);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","PlayerInventoryExpansionLayout");
    REQUIRE(tc.navigation_mode==0 && observed_panel==0 && panel_calls==3);
    dm_android_touch_uimsg("PanelManager","OpenPanel","NpcDialogPanel");
    REQUIRE(tc.navigation_mode!=0 && dm_tc_scene_dialog_only(&scene));
    REQUIRE(observed_panel==0 && panel_calls==4);
    dm_android_touch_uimsg("PanelManager","UnloadAll","");
    REQUIRE(tc.navigation_mode==0 && observed_panel==0 && panel_calls==5);
}


static void test_independent_android_margins(void) {
    fixture(0);
    dm_tc_target main0=tc.layout.target[DM_TC_MAIN];
    float joy0=tc.layout.joy_x;
    dm_android_touch_margins(48,0); queue_sample();
    REQUIRE(tc.layout.joy_x>joy0+47 && tc.layout.joy_x<joy0+49);
    REQUIRE(tc.layout.target[DM_TC_MAIN].r==main0.r);
    REQUIRE(tc.layout.target[DM_TC_MAIN].x==main0.x);
    dm_tc_begin_at(&tc, 0, 4, tc.layout.height-40, 101.0);
    REQUIRE(tc.finger[0].target!=DM_TC_JOYSTICK);
    dm_tc_cancel_all(&tc);
    dm_tc_begin_at(&tc, 0, tc.layout.joy_x, tc.layout.joy_y, 102.0);
    REQUIRE(tc.finger[0].target==DM_TC_JOYSTICK);
    dm_tc_cancel_all(&tc);
    dm_android_touch_margins(0,48); queue_sample();
    REQUIRE(tc.layout.joy_x==joy0);
    REQUIRE(tc.layout.target[DM_TC_MAIN].x<main0.x-47 && tc.layout.target[DM_TC_MAIN].x>main0.x-49);
    REQUIRE(tc.layout.target[DM_TC_MAIN].r==main0.r);
    dm_android_touch_margins(0,0); queue_sample();
    REQUIRE(tc.layout.joy_x==joy0 && tc.layout.target[DM_TC_MAIN].x==main0.x);
}

static void test_zero_margin_keeps_original_layout(void) {
    dm_tc_layout before, after;
    dm_tc_set_safe_insets(18,16);
    dm_tc_layout_make(&before,800,375);
    dm_tc_set_android_edge_margins(0,0);
    dm_tc_layout_make(&after,800,375);
    REQUIRE(memcmp(&before,&after,sizeof before)==0);
}

static void test_margins_share_draw_and_hit_geometry(void) {
    fixture(0);
    dm_android_touch_margins(28,44); queue_sample();
    float draw[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(draw);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    REQUIRE(draw[field(DM_TC_MAIN,0)]==main.x);
    REQUIRE(draw[field(DM_TC_MAIN,1)]==main.y);
    REQUIRE(dm_tc_hit(&tc.layout,main.x,main.y)==DM_TC_MAIN);
    dm_android_touch_event(0,0,main.x,main.y);
    REQUIRE((queue_sample()&DM_BTN_A)!=0);
    REQUIRE(tc.finger[0].target==DM_TC_MAIN);
}

static void test_safe_insets_and_tiny_margins(void) {
    fixture(0);
    dm_android_touch_resize(280,180,18,16,0);
    dm_android_touch_margins(96,96); queue_sample();
    float draw[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(draw);
    float left_end=tc.layout.joy_x+tc.layout.joy_radius;
    float right_begin=tc.layout.width;
    REQUIRE(tc.layout.joy_x-tc.layout.joy_radius>=18);
    for(int i=DM_TC_MAIN;i<=DM_TC_PORTAL;i++) {
        dm_tc_target t=tc.layout.target[i];
        REQUIRE(t.x-t.r>=0 && t.x+t.r<=tc.layout.width);
        REQUIRE(t.y-t.r>=0 && t.y+t.r<=tc.layout.height);
        if(t.x-t.r<right_begin) right_begin=t.x-t.r;
    }
    REQUIRE(left_end+2<=right_begin);
    REQUIRE(tc.layout.target[DM_TC_MAIN].x+tc.layout.target[DM_TC_MAIN].r<=tc.layout.width-16);
    REQUIRE(draw[field(DRAW_JOY,0)]==tc.layout.joy_x);
}

static void test_margin_relayout_cancels_touch(void) {
    fixture(0);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,main.x,main.y);
    REQUIRE((queue_sample()&DM_BTN_A)!=0);
    dm_android_touch_event(0,1,main.x,main.y);
    dm_android_touch_margins(0,32);
    REQUIRE(queue_sample()==0);
    REQUIRE(head==tail && tc.buttons==0 && !tc.finger[0].active);
}

static void test_android_action_edges_are_not_merged(void) {
    fixture(0); scene.game=1;
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_VISIBILITY);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_VISIBILITY);
    REQUIRE((queue_sample()&DM_BTN_MINUS)!=0);
    REQUIRE((queue_sample()&DM_BTN_MINUS)==0);
    REQUIRE((queue_sample()&DM_BTN_MINUS)!=0);
    REQUIRE((queue_sample()&DM_BTN_MINUS)==0);
}

static void test_android_map_actions_and_mode_gate(void) {
    fixture(0); scene.game=1;
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    queue_sample(); queue_sample();
    REQUIRE(map_position_calls==2 && map_toggle_calls==0 && map_center_calls==0);
    map_mode=0;
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    queue_sample();
    REQUIRE(map_position_calls==3);
    dm_android_touch_action(1); dm_android_touch_action(5);
    REQUIRE(queue_sample()==0 && map_position_calls==3);

    map_mode=1;
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    dm_android_touch_uimsg("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
    REQUIRE(queue_sample()==0 && map_position_calls==3);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    REQUIRE(queue_sample()==0 && map_position_calls==3);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","PlayerInventoryExpansionLayout");
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    dm_android_touch_focus(0);
    REQUIRE(queue_sample()==0 && map_position_calls==3);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    REQUIRE(queue_sample()==0 && map_position_calls==3);
}

static void test_android_gameplay_visibility_gate(void) {
    fixture(0);
    REQUIRE(!dm_android_touch_gameplay());
    scene.game=1;
    REQUIRE(dm_android_touch_gameplay());
    frontend=1; REQUIRE(!dm_android_touch_gameplay());
    frontend=0; dm_tc_set_navigation_mode(&tc,1);
    REQUIRE(!dm_android_touch_gameplay());
    dm_tc_set_navigation_mode(&tc,0);
    dm_android_touch_focus(0);
    REQUIRE(!dm_android_touch_gameplay());
}

/* Activity.releaseTouches -> TouchOverlay.cancelTouches must clear queued
   toolbar actions even when a dialog or shade does not pause the Activity. */
static void test_android_local_cancel_clears_pending_actions(void) {
    fixture(0); scene.game=1;
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_VISIBILITY);
    dm_android_touch_cancel_pending();
    REQUIRE(queue_sample()==0 && action_head==action_tail);
    REQUIRE(focused==1);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    REQUIRE(queue_sample()==0 && map_position_calls==1);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_VISIBILITY);
    REQUIRE((queue_sample()&DM_BTN_MINUS)!=0);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    dm_android_touch_cancel_pending();
    REQUIRE(queue_sample()==0 && action_release==0 && action_head==action_tail);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    REQUIRE(queue_sample()==0 && map_position_calls==2);
}

static void test_android_sort_only_current_supported_page(void) {
    fixture(1); scene.game=1;
    sort_context=0;
    REQUIRE(dm_android_touch_sort_context()==0);
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    REQUIRE(queue_sample()==0 && sort_inventory_calls==1 && sort_personal_calls==0);
    test_clock_ns += UINT64_C(2000000000);
    REQUIRE((queue_sample() & DM_BTN_L)==0 && labels_ready_at==0 && !labels_done);
    sort_context=4;
    REQUIRE(dm_android_touch_sort_context()==4);
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    REQUIRE(queue_sample()==0 && sort_inventory_calls==1 && sort_personal_calls==1);
    sort_context=0;
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    sort_context=4; /* The tab changed before the guest consumed the tap. */
    REQUIRE(queue_sample()==0 && sort_inventory_calls==1 && sort_personal_calls==1);
    sort_context=5; /* Another personal page, still original container page 4. */
    REQUIRE(dm_android_touch_sort_context()==5);
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    REQUIRE(queue_sample()==0 && sort_personal_calls==2 && sort_last_token==5);
    sort_context=4;
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    sort_context=5;
    REQUIRE(queue_sample()==0 && sort_personal_calls==2); /* Do not sort a newly flipped page. */
    sort_context=-1; /* Shared, advanced, and other unsupported pages. */
    REQUIRE(dm_android_touch_sort_context()==-1);
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    REQUIRE(queue_sample()==0 && sort_personal_calls==2);
    sort_context=4;
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    dm_android_touch_cancel_pending();
    REQUIRE(sort_cancel_calls>0);
    REQUIRE(queue_sample()==0 && sort_personal_calls==2);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_VISIBILITY);
    REQUIRE(queue_sample()==0); /* Gameplay map action stays disabled in a panel. */
    dm_tc_set_navigation_mode(&tc,0); sort_context=0;
    REQUIRE(dm_android_touch_sort_context()==-1);
    dm_android_touch_action(DM_ANDROID_ACTION_SORT);
    REQUIRE(queue_sample()==0 && sort_inventory_calls==1); /* No stale panel action in gameplay. */
}


static void test_android_full_map_preserves_existing_controls(void) {
    fixture(0); scene.game=1; map_full=1;
    dm_tc_target swap=tc.layout.swap;
    REQUIRE(dm_tc_hit(&tc.layout,swap.x,swap.y)==DM_TC_SWAP);
    dm_android_touch_event(0,0,swap.x,swap.y); queue_sample();
    test_clock_ns+=UINT64_C(60000000);
    dm_android_touch_event(2,0,swap.x,swap.y); queue_sample();
    REQUIRE(tc.skill_set==1);
    REQUIRE(map_full_calls==0 && map_position_calls==0 && map_absolute==-1);

    fixture(0); scene.game=1; map_full=1;
    float x=width*.5f,y=height*.2f;
    REQUIRE(dm_tc_hit(&tc.layout,x,y)==DM_TC_NONE);
    dm_android_touch_event(0,0,x,y); queue_sample();
    test_clock_ns+=UINT64_C(60000000);
    dm_android_touch_event(2,0,x,y); queue_sample();
    REQUIRE(map_full_calls==1 && map_absolute==0);
}

static void test_android_item_names_default_and_manual(void) {
    fixture(0); scene.game=1;
    REQUIRE(queue_sample()==0);
    test_clock_ns += UINT64_C(1400000000);
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    test_clock_ns += UINT64_C(200000000);
    REQUIRE((queue_sample() & DM_BTN_L)!=0);
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    scene.game=0; REQUIRE((queue_sample() & DM_BTN_L)==0);
    scene.game=1; REQUIRE((queue_sample() & DM_BTN_L)==0);
    test_clock_ns += UINT64_C(2000000000);
    REQUIRE((queue_sample() & DM_BTN_L)==0);

    fixture(0); scene.game=1;
    dm_android_touch_action(DM_ANDROID_ACTION_ITEM_NAMES);
    REQUIRE((queue_sample() & DM_BTN_L)!=0);
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    dm_android_touch_action(DM_ANDROID_ACTION_ITEM_NAMES);
    REQUIRE((queue_sample() & DM_BTN_L)!=0);
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    test_clock_ns += UINT64_C(2000000000);
    REQUIRE((queue_sample() & DM_BTN_L)==0);
}

static void test_android_default_names_wait_for_map_action(void) {
    fixture(0); scene.game=1;
    REQUIRE(queue_sample()==0);
    test_clock_ns += UINT64_C(1600000000);
    dm_android_touch_action(DM_ANDROID_ACTION_MAP_POSITION);
    REQUIRE(queue_sample()==0 && map_position_calls==1);
    REQUIRE((queue_sample() & DM_BTN_L)!=0);
}

static void test_android_default_names_wait_for_loading_and_cursor(void) {
    fixture(0); scene.game=1; observed_ingame=0;
    REQUIRE(queue_sample()==0);
    test_clock_ns += UINT64_C(2000000000);
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    observed_ingame=1;
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    test_clock_ns += UINT64_C(1600000000);
    observed_cursor_pending=1;
    REQUIRE((queue_sample() & DM_BTN_L)==0);
    observed_cursor_pending=0;
    REQUIRE((queue_sample() & DM_BTN_L)!=0);
}


/* Expected appended ABI, intentionally independent of the new production enum
 * so the unchanged 185-float bridge can execute the RED regression. */
static void test_quick_snapshot_append(void) {
    fixture(0);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","LoadScreenPanel");
    float s[200]={0}; dm_android_touch_layout(s);
    REQUIRE(s[4+DM_TC_MAP*5+3]==0); /* Only the appended map toggle is drawn in gameplay. */
    REQUIRE(s[188]==1); REQUIRE(s[193]==1); REQUIRE(s[198]==1);
    REQUIRE(s[185]<tc.layout.width*.5f && s[186]<tc.layout.height*.25f);
}
static void test_quick_input_regression(void) {
    fixture(0); scene.game=1;
    dm_tc_target map=quick_circle(0); float x=map.x, y=map.y;
    dm_android_touch_event(0,0,x,y); dm_android_touch_event(2,0,x,y);
    REQUIRE(queue_sample()==DM_BTN_MINUS);
    REQUIRE(tc.joy_slot<0 && tc.lx==0 && tc.ly==0 && tc.rx==0 && tc.ry==0);
}


static void quick_tap(int index,int slot) {
    dm_tc_target t=quick_circle(index);
    dm_android_touch_event(0,slot,t.x,t.y); dm_android_touch_event(2,slot,t.x,t.y);
}
static void test_quick_feedback_fifo_and_fullmap(void) {
    fixture(0); scene.game=1; map_full=1;
    float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; dm_tc_target q=quick_circle(2);
    dm_android_touch_event(0,0,q.x,q.y);
    REQUIRE(queue_sample()==0 && observed_idle==0); read_twice(s); REQUIRE(s[199]==1);
    test_clock_ns+=UINT64_C(2000000000);
    REQUIRE(queue_sample()==0 && !labels_done); /* Holding manual labels must defer defaults. */
    dm_android_touch_event(2,0,q.x,q.y);
    REQUIRE(queue_sample()==DM_BTN_L && labels_done); read_twice(s); REQUIRE(s[199]==1);
    REQUIRE(queue_sample()==0);
    test_clock_ns+=UINT64_C(150000000);
    read_twice(s); REQUIRE(s[199]==0);
    REQUIRE(queue_sample()==0); /* Manual choice never gets reversed by the default L. */
    quick_tap(0,0); quick_tap(0,0); quick_tap(1,1); quick_tap(2,2);
    REQUIRE(queue_sample()==DM_BTN_MINUS);
    REQUIRE(queue_sample()==0);
    REQUIRE(queue_sample()==DM_BTN_MINUS);
    REQUIRE(queue_sample()==0);
    REQUIRE(queue_sample()==0 && map_position_calls==1);
    REQUIRE(queue_sample()==DM_BTN_L);
    REQUIRE(queue_sample()==0);
    REQUIRE(map_full_calls==0 && map_full==1 && tc.skill_set==0);
    REQUIRE(tc.lx==0 && tc.ly==0 && tc.rx==0 && tc.ry==0 && tc.joy_slot<0);
    REQUIRE(tc.world_tap_seq==0 && tc.mouse_click_seq==0 && product_down_calls==0);
    dm_tc_target main=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,3,main.x,main.y); quick_tap(1,1);
    REQUIRE(queue_sample()==DM_BTN_A && map_position_calls==2 && tc.finger[3].active);
    dm_android_touch_event(3,3,main.x,main.y); REQUIRE(queue_sample()==0);
}
static void test_quick_slide_up_and_system_cancels(void) {
    fixture(0); scene.game=1; dm_tc_target q=quick_circle(0), main=tc.layout.target[DM_TC_MAIN];
    float s[DM_ANDROID_TOUCH_DRAW_FLOATS];
    dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    read_twice(s); REQUIRE(s[189]==1);
    dm_android_touch_event(1,0,main.x,main.y); REQUIRE(queue_sample()==0);
    read_twice(s); REQUIRE(s[189]==0 && !tc.finger[0].active);
    dm_android_touch_event(1,0,q.x,q.y); dm_android_touch_event(2,0,q.x,q.y);
    REQUIRE(queue_sample()==0 && action_head==action_tail);
    dm_android_touch_event(0,0,q.x,q.y); dm_android_touch_event(2,0,q.x+q.r+1,q.y);
    REQUIRE(queue_sample()==0);
    dm_android_touch_event(0,0,q.x,q.y); dm_android_touch_event(3,0,q.x,q.y);
    REQUIRE(queue_sample()==0);
    dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_focus(0); read_twice(s); REQUIRE(s[188]==0 && s[189]==0);
    REQUIRE(queue_sample()==0 && !quick[0].index);
    dm_android_touch_focus(1); REQUIRE(queue_sample()==0);
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_cancel_pending(); REQUIRE(queue_sample()==0 && !quick[0].index);
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_resize(852,393,20,24,16); REQUIRE(queue_sample()==0 && !quick[0].index);
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==0);
    q=quick_circle(0); dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_margins(24,48); REQUIRE(queue_sample()==0 && !quick[0].index);
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==0);
    q=quick_circle(0); dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_uimsg("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
    read_twice(s); REQUIRE(s[188]==0 && !quick[0].index);
    dm_android_touch_uimsg("PanelManager","UnloadPanel","PlayerInventoryExpansionLayout");
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==0);
}
static void test_quick_visibility_and_native_sort_phases(void) {
    fixture(0); float s[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(s); REQUIRE(s[188]==0);
    scene.game=1; read_twice(s); REQUIRE(s[188]==1);
    observed_ingame=0; read_twice(s); REQUIRE(s[188]==0);
    observed_ingame=1; frontend=1; read_twice(s); REQUIRE(s[188]==0);
    frontend=0; dm_tc_set_navigation_mode(&tc,1); read_twice(s);
    REQUIRE(s[188]==0 && s[193]==0 && s[198]==0);
    dm_product_ui_scene(4,91); REQUIRE(dm_product_ui_sort(4,91)==-1);
    dm_tc_target t=tc.layout.target[DM_TC_MAIN];
    dm_android_touch_event(0,0,t.x,t.y); REQUIRE(queue_sample()==DM_BTN_A);
    REQUIRE(product_down_calls==1 && dm_product_ui_sort(4,91)==4);
    REQUIRE(dm_product_ui_sort(4,91)==-1);
    /* requests() may synthesize a mouse DOWN+UP; it must never latch another physical DOWN. */
    int before_down=product_down_calls;
    allow_synthetic_touch=1; ++tc.mouse_click_seq; requests(); allow_synthetic_touch=0;
    REQUIRE(synthetic_touch_calls==2 && product_down_calls==before_down);
    REQUIRE(queue_sample()==DM_BTN_A && product_down_calls==before_down);
    dm_android_touch_cancel_pending(); REQUIRE(queue_sample()==0);
    REQUIRE(product_cancel_calls>0 && dm_product_ui_sort(4,91)==-1);
    dm_android_touch_event(0,0,t.x,t.y); REQUIRE(queue_sample()==DM_BTN_A);
    dm_product_ui_scene(5,92); REQUIRE(dm_product_ui_sort(5,92)==-1);
    dm_android_touch_event(3,0,t.x,t.y); REQUIRE(queue_sample()==0);
    dm_android_touch_event(0,0,t.x,t.y); REQUIRE(queue_sample()==DM_BTN_A);
    int before_cancel=product_cancel_calls;
    dm_android_touch_event(3,0,t.x,t.y); REQUIRE(queue_sample()==0);
    REQUIRE(product_cancel_calls==before_cancel+1 && dm_product_ui_sort(5,92)==-1);
}
static void test_quick_mainstream_geometry_and_margin_binding(void) {
    const float sizes[][2]={{667,375},{800,375},{2608.0f/3,1200.0f/3},{960,440},{1024,768}};
    for (unsigned n=0;n<sizeof sizes/sizeof sizes[0];++n) for (int above=0;above<2;++above) for (int large_margin=0;large_margin<2;++large_margin) {
        fixture(0); scene.game=1; observed_swap_above=above;
        dm_android_touch_tablet(n==4);
        dm_android_touch_resize(sizes[n][0],sizes[n][1],20,24,16); REQUIRE(queue_sample()==0);
        float a[DM_ANDROID_TOUCH_DRAW_FLOATS], b[DM_ANDROID_TOUCH_DRAW_FLOATS]; read_twice(a);
        dm_tc_target old_main=tc.layout.target[DM_TC_MAIN];
        dm_android_touch_margins(large_margin?96:0,large_margin?96:0); REQUIRE(queue_sample()==0); read_twice(b);
        float shift=tc.layout.target[DM_TC_MAIN].x-old_main.x;
        for (int i=0;i<3;++i) {
            int o=DM_ANDROID_TOUCH_QUICK+i*5; dm_tc_target q=quick_circle(i);
            if (i>0) {
                REQUIRE(fabsf((b[o]-a[o])-shift)<.001f && b[o+1]==a[o+1] && b[o+2]==a[o+2]);
                REQUIRE(fabsf(hypotf(q.x-tc.layout.target[DM_TC_MAIN].x,q.y-tc.layout.target[DM_TC_MAIN].y)-214*dm_tc_scale_for_size(width,height))<.01f);
            } else {
                REQUIRE(b[o]>=a[o] && q.x<tc.layout.width*.5f && q.y<tc.layout.height*.25f);
            }
            REQUIRE(b[o]==q.x && b[o+1]==q.y && b[o+2]==q.r && b[o+3]==1);
            REQUIRE(q.x-q.r>=20 && q.x+q.r<=tc.layout.width-24 && q.y-q.r>=0 && q.y+q.r<tc.layout.height-16);
            for (int t=0;t<DM_TC_COUNT;++t) {
                dm_tc_target r=tc.layout.target[t];
                REQUIRE(hypotf(q.x-r.x,q.y-r.y)>q.r+r.r+1);
            }
            /* Expanded four-row belt hit rectangles, including upper rows. */
            float u=tc.layout.height/1320.0f;
            for (int row=1;row<4;++row) for (int col=0;col<4;++col) {
                dm_tc_target belt=tc.layout.target[DM_TC_BELT1+col];
                float bx=belt.x, by=belt.y-(165+162*(row-1))*u;
                float dx=fmaxf(0,fabsf(q.x-bx)-60*u),dy=fmaxf(0,fabsf(q.y-by)-53.75f*u);
                REQUIRE(hypotf(dx,dy)>q.r+1);
            }
            dm_tc_target swap=tc.layout.swap;
            REQUIRE(hypotf(q.x-swap.x,q.y-swap.y)>q.r+swap.r+1);
            REQUIRE(hypotf(q.x-tc.layout.joy_x,q.y-tc.layout.joy_y)>q.r+tc.layout.joy_radius+1);
            for (int j=i+1;j<3;++j) { dm_tc_target r=quick_circle(j); REQUIRE(hypotf(q.x-r.x,q.y-r.y)>q.r+r.r+1); }
            quick_tap(i,i);
            uint64_t expected=i==0 ? DM_BTN_MINUS : i==2 ? DM_BTN_L : 0;
            REQUIRE(queue_sample()==expected && !tc.finger[i].active && tc.joy_slot<0);
            REQUIRE(queue_sample()==0);
        }
        REQUIRE(map_position_calls==1);
        /* Default 48dp monitor offset leaves room for a 44dp phone target in
           the top strip. Tablet scaling falls back beside the panel. */
        dm_android_touch_obstructions(0,30,48,314,212); REQUIRE(queue_sample()==0);
        dm_tc_target top_map=quick_circle(0);
        REQUIRE(top_map.x+top_map.r<=30 || top_map.x-top_map.r>=314 ||
                top_map.y+top_map.r<=48 || top_map.y-top_map.r>=212);
        if (n!=4) {
            REQUIRE(fabsf(top_map.x-(20+(large_margin?96:0)+26))<.001f);
            REQUIRE(top_map.y==26 && top_map.y+top_map.r==48);
        } else REQUIRE(top_map.x-top_map.r>=324 && fabsf(top_map.y-top_map.r-48)<.001f);
        REQUIRE(fabsf(b[190]-b[195])>20 && fabsf(b[191]-b[196])>20);
        printf("DETAIL quick_geometry size=%.0fx%.0f above=%d margin96=%d main=%.2f,%.2f quick=%.2f,%.2f;r=%.2f;dy=%.2f\n",
            sizes[n][0],sizes[n][1],above,large_margin,tc.layout.target[DM_TC_MAIN].x,tc.layout.target[DM_TC_MAIN].y,
            b[185],b[186],b[187],b[191]-b[186]);
    }
}

static void test_quick_monitor_obstructions_and_cancel(void) {
    fixture(0); scene.game=1;
    dm_android_touch_resize(2608.0f/3,400,20,24,16); REQUIRE(queue_sample()==0);
    float before[200], after[200]; read_twice(before);
    /* Actual monitor dimensions in dp, including safe top and its 48dp offset. */
    dm_android_touch_obstructions(14,30,62,314,226); REQUIRE(queue_sample()==0); read_twice(after);
    dm_tc_target q=quick_circle(0);
    REQUIRE(q.x-q.r==24 && q.y-q.r==18 && q.y+q.r==62);
    REQUIRE(!memcmp(before,after,185*sizeof(float))); /* Preserve original controls and ABI. */
    REQUIRE(after[185]==q.x && after[186]==q.y && after[187]==q.r);
    map_full=1;
    dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_obstructions(14,30,62,314,226); /* Identical inset delivery must not cancel a held icon. */
    read_twice(after); REQUIRE(after[188]==1 && after[189]==1);
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==DM_BTN_MINUS);
    REQUIRE(queue_sample()==0 && map_full_calls==0);
    /* A bounds change cancels a captured DOWN and its subsequent UP. */
    dm_android_touch_event(0,0,q.x,q.y); REQUIRE(queue_sample()==0);
    dm_android_touch_obstructions(14,30,62,340,226);
    read_twice(after); REQUIRE(after[188]==0);
    REQUIRE(queue_sample()==0 && !quick[0].index);
    dm_android_touch_event(2,0,q.x,q.y); REQUIRE(queue_sample()==0 && action_head==action_tail);
    /* Wide-panel fallback stays below its actual bounds and clear of the stick. */
    dm_android_touch_obstructions(14,30,54,840,226); REQUIRE(queue_sample()==0);
    q=quick_circle(0);
    REQUIRE(q.y-q.r>=236 && q.x-q.r>=20 && q.x+q.r<=width-24);
    REQUIRE(hypotf(q.x-tc.layout.joy_x,q.y-tc.layout.joy_y)>q.r+tc.layout.joy_radius);
    quick_tap(0,0); REQUIRE(queue_sample()==DM_BTN_MINUS); REQUIRE(queue_sample()==0);
    dm_android_touch_margins(96,0); REQUIRE(queue_sample()==0);
    dm_android_touch_obstructions(14,0,0,0,0); REQUIRE(queue_sample()==0);
    q=quick_circle(0); REQUIRE(q.x-q.r>=120 && q.y-q.r==18);
    dm_android_touch_obstructions(NAN,NAN,0,INFINITY,10); REQUIRE(queue_sample()==0);
    q=quick_circle(0); REQUIRE(isfinite(q.x) && isfinite(q.y) && q.y-q.r==4);
}

static void run_case(const char *name, void (*body)(void)) {
    case_name = name; body(); ++cases; printf("PASS %s\n", name);
}

int main(void) {
    puts("SCOPE native_layout_and_input_bridge; no_Java_Canvas_no_guest_no_save_access");
    run_case("zero_margin_keeps_original_layout", test_zero_margin_keeps_original_layout);
    run_case("idle_200_abi", test_idle_and_abi);
    run_case("press_release_multitouch", test_press_release_and_multitouch);
    run_case("slide_cancelled_finger", test_slide_cancel);
    run_case("release_pulse_highlight", test_release_pulse);
    run_case("swap_and_delayed_pulse", test_swap_and_delayed_pulse);
    run_case("difficulty_hit_and_close", test_difficulty_hit_and_close);
    run_case("belt_diff_target_no_false_highlight", test_belt_diff_target_does_not_light_difficulty);
    run_case("navigation_and_resize", test_navigation_and_resize);
    run_case("android_queue_ten_short_taps", test_android_queue_ten_short_taps);
    run_case("android_focus_cancel", test_android_focus_cancel);
    run_case("android_cancelled_latch", test_android_cancelled_latch);
    run_case("android_swap_release_cancel", test_android_swap_release_cancel);
    run_case("android_panel_observation", test_android_panel_observation);
    run_case("independent_android_margins", test_independent_android_margins);
    run_case("margins_share_draw_and_hit_geometry", test_margins_share_draw_and_hit_geometry);
    run_case("safe_insets_and_tiny_margins", test_safe_insets_and_tiny_margins);
    run_case("margin_relayout_cancels_touch", test_margin_relayout_cancels_touch);
    run_case("android_action_edges_are_not_merged", test_android_action_edges_are_not_merged);
    run_case("android_map_actions_and_mode_gate", test_android_map_actions_and_mode_gate);
    run_case("android_gameplay_visibility_gate", test_android_gameplay_visibility_gate);
    run_case("android_local_cancel_clears_pending_actions", test_android_local_cancel_clears_pending_actions);
    run_case("android_sort_only_current_supported_page", test_android_sort_only_current_supported_page);
    run_case("android_full_map_preserves_existing_controls", test_android_full_map_preserves_existing_controls);
    run_case("android_item_names_default_and_manual", test_android_item_names_default_and_manual);
    run_case("android_default_names_wait_for_map_action", test_android_default_names_wait_for_map_action);
    run_case("android_default_names_wait_for_loading_and_cursor", test_android_default_names_wait_for_loading_and_cursor);
    if (!getenv("DM_TEST_QUICK_INPUT_RED")) run_case("quick_snapshot_append", test_quick_snapshot_append);
    run_case("quick_input_regression", test_quick_input_regression);
    run_case("quick_feedback_fifo_and_fullmap", test_quick_feedback_fifo_and_fullmap);
    run_case("quick_slide_up_and_system_cancels", test_quick_slide_up_and_system_cancels);
    run_case("quick_visibility_and_native_sort_phases", test_quick_visibility_and_native_sort_phases);
    run_case("quick_mainstream_geometry_and_margin_binding", test_quick_mainstream_geometry_and_margin_binding);
    run_case("quick_monitor_obstructions_and_cancel", test_quick_monitor_obstructions_and_cancel);
    printf("RESULT PASS cases=%u assertions=%u; game_quick_taps_10_of_10=NOT_TESTED\n", cases, checks);
    return 0;
}
