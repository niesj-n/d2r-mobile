package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;

/** Pure dp placement; every obstruction comes from the current UI snapshot. */
final class RiftInfoLayout {
    static final class Bounds {
        final float left,top,right,bottom;
        Bounds(float l,float t,float r,float b) { left=l; top=t; right=r; bottom=b; }
        boolean intersects(Bounds b) {
            return left<b.right && right>b.left && top<b.bottom && bottom>b.top;
        }
        boolean intersectsCircle(float x,float y,float r) {
            float dx=x-Math.max(left,Math.min(right,x)), dy=y-Math.max(top,Math.min(bottom,y));
            return dx*dx+dy*dy<r*r;
        }
        boolean valid() {
            return Float.isFinite(left) && Float.isFinite(top) && Float.isFinite(right)
                    && Float.isFinite(bottom) && right>left && bottom>top;
        }
    }
    static Bounds place(float w,float h,float sl,float st,float sr,float sb,Bounds monitor,
                        float[] snapshot,float[] controls) {
        if (!Float.isFinite(w) || !Float.isFinite(h) || w<=0 || h<=0
                || snapshot==null || snapshot.length<RiftInfoModel.FLOATS || snapshot[0]==0
                || controls==null || controls.length!=235 || controls[2]!=0
                || Math.abs(controls[0]-w)>1.5f || Math.abs(controls[1]-h)>1.5f) return null;
        for (float value:controls) if (!Float.isFinite(value)) return null;
        float u=h/614, width=250*u, height=78*u, gap=8*u;
        float left=Math.max(0,sl), top=Math.max(0,st), right=w-Math.max(0,sr), bottom=h-Math.max(0,sb);
        if (width>right-left || height>bottom-top) return null;
        List<Bounds> rectangles=new ArrayList<>();
        List<float[]> circles=new ArrayList<>();
        if (monitor!=null && monitor.valid()) rectangles.add(monitor);
        Bounds mini=new Bounds(snapshot[14]*w,snapshot[15]*h,snapshot[16]*w,snapshot[17]*h);
        if (mini.valid()) rectangles.add(mini);
        // The original HUD's bottom backing/text extends beyond its menu and
        // belt hit circles. Conservatively reserve that band when it is drawn.
        if (controls[167]!=0) rectangles.add(new Bounds(0,h*.8f,w,h));
        for (int i=0;i<23;i++) addCircle(circles,controls,4+i*5);
        for (int i=0;i<3;i++) addCircle(circles,controls,185+i*5);
        addCircle(circles,controls,230);
        List<Float> xs=new ArrayList<>(), ys=new ArrayList<>();
        xs.add(Math.max(left,Math.min(right-width,28*u)));
        ys.add(Math.max(top,Math.min(bottom-height,150*u)));
        for (Bounds b:rectangles) { xs.add(b.right+gap); ys.add(b.bottom+gap); }
        for (float[] c:circles) { xs.add(c[0]+c[2]+gap); ys.add(c[1]+c[2]+gap); }
        xs.add(right-width); ys.add(bottom-height);
        // Original position, below obstructions, to their right, then combined
        // offsets. Candidate order is stable as progress changes.
        for (int xi=0;xi<xs.size();xi++) for (int yi=0;yi<ys.size();yi++) {
            float x=xs.get(xi), y=ys.get(yi);
            Bounds b=new Bounds(x,y,x+width,y+height);
            if (x<left || y<top || b.right>right || b.bottom>bottom) continue;
            Bounds padded=new Bounds(x-gap,y-gap,b.right+gap,b.bottom+gap);
            boolean free=true;
            for (Bounds occupied:rectangles) if (padded.intersects(occupied)) { free=false; break; }
            if (free) for (float[] c:circles)
                if (padded.intersectsCircle(c[0],c[1],c[2])) { free=false; break; }
            if (free) return b;
        }
        return null;
    }
    private static void addCircle(List<float[]> circles,float[] controls,int offset) {
        if (controls[offset+3]!=0 && controls[offset+2]>0)
            circles.add(new float[]{controls[offset],controls[offset+1],controls[offset+2]});
    }
}
