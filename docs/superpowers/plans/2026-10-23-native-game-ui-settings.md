# Native game UI and touch settings integration

> Implement in isolated workers using subagent-driven development, followed by independent SPEC and a different QUALITY reviewer. The user's requested redesign is the approved scope; keep review focused on actual mobile paths.

**Goal:** Replace the added Android toolbar with real game widgets and move screen-edge margins into the game's Custom settings page.

**Baseline:** `bf7b046f0e59ec4b25a4c5e54a0b836aca905f7c`, original upstream `jhaizhou-ops/d2r-mobile` reviewed at `e3756f8942b40eba6c851e881828a5b1bda17fd6`. Bring the reviewed third-party driver module into this package at the user's later explicit request.

## Accepted user revisions

- Game UI only gains panel Sort and Custom touch settings. Map visibility/position and item names remain in the existing touch-button layer, attached to the skill cluster; do not add a new HUD widget strip.
- User explicitly kept the monitor toggle in Custom.
- User explicitly requested third-party drivers in this same APK for joint manual validation. Integrate the previously reviewed driver branch and provide a native Custom driver-management button, using the existing dedicated JNI worker to open the Android document/selection UI.

## Design

- Map visibility, left/center/right position, and item names use the original circle painter in the touch layer, positioned adjacent to the skill fan. Drawing and hit tests use the same native geometry. They disappear with modal panels.
- Sort becomes a game-rendered button inside the inventory / personal-stash panel, using existing panel button styling. Preserve the confirmed inventory/current-personal-page scope, original long-press operation, page-token validation, and original sorting callback.
- The skill-attached touch controls participate before world / attack / map gestures, with down/up capture and cancellation; panel Sort reuses the original mouse/widget path and pins the page token at actual touch-down.
- Remove the added Android toolbar and touch-settings dialog. Preserve the original controller overlay and the passive performance monitor.
- Android Custom settings append `touch_margin_left`, `touch_margin_right` (0–96 logical points, step 1, default 0) and `perf_overlay` (default off). Margins use the existing left/right selectors. Apply commits and saves, Discard preserves applied values, Defaults sets pending defaults and still requires Apply.
- Seed missing new native keys from old SharedPreferences before native configuration is loaded. Existing native-file values always win; avoid UI-thread file reads or reinitialization. Native applied-settings snapshots contain left, right, monitor, and report not-ready until native settings load.
- The Java overlay only reads the applied snapshot and updates margin geometry / monitor state when changed. Pause stops monitoring and cancels input; Resume restores applied monitor state. Preserve logical-point units and safe-area handling.

## Ownership and tasks

- [ ] Settings worker: `dm_custom.c/.h`, Custom section of `touch_layout.py`, `ui_text.tsv` and generated table, focused setting tests. APIs: `void dm_custom_android_seed_legacy(int left,int right,int monitor)` (seed only, no load/IO), `int dm_custom_android_read_applied(int out[3])` (read applied atomics, return 0 before load).
- [ ] Game-widget worker: new product-widget generator module, `gen_hud_v4.py`, native product-widget module, `dm_cursor.c` hooks if required, `touch_android.c/.h`, focused widget/input tests. Keep `touch_layout.py` edits out of this worker; expose a panel augmentation helper for integration. Do not own Java/JNI or Custom/i18n files. Communicate widget message names and cached hit/dispatch APIs.
- [ ] Root: Java/JNI toolbar removal, legacy seed / applied-snapshot binding, performance lifecycle; connect inventory augmentation into `touch_layout.py`; integrate both commits and resolve actual boundary issues.
- [ ] Regenerate HUD/overlay and localized strings, stage a fresh matching import pack, verify APK/overlay fingerprint and unchanged 6931 shaders. Reuse cached graphics only when their generation/output is demonstrably unchanged.
- [ ] Run focused settings Apply/Discard/Defaults/reload/migration, widget scene and pointer cancellation checks, source Release build and lint, independent SPEC then different QUALITY, signing/16KB/content/identity checks. Keep real game layout, item rearrangement, GPU performance and touch feel as user manual acceptance.
- [ ] Publish only user-fork development code and deliver a new APK plus matching import materials to versioned G: and `outputs/` folders, retaining prior packages and saves.
