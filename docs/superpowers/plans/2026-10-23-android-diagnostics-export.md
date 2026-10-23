# Android diagnostics export plan (parent-approved 2026-10-23)

Goal: the existing TouchShareLog pause-menu control saves a bounded local ZIP through ACTION_CREATE_DOCUMENT.

Files: DiagnosticsArchive.java owns strict allowlist/tail/sanitization/ZIP I/O; DiagnosticsExporter.java owns Android picker/background save/status; diagnostics_android.[ch] owns immutable startup and existing perf snapshots; front_android.c registers the hook; text_input_jni.c dispatches only on its original-stack worker. DmActivity forwards to the exporter. Vulkan initialization caches only already queried properties and actual loaded-driver mode. No gameplay assets, keys, saves, directory walks, networking, or iOS changes.

Alternatives: TXT reduces packaging but loses per-file boundaries; system share sheet allows external sends. ZIP plus a local save picker provides explicit bounded entries and user-controlled destination.

- [x] Red: Java behavioral tests for allowlist, tail truncation, symlink rejection, output/close failures.
- [x] Green: bounded ZIP module and error propagation.
- [x] Red/green: missing native wiring fails, then actual production queue callback runs on worker rather than requesting thread; existing jobs survive.
- [x] Wire original menu hook, cached metadata, Android picker and background write.
- [x] Verify Java tests, NDK compilation, Android resource/Java build and documented device limitations.

Native source identity: compiled overlay fingerprint and git revision/dirty state where available; Android version/product/build type. GPU/Vulkan properties come from the existing initialization query; FPS/frame/GPU/scale are the existing cached optional performance sample. Missing/stale/disabled values stay unknown. perf.txt is in filesDir (cwd), boot.log is in the selected runtime dm directory; crash stderr is captured in boot.log, with no separate crash file guaranteed.

Validation: source-built Android debug APK and lint passed (0 errors; 54 pre-existing/general warnings). Pure-JDK ZIP tests and actual native queue execution passed. Independent review found the standard Android parent-path alias issue; the archive now resolves the trusted root before checking the allowed leaf, keeps NOFOLLOW_LINKS for the read, and tests a real parent alias. No device was available: picker/provider UI, GPU behavior and black-block symptoms require real-device acceptance; no rendering root cause is claimed.
