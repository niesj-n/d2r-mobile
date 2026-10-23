# Performance monitor implementation plan

> Execute inline with the executing-plans skill. The user has already authorized this exact design and implementation.

**Goal:** readable, smaller adjustable monitor that can be dragged and remembers its position.

**Architecture:** Android-independent panel geometry/drag state lives in PerformanceOverlay.java. PerformanceOverlay draws cached metric strings and maps its actual View bounds to that state. TouchOverlay polls a two-value native style snapshot and DmActivity connects it.

**Stack:** Java 21, Android 36, existing JNI and SharedPreferences.

- [x] Add `dm/android/tests/PerformancePanelStateTest.java`, exercise the production helper with JDK assertions, and observe the missing-feature failure.
- [x] Implement `PerformancePanelState` in `dm/android/app/src/main/java/com/xiaoboluo/d2r/PerformanceOverlay.java`: safe area layout, default 9-font scaling, normalized placement, primary pointer ownership, movement, end and cancel. Run the test and require PASS.
- [x] Extend PerformanceOverlay with `setStyle(int,int)`, parent layout tracking, panel-sized View bounds, opaque cached text, background/border transparency and dedicated position preferences. `onTouchEvent` accepts only in-panel DOWN; MOVE follows the primary pointer; matching UP persists, CANCEL clears ownership.
- [x] Add `MonitorStyleListener` and `nativeReadMonitorStyle(int[])` to TouchOverlay's existing settings poll; no edit to quick-button painters. Wire `performanceOverlay::setStyle` and top ordering in DmActivity; explicitly split FrameLayout motion events.
- [x] Compile all app sources with Java 21/Android 36, run production-state tests and relevant regressions, inspect diff, write `G:/d2r/artifacts/original-product-v25-20261024/monitor/verification.md`, commit cleanly and provide the SHA/interface to the parent for integration.
