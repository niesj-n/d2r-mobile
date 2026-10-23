package com.xiaoboluo.d2r;

import java.nio.file.Files;
import java.nio.file.Path;

/** Uses the production touch queue fixture, not hand-built skill coordinates. */
public final class MapGoalVisualBoundsTest {
    private static void check(boolean v,String message){if(!v)throw new AssertionError(message);}
    private static boolean covers(MapGoalLayout.Box b,float x,float y){return x>=b.left&&x<=b.right&&y>=b.top&&y<=b.bottom;}
    private static boolean ringOverlap(MapGoalLayout.Box b,float x,float y,float r){
        float dx=x-Math.max(b.left,Math.min(b.right,x)),dy=y-Math.max(b.top,Math.min(b.bottom,y));
        float min=(float)Math.hypot(dx,dy),max=0;
        for(float bx:new float[]{b.left,b.right})for(float by:new float[]{b.top,b.bottom})max=Math.max(max,(float)Math.hypot(bx-x,by-y));
        return min<=r&&max>=r;
    }
    public static void main(String[] args)throws Exception {
        MapGoalLayout.Box[] idle=null;int rows=0;
        for(String line:Files.readAllLines(Path.of(args[0]))){
            String[] values=line.split(",");if(args.length>1&&!values[0].equals("held3"))continue;float[] c=new float[235];check(values.length==236,"real 235-float fixture");
            for(int i=0;i<c.length;i++)c[i]=Float.parseFloat(values[i+1]);
            MapGoalLayout.Box[] blocks=MapGoalLayout.controlBounds(c,c[0],c[1]);
            if(values[0].equals("held3")){
                float[] s=new float[52];s[0]=2;s[1]=1;s[2]=4;s[3]=800;s[4]=375;s[7]=.5f;s[8]=.5f;s[16]=3;s[17]=-33;s[18]=2;
                MapGoalLayout.Goal g=MapGoalLayout.project(s,0,800,375);check(g!=null&&g.inside&&Math.abs(g.x-630.4f)<.01f&&Math.abs(g.y-91.5f)<.01f,"original reachable target");
                float lh=g.labelHeight,ch=lh*1.4f*18/34,cw=lh*1.4f;
                MapGoalLayout.Box mark=new MapGoalLayout.Box(g.x-cw/2,g.y-ch/2,g.x+cw/2,g.y+ch/2);
                boolean markVisible=true;for(MapGoalLayout.Box b:blocks)if(mark.intersects(b))markVisible=false;
                float y=markVisible?g.y-ch/2-2-lh/2:g.y;
                MapGoalLayout.Box plaque=MapGoalLayout.place(g.x,y,lh*275/60,lh,g.area,blocks);
                check(plaque!=null&&!ringOverlap(plaque,c[19],c[20],c[21]+18*c[66]/60),"reachable original label_2_en plaque overlaps held SKILL3 direction ring");
                if(args.length>1){System.out.println("PASS exact reachable held SKILL3 plaque regression");return;}
            }
            float k=c[66]/60;
            for(int slot=0;slot<5;slot++){
                int p=4+slot*5;MapGoalLayout.Box b=blocks[slot];float r=c[p+2];
                // Verify actual draw primitives, independently of the envelope formula.
                for(int deg=0;deg<360;deg++){
                    double angle=Math.toRadians(deg);float ux=(float)Math.cos(angle),uy=(float)Math.sin(angle);
                    check(covers(b,c[p]+ux*(r+19*k+6),c[p+1]+uy*(r+19*k+6)),"direction arrow outside reserved bounds");
                    float cx=c[p]+ux*18*k+r*.74f,cy=c[p+1]+uy*18*k-r*.74f;
                    for(int disc=0;disc<360;disc+=15){double q=Math.toRadians(disc);check(covers(b,cx+(float)Math.cos(q)*r*.30f,cy+(float)Math.sin(q)*r*.30f),"displaced offhand badge outside reserved bounds");}
                }
            }
            if(idle==null)idle=blocks;
            else for(int i=0;i<5;i++)check(blocks[i].left==idle[i].left&&blocks[i].right==idle[i].right&&blocks[i].top==idle[i].top&&blocks[i].bottom==idle[i].bottom,"idle/held/released skill envelope jumps");
            boolean cancelReserved=false;for(MapGoalLayout.Box b:blocks)if(covers(b,c[230]-c[232],c[231])&&covers(b,c[230]+c[232],c[231]))cancelReserved=true;
            check(c[232]>0&&cancelReserved,"idle cancel location unreserved");rows++;
        }
        check(rows==11,"all real idle/held/released fixture rows");
        System.out.println("PASS real-source held SKILL3 plaque, 360-direction arrows/moved badges, stable idle envelopes and reserved cancel");
    }
}
