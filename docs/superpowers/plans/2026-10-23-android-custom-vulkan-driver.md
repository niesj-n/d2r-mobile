# Android custom Vulkan driver implementation plan

> Worker: implement this plan in the dedicated `work/original-driver-v3-20261023` worktree using test-driven changes. Do not modify the v2 delivery checkout or the designated upstream checkout.

**Goal:** Let an Android player import a private ARM64 Vulkan driver package and choose it for the next app process, while preserving the system driver as the default and keeping all Vulkan calls on one selected loader.

**Architecture:** Vendored, pinned libadrenotools loads an isolated Android Vulkan loader; pinned Volk resolves one global dispatch for the presenter and NVN engine. A Java importer owns validated packages and selection. Native startup verifies the selected library was actually loaded and isolates caches.

**Stack:** Android Java/Gradle, C++17/C17, NDK 29, Vulkan 1.1, CMake, libadrenotools and Volk.

## Files and ownership

- `dm/android/app/src/main/java/com/xiaoboluo/d2r/DriverPackageStore.java`: pure package validation, private import and saved choice. `DriverPackageStoreTest.java`: ZIP and persistence tests.
- `DmActivity.java`: a small driver menu in existing settings and startup handoff; leave touch toolbar creation untouched.
- `dm/android/driver_loader.cpp/.h`: selected/system loader opening, path verification, Volk initialization and fallback decisions. `dm/android/tests/test_driver_loader.cpp`: controlled loader tests.
- `dm/android/vulkan_present.cpp`, `jni_android.c`: loader ordering, instance/device dispatch and startup result.
- `dm/gfx/vulkan/CMakeLists.txt`, `dm/android/CMakeLists.txt`, `dm/android/app/build.gradle`: compile/link Vulkan through Volk, build and package the pinned hooks, use extracted native libraries.
- `dm/third_party/{adrenotools,volk}` and `docs/DEPENDENCIES*.md`: exact sources and licenses.

## Task 1: establish dispatch and dependency boundary

- [ ] Write a source/build regression proving current `libvulkan` linkage and a fake-loader test that expects the presenter to use a supplied `vkGetInstanceProcAddr`; run red.
- [ ] Vendor the three pinned upstream source snapshots and licenses. Add Volk with `VK_NO_PROTOTYPES` to Vulkan translation units, remove `vulkan` from links, and add `volkInitializeCustom` / `volkLoadInstance` / `volkLoadDevice` at the instance/device creation boundary.
- [ ] Run the fake-loader test and NDK link check green; inspect `DT_NEEDED` and symbol definitions.

## Task 2: private driver import and selection

- [ ] Write Java tests for path traversal, duplicate entries, unsupported ELF, large archives, interrupted import, digest selection and system default; run red.
- [ ] Implement `DriverPackageStore` with `ACTION_OPEN_DOCUMENT`, bounded streaming extraction into private staging, ARM64 ELF validation, atomic promotion and digest-only preferences. Add a small entry from existing settings; show restart-required text on changes.
- [ ] Run importer tests green and compile the Android Java app.

## Task 3: custom loader and safe fallback

- [ ] Write native tests with a controlled fake AdrenoTools opener and fake Vulkan loader. Check startup happens before any instance, custom and system paths never mix, a silent hook fallback is rejected, and failure clears the selection for next launch; run red.
- [ ] Pass `ApplicationInfo.nativeLibraryDir`, private root and digest through startup; open custom loader only for a validated selection, verify the loaded driver via the process library map after enumeration, and bind Volk to that loader. On early failure, use system; after an instance/hook ambiguity, persist system selection and require a fresh process before game startup.
- [ ] Run native tests green and build the NDK shared library with no unresolved symbols.

## Task 4: cache, package and release gates

- [ ] Write a cache-path test showing different package digests/driver identities cannot share a pipeline cache; run red.
- [ ] Namespace the existing cache files, retain v2 files for the system path, and test reload/rollback.
- [ ] Build an APK, verify all ELF `PT_LOAD` alignments, confirm packaged hooks and 16 KB APK page alignment, inspect app identity/version and run repository checks relevant to touched files. Record exact build commands, artifacts and the absence of a hardware compatibility claim.
