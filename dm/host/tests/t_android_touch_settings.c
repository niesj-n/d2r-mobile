/* Exercise the Android Custom touch settings using the real load/apply path. */
#define main render_settings_main
#include "t_android_render_settings.c"
#undef main

static void message(const char *action) { dm_custom_on_uimsg("OptionsPanelMessage", action, ""); }
static void arrow(const char *key, const char *direction) {
    char name[80]; snprintf(name, sizeof name, "TouchCustom_%s_%s", key, direction);
    dm_custom_on_uimsg("PanelManager", "ClosePanel", name);
}
static void write_settings(const char *path, const char *text) {
    FILE *f = fopen(path, "w"); assert(f); assert(fputs(text, f) >= 0); assert(fclose(f) == 0);
    atomic_store(&g_loaded, 0);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    int snapshot[3] = { -7, -7, -7 };
    assert(dm_custom_android_read_applied(snapshot) == 0 && snapshot[0] == -7);
    dm_custom_init(argv[1]); dm_custom_android_seed_legacy(0, 0, 0);
    assert(!atomic_load(&g_loaded) && dm_custom_android_read_applied(snapshot) == 0);
    assert(test_mkdir(argv[1]) == 0);
    assert(key_index("touch_margin_left") == K_TOUCH_SWAP + 1);
    assert(key_index("touch_margin_right") == K_TOUCH_SWAP + 2);
    assert(key_index("perf_overlay") == K_TOUCH_SWAP + 3);
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("touch_margin_right") == 0 && dm_custom_get("perf_overlay") == 0);
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 0 && snapshot[1] == 0 && snapshot[2] == 0);
    arrow("touch_margin_left", "prev"); assert(atomic_load(&g_pend[key_index("touch_margin_left")]) <= 0);
    arrow("touch_margin_left", "next"); arrow("touch_margin_right", "next");
    atomic_store(&g_pend[key_index("perf_overlay")], 1);
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("perf_overlay") == 0);
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 0 && snapshot[2] == 0);
    message("CancelChanges"); message("AcceptChanges");
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("perf_overlay") == 0);
    for (int i = 0; i < 110; i++) arrow("touch_margin_left", "next");
    for (int i = 0; i < 14; i++) arrow("touch_margin_right", "next");
    atomic_store(&g_pend[key_index("perf_overlay")], 1); message("AcceptChanges");
    assert(dm_custom_get("touch_margin_left") == 96 && dm_custom_get("touch_margin_right") == 14 && dm_custom_get("perf_overlay") == 1);
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 96 && snapshot[1] == 14 && snapshot[2] == 1);
    char path[1200]; snprintf(path, sizeof path, "%s/sw_custom.txt", argv[1]);
    FILE *f = fopen(path, "r"); assert(f); char text[4096]; size_t n = fread(text, 1, sizeof text - 1, f); fclose(f); text[n] = 0;
    assert(strstr(text, "touch_margin_left=96\n") && strstr(text, "touch_margin_right=14\n") && strstr(text, "perf_overlay=1\n"));
    atomic_store(&g_loaded, 0); assert(dm_custom_get("touch_margin_right") == 14);
    message("ResetToDefaults"); assert(dm_custom_get("touch_margin_left") == 96 && dm_custom_get("perf_overlay") == 1);
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 96 && snapshot[2] == 1);
    message("CancelChanges"); message("AcceptChanges"); assert(dm_custom_get("touch_margin_right") == 14);
    message("ResetToDefaults"); message("AcceptChanges"); atomic_store(&g_loaded, 0);
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("touch_margin_right") == 0 && dm_custom_get("perf_overlay") == 0);
    const char *invalid[] = { "-1", "97", "999999999999999999999999999", "nan", "1x", "" };
    for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        char data[256]; snprintf(data, sizeof data, "touch_margin_left=%s\ntouch_margin_right=%s\nperf_overlay=%s\n", invalid[i], invalid[i], invalid[i]);
        write_settings(path, data); assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("touch_margin_right") == 0 && dm_custom_get("perf_overlay") == 0);
    }
    write_settings(path, "touch_margin_left=96\ntouch_margin_right=1\nperf_overlay=1\n");
    assert(dm_custom_get("touch_margin_left") == 96 && dm_custom_get("touch_margin_right") == 1 && dm_custom_get("perf_overlay") == 1);
    arrow("touch_margin_left", "prev"); arrow("touch_margin_right", "prev"); message("AcceptChanges");
    assert(dm_custom_get("touch_margin_left") == 95 && dm_custom_get("touch_margin_right") == 0);
    /* Old preferences seed only missing native keys; all native values win. */
    write_settings(path, "touch_margin_right=7\n");
    dm_custom_android_seed_legacy(19, 37, 1);
    snapshot[0] = -7;
    assert(dm_custom_android_read_applied(snapshot) == 0 && snapshot[0] == -7 && !atomic_load(&g_loaded));
    assert(dm_custom_get("touch_margin_left") == 19 && dm_custom_get("touch_margin_right") == 7 && dm_custom_get("perf_overlay") == 1);
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 19 && snapshot[1] == 7 && snapshot[2] == 1);
    dm_custom_android_seed_legacy(96, 96, 0); /* Late seed cannot reset loaded/applied settings. */
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 19 && snapshot[1] == 7 && snapshot[2] == 1);
    write_settings(path, "touch_margin_left=0\ntouch_margin_right=96\nperf_overlay=0\n");
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("touch_margin_right") == 96 && dm_custom_get("perf_overlay") == 0);
    write_settings(path, "touch_margin_left=97\ntouch_margin_right=nan\nperf_overlay=2\n");
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("touch_margin_right") == 0 && dm_custom_get("perf_overlay") == 0);
    write_settings(path, ""); dm_custom_android_seed_legacy(-1, 999, 1);
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("touch_margin_right") == 96 && dm_custom_get("perf_overlay") == 1);
    arrow("touch_margin_left", "next"); message("AcceptChanges");
    atomic_store(&g_loaded, 0); dm_custom_android_seed_legacy(50, 50, 0);
    assert(dm_custom_get("touch_margin_left") == 1 && dm_custom_get("touch_margin_right") == 96 && dm_custom_get("perf_overlay") == 1);
    /* Reading the snapshot cannot reload even if the file changed underneath. */
    FILE *changed = fopen(path, "w"); assert(changed); fputs("touch_margin_left=22\n", changed); fclose(changed);
    assert(dm_custom_android_read_applied(snapshot) == 1 && snapshot[0] == 1 && snapshot[1] == 96 && snapshot[2] == 1);
    assert(dm_custom_android_read_applied(NULL) == 0);
    puts("PASS Android touch Custom defaults, append-only indices, selectors, pending/discard/apply, reload, defaults, invalid boundaries, legacy precedence and IO-free applied snapshot");
    return 0;
}
