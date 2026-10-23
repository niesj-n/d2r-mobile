# Android skill drag implementation

Base: ordinary 71583457885b97fbed00964c127019edc2ee507a. Work is isolated from the delivered APK and all BC probes.

- [x] Capture the current phone UI and inspect Android queue/shared gesture timing. User approved release-to-cast.
- [x] Run the actual bridge/state baseline: 35 cases, 20849 assertions.
- [ ] Update utility geometry assertions to require a smaller aligned pair with no skill/swap/belt overlap; run the actual native harness and observe the expected geometry failure.
- [ ] Replace only Android quick-circle placement and Java utility rendering; rerun geometry/input regressions.
- [ ] Verify available binding semantics before choosing per-skill timing. Add native tests for no pre-release key, one release pulse, cancel/no pulse and modifier lead. Keep shared non-Android defaults unchanged.
- [ ] Append authoritative per-gesture draw data and cancel target after the existing 200-float snapshot; update Java/JNI size together and test prefix safety.
- [ ] Render bounded drag displacement and rotated direction rim using existing art; display the same native cancel target that hit testing uses.
- [ ] Run actual bridge, native mode, frontend drag, Java compile and monitor regressions. Independently review native release/cancel correctness before packaging.
- [ ] Build signed ordinary UI package with increased version, verify source/material/signature/ELF identity, deliver APK and matching material guidance. Install wirelessly after the user saves/exits, then request manual cast/drag validation.
