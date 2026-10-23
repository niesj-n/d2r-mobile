# v2 implementation plan — map position, original controls, more FPS

User revision: remove Run/Walk, map quick control means left/center/right positioning, controls must match original visual style. Add selectable FPS beyond 30/60.

## Design
- Remove the added Run/Walk UI/native action. Restore Android import's original forced-running profile (same existing baseline out); preserve v1 artifacts and app/save/data identity.
- Replace ambiguous Size/Center UI with Map visibility and Position (left -> center -> right -> left). A position change is a session quick action; game Options remain the saved startup preference. Apply through the existing game map callback on the guest thread. Preserve command order and discard pending actions on scene/focus cancellation.
- Reuse original TouchOverlay control drawing assets and pressed/caption rendering for all added toolbar controls including settings. No Android rectangular button backgrounds. Keep actual View hit regions and safe cutout positioning.
- FPS choices 30/40/45/60/90/120; default60, 90/120 labeled experimental. Preserve existing eco_fps stored 0=60/1=30; extend safely or migrate explicitly. UI uses arrows, Apply/Discard/Defaults/save/reload. Actual Android pacing uses requested target period capped by display capability, without integer-divisor reductions (120Hz+90target means90target). Dynamic-resolution budget reads the same effective target. Default fixed0.85 and optional dynamic behavior remain.

## Execution and ownership
1. Root owns host map queue/pump in dm_cursor.c/.h and production map regressions. Add failing left-center-right and click-order cases, implement the guest callback path, run native tests.
2. UI worker in original-ui-v2 owns DmActivity/TouchOverlay/new action view, Android touch bridge/JNI/header, strings and layout sizing/tests. Remove RUN and old Size/Center actions; send Map Position cycle via dm_ui_map_cycle_position. Reuse original painting implementation, not approximate colors. Test actual button count/hit/safearea/cancellation.
3. FPS worker in original-fps-v2 owns dm_custom, render-scale/FPS policy header, hle_oe, nvn_pace.h/hle_nvn.c/nvn_vk.c, i18n/settings layout and corresponding tests. Verify 120Hz+90=90 and120Hz+45=45 production pacing, valid values/legacy persistence and dynamic GPU budgets. No public phone FPS achievement claims.
4. Root restores original Android import default without discarding the optional rewriter support; cache must distinguish producer policy, pack_info RUN_POLICY=forced. Match output to baseline source/policy.
5. Independent SPEC then fresh QUALITY of final integrated snapshot. Review realistic operations, avoid unobservable/artificial cases.
6. Build/sign/lint actual source, verify ELF/APK16KB and identity. Deliver versioned v2 APK and matching import in artifacts and GameViewer Download; retain v1, same app ID/signing key allows update, no uninstall/data clearing. Copy deliverables into this chat outputs and push user fork branch only.

No device is attached. All physical map positions, >60FPS, 1.25 GPU behavior, and visual feel remain manual acceptance; no simulated phone input.
