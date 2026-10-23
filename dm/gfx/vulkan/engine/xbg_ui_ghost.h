#pragma once
#include <vulkan/vulkan.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
struct XbgGhostDraw {
    VkCommandBuffer cmd{};VkPipeline pipe{};VkPipelineLayout layout{};VkDescriptorSet set{};
    uint32_t noff=0,off[32]{};VkViewport vp{};VkRect2D sc{};VkBuffer vb[16]{};VkDeviceSize vbo[16]{};
    VkBuffer ib{};VkDeviceSize ibo=0;VkIndexType it=VK_INDEX_TYPE_UINT16;
    bool indexed=false;uint32_t count=0,instances=1,first=0,first_instance=0;int32_t base_vertex=0;
};
struct XbgGhostRequest {bool on=false;uint64_t epoch=0;float px=0,py=0,grid[4]{},cx=0,cy=0;};
// No allocation, guest reads, descriptor updates or recursive draw translation.
inline void xbg_ghost_bind(const XbgGhostDraw &d){
    vkCmdBindPipeline(d.cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,d.pipe);
    vkCmdBindDescriptorSets(d.cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,d.layout,0,1,&d.set,d.noff,d.off);
    vkCmdSetViewport(d.cmd,0,1,&d.vp);vkCmdSetScissor(d.cmd,0,1,&d.sc);
    for(uint32_t i=0;i<16;i++)if(d.vb[i])vkCmdBindVertexBuffers(d.cmd,i,1,&d.vb[i],&d.vbo[i]);
    if(d.ib)vkCmdBindIndexBuffer(d.cmd,d.ib,d.ibo,d.it);
}
struct XbgGhostPass {
    struct Candidate {XbgGhostDraw draw;float r[4];uint64_t texture;float ui_height=1;};
    Candidate c[4]{};int n=0,nt=0;uint64_t tid[128]{},pass=0,epoch=0;uint16_t tn[128]{};
    void reset(){n=nt=0;pass=epoch=0;}
    int best()const{int b=-1;uint16_t least=UINT16_MAX;for(int i=0;i<n;i++)for(int k=0;k<nt;k++)if(c[i].texture==tid[k] && (b<0 || tn[k]<least)){b=i;least=tn[k];}return b;}
    void note(const XbgGhostRequest &q,uint64_t p,uint64_t texture,const XbgGhostDraw &draw,
              const void *vertices,size_t vb_bytes,const void *projection,size_t matrix_bytes,float hf,float uy){
        if(!q.on)return;
        if(pass!=p || epoch!=q.epoch){reset();pass=p;epoch=q.epoch;}
        if(!texture)return;
        int k=0;for(;k<nt;k++)if(tid[k]==texture)break;
        if(k==nt){if(nt==128)return;tid[nt]=texture;tn[nt++]=0;}
        if(tn[k]<UINT16_MAX)++tn[k];
        if(!vertices || vb_bytes!=80 || !projection || matrix_bytes<64 || !std::isfinite(hf) || hf<=0 || hf>1)return;
        float m[16],r[4]={9,9,-9,-9};memcpy(m,projection,64);
        for(float f:m)if(!std::isfinite(f))return;
        for(int i=0;i<4;i++){
            float f[2];memcpy(f,static_cast<const unsigned char*>(vertices)+20*i,8);
            if(!std::isfinite(f[0]) || !std::isfinite(f[1]))return;
            const float x=(m[0]*f[0]+m[4]*f[1]+m[12]+1)/2;
            const float y=((m[1]*f[0]+m[5]*f[1]+m[13]+1)/2-uy)/hf;
            if(!std::isfinite(x) || !std::isfinite(y))return;
            r[0]=std::min(r[0],x);r[1]=std::min(r[1],y);r[2]=std::max(r[2],x);r[3]=std::max(r[3],y);
        }
        if(q.px<r[0] || q.px>r[2] || q.py<r[1] || q.py>r[3] || q.grid[2]<=0 || q.grid[3]<=0)return;
        const float w=r[2]-r[0],h=r[3]-r[1],area=w*h;
        const float cells[4]={(r[0]-q.grid[0])/q.grid[2],(r[1]-q.grid[1])/q.grid[3],w/q.grid[2],h/q.grid[3]};
        for(float v:cells)if(std::fabs(v-std::round(v))>.15f)return;
        if(cells[2]<.85f || cells[2]>2.15f || cells[3]<.85f || cells[3]>4.15f)return;
        const float previous=n?(c[n-1].r[2]-c[n-1].r[0])*(c[n-1].r[3]-c[n-1].r[1]):0;
        if(n && area>previous*1.03f)return;
        if(n && area<previous*.97f)n=0;
        if(n==4){for(int i=0;i<3;i++)c[i]=c[i+1];n=3;}
        c[n].draw=draw;memcpy(c[n].r,r,sizeof r);c[n].ui_height=hf;c[n++].texture=texture;
    }
};
struct XbgGhostControl {
    std::atomic<bool> active{false}; // idle draws avoid the gate and all probing
    std::mutex gate;XbgGhostRequest request;bool found=false;float result[4]{};
    XbgGhostRequest snapshot(){std::lock_guard<std::mutex> lock(gate);return request;}
    void set(int on,float px,float py,const float *grid,float cx,float cy){
        XbgGhostRequest q;q.on=on!=0;q.px=px;q.py=py;q.cx=cx;q.cy=cy;
        if(q.on){
            if(!grid || !std::isfinite(px) || !std::isfinite(py) || px<0 || px>1 || py<0 || py>1 ||
               !std::isfinite(cx) || !std::isfinite(cy) || std::fabs(cx)>8 || std::fabs(cy)>8)q.on=false;
            if(grid)for(int i=0;i<4;i++){q.grid[i]=grid[i];if(!std::isfinite(grid[i]) || grid[i]<0 || grid[i]>1)q.on=false;}
            if(q.grid[2]<=0 || q.grid[3]<=0)q.on=false;
        }
        std::lock_guard<std::mutex> lock(gate);
        q.epoch=request.epoch;
        if(!q.on || q.on!=request.on || px!=request.px || py!=request.py || memcmp(q.grid,request.grid,sizeof q.grid)){
            ++q.epoch;found=false;
        }
        request=q;active.store(q.on,std::memory_order_release);
    }
    bool rect(float *r){std::lock_guard<std::mutex> lock(gate);if(!r || !request.on || !found)return false;memcpy(r,result,sizeof result);return true;}
    bool flush(XbgGhostPass &p,uint64_t pass,const XbgGhostDraw &saved,uint32_t width,uint32_t height,float low,float high){
        std::lock_guard<std::mutex> lock(gate);
        const int best=p.best();
        if(!request.on || p.epoch!=request.epoch || p.pass!=pass || best<0){p.reset();return false;}
        const auto c=p.c[best];p.reset();
        XbgGhostDraw d=c.draw;
        if(d.cmd!=saved.cmd || !saved.pipe || !saved.layout || !saved.set || saved.noff>32)return false;
        // Vulkan cannot unbind a vertex/index buffer on the supported baseline.
        // If an intervening external bind site invalidated our knowledge, skip
        // the optional replay rather than inventing a state to restore.
        if(d.ib && !saved.ib)return false;
        for(int i=0;i<16;i++)if(d.vb[i] && !saved.vb[i])return false;
        // Translate the actual Vulkan viewport (including its negative height).
        d.vp.x+=(request.cx-(c.r[0]+c.r[2])/2)*width;
        d.vp.y+=(request.cy-(c.r[1]+c.r[3])/2)*height*c.ui_height;
        if(!std::isfinite(d.vp.x) || !std::isfinite(d.vp.y) || d.vp.x<low || d.vp.x+d.vp.width>high || d.vp.y<low || d.vp.y>high || d.vp.y+d.vp.height<low)return false;
        d.sc={{0,0},{width,height}};
        xbg_ghost_bind(d);
        if(d.indexed)vkCmdDrawIndexed(d.cmd,d.count,d.instances,0,d.base_vertex,d.first_instance);
        else vkCmdDraw(d.cmd,d.count,d.instances,d.first,d.first_instance);
        xbg_ghost_bind(saved);
        for(int i=0;i<4;i++)result[i]=std::clamp(c.r[i],0.f,1.f);
        found=result[2]>result[0] && result[3]>result[1];return true;
    }
};
