package com.xiaoboluo.d2r;

public final class TouchActionBarSizingTest {
    private static void check(boolean ok) { if (!ok) throw new AssertionError("action bar exceeds safe width"); }
    public static void main(String[] args) {
        check(TouchActionBarSizing.cellWidth(800, 0, 0, 1f)==48);
        int compact = TouchActionBarSizing.cellWidth(280, 18, 16, 1f);
        check(compact==49 || compact==48); // 246 usable px still permits full-size controls.
        int narrow = TouchActionBarSizing.cellWidth(240, 18, 16, 1f);
        check(narrow>0 && narrow*5<=240-18-16);
        check(TouchActionBarSizing.cellWidth(100, 20, 20, 2f)*5<=60);
        System.out.println("PASS gameplay action bar fits safe width");
    }
}
