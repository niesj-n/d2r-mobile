#include "../xbg_pipeline_diagnostics.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
    const uint32_t words[] = {0x12345678, 0xabcdef01};
    auto h = xbg_diag::key_hash(words, 2);
    assert(h == 0xf11e576566c04b95ull); // Independent canonical-byte FNV-1a fixture.
    assert(h != xbg_diag::key_hash(words, 1));
    xbg_diag::Budget budget;
    assert(!budget.take(19999999));
    for (unsigned i=0; i<128; i++) assert(budget.take(20000000));
    assert(!budget.take(500000000));
    xbg_diag::Warm state; state.known=true; state.building=true;
    const char *keys[] = {"abc\nINJECT", "0123456789"};
    uint32_t stages[] = {1,16}; char line[768];
    xbg_diag::format(line, sizeof line, 74706, h, true, true, state, 485800000, 2, stages, keys);
    assert(strstr(line, "frame=74706") && strstr(line, "create_ms=485.80"));
    assert(strstr(line, "known=1 ready=0 queued=0 building=1 failed=0"));
    assert(strstr(line, "shaders=1:abc_INJECT,16:0123456789"));
    assert(!strchr(line,'\n'));
    state = {}; state.failed=true;
    xbg_diag::format(line, sizeof line, 1, h, false, false, state, 20000000, 1, stages, keys);
    assert(strstr(line, "kind=compute cacheable=0 key_stable=0"));
    assert(strstr(line, "failed=1") && strstr(line, "create_ms=20.00"));
    char tiny[3]; xbg_diag::format(tiny, sizeof tiny, 1, h, true, true, state, 1, 0, nullptr, nullptr); assert(!tiny[2]);
    puts("PASS production slow-PSO hash, threshold/budget, warm states, graphics/compute format and bounded strings");
}
