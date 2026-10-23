package com.xiaoboluo.d2r;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.BatteryManager;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import java.util.Locale;

/** Optional cached metrics. Only its actual panel-sized View captures monitor drags. */
public final class PerformanceOverlay extends View {
    private static native void nativeSetMonitoringEnabled(boolean enabled);
    private static native void nativeReadSnapshot(float[] values, int[] dimensions);
    private static final String MISSING = "—";
    private static final long REFRESH_MS = 500;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF panel = new RectF();
    private final PerformancePanelState position = new PerformancePanelState();
    private final SharedPreferences positionPreferences;
    private final String title;
    private final String[] labels = new String[6];
    private final String[] metricText = {MISSING, MISSING, MISSING, MISSING, MISSING, MISSING};
    private final float[] values = {-1, -1, -1, -1};
    private final int[] dimensions = new int[4];
    private final float density;
    private final BroadcastReceiver batteryReceiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            if (!Intent.ACTION_BATTERY_CHANGED.equals(intent.getAction())) return;
            int t = intent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, Integer.MIN_VALUE);
            batteryTenths = t == Integer.MIN_VALUE ? Integer.MIN_VALUE : t;
            updateMetricText();
            invalidate();
        }
    };
    private final Runnable refresh = new Runnable() {
        @Override public void run() {
            if (!running) return;
            nativeReadSnapshot(values, dimensions);
            updateMetricText();
            invalidate();
            postDelayed(this, REFRESH_MS);
        }
    };
    private boolean enabled, running;
    private int fontSize = 9, transparency = 45;
    private int backgroundColor = 0x8c15171b, borderColor = 0x64947649;
    private View container;
    private final OnLayoutChangeListener containerLayout = (v, l, t, r, b, ol, ot, or, ob) -> updatePanel();
    private int batteryTenths = Integer.MIN_VALUE;
    private int safeLeft, safeTop, safeRight, safeBottom;
    public interface BoundsListener { void onBounds(float left, float top, float right, float bottom); }
    private BoundsListener boundsListener;
    private boolean reportedBounds;
    private float reportedLeft, reportedTop, reportedRight, reportedBottom;

    public void setBoundsListener(BoundsListener listener) {
        boundsListener=listener;
        reportedBounds=false;
        publishBounds();
    }

    /* Recompute on parent layout, insets or font changes, never during drawing.
       A panel-sized child also lets FrameLayout split an outside second finger
       to TouchOverlay while another finger is dragging this monitor. */
    private void updatePanel() {
        if (container == null) return;
        cancelDrag();
        position.layout(container.getWidth(), container.getHeight(), safeLeft, safeTop,
                safeRight, safeBottom, density, fontSize);
        applyPanel();
    }

    private void applyPanel() {
        ViewGroup.LayoutParams params = getLayoutParams();
        int width = (int) position.width, height = (int) position.height;
        if (params != null && (params.width != width || params.height != height)) {
            params.width = width; params.height = height;
            setLayoutParams(params);
        }
        setX(position.left); setY(position.top);
        panel.set(0, 0, position.width, position.height);
        publishBounds();
        invalidate();
    }

    private void publishBounds() {
        if (boundsListener == null) return;
        boolean visible = enabled && container != null && container.getWidth() > 0
                && container.getHeight() > 0 && position.width > 0 && position.height > 0;
        float l = visible ? position.left : 0, t = visible ? position.top : 0;
        float r = visible ? l + position.width : 0, b = visible ? t + position.height : 0;
        if (reportedBounds && reportedLeft == l && reportedTop == t
                && reportedRight == r && reportedBottom == b) return;
        reportedBounds = true;
        reportedLeft = l; reportedTop = t; reportedRight = r; reportedBottom = b;
        boundsListener.onBounds(l, t, r, b);
    }

    public PerformanceOverlay(Context context) {
        super(context);
        density = getResources().getDisplayMetrics().density;
        positionPreferences = context.getSharedPreferences("performance_overlay_position", Context.MODE_PRIVATE);
        if (positionPreferences.contains("normalized_x") && positionPreferences.contains("normalized_y"))
            position.restore(positionPreferences.getFloat("normalized_x", 0),
                    positionPreferences.getFloat("normalized_y", 0));
        title = context.getString(R.string.performance_title);
        int[] ids = {R.string.performance_fps, R.string.performance_frame, R.string.performance_gpu,
                R.string.performance_scale, R.string.performance_dimensions, R.string.performance_battery};
        for (int i = 0; i < labels.length; ++i) labels[i] = context.getString(ids[i]);
        setVisibility(GONE);
        setClickable(false);
        setFocusable(false);
    }

    /** Transparency affects only the backing panel; metric text stays opaque. */
    public void setStyle(int fontSize, int transparency) {
        fontSize = Math.max(8, Math.min(14, fontSize));
        transparency = Math.max(0, Math.min(80, transparency));
        boolean resized = this.fontSize != fontSize;
        if (!resized && this.transparency == transparency) return;
        this.fontSize = fontSize; this.transparency = transparency;
        float opacity = (100 - transparency) / 100f;
        backgroundColor = (Math.round(255 * opacity) << 24) | 0x15171b;
        borderColor = (Math.round(182 * opacity) << 24) | 0x947649;
        if (resized) updatePanel();
        else invalidate();
    }

    public void setMonitoringEnabled(boolean enabled) {
        if (this.enabled == enabled) return;
        this.enabled = enabled;
        if (!enabled) cancelDrag();
        setVisibility(enabled ? VISIBLE : GONE);
        publishBounds();
        if (enabled && isAttachedToWindow()) start();
        else if (!enabled) stop();
    }

    public void setSafeInsets(int left, int top, int right, int bottom) {
        left = Math.max(0, left); top = Math.max(0, top);
        right = Math.max(0, right); bottom = Math.max(0, bottom);
        if (safeLeft == left && safeTop == top && safeRight == right && safeBottom == bottom) return;
        safeLeft = left; safeTop = top; safeRight = right; safeBottom = bottom;
        updatePanel();
    }

    @Override protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (getParent() instanceof View) {
            container = (View) getParent();
            container.addOnLayoutChangeListener(containerLayout);
            updatePanel();
        }
        if (enabled) start();
    }

    @Override protected void onDetachedFromWindow() {
        cancelDrag();
        stop();
        if (container != null) container.removeOnLayoutChangeListener(containerLayout);
        container = null;
        publishBounds();
        super.onDetachedFromWindow();
    }

    @Override public void onWindowFocusChanged(boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);
        if (!hasWindowFocus) cancelDrag();
    }

    private void start() {
        if (running) return;
        running = true;
        nativeSetMonitoringEnabled(true);
        getContext().registerReceiver(batteryReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        post(refresh);
    }

    private void stop() {
        if (!running) return;
        running = false;
        removeCallbacks(refresh);
        getContext().unregisterReceiver(batteryReceiver);
        nativeSetMonitoringEnabled(false);
        for (int i = 0; i < values.length; ++i) values[i] = -1;
        updateMetricText();
    }

    private float dp(float amount) { return amount * density; }
    private float styledDp(float amount) { return dp(amount * fontSize / 11f); }

    private String number(float value, String unit, int decimals) {
        if (!Float.isFinite(value) || value < 0) return MISSING;
        return String.format(Locale.getDefault(), "%1$." + decimals + "f%2$s", value, unit);
    }

    private void updateMetricText() {
        metricText[0] = number(values[0], "", 1);
        metricText[1] = number(values[1], " ms", 1);
        metricText[2] = number(values[2], " ms", 1);
        metricText[3] = number(values[3] * 100, "%", 0);
        metricText[4] = dimensions[0] > 0 && dimensions[1] > 0 && dimensions[2] > 0 && dimensions[3] > 0
                ? dimensions[0] + "×" + dimensions[1] + " / " + dimensions[2] + "×" + dimensions[3] : MISSING;
        metricText[5] = batteryTenths == Integer.MIN_VALUE ? MISSING
                : String.format(Locale.getDefault(), "%.1f °C", batteryTenths / 10.0);
    }

    private void row(Canvas canvas, float x, float right, float baseline, String label, String value) {
        paint.setTypeface(null);
        paint.setTextSize(dp(fontSize));
        paint.setColor(0xffbdad91);
        canvas.drawText(label, x, baseline, paint);
        paint.setColor(0xfff3e3be);
        paint.setTextAlign(Paint.Align.RIGHT);
        canvas.drawText(value, right, baseline, paint);
        paint.setTextAlign(Paint.Align.LEFT);
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (!enabled) return;
        float top=panel.top;
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(backgroundColor);
        canvas.drawRoundRect(panel, styledDp(8), styledDp(8), paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(dp(1));
        paint.setColor(borderColor);
        canvas.drawRoundRect(panel, styledDp(8), styledDp(8), paint);
        paint.setStyle(Paint.Style.FILL);
        float x = panel.left + styledDp(12), valueRight = panel.right - styledDp(12);
        paint.setColor(0xffcaa96b);
        paint.setTextSize(dp(fontSize));
        paint.setFakeBoldText(true);
        canvas.drawText(title, x, top + styledDp(19), paint);
        paint.setFakeBoldText(false);
        float y = top + styledDp(39), gap = styledDp(19);
        for (int i = 0; i < labels.length; ++i)
            row(canvas, x, valueRight, y + gap * i, labels[i], metricText[i]);
    }

    private void cancelDrag() {
        if (position.gestureOwned && getParent() != null)
            getParent().requestDisallowInterceptTouchEvent(false);
        position.cancel();
    }

    private void savePosition() {
        positionPreferences.edit().putFloat("normalized_x", position.normalizedX)
                .putFloat("normalized_y", position.normalizedY).apply();
    }

    private void movePrimary(MotionEvent event) {
        int index = event.findPointerIndex(position.primaryPointer());
        if (index >= 0 && position.move(event.getPointerId(index),
                event.getX(index) + position.left, event.getY(index) + position.top)) applyPanel();
    }

    @Override public boolean onTouchEvent(MotionEvent event) {
        if (!enabled) return false;
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            cancelDrag();
            if (!position.begin(event.getPointerId(0), event.getX() + position.left,
                    event.getY() + position.top)) return false;
            if (getParent() != null) getParent().requestDisallowInterceptTouchEvent(true);
            return true;
        }
        if (!position.gestureOwned) return false;
        if (action == MotionEvent.ACTION_MOVE) movePrimary(event);
        else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            movePrimary(event);
            boolean released = position.release(event.getPointerId(event.getActionIndex()));
            if (released) savePosition();
            if (action == MotionEvent.ACTION_UP) {
                if (released) performClick();
                cancelDrag();
            }
        } else if (action == MotionEvent.ACTION_CANCEL) cancelDrag();
        return true;
    }

    @Override public boolean performClick() { super.performClick(); return true; }
}

