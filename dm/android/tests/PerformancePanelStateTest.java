package com.xiaoboluo.d2r;

/** Runs against the actual package-private state compiled from PerformanceOverlay.java. */
public final class PerformancePanelStateTest {
    private static int checks;
    private static void check(boolean value, String message) {
        ++checks;
        if (!value) throw new AssertionError(message);
    }
    private static void near(float expected, float actual, String message) {
        check(Math.abs(expected - actual) < .01f, message + ": " + actual);
    }
    private static PerformancePanelState panel() {
        PerformancePanelState state = new PerformancePanelState();
        state.layout(900, 500, 20, 12, 30, 8, 1, 9);
        return state;
    }
    public static void main(String[] args) {
        PerformancePanelState edge = new PerformancePanelState();
        edge.layout(900, 500, 0, 0, 0, 0, 1, 9);
        near(10, edge.left, "unsaved default keeps left offset");
        near(48, edge.top, "unsaved default keeps top offset");
        check(edge.begin(1, edge.left + 2, edge.top + 2), "capture default panel for edge drag");
        edge.move(1, -100, -100);
        near(0, edge.top, "manual drag reaches actual top edge");
        near(0, edge.left, "manual drag reaches actual left edge");
        near(0, edge.normalizedY, "actual top saves normalized zero");
        near(0, edge.normalizedX, "actual left saves normalized zero");
        edge.release(1); edge.cancel();
        PerformancePanelState topRestored = new PerformancePanelState();
        topRestored.restore(edge.normalizedX, edge.normalizedY);
        topRestored.layout(900, 500, 0, 0, 0, 0, 1, 9);
        near(0, topRestored.top, "saved normalized zero restores to top, not default 48");
        topRestored.layout(500, 900, 17, 27, 20, 30, 1, 14);
        near(27, topRestored.top, "rotation and font change retain actual safe top");
        near(17, topRestored.left, "rotation and font change retain actual safe left");
        check(topRestored.begin(3, 19, 29), "capture at nonzero safe inset");
        topRestored.move(3, -100, -100);
        near(27, topRestored.top, "manual drag clamps to nonzero safe top");
        near(17, topRestored.left, "manual drag clamps to nonzero safe left");
        topRestored.cancel();
        PerformancePanelState p = panel();
        near(30, p.left, "10 dp left reservation");
        near(60, p.top, "48 dp top reservation");
        near((float) Math.floor(284 * 9f / 11), p.width, "smaller default width");
        near((float) Math.floor(164 * 9f / 11), p.height, "smaller default height");
        check(!p.begin(2, p.left - 1, p.top), "outside DOWN passes through");
        check(!p.gestureOwned, "outside DOWN owns no pointer");
        check(!p.begin(2, p.left + p.width, p.top), "right edge is outside");
        check(p.begin(2, p.left + 8, p.top + 9), "inside DOWN captures");
        check(!p.begin(3, p.left + 10, p.top + 10), "second pointer cannot replace primary");
        check(!p.move(3, 800, 400), "secondary MOVE ignored");
        check(p.move(2, 508, 209), "primary MOVE changes position");
        near(500, p.left, "drag preserves x grab offset");
        near(200, p.top, "drag preserves y grab offset");
        check(!p.release(3), "secondary UP cannot finish primary");
        check(p.release(2), "primary UP completes drag");
        check(p.gestureOwned, "remaining captured pointers consumed until final UP");
        check(!p.move(2, 300, 200), "released primary cannot move");
        p.cancel();
        check(!p.gestureOwned, "CANCEL clears touch ownership");
        check(p.begin(7, p.left + 4, p.top + 4), "next DOWN after CANCEL works");
        p.move(7, -200, -300);
        near(20, p.left, "left clamped to actual safe inset");
        near(12, p.top, "top clamped to actual safe inset");
        p.move(7, 5000, 5000);
        near(870, p.left + p.width, "right clamped at actual safe inset");
        near(492, p.top + p.height, "bottom clamped at actual safe inset");
        near(1, p.normalizedX, "right edge normalizes to 1");
        near(1, p.normalizedY, "bottom edge normalizes to 1");
        p.release(7); p.cancel();
        PerformancePanelState restored = new PerformancePanelState();
        restored.restore(p.normalizedX, p.normalizedY);
        restored.layout(500, 900, 4, 25, 6, 15, 1, 14);
        near(494, restored.left + restored.width, "rotation/font restore at right edge");
        near(885, restored.top + restored.height, "rotation/font restore at bottom edge");
        restored.restore(.35f, .65f);
        restored.layout(900, 500, 20, 12, 30, 8, 1, 8);
        float rememberedLeft = restored.left, rememberedTop = restored.top;
        restored.layout(300, 120, 20, 12, 30, 8, 1, 14);
        check(restored.left >= 20 && restored.top >= 12, "small screen stays inside safe origin");
        check(restored.left + restored.width <= 270 && restored.top + restored.height <= 112,
                "oversized panel shrinks within safe end");
        restored.layout(900, 500, 20, 12, 30, 8, 1, 8);
        near(rememberedLeft, restored.left, "normalized x survives zero available travel");
        near(rememberedTop, restored.top, "normalized y survives zero available travel");
        restored.restore(Float.NaN, Float.POSITIVE_INFINITY);
        restored.layout(900, 500, 20, 12, 30, 8, 1, 9);
        near(30, restored.left, "invalid saved x falls back safely");
        near(60, restored.top, "invalid saved y falls back safely");
        check(restored.begin(8, restored.left + 1, restored.top + 1), "capture before layout change");
        restored.cancel();
        check(!restored.release(8), "cancelled gesture cannot persist on later UP");
        restored.layout(0, 0, 0, 0, 0, 0, 1, 9);
        check(!restored.begin(9, 0, 0), "empty panel never captures");
        System.out.println("PASS PerformancePanelState: " + checks + " assertions");
    }
}
