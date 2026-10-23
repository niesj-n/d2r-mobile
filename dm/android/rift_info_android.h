#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { DM_ANDROID_RIFT_INFO_FLOATS = 18 };
#define DM_ANDROID_RIFT_INFO_MAX_AGE_NS UINT64_C(1000000000)
/* Client/lifecycle gate only. False clears without accessing guest state.
 * A false->true transition requires a new owner publication before visible. */
void dm_android_rift_info_set_active(int active);
/* SERVER owner only, immediately after dm_rift_tick. Never call from JNI or
 * the client controller thread: dm_rift_hud has plain server fields. A gate
 * generation check prevents a revoked in-flight read from becoming visible. */
void dm_android_rift_info_publish(void);
/* Client UI sampling path publishes normalized small-map bounds; NULL clears.
 * Signed top-lift coordinates are clipped for obstruction only. Full map must
 * pass NULL. No controls or map positions are changed. */
void dm_android_rift_info_set_minimap(const float normalized[4]);
/* Read-only coherent copy; monotonic now_ns is supplied by JNI, not guest time.
 * [visible,tier,progress,secs_left,done,guardian,inc,next,overtime,goal,
 *  gdead,gtotal,wave,nwave,mini_left,mini_top,mini_right,mini_bottom].
 * This separate ABI does not append to the accepted 235-float controls. */
void dm_android_rift_info_read(uint64_t now_ns, float out[DM_ANDROID_RIFT_INFO_FLOATS]);
#ifdef __cplusplus
}
#endif
