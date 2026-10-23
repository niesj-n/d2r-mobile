package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.Choreographer;
import android.view.View;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;

/** Passive information layer; receives no touches and never reads guest state. */
public final class MapGoalOverlay extends View {
    private static native void nativeSnapshot(float[] values);
    private final float[] snapshot=new float[MapGoalLayout.FLOATS],probe=new float[MapGoalLayout.FLOATS];
    private final float[] controls=new float[235];
    private final HashMap<String,Bitmap> images=new HashMap<>();
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final File root;
    private boolean enabled,running;
    private int safeLeft,safeTop,safeRight,safeBottom;
    private MapGoalLayout.Box monitor,info;
    private final Choreographer.FrameCallback poll=new Choreographer.FrameCallback(){
        @Override public void doFrame(long time){
            if(!running)return;
            nativeSnapshot(probe);
            if(!Arrays.equals(snapshot,probe)){System.arraycopy(probe,0,snapshot,0,snapshot.length);invalidate();}
            Choreographer.getInstance().postFrameCallback(this);
        }
    };
    public MapGoalOverlay(Context context){
        super(context);root=new File(context.getFilesDir(),"dm/overlay/mapgoal");
        setWillNotDraw(false);setClickable(false);setFocusable(false);
        setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
    }
    public void setRuntimeEnabled(boolean active){
        if(enabled==active)return;enabled=active;
        if(active)images.clear();else Arrays.fill(snapshot,0);
        updatePolling();invalidate();
    }
    public void setSafeInsets(int l,int t,int r,int b){if(safeLeft==l&&safeTop==t&&safeRight==r&&safeBottom==b)return;safeLeft=l;safeTop=t;safeRight=r;safeBottom=b;invalidate();}
    public void setMonitorBounds(float l,float t,float r,float b){monitor=new MapGoalLayout.Box(l,t,r,b);invalidate();}
    public void setInfoBounds(float l,float t,float r,float b){info=new MapGoalLayout.Box(l,t,r,b);invalidate();}
    public void setControlLayout(float[] values){
        if(values==null||values.length<235){Arrays.fill(controls,0);invalidate();return;}
        boolean changed=false;for(int i=0;i<235;i++)if(controls[i]!=values[i]){changed=true;break;}
        if(changed){System.arraycopy(values,0,controls,0,235);invalidate();}
    }
    private void updatePolling(){boolean want=enabled&&isAttachedToWindow()&&getWindowVisibility()==VISIBLE;
        if(want==running)return;running=want;
        if(running)Choreographer.getInstance().postFrameCallback(poll);else Choreographer.getInstance().removeFrameCallback(poll);
    }
    @Override protected void onAttachedToWindow(){super.onAttachedToWindow();updatePolling();}
    @Override protected void onDetachedFromWindow(){running=false;Choreographer.getInstance().removeFrameCallback(poll);super.onDetachedFromWindow();}
    @Override protected void onWindowVisibilityChanged(int v){super.onWindowVisibilityChanged(v);updatePolling();}
    private Bitmap image(String name){
        if(images.containsKey(name))return images.get(name);
        BitmapFactory.Options options=new BitmapFactory.Options();options.inScaled=false;
        Bitmap result=BitmapFactory.decodeFile(new File(root,name).getAbsolutePath(),options);
        images.put(name,result);return result;
    }
    private static MapGoalLayout.Box box(float x,float y,float w,float h){return new MapGoalLayout.Box(x-w/2,y-h/2,x+w/2,y+h/2);}
    private static boolean clear(MapGoalLayout.Box b,MapGoalLayout.Box[] blocked){for(MapGoalLayout.Box ob:blocked)if(b.intersects(ob))return false;return true;}
    private void draw(Canvas c,Bitmap bitmap,MapGoalLayout.Box b){if(bitmap!=null&&b!=null)c.drawBitmap(bitmap,null,new RectF(b.left,b.top,b.right,b.bottom),paint);}
    @Override protected void onDraw(Canvas c){
        super.onDraw(c);if(!enabled||!MapGoalLayout.gameplay(controls))return;
        float w=getWidth(),h=getHeight();MapGoalLayout.Box safe=new MapGoalLayout.Box(safeLeft,safeTop,w-safeRight,h-safeBottom);if(!safe.valid())return;
        ArrayList<MapGoalLayout.Box> obstacles=new ArrayList<>(Arrays.asList(MapGoalLayout.controlBounds(controls,w,h)));
        if(monitor!=null&&monitor.valid())obstacles.add(monitor);if(info!=null&&info.valid())obstacles.add(info);
        MapGoalLayout.Box[] blocked=obstacles.toArray(new MapGoalLayout.Box[0]);
        for(int i=0;i<12;i++){
            MapGoalLayout.Goal g=MapGoalLayout.project(snapshot,i,w,h);if(g==null)continue;
            Bitmap mark=g.inside?image(MapGoalLayout.markerName(g)):null;
            if(mark!=null){float mh=g.symbol?g.symbolHeight:g.labelHeight*1.4f*18/34,mw=g.symbol?mh*mark.getWidth()/mark.getHeight():g.labelHeight*1.4f;
                MapGoalLayout.Box mb=box(g.x,g.y,mw,mh);
                // Markers retain the host's exact position: obstruction hides them.
                if(mb.left>=safe.left&&mb.right<=safe.right&&mb.top>=safe.top&&mb.bottom<=safe.bottom&&clear(mb,blocked))draw(c,mark,mb);else mark=null;
            }
            if(g.inside&&g.symbol)continue;
            Bitmap label=MapGoalLayout.label(g,(int)snapshot[13],snapshot[14]!=0,this::image);
            if(label==null)continue; // Original fallback only; never invent text/art.
            float lh=g.labelHeight,lw=lh*label.getWidth()/label.getHeight();
            float y=mark!=null?g.y-g.labelHeight*1.4f*18/34/2-2-lh/2:g.y;
            MapGoalLayout.Box area=new MapGoalLayout.Box(Math.max(safe.left,g.area.left),Math.max(safe.top,g.area.top),Math.min(safe.right,g.area.right),Math.min(safe.bottom,g.area.bottom));
            MapGoalLayout.Box placed=MapGoalLayout.place(g.x,y,lw,lh,area,blocked);
            draw(c,label,placed);
        }
    }
}
