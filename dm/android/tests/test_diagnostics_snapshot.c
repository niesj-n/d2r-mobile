#include "../diagnostics_android.h"
#include "../perf_android.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions required
#endif
static int sample;
void dm_android_perf_read(uint64_t now, dm_android_perf_snapshot *s) {
    (void)now; memset(s, 0, sizeof *s);
    s->valid = sample; s->fps = 59.5; s->frame_ms = 16.8; s->gpu_ms = -1; s->scale = .85;
}
int dm_android_perf_enabled(void) { return sample; }
int main(void) {
    char metadata[4096];
    dm_android_diagnostics_snapshot(metadata, sizeof metadata, 1);
    assert(strstr(metadata, "gpu_name=unknown\n"));
    assert(strstr(metadata, "fps=unknown\n"));
    assert(strstr(metadata, "target_dimensions=unknown\n"));
    assert(strstr(metadata, "not measured display scanout"));
    dm_android_diagnostics_configure("/private/runtime", "overlay-fingerprint", "revision-dirty");
    assert(!strcmp(dm_android_diagnostics_runtime(), "/private/runtime"));
    dm_android_diagnostics_gpu("Adreno Test", 0x00403000, 0x1234, 0, NULL, NULL);
    dm_android_diagnostics_snapshot(metadata, sizeof metadata, 1);
    assert(strstr(metadata, "loaded_driver=system\n"));
    assert(strstr(metadata, "overlay_source_sha256=overlay-fingerprint\n"));
    assert(strstr(metadata, "source_revision=revision-dirty\n"));
    assert(!strstr(metadata, "/private"));
    sample = 1;
    dm_android_diagnostics_gpu("Adreno Test", 0x00403000, 0x1234, 1, "abcd", "vulkan.custom.so");
    dm_android_diagnostics_snapshot(metadata, sizeof metadata, 2);
    assert(strstr(metadata, "loaded_driver=custom\n"));
    assert(strstr(metadata, "loaded_driver_digest=abcd\n"));
    assert(strstr(metadata, "fps=59.500\n"));
    assert(strstr(metadata, "gpu_ms=unknown\n"));
    assert(strstr(metadata, "scale=0.850\n"));
    puts("PASS cached driver identity, native source identity and missing/stale metric semantics");
}
