# Android passive information integration

The accepted v2.6.4 controls keep their positions, hit geometry, held casting,
cancel behavior and 235-float snapshot. TouchOverlay forwards a copy of that
existing snapshot to two passive sibling views. These views return false for
touches; PerformanceOverlay remains above them and the controls.

Map targets are sampled only by the guest controller path. Rift plain fields
are sampled only by the server immediately after dm_rift_tick; the controller
publishes an activity gate and cached small-map obstruction. JNI only reads
module-owned coherent caches. Focus loss, cancellation and scene changes
revoke publication; pause, surface destruction and startup errors hide views.
Full or hidden maps clear only the Rift small-map obstruction. Rift actual
occupied bounds feed map label avoidance, and both views receive monitor
bounds and cutout insets in pixels. Existing control snapshots remain in dp.

Offline evidence lives in original-product-v27-20261024:

- cursor/touch-info-red.log: routing test fails before integration.
- cursor/touch-info-green.log: 42 cases, 24039 assertions, including activity,
  full-map, cancellation, focus, navigation and frontend publication gates.
- cursor/hold-info.log: 13 cases, 25855 assertions; accepted geometry and held
  casting/cancel regressions remain green.
- cursor/frontend-info.log: six real-source frontend drag tests pass.
- integration-native.log: NDK29 actual touch bridge syntax succeeds.
- integration-java.log: actual generated resources and app Java compile pass.

This evidence does not verify a real Rift run or map target presentation on
the phone. Those remain manual acceptance, including readability, map
position/full-map changes, inventory transitions and background/resume.