/** Android-independent geometry and pointer ownership used by the real monitor View. */
final class PerformancePanelState {
    float left, top, width, height;
    float normalizedX, normalizedY;
    boolean gestureOwned;
    private boolean hasPosition;
    private float minLeft, minTop, travelX, travelY, grabX, grabY;
    private int primaryPointer = -1;

    void restore(float x, float y) {
        hasPosition = Float.isFinite(x) && Float.isFinite(y);
        normalizedX = normalized(x);
        normalizedY = normalized(y);
    }

    void layout(float containerWidth, float containerHeight, float safeLeft, float safeTop,
                float safeRight, float safeBottom, float density, int fontSize) {
        float usableWidth = Math.max(0, containerWidth - safeLeft - safeRight);
        float usableHeight = Math.max(0, containerHeight - safeTop - safeBottom);
        minLeft = safeLeft;
        minTop = safeTop;
        float scale = density * Math.max(8, Math.min(14, fontSize)) / 11f;
        width = (float) Math.floor(Math.min(284 * scale, usableWidth));
        height = (float) Math.floor(Math.min(164 * scale, usableHeight));
        travelX = Math.max(0, usableWidth - width);
        travelY = Math.max(0, usableHeight - height);
        if (hasPosition) {
            left = minLeft + normalizedX * travelX;
            top = minTop + normalizedY * travelY;
        } else {
            // Initial placement leaves room for the map, but is not a drag boundary.
            left = minLeft + Math.min(10 * density, travelX);
            top = minTop + Math.min(48 * density, travelY);
            normalizedX = travelX > 0 ? (left - minLeft) / travelX : 0;
            normalizedY = travelY > 0 ? (top - minTop) / travelY : 0;
        }
    }

