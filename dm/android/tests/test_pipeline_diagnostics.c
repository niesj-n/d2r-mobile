/* Exercise the production ring; no Android/driver/guest substitutes in storage. */
#include "../diagnostics_android.h"
#include "../perf_android.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void dm_android_perf_read(uint64_t now, dm_android_perf_snapshot *s) { (void)now; memset(s, 0, sizeof *s); }
int dm_android_perf_enabled(void) { return 0; }
int main(void) {
    char *out = malloc(DM_ANDROID_PIPELINE_LOG_CAPACITY);
    assert(out);
    dm_android_diagnostics_pipeline_log("PRIVATE arbitrary engine output");
    dm_android_diagnostics_pipeline_log(NULL);
    dm_android_diagnostics_pipeline_log("NVN pipeline cache namespace=custom:123");
    dm_android_diagnostics_pipeline_log("NVN pipeline cache load bytes=123 result=accepted");
    dm_android_diagnostics_pipeline_log("NVN warm list=123 enabled=1");
    dm_android_diagnostics_pipeline_log("NVN shader abc stage=1 loaded words=5 ms read=2 rewrite=3 analyse=4 module=5 reused=0");
    dm_android_diagnostics_pipeline_log("NVN slow-pso frame=2 key=123 create_ms=25\nINJECT\rBAD");
    for (int i = 0; i < 200; i++) {
        char line[2048]; memset(line, 'x', sizeof line);
        snprintf(line, 90, "NVN shader %03d stage=1 loaded words=5 ms read=2 rewrite=3 analyse=4 module=5 ", i);
        memset(line + strlen(line), 'x', sizeof line - strlen(line) - 1); line[sizeof line - 1] = 0;
        dm_android_diagnostics_pipeline_log(line);
    }
    for (int i=0;i<200;i++)dm_android_diagnostics_pipeline_log("NVN pipeline cache save bytes=10 new=1 ok=1 ms=2 on=saver");
    size_t n = dm_android_diagnostics_pipeline_copy(out, DM_ANDROID_PIPELINE_LOG_CAPACITY);
    assert(n == strlen(out) && n < DM_ANDROID_PIPELINE_LOG_CAPACITY);
    assert(strstr(out, "namespace=custom:123")); /* Shader traffic must not evict cache/slow evidence. */
    assert(strstr(out, "load bytes=123 result=accepted") && strstr(out, "warm list=123 enabled=1"));
    assert(strstr(out, "slow-pso frame=2"));
    assert(!strstr(out, "\nINJECT") && !strchr(out, '\r'));
    assert(!strstr(out, "PRIVATE") && !strstr(out, "shader 000"));
    assert(strstr(out, "shader 199") && strstr(out, "overwritten=137"));
    for (char *p = out, *end; (end = strchr(p, '\n')); p = end + 1) assert(end - p < 820);
    char tiny[7] = "guard";
    assert(dm_android_diagnostics_pipeline_copy(tiny, 0) == 0 && !strcmp(tiny, "guard"));
    assert(dm_android_diagnostics_pipeline_copy(tiny, 1) == 0 && tiny[0] == 0);
    dm_android_diagnostics_pipeline_copy(tiny, sizeof tiny); assert(tiny[sizeof tiny - 1] == 0);
    assert(dm_android_diagnostics_pipeline_copy(NULL, 100) == 0);
    /* Fill every category with maximum lines: the public export capacity must fit
     * all retained rows plus startup identities and headers, with no tail loss. */
    for (int c=0;c<3;c++)for(int row=0;row<64;row++) {
        char line[1800]; memset(line,'x',sizeof line);
        const char *prefix=c==0?"NVN pipeline cache save ":c==1?"NVN shader key loaded words=5 ":"NVN slow-pso ";
        memcpy(line,prefix,strlen(prefix));line[sizeof line-1]=0;dm_android_diagnostics_pipeline_log(line);
    }
    n=dm_android_diagnostics_pipeline_copy(out,DM_ANDROID_PIPELINE_LOG_CAPACITY);
    assert(n<DM_ANDROID_PIPELINE_LOG_CAPACITY-1 && strstr(out,"category=slow-pso"));
    assert(strstr(out,"namespace=custom:123") && out[n-1]=='\n');
    free(out); puts("PASS production diagnostic ring: filtering, category retention, bounds, sanitizing and truncation");
}
