# Android map goal information layer (v2.7)

Reference: only `jhaizhou-ops/d2r-mobile`, reviewed upstream
`e3756f8942b40eba6c851e881828a5b1bda17fd6`, `front_ios.m:mapGoalSync`.
This adds a read-only consumer of the original quest target/entrance table.
It does not add generic waypoint/exit discovery or pathfinding.

## Root integration contract

- Build `dm/android/map_goal_android.c` and `map_goal_android_jni.c`.
- Include `map_goal_android.h` in the existing guest sampling bridge and call
  `dm_android_map_goal_publish(gameplay)` after the host scene/map samples.
  Active sampling must remain on that guest owner path: `dm_screen_real` may
  fall back to guest addresses. Do not invoke active publication from Java.
- Call `dm_android_map_goal_publish(0)` when focus/runtime/scene is invalid.
  This branch reads no host state and invalidates any active sample in flight.
- `dm_map_goals` is the existing bounded, main-thread-published goal copy
  (`dm_world.c`); `dm_ui_map_geom` is a packed atomic; the minimap rectangle
  is the existing atomic publication from the real AutoMap panel. No guest
  pointers are retained. Our own mutex protects a separate 52-float copy.
  JNI only copies this publication. The 235-float controls ABI stays unchanged.
- Add `MapGoalOverlay(Context)` as a full-surface passive child, below actual
  controls and monitor. Supply runtime enable, safe insets, monitor rectangle,
  the exact 235-float control layout and Rift info bounds via `setInfoBounds`.
  All rectangle setters receive view pixels. Existing controls are only read.
- Stage `mapgoal/` into imported overlay using `stage_touch_assets.py`;
  `--map-goal` can point to cached/generated assets. Default inference uses
  `hud.parent.parent / mapgoal`, preserving the existing `d2rm_android` caller.
  Missing required art, invalid JSON identity, corrupt/truncated PNGs and links
  are rejected before copying. Only generator names are copied; fonts/extras
  are excluded. Existing manifest byte hashes cover all map goal files and
  startup rejects modified payloads. The changed stage source updates the
  existing source fingerprint closure; no new generator dependencies were added.

## Geometry and fallback

Uses the original isometric factors 1.6 / 0.8, current real pixel cell scale,
actual minimap rectangle, full-map player feet position, original map edge
clamping, label/symbol heights and full-map lower-right skill exclusion.
Visible control bounds (including quick actions/cancel), safe insets, monitor
and Rift info are additional obstructions. Goal crosses/symbols keep their
projected location and hide when obstructed; plaques choose the nearest clear
position within map bounds and hide if none fits. True tombs use original
symbol frames only while inside; offscreen they use label 0 without a cross.

Language groups and simplified-Chinese restart-scoped old names match the
original. Missing localized/old labels fall back to the original Chinese label;
missing art is skipped. No system font, fabricated caption or substitute symbol.

## Offline verification and device boundaries

Tests exercise native inactive/no-read behavior, invalid/empty snapshots,
concurrent publication and lifecycle cancellation; Java tests exercise all map
positions, projection, edge/skill-region clamping, true tomb/locales, missing-art
fallback, panel hiding and actual obstruction rectangles. Synthetic staging
checks cover missing/corrupt/extra art and manifest tampering. Compile the real
native module/JNI with NDK and both Java files against the Android SDK.

The focused original generator can be run through `dm/tools/winlf/run.py` for
canonical LF JSON output. This does not require regenerating shaders/imports.
No private images, fonts, JSON strings or game files belong in Git/APK.

Device checks still required: real host target/entrance plaques, seven-tomb symbol
identity, mini left/center/right, full map, panel/frontend/focus hide, wide/tablet
bounds, monitor/Rift obstruction, and missing-art runtime fallback. Offline
success does not establish real game appearance or discovery completeness.
