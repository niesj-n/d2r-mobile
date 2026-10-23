#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>

// Diagnostic identifiers only: equality and rendering still use the exact PSO key.
namespace xbg_diag {
inline uint64_t bytes_hash(const char *bytes,size_t count) {
    uint64_t h=14695981039346656037ull;
    for(size_t i=0;i<count;i++){h^=uint8_t(bytes[i]);h*=1099511628211ull;}
    return h;
}
inline uint64_t key_hash(const uint32_t *words, uint32_t count) {
    uint64_t h=14695981039346656037ull;
    for(uint32_t i=0;i<count;i++)for(unsigned b=0;b<4;b++){h^=uint8_t(words[i]>>(8*b));h*=1099511628211ull;}
    return h;
}
constexpr size_t RecordLimit=4096;
// listed=-1 means not applicable/unknown (including a capped startup index).
struct Warm {bool known=false,ready=false,queued=false,building=false,failed=false,enabled=false,built=false,async_pending=false;int listed=-1;};
struct Budget {
    unsigned emitted=0;
    bool take(uint64_t ns){if(ns<20000000 || emitted>=128)return false;++emitted;return true;}
};
inline void format(char *out,size_t cap,uint64_t frame,uint64_t hash,bool graphics,bool cacheable,
                   const Warm &w,uint64_t ns,uint32_t count,const uint32_t *stages,const char *const *keys){
    if(!out || !cap)return;
    bool stable=cacheable && count>0;
    for(uint32_t i=0;i<std::min(count,6u);i++)if(!keys || !keys[i] || !strcmp(keys[i],"unknown"))stable=false;
    int n=snprintf(out,cap,"NVN slow-pso frame=%llu key=%016llx kind=%s cacheable=%d key_stable=%d warm_at=before_create known=%d ready=%d queued=%d building=%d failed=%d enabled=%d listed=%d built=%d async_pending=%d create_ms=%.2f shaders=",
        (unsigned long long)frame,(unsigned long long)hash,graphics?"graphics":"compute",int(cacheable),int(stable),
        int(w.known),int(w.ready),int(w.queued),int(w.building),int(w.failed),int(w.enabled),int(w.listed),int(w.built),int(w.async_pending),ns/1e6);
    size_t used=n<0?0:std::min(size_t(n),cap-1);
    for(uint32_t i=0;i<std::min(count,6u) && used+1<cap;i++){
        char key[81];size_t j=0;const char *s=keys && keys[i]?keys[i]:"unknown";
        for(;j+1<sizeof key && s[j];j++){unsigned char c=s[j];key[j]=(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='~'?char(c):'_';}
        key[j]=0;n=snprintf(out+used,cap-used,"%s%u:%s%s",i?",":"",stages?stages[i]:0,key,s[j]?"~":"");
        used+=n<0?0:std::min(size_t(n),cap-used-1);
    }
}
}
