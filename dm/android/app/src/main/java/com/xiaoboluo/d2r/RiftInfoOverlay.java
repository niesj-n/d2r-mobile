package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/** Passive original-style Rift readout. JNI only copies a server-owned cache. */
public final class RiftInfoOverlay extends View {
    private static native void nativeReadSnapshot(float[] values);
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path diamond=new Path();
    private final float density;
    private final float[] snapshot=new float[RiftInfoModel.FLOATS], probe=new float[RiftInfoModel.FLOATS];
    private final float[] controls=new float[235];
    private final Map<String,String> strings=new HashMap<>();
    private final RiftInfoModel.TextSource textSource=(key,args)->String.format(Locale.ROOT,strings.get(key),args);
    private RiftInfoModel.Text text;
    private RiftInfoLayout.Bounds box,monitor;
    private Shader fill;
    private boolean enabled,running,controlsValid;
    private int safeLeft,safeTop,safeRight,safeBottom;
    private BoundsListener boundsListener;
    private final float[] publishedBounds=new float[4];
    private boolean boundsPublished;
    public interface BoundsListener { void onBounds(float left,float top,float right,float bottom); }

    private final Choreographer.FrameCallback refresh=new Choreographer.FrameCallback() {
        @Override public void doFrame(long frameTimeNanos) {
            if (!running) return;
            nativeReadSnapshot(probe);
            if (!Arrays.equals(probe,snapshot)) {
                System.arraycopy(probe,0,snapshot,0,snapshot.length);
                updatePresentation();
            }
            Choreographer.getInstance().postFrameCallback(this);
        }
    };

    public RiftInfoOverlay(Context context) {
        super(context);
        density=getResources().getDisplayMetrics().density;
        String[] keys={"dmRiftHudTitle","dmRiftTierN","dmRiftHudDone","dmRiftHudLate","dmRiftHudGuard",
                "dmRiftHudNext","dmRiftHudTop","dmRiftHudOver","dmRiftHudKillWave","dmRiftHudKillN",
                "dmRiftHudOverKill","dmRiftHudOverKillN","dmRiftHudSame","dmRiftHudKill","dmRiftHudGoal"};
        int[] ids={R.string.dmRiftHudTitle,R.string.dmRiftTierN,R.string.dmRiftHudDone,R.string.dmRiftHudLate,
                R.string.dmRiftHudGuard,R.string.dmRiftHudNext,R.string.dmRiftHudTop,R.string.dmRiftHudOver,
                R.string.dmRiftHudKillWave,R.string.dmRiftHudKillN,R.string.dmRiftHudOverKill,
                R.string.dmRiftHudOverKillN,R.string.dmRiftHudSame,R.string.dmRiftHudKill,R.string.dmRiftHudGoal};
        for (int i=0;i<keys.length;i++) strings.put(keys[i],context.getString(ids[i]));
        paint.setTypeface(Typeface.DEFAULT_BOLD);
        setClickable(false); setLongClickable(false); setFocusable(false);
        setImportantForAccessibility(IMPORTANT_FOR_ACCESSIBILITY_NO);
        setVisibility(GONE);
    }

    public void setRuntimeEnabled(boolean value) {
        if (enabled==value) return;
        enabled=value;
        setVisibility(value ? VISIBLE : GONE);
        if (value && isAttachedToWindow()) start(); else if (!value) stop();
        updatePresentation();
    }

