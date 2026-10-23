"""Host execution of actual cache-load and warm-snapshot production bodies."""
from pathlib import Path
import os
import subprocess
import sys
from texture_cache_regression import balanced

root = Path(__file__).resolve().parents[1]
source = (root / 'xbg_game.cpp').read_text(encoding='utf-8')
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
load = balanced(source, source.index('std::vector<uint8_t> load_pipeline_cache('))
state = balanced(source, source.index('xbg_diag::Warm diagnostic_state('))
harness = r'''
#include "xbg_pipeline_diagnostics.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cerrno>
std::vector<std::string> logs;
void xbg_logf(const char *fmt,...){char b[1024];va_list a;va_start(a,fmt);vsnprintf(b,sizeof b,fmt,a);va_end(a);logs.emplace_back(b);}
std::string test_path;std::string pipeline_cache_path(){return test_path;}
VkPhysicalDeviceProperties properties{};
namespace warm {
struct Rec{std::vector<std::string> keys;std::vector<uint32_t> words;};
std::mutex mu;bool enabled=true;std::unordered_set<std::string> known,async_pending,async_failed;
std::unordered_map<std::string,VkPipeline> ready;std::vector<Rec> recs;
std::unordered_multimap<uint64_t,size_t> diagnostic_recs;
std::array<unsigned char,xbg_diag::RecordLimit> diagnostic_phases{};
'''
main = r'''
}
void write_bytes(const void *p,size_t n){FILE *f=fopen(test_path.c_str(),"wb");assert(f);assert(fwrite(p,1,n,f)==n);assert(!fclose(f));}
int main(int argc,char **argv){
 assert(argc==2);test_path=argv[1];remove(test_path.c_str());
 assert(load_pipeline_cache().empty() && logs.back().find("result=missing")!=std::string::npos);
 uint32_t bytes[9]={32,1,0x5143,0x1234};properties.vendorID=0x5143;properties.deviceID=0x1234;
 write_bytes(bytes,4);assert(load_pipeline_cache().empty() && logs.back().find("result=invalid_size")!=std::string::npos);
 write_bytes(bytes,sizeof bytes);assert(load_pipeline_cache().size()==sizeof bytes && logs.back().find("result=accepted")!=std::string::npos);
 bytes[2]++;write_bytes(bytes,sizeof bytes);assert(load_pipeline_cache().empty() && logs.back().find("result=header_mismatch")!=std::string::npos);
 remove(test_path.c_str());
 const uint32_t words[]={1,2,3};std::string key(reinterpret_cast<const char*>(words),sizeof words);
 auto s=warm::diagnostic_state(key);assert(!s.known && !s.listed && !s.ready);
 warm::known.insert(key);warm::recs.push_back({{}, {1,2,3}});
 warm::diagnostic_recs.emplace(xbg_diag::bytes_hash(key.data(),key.size()),0);
 s=warm::diagnostic_state(key);assert(s.known && s.listed && s.queued && !s.building && !s.failed);
 warm::enabled=false;s=warm::diagnostic_state(key);assert(s.listed && !s.queued && !s.enabled);
 warm::enabled=true;warm::diagnostic_phases[0]=1;s=warm::diagnostic_state(key);assert(s.building && !s.queued);
 warm::diagnostic_phases[0]=3;s=warm::diagnostic_state(key);assert(s.failed && !s.building);
 warm::diagnostic_phases[0]=2;warm::ready[key]=(VkPipeline)1;s=warm::diagnostic_state(key);assert(s.built && s.ready && !s.failed);
 warm::ready.clear();s=warm::diagnostic_state(key);assert(s.built && !s.ready); // Previously consumed/evicted warm record.
 warm::async_pending.insert(key);s=warm::diagnostic_state(key);assert(s.async_pending && !s.building); // Pending is not proof of running.
 warm::async_failed.insert(key);s=warm::diagnostic_state(key);assert(s.failed);
 // A hash collision in the diagnostic index cannot turn a new exact key into a listed one.
 std::string other=key;other[0]=9;warm::diagnostic_recs.emplace(xbg_diag::bytes_hash(other.data(),other.size()),0);
 s=warm::diagnostic_state(other);assert(!s.listed && !s.known);
 warm::known.insert(other);warm::recs.resize(xbg_diag::RecordLimit+1);s=warm::diagnostic_state(other);assert(s.listed==-1);
 s=warm::diagnostic_state(key);assert(s.listed==1 && s.built); // Indexed records remain precise after cap.
 puts("PASS extracted production cache reads and exact-key warm states: new/recorded/queued/disabled/building/failed/built/ready/async/collision");
}
'''
(out / 'cache_warm_harness.cpp').write_text(harness.split('namespace warm {')[0] + load + '\nnamespace warm {' + harness.split('namespace warm {')[1] + state + main, encoding='utf-8')
subprocess.run([os.environ['DM_TEST_ZIG'], 'c++', '-std=c++17', '-I'+str(root), '-I'+os.environ['DM_TEST_VULKAN_INCLUDE'], str(out / 'cache_warm_harness.cpp'), '-o', str(out / 'cache_warm.exe')], check=True)
subprocess.run([str(out / 'cache_warm.exe'), str(out / 'cache-read-fixture.bin')], check=True)
