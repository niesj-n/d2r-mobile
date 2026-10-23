#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* UI线程仅排队；step必须挂在dm_pad_hook游戏采样线程。 | UI only queues; step must run on the guest sampling thread through dm_pad_hook. */
void dm_android_touch_resize(float width_dp, float height_dp, float left_dp, float right_dp, float bottom_dp);
void dm_android_touch_tablet(int tablet);
void dm_android_touch_margins(float left_dp, float right_dp);
#define DM_ANDROID_TOUCH_OBSTRUCTIONS_AVAILABLE 1
/* Same dp bounds that PerformanceOverlay draws; zero rectangle when hidden. */
void dm_android_touch_obstructions(float safe_top_dp, float left_dp, float top_dp, float right_dp, float bottom_dp);
#define DM_ANDROID_TOUCH_ACTIONS_AVAILABLE 1
enum { DM_ANDROID_ACTION_MAP_VISIBILITY=2, DM_ANDROID_ACTION_MAP_POSITION=3,
       DM_ANDROID_ACTION_LOOT_FILTER=4, DM_ANDROID_ACTION_SORT=5 };
void dm_android_touch_action(int action);
int dm_android_touch_sort_context(void);  // 0背包、4..8个人仓库页快照，其他-1 | Inventory or personal-page snapshot, else -1
#define DM_ANDROID_TOUCH_CANCEL_PENDING_AVAILABLE 1
void dm_android_touch_cancel_pending(void);
#define DM_ANDROID_TOUCH_GAMEPLAY_GATE_AVAILABLE 1
int dm_android_touch_gameplay(void);
/* phase: 0 down, 1 move, 2 up, 3 cancel; slot: stable 0..15; coordinates in dp. | 阶段：按下/移动/抬起/取消；槽固定0..15；坐标dp。 */
void dm_android_touch_event(int phase, int slot, float x, float y);
void dm_android_touch_focus(int active);
void dm_android_touch_uimsg(const char *cat, const char *act, const char *arg);
void dm_android_touch_step(uint64_t guest_now, uint64_t *buttons, int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);
/* 绘图快照：header[width,height,navigation,skill_set]，23个[x,y,r,enabled,pressed]；12键、摇杆、换组、难度及8选择格。 | Drawing snapshot: header followed by 23 records (12 keys, stick, set switch, difficulty, 8 choices). */
/* 后接六个技能图标记录（状态、图集、裁切矩形、副手/充能、次数）和18个HUD数值。
   Append six icon records (status, atlas, crop, secondary/charge flags, charges) and 18 HUD values. */
enum { DM_ANDROID_TOUCH_GLYPHS = 4 + 23 * 5, DM_ANDROID_TOUCH_HUD = DM_ANDROID_TOUCH_GLYPHS + 6 * 8,
       DM_ANDROID_TOUCH_QUICK = DM_ANDROID_TOUCH_HUD + 18, DM_ANDROID_TOUCH_QUICK_COUNT = 3,
       DM_ANDROID_TOUCH_AIM = DM_ANDROID_TOUCH_QUICK + DM_ANDROID_TOUCH_QUICK_COUNT * 5,
       DM_ANDROID_TOUCH_CANCEL = DM_ANDROID_TOUCH_AIM + 5 * 6,
       DM_ANDROID_TOUCH_DRAW_FLOATS = DM_ANDROID_TOUCH_CANCEL + 5 };
/* Android aim extension: five [active, thumb_dx_dp, thumb_dy_dp, screen_ux,
 * screen_uy, cancel_hover] records; then [cancel_cx,cy,r,visible,hover].
 * Screen directions use +Y down. The existing first 200 floats are stable. */
/* Appended quick hit circles [x,y,r,visible,pressed]: bare map icon, map position, loot filter. */
void dm_android_touch_layout(float out[DM_ANDROID_TOUCH_DRAW_FLOATS]);
#ifdef __cplusplus
}
#endif
