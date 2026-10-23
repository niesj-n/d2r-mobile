# Bounded Vulkan shop drag replay

Reference: the original `jhaizhou-ops/d2r-mobile` at
`e3756f8942b40eba6c851e881828a5b1bda17fd6`, specifically
`nvn_mtl.m`'s `ghost_note` / `ghost_flush` and `front_ios.m`'s
`shopGhostSync`. No alternate fork or moving upstream revision is used.

The host publishes `mtl_ui_ghost(on, pressX, pressY, grid4, centerX,
centerY)` and obtains the last authentic source rectangle through
`mtl_ui_ghost_rect(rect4)`. Coordinates are normalized within the UI rectangle.
The existing public signatures remain unchanged. Vulkan's shim forwards to
thread-safe `xbg_ui_ghost` / `xbg_ui_ghost_rect`; Android frontend wiring is a
separate change. Ordinary game-held inventory items retain their existing path.

The renderer recognizes only active, original nonindexed four-vertex UI draws
with two reflected inputs, one 80-byte buffer, 20-byte stride, position float3
at offset 0 and UV float2 at offset 12. Instanced/nonzero-first layouts,
additional geometry stages, storage and texel descriptors, writable shaders,
and unresolved GPU-produced CPU ranges fail closed. The fragment texture is
read from the shader's reflected sampled slot in NVN namespace 1, not from a
stale slot or Vulkan stage number 4.

Recognition reads the exact VB and uniform-buffer snapshots already prepared
for this draw. The matrix is copied while binding the actual vertex UBO 0;
the active eligible path bypasses descriptor memoization so this observation
cannot use a previous draw's matrix. Idle draws take one atomic gate read and
retain the previous memo behavior. The projected rectangle must contain the
press point, have its four edges aligned to the supplied cell grid within 0.15
cell, and occupy 1..2 by 1..4 cells. Up to four minimum-area candidates and 128
saturating texture counts are retained. Untracked textures are rejected when
the count table fills. The least-used candidate texture wins, with stable
submission order for ties. This remains the original heuristic, not an item-ID
lookup or a guarantee that every game UI uses this bounded layout.

After the real draw is recorded, the candidate copies its actual Vulkan
pipeline, descriptor set, dynamic offsets, VB/IB bindings, viewport, scissor
and draw arguments. For an original primitive-7 quad this is the already
expanded indexed draw, preserving base vertex and instance arguments. The
replay does not call the translator recursively or revisit guest memory. It
translates the recorded viewport, opens the scissor to the attachment, records
one draw, and restores all known bindings. Unknown restoration state is
rejected. Only viewport/scissor are dynamic in this pipeline; blend constants,
depth bias and stencil states are baked into the captured pipeline.

`end_open_pass` ends the game's active occlusion query before replay and ends
the render pass after replay. The host draw therefore does not change game
samples-passed results. Candidates are reset at every pass end and match a
monotonic pass serial plus gesture epoch. A separate control mutex guards the
complete input/result and the bounded recording operation: when cancellation
returns, an old gesture cannot record or publish a later rectangle. Closing,
changing press/grid, renderer initialization and renderer shutdown revoke it.
Moving the desired center keeps the same gesture epoch.

The captured bytes remain stable through replay: VB80 is below the resident
vertex-cache threshold and uses immutable recording-slot arena copies; UBO
snapshots use slot arenas; driver/support/c1 blocks bind immutable allocations.
Storage and pending GPU views are excluded. Descriptor sets are immutable once
recorded and stay in the slot's descriptor list until its fence retires.
Expanded indices also live in that arena. Transient/evicted pipelines and
replaced images use the existing fence retirement lists. Replay uses the same
command buffer/pass before those lists or arenas can retire.

Tests execute production selection, control, viewport translation and replay
against a fake Vulkan command boundary. Extracted production functions cover
actual bind capture, reflection slots, layout gating, range readiness and pass
integration ordering. Tests cover minimum area, background/icon frequency,
bounds, snapshot isolation, indexed arguments, restoration, cancellation races,
stale epochs/passes and UI-height conversion. The modified engine and Vulkan
shim are additionally compiled with Android NDK 29. These are CPU/compile
checks; no device pixels, GPU driver behavior, FPS, signed APK or phone run is
claimed. The Android UI aspect callback remains unregistered/default 1; iOS
sources are unchanged.
