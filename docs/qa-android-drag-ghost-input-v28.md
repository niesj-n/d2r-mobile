# Merchant/stack drag frontend integration

The existing shop_drag state owns the image request. No new item classification,
purchase/drop command or control target is introduced. The approved guest sampling
path provides its captured origin and shelf grid and lifts the current centre by
the same DM_TC_POINTER_LIFT_PT times scale used by release_pointer.

Publishing the request before reading the renderer rectangle lets the renderer
revoke the old result on a new origin/grid. Ordinary inventory drags do not enter
this path. The dimmed source cell is read from the authentic renderer result;
advanced stacks dim only when dm_ui_stack_left reports exactly one remaining.
Unavailable shelf geometry, release/cancel, focus/scene/navigation changes,
loading/frontend and resize revoke the request and the dim cache.

TouchOverlay polls a separate five-float normalized cache. JNI only copies that
cache under the existing touch lock; it never reads game state or invokes the
renderer. Changed dim bounds participate in invalidation even if the235 control
fields do not change. The original .82 black dim draws below controls. Pausing
clears visible dim state. The original235-float drawing ABI remains unchanged.

Fresh frontend evidence: original-product-v28-20261024/input.

- ghost-red.log fails on the absent active renderer request before implementation.
- test_touch_ghost.log: five actual Android queue/shared-state-machine cases,
  156 assertions; guest item hits and renderer boundary are fake.
- test_touch_snapshot.log:42 cases/24039 assertions; hold13/25855 assertions.
- frontend.log: six actual-source frontend drag regressions pass.
- ndk-syntax.log: actual touch bridge and JNI pass NDK29 ARM64 syntax.
- java-compile.log: actual generated-resource/app Java compilation passes.

Renderer recognition/replay and actual Vulkan/GPU behavior are separate evidence.
The engine implementation must be integrated and independently reviewed before
APK delivery. Real merchant/advanced-stack image appearance and buy/no-buy/drop
behavior remain manual phone acceptance; no phone touch injection is performed.
