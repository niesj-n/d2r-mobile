package com.xiaoboluo.d2r;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.Map;

/** Uses real 235-float bridge output, not hand-authored control coordinates. */
public final class RiftInfoVisualBoundsTest {
    private static int checks,placed,hidden;
    private static String scenario;
    private static void check(boolean ok,String detail) {
        ++checks; if (!ok) throw new AssertionError(scenario+": "+detail);
    }
    private static boolean pointInside(RiftInfoLayout.Bounds b,float x,float y) {
        return x>b.left && x<b.right && y>b.top && y<b.bottom;
    }
    private static void allSkillVisualsClear(RiftInfoLayout.Bounds b,float[] c) {
        if (b==null) return;
        float k=c[4+12*5+2]/60;
        for (int skill=0;skill<5;skill++) {
            int p=4+skill*5;
            if (c[p+3]==0) continue;
            float x=c[p],y=c[p+1],r=c[p+2];
            // Independent primitive checks follow TouchOverlay's drawing:
            // ring at r+18*k, 1.25dp stroke; arc at r+19*k; +6dp arrow tip.
            check(!b.intersectsCircle(x,y,r+18*k+.625f),"skill "+skill+" ring intersects readout");
            check(!b.intersectsCircle(x,y,r+19*k+.625f),"skill "+skill+" direction arc intersects readout");
            for (int angle=0;angle<360;angle++) {
                double radians=Math.toRadians(angle);
                float ux=(float)Math.cos(radians),uy=(float)Math.sin(radians);
                check(!pointInside(b,x+(r+19*k+6)*ux,y+(r+19*k+6)*uy),"skill "+skill+" arrow intersects readout");
                float dx=18*k*ux,dy=18*k*uy;
                check(!b.intersectsCircle(x+dx,y+dy,r),"skill "+skill+" displaced socket intersects readout");
                // The original offhand disc is at (+.74r,-.74r), radius .30r.
                // Its border/text stay inside; glyph/glow have an .82r clip.
                check(!b.intersectsCircle(x+dx+.74f*r,y+dy-.74f*r,.30f*r),
                        "skill "+skill+" displaced offhand badge intersects readout");
            }
        }
    }
    private static boolean same(RiftInfoLayout.Bounds a,RiftInfoLayout.Bounds b) {
        return a==null ? b==null : b!=null && a.left==b.left && a.top==b.top && a.right==b.right && a.bottom==b.bottom;
    }
    public static void main(String[] args) throws Exception {
        Map<String,RiftInfoLayout.Bounds> idle=new HashMap<>();
        boolean stable=true;
        for (String line:Files.readAllLines(Path.of(args[0]))) {
            String[] fields=line.trim().split("\\s+");
            check(fields.length==240,"fixture must retain exactly 235 fields");
            scenario=String.join("/",fields[0],fields[1],fields[2],fields[3],fields[4]);
            float[] c=new float[235];
            for (int i=0;i<c.length;i++) c[i]=Float.parseFloat(fields[i+5]);
            float[] hud=new float[]{1,95,.5f,300,0,0,3,98,0,0,0,0,0,0,0,0,0,0};
            boolean monitor=fields[2].equals("1");
            RiftInfoLayout.Bounds occupied=monitor ? new RiftInfoLayout.Bounds(30,62,314,226) : null;
            RiftInfoLayout.Bounds box=RiftInfoLayout.place(c[0],c[1],20,14,24,16,occupied,hud,c);
            if (box==null) hidden++; else placed++;
            allSkillVisualsClear(box,c);
            String key=fields[0]+"/"+fields[1]+"/"+fields[2];
            if (fields[3].equals("-1")) idle.put(key,box);
            else stable &= same(idle.get(key),box);
        }
        check(stable,"idle -> held/drag must keep the same readout bounds");
        check(placed>0 && hidden>0,"reachable layouts must include shown and safely hidden readouts");
        System.out.println("rift actual control visual bounds: "+checks+" checks; "+placed+" placed, "+hidden+" hidden");
    }
}
