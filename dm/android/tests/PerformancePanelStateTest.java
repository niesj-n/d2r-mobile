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
        near(30, p.left, "left clamped inside safe inset");
        near(60, p.top, "top clamped below reserved band");
        p.move(7, 5000, 5000);
        near(860, p.left + p.width, "right clamped before inset");
        near(488, p.top + p.height, "bottom clamped before inset");
        near(1, p.normalizedX, "right edge normalizes to 1");
        near(1, p.normalizedY, "bottom edge normalizes to 1");
        p.release(7); p.cancel();
        PerformancePanelState restored = new PerformancePanelState();
        restored.restore(p.normalizedX, p.normalizedY);
        restored.layout(500, 900, 4, 25, 6, 15, 1, 14);
        near(484, restored.left + restored.width, "rotation/font restore at right edge");
        near(881, restored.top + restored.height, "rotation/font restore at bottom edge");
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
