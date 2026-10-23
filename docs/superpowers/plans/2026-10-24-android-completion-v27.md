# Android completion batch v2.7

Approved by the user: arrange remaining work autonomously and parallelize independent tasks. Base4dbeaef447e3c63bd22ad4b4255e814d201b9df5 (installed v2.6.4, UI accepted as approximately right). Preserve its uniform skill layout,235-float drawing ABI, held casting, selected third-party driver and existing data. No save-readiness confirmation is required for future wireless updates.

## Work ownership and interfaces

- Root owns integration, cursor-parking adaptation, all changes to touch_android.c/.h, TouchOverlay.java, DmActivity.java and CMakeLists.txt. Native additions publish only read-only information and must not alter the accepted controls or Rift/map gameplay.
- Rift worker owns new rift_info_android.c/.h/JNI, RiftInfoOverlay.java and focused tests, plus Android resource string selection in gen_strings.py. Investigate actual producer/thread ownership first. Provide dm_android_rift_info_publish(int active): inactive clears without reading guest state; active is called from the approved guest/owner path, never Java. Its own coherent read-only snapshot/JNI must not append fields to235. Overlay methods: setRuntimeEnabled(boolean), setSafeInsets(int,int,int,int), setMonitorBounds(float,float,float,float), setControlLayout(float[]) using the existing235 schema. Use original host state/text, transparent non-touchable View, original palette; system bold fallback is permitted without introducing private fonts into APK/Git.
- Map worker owns new map_goal_android.c/.h/JNI, MapGoalOverlay.java and focused tests; stage_touch_assets.py and map-art preparation/contract inputs as required. Provide dm_android_map_goal_publish(int active) with the same inactive rule and separate snapshot. No edit to shared touch/Activity/CMake root files. Preserve original map projection, target data, all map positions and full-map behavior. Overlay has the same four setters; both info views must avoid actual controls and monitor. Generate/stage original goal labels/symbols from existing private materials; no new gameplay or fake targets.
- Pipeline worker owns bounded slow-pipeline diagnostic instrumentation and its log/export path in Vulkan engine/Android diagnostics, plus focused tests. Start from the existing505ms investigation. Record stable keys, warm/cache status and slow timings with bounded overhead; no per-draw logging, GPU queries, async skipped draws, cache clearing or BC experiments. Diagnose rather than claim this changes observed performance. Do not alter power/CPU-affinity policies.
- Only root operates the phone. Workers use isolated worktrees, commit their reviewed scope and return hashes/evidence. New file lists and integration calls are reported to root before final integration. Changes to any shared file outside ownership require root coordination.

## Sequence and verification

- [x] Rebase/adapt previously reviewed cursor candidate8e36b58 onto the current touch bridge and current235 tests. Preserve Sort physical-DOWN ownership, held gestures, fallback captions and accepted geometry. Production-queue regressions pass; independent SPEC passes, quality review pending.
- [ ] Rift snapshot/format/layout: exercise inactive, progress, Guardian/waves, overtime, completion/next-tier and stale/hidden states; preserve host seconds and advancement rather than recomputing them in Java. Validate publication threading before implementation.
- [ ] Map snapshot/coordinates/assets: exercise targets inside/outside map, left/center/right/full map, true-tomb symbols, no targets, actual UI obstructions and missing art. Keep all existing map controls and fog behavior.
- [ ] Pipeline diagnostics: real-source tests of bounded log formatting/copying and no hot-path I/O; independently inspect privacy, lock ordering and unchanged rendering results. Clearly label diagnosis as not a measured performance fix.
- [ ] For each lane, root verifies the diff and tests; independent specification review then a different quality review before inclusion. Root resolves narrow integration conflicts and wires views/lifecycle/publish calls without changing control geometry.
- [ ] Compile/lint the actual signed ordinary APK with the next version code. Verify source SHA, persistent signer,3 ARM64 libraries/16KB alignment, clean content, effective driver selection and no BC probes.
- [ ] If staging/resources change, regenerate matching overlay manifest/fingerprint and the private import ZIP using existing cached runtime/shaders. Verify package/material identities, deploy the needed delta and ZIP to the user's phone via wireless ADB, then install in place without asking save readiness. Preserve original app, data and saves.
- [ ] Record manual validation boundaries: real Rift progression/completion, map targets/tomb symbols, shop/skill tooltip and diagnosis capture remain distinct from offline success. Follow with remaining merchant/stacked-storage drag-ghost work as a separately bounded renderer feature; do not hide that remaining scope.

Deferred by the user: high-FPS power optimization. System-driver artifact experiments stay paused; use the selected purple driver workaround. QQ integration and phone touch injection remain disabled.

## Integration refinements after producer inspection

Rift's plain state belongs to the server. Its actual API is therefore
`set_active(int)` on the client/lifecycle path, `publish(void)` immediately
after the server's existing `dm_rift_tick`, and `set_minimap(float[4])` for
atomic small-map obstruction. JNI reads only the coherent host cache. Never
call its HUD getter from Java or the controller thread. The Android-only weak
server hook does not acquire the touch mutex.

Map targets match the original fixed table: quest goals, supported entrances
and true-tomb symbols. This does not add discovery for every exit/waypoint.
All 195 original map art files were regenerated and byte-compared with cache.
Only matching overlay/art material changes require distribution; runtime
modules and all 6931 imported shader bundles remain reusable.

Integrated sources so far: cursor be579e2, map0f84e30, Rift112ba49,
lifecycle wiring605a51e. Native route42/24037, hold13/25855, actual frontend
drag6, NDK touch syntax and actual app Java/resource compilation pass.
Device Rift/map acceptance remains pending. Merchant/advanced-storage ghost
replay follows this information/diagnostics package as its own renderer change.
