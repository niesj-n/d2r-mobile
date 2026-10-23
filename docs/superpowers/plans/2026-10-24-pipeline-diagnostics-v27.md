# Bounded pipeline evidence (v2.7)

This change adds evidence for the observed synchronous shader/PSO stall. It does
not claim to reduce or fix that stall. Reviewed original upstream reference:
`jhaizhou-ops/d2r-mobile` at `e3756f8942b40eba6c851e881828a5b1bda17fd6`.

## Runtime and export contract

- Existing Android engine log callback copies only cache, warm-list, loaded-shader
  timing and slow-PSO messages into process memory. It never writes a file, allocates,
  waits for a lock, reads guest memory or queries Vulkan. Contention drops a row and
  increments a reported counter. Other engine output is excluded.
- Cache, shader and slow-PSO categories each retain the last64 rows, at most767 ASCII
  bytes each. Control/non-ASCII bytes become underscores. Namespace, startup cache
  load result and startup warm-list summary have three separate pinned slots so
  subsequent saves cannot evict them. Fixed storage is about148KiB. Sequence numbers
  preserve cross-category ordering; category overwrite and contention counts state
  loss explicitly. A160KiB buffer fits every maximum-size row plus headers.
- The existing `dm-diagnostics-prepare` Java worker calls
  `DiagnosticsExporter.nativePipelineDiagnostics`. JNI copies the bounded snapshot
  and converts it to a Java string on that worker's ART stack. The save worker adds
  `pipeline.log` to the existing ZIP using the existing path scrubber and byte cap.
  The snapshot represents the export request, before destination selection. It is
  lost on process exit; this is not a crash-persistent logging service.
- `pipeline.log` contains identifiers/timings/results only. Shader text, SPIR-V,
  driver-cache binaries and saves are neither read nor exported. No new CMake source
  registration, Activity hook, resource string, UI or control change is required.

## Slow-PSO interpretation

On the existing synchronous graphics/compute miss branch, creation of at least20ms
emits one diagnostic, with a maximum128 events per process. Cache hits and warm
hits do not emit records. Exhausting the diagnostic budget never skips creation.
The existing clock pair is reused; no additional clock reads or GPU query are added.

The diagnostic gives the HUD frame, full serialized-key FNV-1a hash, graphics/compute,
cacheable status, shader bundle identifiers, creation time and warm state sampled
immediately before synchronous creation. `key_stable=0` means the key is incomplete
or a shader identity is unavailable; its hash must not be treated as a stable key.
Hash values are diagnostic identifiers, never substitutes for exact cache equality.

`known` refers to the current process's exact-key warm recorder set. `listed=1`
means the exact key appeared in the startup list; `listed=0` means it did not;
`listed=-1` means unknown/not applicable. A known runtime key can be absent from the
startup list. `queued/building/built/failed/ready` distinguish the tracked startup
record's current phase and pipeline availability. `async_pending` is separate:
pending does not prove that a background worker has begun. Startup diagnostic
indexing is capped at4096 records, with a fixed4KiB phase array and at most4096
small index entries referencing existing records. Beyond that cap, unindexed known
records report `listed=-1`; rendering and warm-list loading remain unchanged.

Warm snapshots/phase changes and fixed-size shader identity copies use the
existing warm mutex. Driver calls, diagnostic record formatting and log callbacks
occur outside that mutex. Extra work occurs on startup, warm
worker transitions, or actual PSO misses; existing hits and per-draw fast paths are
unchanged. The exporter can briefly own the ring lock while formatting; producers
try once and drop instead of waiting. No new renderer disk I/O or fsync is added.

Cache-load records now distinguish missing/open failure, invalid size, short read,
header mismatch and accepted bytes. Existing cache-save bytes/new/ok/ms and shader
read/rewrite/analyse/module timing lines become exportable; shader lines also carry
the HUD frame. Warm-list flush outcomes are reported without changing dirty,
retry, flush scheduling or write behavior. Existing independent persistence risks
remain separate, unproven explanations for the observed stall.

## Offline verification

Use `ZIG_GLOBAL_CACHE_DIR` for the shared Windows compiler cache. The focused C/C++
tests require Zig0.14.1; source harnesses use `DM_TEST_ZIG`,
`DM_TEST_VULKAN_INCLUDE` and an output directory argument. The JNI harness also uses
`DM_TEST_JDK` and executes the extracted production getter/converter with fake ART
and the actual C ring. The existing Java archive test runs with assertions enabled.

- Actual C ring: filtering, NULL inputs, control sanitization, independent retention,
  pinned identities after200 saves, overwrite counts, small buffers and every
  category filled to maximum size.
- Actual C ring concurrency: four producers/60000 attempted writes with concurrent
  snapshots, coherent rows, bounded outputs and nonblocking producer drops.
- Production formatter/hash/budget: canonical full-key hash fixture, graphics/compute,
 20ms boundary,128-event cap, warm fields and output bounds.
- Extracted actual lookup/diagnostic bodies against fake Vulkan: runtime hit, warm
  hit, existing async branch, graphics/compute create, eviction, slow timing,
  preserved returned pipelines and continued creation after diagnostic exhaustion.
- Extracted actual cache-load/warm-state bodies: missing/invalid-size/header mismatch/
  accepted fixture files, new/recorded/listed/disabled/queued/building/failed/built/
  ready/consumed/async states, exact-key collision rejection and index-cap unknown.
- Actual archive class: `pipeline.log` inclusion, unavailable state, byte cap, path
  scrubbing, explicit file whitelist, missing logs and output/close failure propagation.
- NDK29 ARM64 compilation/syntax checks cover the real engine and changed Android
  native sources. No phone/ADB, signed APK build, install, cache clearing, driver
  experiment or measured GPU reproduction is part of this worker's evidence.

The next real capture should compare the same actual driver namespace and resource
set across launches. Matching stable keys plus startup listed/phase and save/load
results can identify a concrete cache target; unseen keys remain first-seen coverage
evidence. Neither interpretation is established by these host tests alone.
