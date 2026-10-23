package com.xiaoboluo.d2r;

import java.util.HashMap;
import java.util.Map;

public final class TouchMarginSettingsTest {
    private static final class MemoryStore implements TouchMarginSettings.Store {
        final Map<String, Integer> values = new HashMap<>();
        boolean fail;
        public int get(String key) { return values.getOrDefault(key, 0); }
        public boolean put(int left, int right) {
            if (fail) return false;
            values.put("left", left);
            values.put("right", right);
            return true;
        }
    }
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
    public static void main(String[] args) {
        MemoryStore store = new MemoryStore();
        TouchMarginSettings.Values defaults = TouchMarginSettings.load(store);
        check(defaults.left == 0 && defaults.right == 0, "default margins");
        TouchMarginSettings.save(store, 29, 77);
        TouchMarginSettings.Values restarted = TouchMarginSettings.load(store);
        check(restarted.left == 29 && restarted.right == 77, "independent values survive restart");
        TouchMarginSettings.save(store, 130, -7);
        restarted = TouchMarginSettings.load(store);
        check(restarted.left == 96 && restarted.right == 0, "persisted values bounded");
        store.values.put("left", -5);
        store.values.put("right", 1000);
        restarted = TouchMarginSettings.load(store);
        check(restarted.left == 0 && restarted.right == 96, "stored invalid values bounded");
        TouchMarginSettings.reset(store);
        restarted = TouchMarginSettings.load(store);
        check(restarted.left == 0 && restarted.right == 0, "reset persisted");
        store.fail = true;
        check(!TouchMarginSettings.save(store, 12, 34), "failed disk write reported");
        restarted = TouchMarginSettings.load(store);
        check(restarted.left == 0 && restarted.right == 0, "failed write does not claim new values");
        System.out.println("PASS touch margin defaults, independent persistence, bounds, reset");
    }
}