    boolean begin(int pointer, float x, float y) {
        if (gestureOwned || width <= 0 || height <= 0 || x < left || y < top
                || x >= left + width || y >= top + height) return false;
        gestureOwned = true;
        primaryPointer = pointer;
        grabX = x - left;
        grabY = y - top;
        return true;
    }

    boolean move(int pointer, float x, float y) {
        if (!gestureOwned || primaryPointer != pointer || primaryPointer < 0) return false;
        float nextLeft = Math.max(minLeft, Math.min(minLeft + travelX, x - grabX));
        float nextTop = Math.max(minTop, Math.min(minTop + travelY, y - grabY));
        if (nextLeft == left && nextTop == top) return false;
        hasPosition = true;
        left = nextLeft;
        top = nextTop;
        if (travelX > 0) normalizedX = (left - minLeft) / travelX;
        if (travelY > 0) normalizedY = (top - minTop) / travelY;
        return true;
    }

    boolean release(int pointer) {
        if (!gestureOwned || primaryPointer < 0 || primaryPointer != pointer) return false;
        hasPosition = true;
        primaryPointer = -1;
        return true;
    }

    int primaryPointer() { return primaryPointer; }

    void cancel() {
        gestureOwned = false;
        primaryPointer = -1;
    }

    private static float normalized(float value) {
        return Float.isFinite(value) ? Math.max(0, Math.min(1, value)) : 0;
    }
}
