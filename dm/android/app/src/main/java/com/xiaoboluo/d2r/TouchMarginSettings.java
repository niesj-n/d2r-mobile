package com.xiaoboluo.d2r;

/** Small platform-free policy for the two persisted Android touch margins. */
final class TouchMarginSettings {
    static final int MAX_DP = 96;
    interface Store {
        int get(String key);
        boolean put(int left, int right);
    }
    static final class Values {
        final int left, right;
        Values(int left, int right) { this.left = left; this.right = right; }
    }
    private static int bounded(int value) { return Math.max(0, Math.min(MAX_DP, value)); }
    static Values load(Store store) { return new Values(bounded(store.get("left")), bounded(store.get("right"))); }
    static boolean save(Store store, int left, int right) { return store.put(bounded(left), bounded(right)); }
    static boolean reset(Store store) { return save(store, 0, 0); }
    private TouchMarginSettings() {}
}
