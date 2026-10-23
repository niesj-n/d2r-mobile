"""Compile the actual PSO lookup/diagnostic bodies against a fake Vulkan boundary.

No GPU claims: this exercises production cache/miss/warm/slow branches and tests
that instrumentation preserves returned pipelines, recording and creation counts.
"""
from pathlib import Path
import os
import subprocess
import sys
from texture_cache_regression import balanced

root = Path(__file__).resolve().parents[1]
source = (root / 'xbg_game.cpp').read_text(encoding='utf-8')
assert 'void pipeline_diagnostic(' in source, 'Missing bounded slow-PSO production instrumentation'
diag = balanced(source, source.index('void pipeline_diagnostic('))
lookup = balanced(source, source.index('void pipeline_lookup('))
assert not any(s in diag for s in ['fopen', 'fwrite', 'fsync', 'vkGet', 'vkCreate'])
android = root.parents[2] / 'android'
ring = (android / 'diagnostics_android.c').read_text(encoding='utf-8')
producer = balanced(ring, ring.index('void dm_android_diagnostics_pipeline_log('))
assert 'while' not in producer and not any(s in producer for s in ['malloc', 'fopen', 'fwrite', 'fsync'])
assert 'dm_android_diagnostics_pipeline_log(text)' in (android / 'vulkan_present.cpp').read_text(encoding='utf-8')
assert 'DiagnosticsExporter_nativePipelineDiagnostics' in (android / 'text_input_jni.c').read_text(encoding='utf-8')

out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
harness = r'''
#include "xbg_pipeline_key.h"
#include "xbg_pipeline_diagnostics.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>
#include <cassert>
#include <cstdarg>
struct PipelineLease {VkPipeline pipeline=VK_NULL_HANDLE;bool transient=false;};
struct PipelineEntry {VkPipeline pipeline;uint64_t last_used;};
struct {uint64_t hits=0,misses=0,bypass=0,evictions=0;} pipeline_counts;
std::unordered_map<std::string,PipelineEntry> pipeline_objects;
std::unordered_map<int,VkPipeline> pipeline_fast;
uint64_t pipeline_tick=0;const size_t pipeline_object_limit=2;bool pipe_memo_valid=true,pipe_stats=false;
struct {VkDevice device=VK_NULL_HANDLE;} cfg;VkPipelineCache pipeline_cache=VK_NULL_HANDLE;
size_t pipelines_unsaved=0;const uint64_t compile_ns=2000000;
namespace hud {std::atomic<uint64_t> frame{74706};uint64_t pipelines=0,pipeline_ns=0;}
uint64_t clock_value=0,clock_step=1000000;
uint64_t timing_clock(){uint64_t r=clock_value;clock_value+=clock_step;return r;}
int created=0,recorded=0,stats_logged=0,retired=0;std::vector<std::string> logs;
void xbg_logf(const char *fmt,...){char b[1024];va_list a;va_start(a,fmt);vsnprintf(b,sizeof b,fmt,a);va_end(a);logs.emplace_back(b);}
void pstat_log(VkPipeline){++stats_logged;}
template<class F>void retire_handle(F f){++retired;f();}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice,VkPipelineCache,uint32_t,const VkGraphicsPipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline *p){*p=(VkPipeline)(uintptr_t)(++created);return VK_SUCCESS;}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(VkDevice,VkPipelineCache,uint32_t,const VkComputePipelineCreateInfo*,const VkAllocationCallbacks*,VkPipeline *p){*p=(VkPipeline)(uintptr_t)(++created);return VK_SUCCESS;}
VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice,VkPipeline,const VkAllocationCallbacks*){}
#define VK(call) assert((call)==VK_SUCCESS)
namespace warm {
bool hit=false,skip=false;int snapshots=0;uint64_t hits=0,creates=0,create_ns=0,create_max_ns=0,async_skips=0;
xbg_diag::Warm observed;
bool take(const std::string&,VkPipeline &p){if(hit)p=(VkPipeline)88;return hit;}
bool async_request(const VkGraphicsPipelineCreateInfo&,const XbgPipelineKey&,const std::string&){return skip;}
void record(const VkGraphicsPipelineCreateInfo&,const XbgPipelineKey&){++recorded;}
xbg_diag::Warm diagnostic_state(const std::string&){++snapshots;return observed;}
void diagnostic_keys(uint32_t count,const VkPipelineShaderStageCreateInfo *s,char keys[6][81]){for(uint32_t i=0;i<count;i++)snprintf(keys[i],81,"bundle-%u",unsigned(s[i].stage));}
}
'''
main = r'''
int main(){
  VkGraphicsPipelineCreateInfo g{};VkPipelineShaderStageCreateInfo s{};s.stage=VK_SHADER_STAGE_VERTEX_BIT;g.stageCount=1;g.pStages=&s;
  VkComputePipelineCreateInfo c{};c.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;
  XbgPipelineKey k;k.u(1);PipelineLease p;
  pipeline_lookup(p,k,true,&g,nullptr);assert(created==1 && p.pipeline==(VkPipeline)1 && recorded==1 && logs.empty());
  int snapshots=warm::snapshots;pipeline_lookup(p,k,true,&g,nullptr);assert(created==1 && pipeline_counts.hits==1 && warm::snapshots==snapshots);
  warm::hit=true;k.words[0]=2;pipeline_lookup(p,k,true,&g,nullptr);assert(created==1 && p.pipeline==(VkPipeline)88 && warm::snapshots==snapshots);
  warm::hit=false;warm::skip=true;k.words[0]=3;pipeline_lookup(p,k,true,&g,nullptr,true);assert(created==1 && !p.pipeline && p.transient && logs.empty());
  warm::skip=false;warm::observed.known=true;warm::observed.failed=true;clock_step=20000000;k.words[0]=4;p={};
  pipeline_lookup(p,k,true,&g,nullptr);assert(created==2 && p.pipeline==(VkPipeline)2 && recorded==2 && logs.size()==1 && retired==1);
  assert(logs.back().find("known=1 ready=0 queued=0 building=0 failed=1")!=std::string::npos);
  assert(logs.back().find("shaders=1:bundle-1")!=std::string::npos && logs.back().find("create_ms=20.00")!=std::string::npos);
  k.words[0]=5;p={};pipeline_lookup(p,k,false,nullptr,&c);assert(created==3 && p.pipeline==(VkPipeline)3 && p.transient);
  assert(logs.back().find("kind=compute cacheable=0 key_stable=0")!=std::string::npos);
  assert(hud::pipelines==3 && pipelines_unsaved==2 && warm::creates==2);
  for(int i=0;i<150;i++)pipeline_lookup(p,k,false,&g,nullptr);
  assert(logs.size()==128 && created==153); // Reaching the diagnostic budget never skips driver work.
  puts("PASS extracted production PSO lookup: hits/warm/async/graphics/compute/eviction/slow threshold and bounded logs; Vulkan is fake");
}
'''
(out / 'pipeline_lookup_harness.cpp').write_text(harness + '\n' + diag + '\n' + lookup + '\n' + main, encoding='utf-8')
zig = os.environ['DM_TEST_ZIG']
command = [zig, 'c++', '-std=c++17', '-I'+str(root), '-I'+os.environ['DM_TEST_VULKAN_INCLUDE'], str(out / 'pipeline_lookup_harness.cpp'), '-o', str(out / 'pipeline_lookup.exe')]
subprocess.run(command, check=True)
subprocess.run([str(out / 'pipeline_lookup.exe')], check=True)
