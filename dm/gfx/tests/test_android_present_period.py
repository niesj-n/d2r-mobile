"""Compile the production Android present-period path with minimal platform stubs."""
from pathlib import Path
import os
import re
import subprocess
import tempfile


def function(source, signature):
    match = re.search(re.escape(signature) + r"\s*\{", source)
    if match is None:
        raise AssertionError(signature)
    start = match.start()
    brace = source.index("{", start)
    depth = 0
    for end in range(brace, len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if not depth:
                return source[start:end + 1]
    raise AssertionError(signature)


root = Path(__file__).resolve().parents[2]
hle = (root / "gfx/nvn/hle_nvn.c").read_text(encoding="utf-8")
vk = (root / "gfx/vulkan/nvn_vk.c").read_text(encoding="utf-8")
parts = [
    function(vk, "double dm_pace_hz(double screen_hz, double floor_hz)"),
    function(vk, "int mtl_fps_target(void)"),
    function(hle, "static double present_auto_hz(double screen_hz)"),
    function(hle, "static uint64_t present_period(const win_t *o)"),
]
prefix = r'''
#define __ANDROID__ 1
#ifdef NDEBUG
#error Android present-period regression requires active assertions
#endif
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include "nvn_pace.h"
#define LOG(...) ((void)0)
typedef struct { int interval; } win_t;
static int g_hz = 60, g_display_mode, g_hz_auto = 1;
static double screen;
static int selected;
static void present_cfg(const win_t *o) { (void)o; }
double mtl_display_refresh(void) { return screen; }
int dm_fps_setting(void) { return selected; }
'''
suffix = r'''
int main(void) {
    win_t window = { .interval = 1 };
    struct { double refresh; int target; double expected; } cases[] = {
        {120, 90, 90}, {120, 45, 45}, {120, 120, 120},
        {120, 40, 40}, {60, 90, 60}, {0, 90, 90},
        {144, 90, 90}, {120, 999, 60}
    };
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; ++i) {
        screen = cases[i].refresh; selected = cases[i].target;
        uint64_t got = present_period(&window);
        uint64_t want = (uint64_t)(1e9 / cases[i].expected);
        if (got != want) return (int)i + 1;
    }
    return 0;
}
'''
compiler = os.environ.get("DM_TEST_CC", "zig cc").split()
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp) / "present.c"
    program = Path(temp) / "present.exe"
    source.write_text(prefix + "\n".join(parts) + suffix, encoding="utf-8")
    subprocess.run(compiler + ["-O2", "-UNDEBUG", "-I", str(root / "gfx/nvn"), str(source), "-o", str(program)], check=True)
    subprocess.run([str(program)], check=True)
print("PASS production Android present_period")
