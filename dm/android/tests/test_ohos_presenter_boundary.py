"""Preprocess the shared presenter as HarmonyOS without a device SDK.

The only stand-in is the HiTrace declaration, which is irrelevant to Vulkan.
This is a compile-boundary check, not a full HarmonyOS build.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

dm = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as directory:
    include = Path(directory) / "hitrace"
    include.mkdir()
    (include / "trace.h").write_text("\n", encoding="ascii")
    command = [sys.argv[1], "--target=aarch64-linux-android31", "-U__ANDROID__",
               "-D__OHOS__", "-D_GNU_SOURCE", "-std=c++17", "-E",
               "-I", directory, "-I", str(dm / "android"),
               "-I", str(dm / "host"), "-I", str(dm / "gfx/nvn/include"),
               str(dm / "android/vulkan_present.cpp")]
    source = subprocess.run(command, check=True, capture_output=True, text=True).stdout

for symbol in ("dm_driver_open(", "dm_driver_fallback_system(", "dm_driver_verify(",
               "dm_vk_dispatch_", "dm_driver_cache_key(", "volkInitializeCustom"):
    assert symbol not in source, symbol
assert "vkCreateInstance(&ic" in source
assert "vkCreateDevice(physical" in source
print("PASS OHOS preprocessing excludes Android driver/Volk calls and retains direct Vulkan creation")