    /** Insets and monitor bounds are View pixels, matching PerformanceOverlay. */
    public void setSafeInsets(int left,int top,int right,int bottom) {
        left=Math.max(0,left); top=Math.max(0,top); right=Math.max(0,right); bottom=Math.max(0,bottom);
        if (safeLeft==left && safeTop==top && safeRight==right && safeBottom==bottom) return;
        safeLeft=left; safeTop=top; safeRight=right; safeBottom=bottom;
        updatePresentation();
    }
    public void setMonitorBounds(float left,float top,float right,float bottom) {
        RiftInfoLayout.Bounds next=new RiftInfoLayout.Bounds(left/density,top/density,right/density,bottom/density);
        if (monitor!=null && monitor.left==next.left && monitor.top==next.top
                && monitor.right==next.right && monitor.bottom==next.bottom) return;
        monitor=next; updatePresentation();
    }
    /** Existing 235-float control snapshot, in dp; no appended Rift fields. */
    public void setControlLayout(float[] values) {
        if (values==null || values.length!=controls.length) {
            if (!controlsValid) return;
            controlsValid=false; updatePresentation(); return;
        }
        if (controlsValid && Arrays.equals(values,controls)) return;
        System.arraycopy(values,0,controls,0,controls.length);
        controlsValid=true; updatePresentation();
    }
    public void setBoundsListener(BoundsListener listener) {
        boundsListener=listener; boundsPublished=false; publishBounds();
    }
    /** Actual occupied bounds in View pixels, or four zeroes when hidden. */
    public float[] getInfoBounds() {
        return box==null ? new float[4] : new float[]{box.left*density,box.top*density,box.right*density,box.bottom*density};
    }
    private void publishBounds() {
        if (boundsListener==null) return;
        float[] current=getInfoBounds();
        if (boundsPublished && Arrays.equals(current,publishedBounds)) return;
        System.arraycopy(current,0,publishedBounds,0,4); boundsPublished=true;
        boundsListener.onBounds(current[0],current[1],current[2],current[3]);
    }
    private void updatePresentation() {
        text=enabled && running ? RiftInfoModel.format(snapshot,textSource) : null;
        box=text==null || !controlsValid ? null : RiftInfoLayout.place(getWidth()/density,getHeight()/density,
                safeLeft/density,safeTop/density,safeRight/density,safeBottom/density,monitor,snapshot,controls);
        if (box!=null) {
            float u=getHeight()/density/614;
            int light=text.guardian ? 0xffbe3228 : 0xffcda050;
            int dark=text.guardian ? 0xff6e140f : 0xff8c6428;
            fill=new LinearGradient(0,56*u,0,64*u,light,dark,Shader.TileMode.CLAMP);
        }
        publishBounds(); invalidate();
    }
    private void start() {
        if (running) return;
        running=true;
        Choreographer.getInstance().postFrameCallback(refresh);
    }
    private void stop() {
        running=false;
        Choreographer.getInstance().removeFrameCallback(refresh);
        Arrays.fill(snapshot,0); Arrays.fill(probe,0);
    }
    @Override protected void onAttachedToWindow() {
        super.onAttachedToWindow(); if (enabled) start(); updatePresentation();
    }
    @Override protected void onDetachedFromWindow() {
        stop(); updatePresentation(); super.onDetachedFromWindow();
    }
    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh) {
        super.onSizeChanged(w,h,oldw,oldh); updatePresentation();
    }
    @Override public boolean onTouchEvent(MotionEvent event) { return false; }

    private void label(Canvas canvas,String value,float x,float y,float w,float h,float size,int color,boolean right,float u) {
        int saved=canvas.save(); canvas.clipRect(x,y,x+w,y+h);
        paint.setTextSize(size); paint.setTextAlign(right ? Paint.Align.RIGHT : Paint.Align.LEFT);
        float anchor=right ? x+w : x, baseline=y-paint.ascent();
        paint.setColor(0xdc000000); canvas.drawText(value,anchor+u,baseline+u,paint);
        paint.setColor(color); canvas.drawText(value,anchor,baseline,paint);
        canvas.restoreToCount(saved);
    }
    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (box==null || text==null) return;
        int saved=canvas.save(); canvas.scale(density,density); canvas.translate(box.left,box.top);
        float u=getHeight()/density/614, w=250*u, h=78*u, stroke=Math.max(1/density,u);
        paint.setStyle(Paint.Style.FILL); paint.setShader(null); paint.setColor(0xaa000000);
        canvas.drawRect(0,0,w,h,paint);
        paint.setColor(0xe678643c); canvas.drawRect(0,0,w,stroke,paint); canvas.drawRect(0,h-stroke,w,h,paint);
        paint.setColor(0xff967d4b); canvas.drawRect(0,0,2*u,h,paint);
        int gold=0xffc7b377, red=0xffc83228, white=0xffd7d2c8, grey=0xff969187;
        label(canvas,text.title,10*u,6*u,40*u,22*u,17*u,text.guardian ? red : gold,false,u);
        // The timer owns its original 80u cell. Clip long status translations
        // before that cell rather than painting through the countdown.
        label(canvas,text.status,48*u,9*u,108*u,19*u,14*u,white,false,u);
        label(canvas,text.timer,160*u,6*u,80*u,22*u,17*u,text.urgent ? red : gold,true,u);
        label(canvas,text.goal,10*u,32*u,230*u,17*u,12*u,grey,false,u);
        float l=11*u,t=55*u,r=239*u,b=65*u;
        paint.setColor(0xff19140e); canvas.drawRect(l,t,r,b,paint);
        paint.setColor(0xff5f4e30); paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(stroke);
        canvas.drawRect(l+stroke/2,t+stroke/2,r-stroke/2,b-stroke/2,paint); paint.setStyle(Paint.Style.FILL);
        // Shader coordinates are local after Canvas translation.
        paint.setShader(fill);
        canvas.drawRect(l+stroke,t+stroke,l+stroke+(r-l-2*stroke)*text.progress,b-stroke,paint);
        paint.setShader(null); paint.setColor(0x6e000000);
        for (int i=1;i<=9;i++) {
            float x=l+stroke+(r-l-2*stroke)*i/10;
            canvas.drawRect(x,t+stroke,x+stroke,b-stroke,paint);
        }
        for (int i=0;i<2;i++) {
            float x=i==0 ? l-4*u : r+4*u, y=(t+b)/2, radius=4*u;
            diamond.reset(); diamond.moveTo(x,y-radius); diamond.lineTo(x+radius,y);
            diamond.lineTo(x,y+radius); diamond.lineTo(x-radius,y); diamond.close();
            paint.setColor(0xffa08750); canvas.drawPath(diamond,paint);
            paint.setStyle(Paint.Style.STROKE); paint.setColor(0xff3c301e); paint.setStrokeWidth(Math.max(1/density,.8f*u));
            canvas.drawPath(diamond,paint); paint.setStyle(Paint.Style.FILL);
        }
        canvas.restoreToCount(saved);
    }
}
