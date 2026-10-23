package com.xiaoboluo.d2r;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/** Runs the same formatting and placement used by the real View without Android. */
public final class RiftInfoTest {
    private static int checks;
    private static void check(boolean value) { ++checks; if (!value) throw new AssertionError("check " + checks); }
    private static float[] hud() { return new float[]{1,95,.625f,389,0,0,3,98,0,0,1,2,2,3,0,0,0,0}; }
    private static RiftInfoModel.TextSource strings(Path table, int column) throws Exception {
        Map<String,String> values=new HashMap<>();
        for (String row:Files.readAllLines(table)) {
            String[] cells=row.split("\t",-1);
            if (cells.length>column) values.put(cells[0],cells[column]);
        }
        return (key,args)->String.format(Locale.ROOT,values.get(key),args);
    }
    private static void textTests(RiftInfoModel.TextSource en, RiftInfoModel.TextSource zh) {
        float[] h=hud();
        RiftInfoModel.Text t=RiftInfoModel.format(h,en);
        check("Rift".equals(t.title) && "Level 95".equals(t.status));
        check(t.timer.equals("6:29") && t.goal.equals("Slay the monsters   62%"));
        check(t.progress==.625f && !t.urgent && !t.guardian);
        h[3]=60; check(RiftInfoModel.format(h,en).urgent);
        h[3]=-9; check(RiftInfoModel.format(h,en).timer.equals("0:00"));
        h[5]=1; h[9]=1; h[13]=1;
        t=RiftInfoModel.format(h,en);
        check(t.guardian && t.status.equals("Rift Guardian") && t.goal.equals("Slay the Rift Guardian"));
        h[9]=2; check(RiftInfoModel.format(h,en).goal.equals("Slay the Guardians (1/2)"));
        h[13]=3; check(RiftInfoModel.format(h,en).goal.equals("Guardians 2/3 (1/2)"));
        h[9]=3; check(RiftInfoModel.format(h,en).goal.equals("Overtime: drops, no clear"));
        h[9]=4; check(RiftInfoModel.format(h,en).goal.equals("Overtime: slay it for drops"));
        h[9]=5; check(RiftInfoModel.format(h,en).goal.equals("Overtime: slay 1/2 for drops"));
        // Host goal chooses the line, including overtime, rather than Java seconds/overtime guesses.
        h[8]=0; h[3]=700; check(RiftInfoModel.format(h,en).goal.startsWith("Overtime:"));
        h[4]=1; h[6]=3; h[7]=96;
        t=RiftInfoModel.format(h,en);
        check(t.status.equals("Level 95 complete") && t.goal.equals("Next: Level 96"));
        check(!t.urgent && t.progress==1); // Host next=96 is not tier+inc=98.
        h[1]=100; h[7]=100;
        check(RiftInfoModel.format(h,en).goal.equals("Highest level cleared"));
        h[6]=0; check(RiftInfoModel.format(h,en).status.equals("Level 100 overtime"));
        check(RiftInfoModel.format(h,en).goal.equals("Next: same level"));
        h=hud(); check(RiftInfoModel.format(h,zh).goal.equals("消灭秘境中的怪物   62%"));
        h[0]=0; check(RiftInfoModel.format(h,en)==null);
        h[0]=1; h[2]=Float.NaN; check(RiftInfoModel.format(h,en)==null);
    }
    private static float[] controls(float width,float height) {
        float[] c=new float[235]; c[0]=width; c[1]=height;
        return c;
    }
    private static void placementTests() {
        float[] h=hud(), c=controls(800,375);
        RiftInfoLayout.Bounds b=RiftInfoLayout.place(800,375,0,0,0,0,null,h,c);
        float u=375f/614;
        check(b!=null && Math.abs(b.left-28*u)<.001f && Math.abs(b.top-150*u)<.001f);
        RiftInfoLayout.Bounds monitor=new RiftInfoLayout.Bounds(10,48,294,212);
        b=RiftInfoLayout.place(800,375,0,0,0,0,monitor,h,c);
        check(b!=null && !b.intersects(monitor));
        h[14]=.01f; h[15]=.1f; h[16]=.4f; h[17]=.6f;
        b=RiftInfoLayout.place(800,375,0,0,0,0,monitor,h,c);
        RiftInfoLayout.Bounds mini=new RiftInfoLayout.Bounds(8,37.5f,320,225);
        check(b!=null && !b.intersects(monitor) && !b.intersects(mini));
        // Preserve real control circles and the current 235-float ABI, including quick/cancel.
        c[4]=400; c[5]=230; c[6]=60; c[7]=1;
        c[185]=550; c[186]=150; c[187]=35; c[188]=1;
        c[230]=650; c[231]=190; c[232]=40; c[233]=1;
        c[167]=1;
        b=RiftInfoLayout.place(800,375,0,12,0,10,monitor,h,c);
        check(b!=null && b.top>=12 && b.bottom<=365);
        check(!b.intersectsCircle(400,230,60) && !b.intersectsCircle(550,150,35));
        check(!b.intersectsCircle(650,190,40) && b.bottom<=375*.8f);
        c[2]=1; check(RiftInfoLayout.place(800,375,0,0,0,0,null,h,c)==null);
        c[2]=0;
        check(RiftInfoLayout.place(800,375,0,0,0,0,new RiftInfoLayout.Bounds(0,0,800,375),h,c)==null);
        check(RiftInfoLayout.place(800,375,0,0,0,0,null,h,new float[234])==null);
        check(RiftInfoLayout.place(900,375,0,0,0,0,null,h,c)==null); // stale rotation geometry
        check(RiftInfoLayout.place(120,40,0,0,0,0,null,h,controls(120,40))!=null);
    }
    public static void main(String[] args) throws Exception {
        Path table=Path.of(args[0]); textTests(strings(table,2),strings(table,1)); placementTests();
        System.out.println("rift Java format/layout: " + checks + " checks passed");
    }
}
