"""Exercise the production Android GPU controller with the selected FPS cadence."""
from pathlib import Path
import os
import re
import subprocess
import tempfile


def function(source, signature):
    match = re.search(re.escape(signature) + r"\s*\{", source)
    assert match, signature
    start = match.start()
    depth = 0
    for end in range(source.index("{", start), len(source)):
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
            if not depth:
                return source[start:end + 1]
    raise AssertionError(signature)


root = Path(__file__).resolve().parents[2]
oe = (root / "host/hle_oe.c").read_text(encoding="utf-8")
vk = (root / "gfx/vulkan/nvn_vk.c").read_text(encoding="utf-8")
prefix = r'''
#define __ANDROID__ 1
#ifdef NDEBUG
#error Android GPU-budget regression requires active assertions
#endif
#include <assert.h>
#include <math.h>
#include <stdatomic.h>
#include "dm_render_scale.h"
#include "nvn_pace.h"
#define LOG(...) ((void)0)
static _Atomic float g_s3g_scale;
static _Atomic int g_s3g_reset;
static double screen = 120, gpu_ms;
static int selected;
static float s3_cfg(void) { return 1.25f; }
int dm_ingame_now(void) { return 1; }
double xbg_recent_gpu_ms(void) { return gpu_ms; }
double mtl_display_refresh(void) { return screen; }
int dm_fps_setting(void) { return selected; }
'''
suffix = r'''
static void sample(int fps, double frame, double gpu) {
    selected = fps; gpu_ms = gpu; atomic_store(&g_s3g_reset, 1);
    s3g_feed(0); s3g_feed(0);  /* consume the first-load and explicit reset flags */
    for (int window = 0; window < 5; ++window)
        for (int i = 0; i < 2000 / (int)frame; ++i) s3g_feed(frame);
}
int main(void) {
    sample(90, 20, 18);
    assert(fabsf(atomic_load(&g_s3g_scale) - 1.25f * .92f) < .0001f);
    sample(45, 20, 18);  /* below 45 FPS GPU budget, so no quality reduction */
    assert(atomic_load(&g_s3g_scale) == 0);
    sample(90, 40, 18);  /* 40 ms frame remains CPU bound */
    assert(atomic_load(&g_s3g_scale) == 0);
    return 0;
}
'''
source = prefix + "\n".join([
    function(vk, "double dm_pace_hz(double screen_hz, double floor_hz)"),
    function(vk, "int mtl_fps_target(void)"),
    function(oe, "static void s3g_feed(double iv_ms)"),
]) + suffix
compiler = os.environ.get("DM_TEST_CC", "zig cc").split()
with tempfile.TemporaryDirectory() as temp:
    code = Path(temp) / "budget.c"
    program = Path(temp) / "budget.exe"
    code.write_text(source, encoding="utf-8")
    subprocess.run(compiler + ["-O2", "-UNDEBUG", "-I", str(root / "host"),
                    "-I", str(root / "gfx/nvn"), str(code), "-o", str(program)], check=True)
    subprocess.run([str(program)], check=True)
print("PASS production Android GPU budget at 45/90 FPS and CPU-bound guard")
