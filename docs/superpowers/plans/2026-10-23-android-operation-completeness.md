# Android operation completeness — design and implementation (2026-10-23)

Authority: user explicitly reports missing Run/Walk and map-centering buttons; identifies map settings as ineffective; asks to inspect other omissions. This extends the approved product plan.

## Findings and behavior
`dm_cursor.c:ui_map_pump` rewrites AutoMapMode=0 to 2 every touch frame. Android must respect the player's persisted full/left/right selection. Add an always reachable gameplay map-size toggle and recenter action, so honoring fullscreen does not remove its return path. Temporary fullscreen/minimap switching does not rewrite saved options. The last explicitly selected minimap side is used for temporary return; use right if none. Preserve iOS/Harmony policy. Recenter runs on the guest-owning thread and resets horizontal scrolling plus the existing vertical alignment correction. Requests outside gameplay are discarded.

Run/Walk must use the game's verified original action. Buttons are visible in gameplay, safe-area aware, do not generate repeated action while held, and are disabled in menus/death/loading. Do not add infinite stamina or force running. Map open/close, map size, and recenter are separate user actions.

## Ownership and sequence
- Root: host map policy/queue, `dm_cursor.c/.h`, `tests/t_android_map_control.c`.
- Touch worker: finish independent margin commit first; then native/Java action buttons and action-queue tests in a second commit.
- Render worker keeps render/settings ownership.

## Implementation steps
- [ ] Add failing host tests for persisted cfg=0, both minimap sides, temporary toggle/return, explicit center, non-request preserving manually panned offsets, and non-gameplay request rejection.
- [ ] Implement the existing map pump policy and Android integration; call the original mode-change callback on the guest thread, never write AutoMapMode during normal operation.
- [ ] Add `void dm_ui_map_recenter(void)` and drain on the existing guest map pump; match existing geometry correction rather than unconditionally setting visual Y=0.
- [ ] Compile/run focused policy tests plus compile the real Android host; record boundaries where no device is present.
- [ ] Add and test gameplay action buttons via existing touch pipeline, with cancel/pause protections and correct side margins.
- [ ] Compare required controller actions against Android touch routes: run/walk, map open/size/center, item labels, weapon/skill set, town portal, belt and mercenary feeding, interaction, panels, character/skill spending, skill rebinding and vendor drag visual. Report implemented/partial/missing separately.
- [ ] Run independent SPEC then a different fresh QUALITY review; integrate and produce APK/import. Actual phone behavior remains manual acceptance by user.
