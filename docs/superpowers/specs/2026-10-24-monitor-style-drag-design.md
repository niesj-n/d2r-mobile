# Performance monitor style and position

Authorized scope: extend the installed v2.4 monitor with smaller default text, adjustable background transparency, direct dragging and remembered placement. Original reference is `jhaizhou-ops/d2r-mobile` at `e3756f8`; implementation base is `44c7b1e`.

Default font is 9 (range 8–14). Transparency is 45 (range 0–80), with alpha equal to `1 - transparency / 100`. Only the background and border fade. Panel dimensions, text and internal padding scale from the former 11-font layout; the safe left 10 dp and top 48 dp reservation remain. Sampling retains its cached 500 ms cadence.

The monitor is the topmost, panel-sized child of the game FrameLayout. FrameLayout splits pointer events, so a finger outside the panel reaches TouchOverlay even during a monitor drag. The first pointer starting inside owns the drag; secondary pointers never replace it. UP saves normalized available-travel coordinates to dedicated `performance_overlay_position` Android preferences. CANCEL, hide, detach and loss of focus release ownership. Size, insets and font changes clamp the panel and restore the normalized placement.

BoundsListener reports the actual parent-coordinate panel rectangle only when it changes or visibility changes. It continues through TouchOverlay's existing native obstruction channel, keeping map drawing and hit testing in agreement. No JNI bounds query is added to drawing. Native style is read with `nativeReadMonitorStyle(int[2])` at the existing settings poll and listeners fire only for changed values; the existing three-value settings ABI is unchanged.

Validation compiles every actual app Java source against Android 36 with Java 21. A JDK executable test targets the production Android-independent panel state in PerformanceOverlay.java, covering outside/inside hit capture, dragging, clamping, normalized restoration, primary-pointer release and cancellation. Source review covers View dispatch, top ordering, persistence and native bounds wiring. No phone operations or version changes are included.
