package com.xiaoboluo.d2r;

/** The two map controls and settings fit between the display cutout edges. */
final class TouchActionBarSizing {
    static int cellWidth(int framePx, int safeLeftPx, int safeRightPx, float density) {
        int inner = Math.max(0, framePx - Math.max(0, safeLeftPx) - Math.max(0, safeRightPx));
        int preferred = Math.max(1, Math.round(48 * density));
        return Math.min(preferred, inner / 3);
    }
    private TouchActionBarSizing() {}
}
