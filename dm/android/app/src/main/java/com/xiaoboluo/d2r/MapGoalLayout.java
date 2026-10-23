package com.xiaoboluo.d2r;

import java.util.ArrayList;

/** Original iOS mapGoalSync geometry, independent of Android drawing. All units
 * here are view pixels; the host publishes normalized minimap bounds. */
final class MapGoalLayout {
    static final int FLOATS=52;
    static final class Box {
        final float left,top,right,bottom;
        Box(float l,float t,float r,float b){left=l;top=t;right=r;bottom=b;}
        boolean valid(){return Float.isFinite(left)&&Float.isFinite(top)&&Float.isFinite(right)&&Float.isFinite(bottom)&&right>left&&bottom>top;}
        boolean intersects(Box b){return b!=null&&b.valid()&&left<b.right&&right>b.left&&top<b.bottom&&bottom>b.top;}
        boolean contains(float x,float y){return x>=left&&x<right&&y>=top&&y<bottom;}
    }
    static final class Goal {
        final float x,y,labelHeight,symbolHeight;
        final int art;
        final boolean inside,symbol;
        final Box area;
        Goal(float x,float y,float lh,float sh,int art,boolean inside,Box area){this.x=x;this.y=y;labelHeight=lh;symbolHeight=sh;this.art=art;this.inside=inside;symbol=art>=64;this.area=area;}
    }
    static Goal project(float[] s,int i,float w,float h) {
        if(s==null||s.length<FLOATS||!Float.isFinite(w)||!Float.isFinite(h)||w<=0||h<=0||i<0||i>=12||s[1]<1||s[1]>12||i>=s[1]||(s[0]!=1&&s[0]!=2))return null;
        for(int j=2;j<13;j++)if(!Float.isFinite(s[j]))return null;
        if(s[2]<=0||s[3]<=0||s[4]<=0)return null;
        int at=16+i*3;for(int j=at;j<at+3;j++)if(!Float.isFinite(s[j]))return null;
        int art=(int)s[at+2];if(!((art>=0&&art<=30)||(art>=64&&art<=70)))return null;
        boolean full=s[0]==2;float ox,oy;Box area;
        if(full){ox=s[7]*w;oy=s[8]*h;area=new Box(w*.04f,h*.06f,w*.96f,h*.78f);}
        else {float l=s[9]*w,t=s[10]*h,r=s[11]*w,b=s[12]*h;if(r<=l||b<=t)return null;ox=(l+r)/2;oy=(t+b)/2;area=new Box(l+(r-l)*.06f,t+(b-t)*.08f,r-(r-l)*.06f,b-(b-t)*.08f);}
        if(!area.valid())return null;
        float dx=s[at]-s[5],dy=s[at+1]-s[6],c=s[2]*w/s[3];
        float x=ox+(dx-dy)*1.6f*c,y=oy+(dx+dy)*.8f*c;
        if(!Float.isFinite(x)||!Float.isFinite(y))return null;
        boolean inside=area.contains(x,y)&&!(full&&x>w*.72f&&y>h*.45f);
        if(!inside){float tx=x>ox?(area.right-ox)/(x-ox):x<ox?(area.left-ox)/(x-ox):1e9f;
            float ty=y>oy?(area.bottom-oy)/(y-oy):y<oy?(area.top-oy)/(y-oy):1e9f;
            float scale=Math.min(tx,ty);if(!Float.isFinite(scale)||scale<0)return null;
            x=ox+(x-ox)*scale;y=oy+(y-oy)*scale;if(full&&x>w*.72f&&y>h*.45f)y=h*.45f;}
        return new Goal(x,y,h*.0236f,h*.085f*(full?1:.6f),art,inside,area);
    }
    static String labelName(Goal g,int language,boolean old){int id=g.symbol?0:g.art;String suffix=language==1?(old?"_old":""):language==2?"_es":language==3?"_mx":language==4?"_ru":language==5?"_ko":"_en";return "label_"+id+suffix+".png";}
    static String markerName(Goal g){return !g.inside?null:g.symbol?"sym_"+(g.art-64)+".png":"cross.png";}
    static <T> T label(Goal g,int language,boolean old,java.util.function.Function<String,T> read){
        T result=read.apply(labelName(g,language,old));
        return result!=null?result:read.apply("label_"+(g.symbol?0:g.art)+".png");
    }
    static boolean gameplay(float[] controls){return controls!=null&&controls.length>=235&&controls[0]>0&&controls[1]>0&&controls[2]==0;}
    static Box[] controlBounds(float[] c,float w,float h){ArrayList<Box> boxes=new ArrayList<>();if(c==null||c.length<235||c[0]<=0||c[1]<=0)return new Box[0];float kx=w/c[0],ky=h/c[1];
        for(int i=0;i<27;i++){int at=i<23?4+i*5:i<26?185+(i-23)*5:230;
            if(c[at+3]==0||c[at+2]<=0)continue;float x=c[at]*kx,y=c[at+1]*ky,rx=c[at+2]*kx+2,ry=c[at+2]*ky+2;Box b=new Box(x-rx,y-ry,x+rx,y+ry);if(b.valid())boxes.add(b);}
        return boxes.toArray(new Box[0]);}
    /** Keep artwork inside safe view bounds and out of actual controls/info.
     * Candidate edges are finite and bounded; a completely covered area hides. */
    static Box place(float x,float y,float w,float h,Box area,Box[] blocked){
        if(!area.valid()||w<=0||h<=0||w>area.right-area.left||h>area.bottom-area.top)return null;
        ArrayList<Float> xs=new ArrayList<>(),ys=new ArrayList<>();xs.add(clamp(x-w/2,area.left,area.right-w));ys.add(clamp(y-h/2,area.top,area.bottom-h));
        Box initial=new Box(xs.get(0),ys.get(0),xs.get(0)+w,ys.get(0)+h);
        boolean unobstructed=true;if(blocked!=null)for(Box b:blocked)if(initial.intersects(b)){unobstructed=false;break;}
        if(unobstructed)return initial;
        xs.add(area.left);xs.add(area.right-w);ys.add(area.top);ys.add(area.bottom-h);
        if(blocked!=null)for(Box b:blocked)if(b!=null&&b.valid()){xs.add(clamp(b.left-w-2,area.left,area.right-w));xs.add(clamp(b.right+2,area.left,area.right-w));ys.add(clamp(b.top-h-2,area.top,area.bottom-h));ys.add(clamp(b.bottom+2,area.top,area.bottom-h));}
        Box best=null;float distance=Float.POSITIVE_INFINITY;
        for(float l:xs)for(float t:ys){Box b=new Box(l,t,l+w,t+h);boolean clear=true;if(blocked!=null)for(Box ob:blocked)if(b.intersects(ob)){clear=false;break;}if(!clear)continue;float d=(l+w/2-x)*(l+w/2-x)+(t+h/2-y)*(t+h/2-y);if(d<distance){distance=d;best=b;}}
        return best;
    }
    private static float clamp(float v,float lo,float hi){return Math.max(lo,Math.min(hi,v));}
}
