# Android product implementation plan

> Worker: execute with subagent-driven-development; independent worktrees and ownership permit the user-authorized parallel tasks. Review each implementation with SPEC then a fresh QUALITY reviewer.

Goal: source-built baseline plus fixed-scale and independent-margin Android product, with honest device evidence.
Architecture: retain existing Android JNI/shared host/Vulkan path; isolate Android settings from other frontends. Private content remains outside Git.
Stack: C/C++, Java 17, Python 3.12, Android SDK 36, NDK 29.0.14206865, CMake 3.31.6, Gradle 9.8.1 / AGP 9.4.1.

## Work ownership
- Baseline worker: G:/d2r/worktrees/original-baseline-20261023; Android build/tool scripts only if needed, private outputs under G:/d2r/artifacts/original-product-20261023; do not modify gameplay/settings.
- Render worker: G:/d2r/worktrees/original-render-20261023; dm/host/hle_oe.c, dm/host/dm_custom.c and corresponding shared settings declarations/tests. Coordinate if JNI settings need changing; root owns integration.
- Touch worker: G:/d2r/worktrees/original-touch-20261023; Android Java overlay/activity, touch_android bridge, shared touch geometry only with Android opt-in, and corresponding tests. Coordinate new settings API with render worker.
- Root: integration repository, review scheduling, evidence and device capture. No worker pushes or cherry-picks another worker.

## Task 1 — Baseline native build and private import
- [ ] Check git HEAD/status, current tools and running jobs; inventory exact existing generated headers/import material before generating anything.
- [ ] Install missing pinned tool versions through official distributions; record versions and checksums. Do not pass dmPrebuiltJni.
- [ ] Inspect shell_apk.sh and use shell runtime tables/headers so no private art or tables enter APK. Build unchanged e3756f8 native sources with alternate ID com.niesj.d2r.baseline and persistent local signing identity; record any build-only compatibility fixes separately.
- [ ] Run source build, APK signature validation, zipalign -c -P 16 -v 4, and ELF LOAD alignment verification. Check APK does not contain game programs, shader bundles, game textures, saves, or private paths.
- [ ] Build matching original-format android/ import from G:/d2r/dm-work/data/switch/3.3.94012/Update using d2rm_android.py; inspect manifest identity and overlay contract. Keep content private.
- [ ] Deliver explicit version directory under G:/d2r/artifacts and G:/game/GameViewer/Download. Record exact commands/logs and missing gates. Never install over original app.

## Task 2 — Fixed scale settings
- [ ] Inspect actual settings persistence/UI path and controller including GPU override.
- [ ] Add host regression cases for 1.25 acceptance, invalid/NaN rejection, fixed under sustained slow/GPU/thermal inputs, default/reset and persistence; run to expected failure.
- [ ] Implement the design's fixed choices in existing settings flow; Android guards retain other platforms. Reuse renderer rebuild and dimensions log. Ensure UI/Final Result/output dimensions do not change.
- [ ] Run focused host tests and source compilation; record red/green commands, commit, request SPEC then fresh QUALITY. Resolve all actionable findings before integration.

## Task 3 — Left/right margins
- [ ] Inspect Java-to-native resize path and shared draw/hit geometry; select Android settings entry preserving existing touch behavior.
- [ ] Add failing behavior tests for left-only/right-only movement, drawing/hit agreement, safe inset preservation, clamped tiny displays, reset/persistence, and touch cancellation on relayout.
- [ ] Implement bounded independent additional margins (0..96 dp, defaults 0) with a visible Android settings entry; pass margin into shared native layout rather than translating only drawing.
- [ ] Run focused Java/native tests and compile; commit and request SPEC then fresh QUALITY. Root integrates only after findings are resolved.

## Task 4 — Integration, playable evidence, diagnosis and performance
- [ ] Cherry-pick accepted feature commits into work/android-fixed-render-touch, build/lint and recheck signature/alignment/content. Push this user fork branch after verified commits; never push upstream or secrets.
- [ ] Query adb availability/device identity, installed packages, API/driver and import state without simulated input. Prepare manual checklist and capture logs/perf/temperature with source/scene/settings identities.
- [ ] Baseline acceptance: import, hero creation, world entry, movement/combat, sound, save/re-entry, background/resume. Record user/device evidence separately from offline successes.
- [ ] If reproducible black block evidence exists, isolate one causal resource/sync hypothesis and add a regression before bounded fix; otherwise retain an explicit evidence gap.
- [ ] Rank measured long-frame sources using existing instrumentation; fix only demonstrated costs and compare same scene/settings/cache/thermal conditions.
- [ ] Audit current unimplemented touch/lifecycle frequent-use features and write a narrow follow-on plan for the next complete feature family. Continue authorized independent work while awaiting manual device steps.
