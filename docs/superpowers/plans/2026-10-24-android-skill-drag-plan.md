# Android skill drag implementation

Base: ordinary 71583457885b97fbed00964c127019edc2ee507a. Work is isolated from the delivered APK and all BC probes.

- [x] Capture the current phone UI and inspect Android queue/shared gesture timing. User subsequently withdrew release-to-cast and requested continuous held casting.
- [x] Run the actual bridge/state baseline: 35 cases, 20849 assertions.
- [x] Update utility geometry assertions to require a smaller aligned pair with no skill/swap/belt overlap; run the actual native harness and observe the expected geometry failure.
- [x] Replace only Android quick-circle placement and Java utility rendering; rerun geometry/input regressions (35 cases / 20809 assertions).
- [x] Keep original held skill timing. Add Android native visible-circle cancellation and tests proving no release make-up pulse, same-batch cancellation and unrelated-finger preservation. The withdrawn release mode/classification/scheduler was not merged.
- [x] Append authoritative per-gesture draw data and cancel target after the existing 200-float snapshot; update Java/JNI size together and test prefix safety (235 floats).
- [x] Render bounded drag displacement and rotated direction rim using existing art; display the same native cancel target that hit testing uses.
- [ ] Run actual bridge, native mode, frontend drag, Java compile and monitor regressions. Independently review native hold/cancel correctness before packaging.
- [ ] Build signed ordinary UI package with increased version, verify source/material/signature/ELF identity, deliver APK and matching material guidance. Install wirelessly after the user saves/exits, then request manual cast/drag validation.

Integrated native checks at 6afc4a9: real Android snapshot 35 cases / 23889 assertions; real hold/cancel queue 10 cases / 12280 assertions. Utility painter early review passed at 5c9a18c. Actual phone casting and visual acceptance remain pending.
