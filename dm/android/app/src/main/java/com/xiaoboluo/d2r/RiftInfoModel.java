package com.xiaoboluo.d2r;

import java.util.Locale;

/** Pure presentation of the host's read-only snapshot; no clocks or gameplay. */
final class RiftInfoModel {
    static final int FLOATS=18;
    interface TextSource { String format(String key,Object... args); }
    static final class Text {
        String title,status,timer,goal;
        float progress;
        boolean guardian,urgent;
    }
    static Text format(float[] h,TextSource strings) {
        if (h==null || h.length<FLOATS || h[0]==0) return null;
        for (int i=0;i<14;i++) if (!Float.isFinite(h[i])) return null;
        Text t=new Text();
        int tier=(int)h[1], seconds=Math.max(0,(int)h[3]), inc=(int)h[6], next=(int)h[7];
        int goal=(int)h[9], dead=(int)h[10], total=(int)h[11], wave=(int)h[12], waves=(int)h[13];
        boolean done=h[4]!=0;
        t.guardian=h[5]!=0; t.urgent=seconds<=60 && !done;
        t.progress=done ? 1 : Math.max(0,Math.min(1,h[2]));
        t.title=strings.format("dmRiftHudTitle");
        t.status=done ? strings.format(inc!=0 ? "dmRiftHudDone" : "dmRiftHudLate",tier)
                : t.guardian ? strings.format("dmRiftHudGuard") : strings.format("dmRiftTierN",tier);
        t.timer=String.format(Locale.ROOT,"%d:%02d",seconds/60,seconds%60);
        // This is the original host goal ordering. In particular overtime wins
        // over waves, and next is never recomputed from tier + inc.
        if (done) t.goal=inc==0 ? strings.format("dmRiftHudSame")
                : next>tier ? strings.format("dmRiftHudNext",next) : strings.format("dmRiftHudTop");
        else if (goal==5) t.goal=strings.format("dmRiftHudOverKillN",dead,total);
        else if (goal==4) t.goal=strings.format("dmRiftHudOverKill");
        else if (goal==3) t.goal=strings.format("dmRiftHudOver");
        else if ((goal==1 || goal==2) && waves>1) t.goal=strings.format("dmRiftHudKillWave",wave,waves,dead,total);
        else if (goal==2) t.goal=strings.format("dmRiftHudKillN",dead,total);
        else if (goal==1) t.goal=strings.format("dmRiftHudKill");
        else t.goal=strings.format("dmRiftHudGoal",(int)(h[2]*100));
        return t;
    }
}
