#pragma once
#include <stdint.h>
/* Separate read-only ABI. Never append these fields to the 235-float controls.
 * 0 mode, 1 count, 2 cell pixels, 3/4 real size, 5/6 player world cell,
 * 7/8 player screen position, 9..12 normalized minimap bounds, 13 language
 * group, 14 simplified-Chinese old names, 15 reserved; then 12 x/y/art triples. */
#define DM_ANDROID_MAP_GOAL_FLOATS 52
/* Call active only on the guest owner path after host map/scene sampling.
 * Inactive is safe on lifecycle threads and clears without reading host state. */
void dm_android_map_goal_publish(int active);
/* Java/UI thread: copies only our mutex-protected publication. */
void dm_android_map_goal_snapshot(float out[DM_ANDROID_MAP_GOAL_FLOATS]);
