#include "../xbg_ui_ghost.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <thread>
#include <chrono>

// Real production selection/control/replay, with Vulkan recording replaced by
// a bounded command log. This does not claim device or pixel correctness.
static std::vector<int> calls;
static VkViewport last_vp{};
static XbgGhostDraw hardware,last_draw;
static std::atomic<bool> block_draw{false},draw_entered{false},release_draw{false};
extern "C" {
VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline p){hardware.pipe=p;calls.push_back(1);}
VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer,VkPipelineBindPoint,VkPipelineLayout l,uint32_t,uint32_t,const VkDescriptorSet *s,uint32_t n,const uint32_t *o){hardware.layout=l;hardware.set=*s;hardware.noff=n;if(n)memcpy(hardware.off,o,n*4);calls.push_back(2);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer,uint32_t,uint32_t,const VkViewport *p){hardware.vp=last_vp=*p;calls.push_back(3);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer,uint32_t,uint32_t,const VkRect2D *s){hardware.sc=*s;calls.push_back(4);}
VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer,uint32_t k,uint32_t,const VkBuffer *b,const VkDeviceSize *o){hardware.vb[k]=*b;hardware.vbo[k]=*o;calls.push_back(5);}
VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer,VkBuffer b,VkDeviceSize o,VkIndexType t){hardware.ib=b;hardware.ibo=o;hardware.it=t;calls.push_back(6);}
VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer,uint32_t c,uint32_t i,uint32_t f,uint32_t fi){last_draw=hardware;last_draw.count=c;last_draw.instances=i;last_draw.first=f;last_draw.first_instance=fi;calls.push_back(7);if(block_draw){draw_entered=true;while(!release_draw)std::this_thread::yield();}}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer,uint32_t c,uint32_t i,uint32_t f,int32_t v,uint32_t fi){last_draw=hardware;last_draw.count=c;last_draw.instances=i;last_draw.first=f;last_draw.base_vertex=v;last_draw.first_instance=fi;calls.push_back(8);}
}
static const float grid[4]={.1f,.2f,.1f,.1f};
static const float matrix[16]={2,0,0,0,0,2,0,0,0,0,1,0,-1,-1,0,1};
static void vertices(float v[20],float x=.1f,float y=.2f,float w=.1f,float h=.2f){
    const float q[20]={x,y,0,0,0,x+w,y,0,1,0,x,y+h,0,0,1,x+w,y+h,0,1,1};
    memcpy(v,q,sizeof q);
}
static XbgGhostDraw draw(){XbgGhostDraw d{};d.cmd=(VkCommandBuffer)1;d.pipe=(VkPipeline)2;d.layout=(VkPipelineLayout)3;d.set=(VkDescriptorSet)4;d.vb[0]=(VkBuffer)5;d.vp={0,1000,1000,-1000,0,1};d.sc={{0,0},{1000,1000}};d.count=4;d.instances=1;return d;}
int main(){
    XbgGhostControl control;float r[4];control.set(1,.15f,.25f,grid,.7f,.6f);
    auto request=control.snapshot();assert(request.on && !control.rect(r));
    float vb[20];vertices(vb);XbgGhostPass pass;auto d=draw();
    pass.note(request,1,100,d,vb,sizeof vb,matrix,sizeof matrix,1,0);
    assert(pass.n==1);assert(pass.best()==0);
    // Common shelf background is frequent; actual icon texture wins ties by
    // stable submission order. No stale tex slot participates.
    pass.note(request,1,101,d,vb,sizeof vb,matrix,sizeof matrix,1,0);
    float elsewhere[20];vertices(elsewhere,.6f,.2f);
    pass.note(request,1,100,d,elsewhere,sizeof elsewhere,matrix,sizeof matrix,1,0);
    assert(pass.best()==1);
    // Snapshot data changed later cannot alter captured geometry/draw.
    vb[0]=99;d.count=99;assert(pass.c[1].draw.count==4 && pass.c[1].r[0]<.11f);
    calls.push_back(90);auto saved=draw();assert(control.flush(pass,1,saved,1000,1000,-32768,32767));calls.push_back(91);
    assert(calls.front()==90 && calls.back()==91);
    assert(std::count(calls.begin(),calls.end(),7)==1);
    assert(calls[calls.size()-2]==5); // complete state restoration precedes pass end
    assert(last_vp.x==saved.vp.x && control.rect(r));assert(pass.n==0);
    assert(std::fabs(last_draw.vp.x-550)<.01f && std::fabs(last_draw.vp.y-1300)<.01f);
    assert(last_draw.sc.extent.width==1000 && hardware.pipe==saved.pipe && hardware.set==saved.set);
    control.set(0,0,0,nullptr,0,0);assert(!control.rect(r));
    // Pending prior gesture and prior pass cannot replay after cancellation.
    vertices(vb);pass.note(request,2,101,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);
    auto before=calls.size();assert(!control.flush(pass,2,saved,1000,1000,-32768,32767));assert(calls.size()==before);
    control.set(1,.15f,.25f,grid,.7f,.6f);auto next=control.snapshot();assert(next.epoch!=request.epoch);
    pass.note(next,3,101,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);
    assert(!control.flush(pass,4,saved,1000,1000,-32768,32767));
    // Off-grid press highlight, whole shelf, malformed/short snapshots fail closed.
    vertices(vb,.105f,.205f);pass.note(next,5,101,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);assert(pass.n==1);
    pass.reset();vertices(vb,.15f,.25f);pass.note(next,5,101,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);assert(!pass.n);
    vertices(vb,.1f,.2f,.8f,.5f);pass.note(next,5,101,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);assert(!pass.n);
    vertices(vb);pass.note(next,5,101,draw(),vb,79,matrix,sizeof matrix,1,0);assert(!pass.n);
    pass.note(next,5,101,draw(),vb,sizeof vb,matrix,63,1,0);assert(!pass.n);
    float bad[16];memcpy(bad,matrix,sizeof bad);bad[0]=NAN;pass.note(next,5,101,draw(),vb,sizeof vb,bad,sizeof bad,1,0);assert(!pass.n);
    // Bounded candidates and frequency accounting reject untracked textures.
    pass.reset();for(uint64_t t=1;t<=130;t++)pass.note(next,6,t,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);
    assert(pass.n==4 && pass.nt==128);assert(pass.c[3].texture==128);
    // Moving the desired center preserves a gesture; changing its origin revokes.
    control.set(1,.15f,.25f,grid,.8f,.7f);assert(control.snapshot().epoch==next.epoch);
    control.set(1,.25f,.25f,grid,.8f,.7f);assert(control.snapshot().epoch!=next.epoch);
    // Minimum-area icon replaces a matching larger candidate; a new pass
    // clears texture frequencies and candidates rather than leaking them.
    control.set(1,.15f,.25f,grid,.7f,.6f);next=control.snapshot();pass.reset();
    vertices(vb,.1f,.2f,.2f,.4f);pass.note(next,7,1,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);
    vertices(vb,.1f,.2f,.1f,.1f);pass.note(next,7,2,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);assert(pass.n==1 && pass.c[0].texture==2);
    pass.note(next,8,3,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);assert(pass.n==1 && pass.nt==1);
    // Replay expanded quad's exact index binding, base vertex and instances;
    // restore a different current pipeline/set/dynamic offsets/VB/IB exactly.
    pass.reset();d=draw();d.indexed=true;d.ib=(VkBuffer)9;d.ibo=512;d.it=VK_INDEX_TYPE_UINT32;d.count=6;d.instances=3;d.base_vertex=11;d.first_instance=13;d.noff=2;d.off[0]=256;d.off[1]=768;
    pass.note(next,9,3,d,vb,sizeof vb,matrix,sizeof matrix,1,0);
    saved=draw();saved.pipe=(VkPipeline)22;saved.set=(VkDescriptorSet)44;saved.noff=2;saved.off[0]=512;saved.off[1]=1024;saved.vbo[0]=128;saved.ib=(VkBuffer)19;saved.ibo=256;
    assert(control.flush(pass,9,saved,1000,1000,-32768,32767));
    assert(last_draw.count==6 && last_draw.instances==3 && last_draw.base_vertex==11 && last_draw.first_instance==13 && last_draw.first==0);
    assert(last_draw.ib==d.ib && last_draw.ibo==512 && last_draw.it==VK_INDEX_TYPE_UINT32 && last_draw.off[1]==768);
    assert(hardware.pipe==saved.pipe && hardware.set==saved.set && hardware.off[1]==1024 && hardware.vbo[0]==128 && hardware.ib==saved.ib && hardware.ibo==256);
    pass.note(next,9,3,d,vb,sizeof vb,matrix,sizeof matrix,1,0);auto unknown=saved;unknown.ib=VK_NULL_HANDLE;before=calls.size();assert(!control.flush(pass,9,unknown,1000,1000,-32768,32767));assert(calls.size()==before);
    // UI occupying the upper half uses the same normalized grid convention
    // and half-height movement; the optional platform offset is removed once.
    float half[16];memcpy(half,matrix,sizeof half);half[5]=1;half[13]=-.8f;
    pass.note(next,10,3,draw(),vb,sizeof vb,half,sizeof half,.5f,.1f);
    assert(pass.n==1);assert(control.flush(pass,10,saved,1000,1000,-32768,32767));assert(std::fabs(last_draw.vp.y-1175)<.01f);
    // Non-finite/unrepresentable viewport translations fail without recording.
    pass.note(next,11,3,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);before=calls.size();assert(!control.flush(pass,11,saved,1000,1000,-10,10));assert(calls.size()==before);
    // The coherent gate serializes a concurrent revoke with recording. Once
    // revoke returns, a prior gesture can neither replay nor publish a rect.
    pass.note(next,12,3,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);
    block_draw=true;std::thread renderer([&]{assert(control.flush(pass,12,saved,1000,1000,-32768,32767));});
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!draw_entered){assert(std::chrono::steady_clock::now()<deadline);std::this_thread::yield();}
    std::atomic<bool> cancelling{false},cancelled{false};std::thread cancel([&]{cancelling=true;control.set(0,0,0,reinterpret_cast<const float*>(1),0,0);cancelled=true;});
    while(!cancelling)std::this_thread::yield();std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(!cancelled);
    release_draw=true;renderer.join();cancel.join();assert(cancelled && !control.active && !control.rect(r));
    block_draw=false;pass.note(next,13,3,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);before=calls.size();assert(!control.flush(pass,13,saved,1000,1000,-32768,32767));assert(calls.size()==before);
    control.set(1,.05f,.25f,grid,.7f,.6f);next=control.snapshot();vertices(vb,-.1f,.2f,.2f,.1f);
    pass.note(next,14,3,draw(),vb,sizeof vb,matrix,sizeof matrix,1,0);assert(control.flush(pass,14,saved,1000,1000,-32768,32767));assert(control.rect(r) && r[0]==0 && r[2]<=1);
    puts("PASS production ghost selection, snapshot isolation, bounded tracking, replay/restoration ordering, cancel and stale epochs/passes");
}
