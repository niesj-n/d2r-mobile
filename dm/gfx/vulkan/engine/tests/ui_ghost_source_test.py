"""Execute production bind-state capture; assert integration ordering separately.

No device/driver/pixel correctness is claimed by this CPU/fake boundary test.
"""
from pathlib import Path
import os
import subprocess
import sys
from texture_cache_regression import balanced

engine = Path(__file__).resolve().parents[1]
source = (engine / 'xbg_game.cpp').read_text(encoding='utf-8')
assert 'void ghost_flush_open_pass()' in source, 'missing actual pass-end ghost integration'
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
struct = balanced(source, source.index('struct BindCache{')) + ';'
capture = balanced(source, source.index('bool ghost_bind_snapshot('))
shape = balanced(source, source.index('bool ghost_draw_shape('))
texture = balanced(source, source.index('uint64_t ghost_texture('))
ready = balanced(source, source.index('bool ghost_cpu_range_ready('))
format_fn = balanced(source, source.index('VkFormat format(int f)'))
retire = balanced(source, source.index('struct Retire {')) + ';'
retire_handle = balanced(source, source.index('void retire_handle('))
lease = balanced(source, source.index('struct PipelineLease {')) + ';'
harness = r'''
#include "xbg_ui_ghost.h"
#include "xbg_game_internal.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <set>
#include <functional>
#include <memory>
#include "xbg_writeback_policy.h"
extern "C" void xbg_fatalf(const char*,...){abort();}
struct Shader {std::vector<uint32_t> inputs;std::vector<xbg_shader_binding> bindings;std::set<uint32_t> storage_writes;};
struct {Shader *shader[5]{};vs_st stream[16]{};NVNbufferRange vb[16]{};uint32_t attrib[16]{};uint64_t tex[6][128]{};} state;
struct W {uint64_t addr,bytes;};struct E {uint64_t addr;};
std::vector<W> pending_writebacks;std::vector<E> pending_reports,pending_events,recording_events;
static unsigned destroyed=0;
struct {VkDevice device{};} cfg;
extern "C" VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice,VkPipeline,const VkAllocationCallbacks*){++destroyed;}
'''
main = r'''
int main(){
 BindCache b{};b.cmd=(VkCommandBuffer)1;b.pipe=(VkPipeline)2;b.layout=(VkPipelineLayout)3;b.set=(VkDescriptorSet)4;
 b.noff=2;b.off[0]=256;b.off[1]=768;b.has_vp=b.has_sc=true;b.vp={10,1010,1000,-1000,0,1};b.sc={{4,5},{600,700}};
 b.vb[0]=(VkBuffer)5;b.vbo[0]=123;b.vb[7]=(VkBuffer)8;b.vbo[7]=456;b.ib=(VkBuffer)9;b.ibo=512;b.it=VK_INDEX_TYPE_UINT32;
 p_draw_idx q{4,2,6,11,13,3,1234};XbgGhostDraw c{};
 assert(ghost_bind_snapshot(c,b,0,6,&q,99,98));
 assert(c.indexed && c.count==6 && c.instances==3 && c.base_vertex==11 && c.first_instance==13);
 assert(c.ib==b.ib && c.ibo==512 && c.it==VK_INDEX_TYPE_UINT32);
 b.off[1]=0;b.vbo[7]=0;assert(c.off[1]==768 && c.vbo[7]==456);
 assert(c.sc.offset.x==4 && c.sc.extent.height==700 && c.vp.height==-1000);
 assert(ghost_bind_snapshot(c,b,4,4,nullptr,2,5));assert(!c.indexed && c.first==4 && c.instances==2 && c.first_instance==5);
 b.noff=33;assert(!ghost_bind_snapshot(c,b,0,4,nullptr,1,0));
 b.noff=2;b.has_sc=false;assert(!ghost_bind_snapshot(c,b,0,4,nullptr,1,0));
 Shader vs,fs;vs.inputs={0,1};state.shader[0]=&vs;state.shader[4]=&fs;state.stream[0]={20,0};state.vb[0]={1234,80};
 va_st position{34,0,0},uv{22,12,0};memcpy(&state.attrib[0],&position,4);memcpy(&state.attrib[1],&uv,4);
 assert(ghost_draw_shape(true,0,4,1,0));assert(!ghost_draw_shape(false,0,4,1,0));assert(!ghost_draw_shape(true,1,4,1,0));
 assert(!ghost_draw_shape(true,0,4,2,0));state.stream[0].divisor=1;assert(!ghost_draw_shape(true,0,4,1,0));state.stream[0].divisor=0;
 uv.offset=8;memcpy(&state.attrib[1],&uv,4);assert(!ghost_draw_shape(true,0,4,1,0));uv.offset=12;memcpy(&state.attrib[1],&uv,4);
 xbg_shader_binding sampled{};sampled.source=XBG_SOURCE_TEXTURE;sampled.type=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;sampled.slot=11;sampled.count=1;
 fs.bindings.push_back(sampled);state.tex[1][0]=999;state.tex[1][11]=111;state.tex[4][11]=444;assert(ghost_texture()==111);
 state.tex[1][11]=0;assert(!ghost_texture());state.tex[1][11]=111;
 auto writable=sampled;writable.type=VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;fs.bindings.push_back(writable);assert(!ghost_draw_shape(true,0,4,1,0));fs.bindings.pop_back();
 assert(ghost_cpu_range_ready(1000,80));pending_writebacks.push_back({1079,80});assert(!ghost_cpu_range_ready(1000,80));pending_writebacks.clear();
 pending_reports.push_back({999});assert(!ghost_cpu_range_ready(1000,80));pending_reports.clear();
 recording_events.push_back({1078});assert(!ghost_cpu_range_ready(1000,80));recording_events.clear();assert(!ghost_cpu_range_ready(UINT64_MAX-4,80));
 {PipelineLease lease;lease.pipeline=(VkPipeline)77;lease.transient=true;}assert(!destroyed);
 // This is the actual retirement helper used by in-pass cache eviction too.
 VkPipeline evicted=(VkPipeline)88;retire_handle([evicted]{vkDestroyPipeline(cfg.device,evicted,nullptr);});assert(!destroyed);
 assert(slot.retire.handles.size()==2);slot.retire.run();assert(destroyed==2);
 puts("PASS extracted production bind capture: snapshots actual offsets/VB/IB/draw ranges and rejects unknown state");
}
'''
retire_harness = retire + '\nstruct {Retire retire;} slot;\nauto &recording_slot(){return slot;}\n' + retire_handle + '\n' + lease
(out / 'ui_ghost_capture.cpp').write_text(harness + struct + '\n' + capture + '\n' + format_fn + '\n' + shape + '\n' + texture + '\n' + ready + '\n' + retire_harness + '\n' + main, encoding='utf-8')
cmd = [os.environ['DM_TEST_ZIG'], 'c++', '-std=c++17', '-I'+str(engine), '-I'+os.environ['DM_TEST_VULKAN'], '-I'+str(engine.parent.parent / 'nvn/include'), str(out / 'ui_ghost_capture.cpp'), '-o', str(out / 'ui-ghost-capture.exe')]
subprocess.run(cmd, check=True); subprocess.run([str(out / 'ui-ghost-capture.exe')], check=True)
end = balanced(source, source.index('void end_open_pass(){'))
assert end.index('end_occlusion_query()') < end.index('ghost_flush_open_pass()') < end.index('vkCmdEndRenderPass(')
draw = balanced(source, source.index('void draw(int prim,'))
assert draw.index('const bool source_nonindexed=') < draw.index('if(prim==7 && !indexed)')
assert draw.index('ghost_note_draw(') > draw.index('else vkCmdDraw(c.command,count,instances,first,first_instance)')
assert '!ghost_probe' in source[source.index('void descriptors('):]
assert 'out.ghost_matrix' in source
retire_slot = balanced(source, source.index('void retire_slot(Slot &s)'))
assert retire_slot.index('vkWaitForFences(') < retire_slot.index('s.retire.run()') < retire_slot.index('s.stream.reset()')
lookup = balanced(source, source.index('void pipeline_lookup('))
assert 'retire_handle([old]{vkDestroyPipeline(cfg.device,old,nullptr);})' in lookup
assert 'xbg_ui_ghost(on,px,py,grid,cx,cy)' in (engine.parent / 'nvn_vk.c').read_text(encoding='utf-8')
print('PASS production integration order: source nonindexed preserved, actual draw captured afterward, query ends before replay before pass end')
