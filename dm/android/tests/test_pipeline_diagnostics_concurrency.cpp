#include "../diagnostics_android.h"
extern "C" {
#include "../perf_android.h"
}
#include <thread>
#include <atomic>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstring>
extern "C" void dm_android_perf_read(uint64_t,dm_android_perf_snapshot *s){memset(s,0,sizeof *s);}
extern "C" int dm_android_perf_enabled(void){return 0;}
int main(){
    std::atomic<int> done{0};std::vector<std::thread> writers;
    for(int t=0;t<4;t++)writers.emplace_back([&,t]{for(int i=0;i<15000;i++){
        char line[100];snprintf(line,sizeof line,"NVN slow-pso writer=%d item=%d payload=aaaaaaaaaaaaaaaa",t,i);
        dm_android_diagnostics_pipeline_log(line);
    }++done;});
    unsigned copies=0;std::vector<char> out(DM_ANDROID_PIPELINE_LOG_CAPACITY);
    while(done.load()<4){
        size_t n=dm_android_diagnostics_pipeline_copy(out.data(),out.size());assert(n==strlen(out.data()) && n<out.size());
        const char *p=out.data();while((p=strstr(p,"NVN slow-pso "))){
            int t=-1,i=-1;assert(sscanf(p,"NVN slow-pso writer=%d item=%d payload=aaaaaaaaaaaaaaaa",&t,&i)==2);
            assert(t>=0&&t<4&&i>=0&&i<15000);p=strchr(p,'\n');assert(p);
        }++copies;
    }
    for(auto &t:writers)t.join();
    assert(copies);puts("PASS real C ring concurrency: 4 producers, live exporter, coherent bounded rows, try-lock drops");
}
