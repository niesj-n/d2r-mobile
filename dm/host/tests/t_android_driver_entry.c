/* The real Custom UI message opens driver management without applying settings. */
#define main render_settings_main
#include "t_android_render_settings.c"
#undef main

static int requests;
void dm_android_driver_settings_request(void) { ++requests; }

static void open_driver(void) { dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchDriverSettings"); }
int main(int argc, char **argv) {
    assert(argc == 2); assert(test_mkdir(argv[1]) == 0); dm_custom_init(argv[1]);
    open_driver(); assert(requests == 1);
    assert(!atomic_load(&g_loaded)); /* The action does not load/save settings. */
    assert(NK == 20 + VOICE_ROW && K_PERF == 19 + VOICE_ROW);
    assert(key_index("TouchDriverSettings") == -1 && key_index("driver_settings") == -1);
    dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchDriverSettingsExtra");
    dm_custom_on_uimsg("PanelManager", "AcceptChanges", "TouchDriverSettings");
    dm_custom_on_uimsg("OtherPanel", "ClosePanel", "TouchDriverSettings");
    dm_custom_on_uimsg(NULL, "ClosePanel", "TouchDriverSettings");
    dm_custom_on_uimsg("PanelManager", NULL, "TouchDriverSettings");
    dm_custom_on_uimsg("PanelManager", "ClosePanel", NULL);
    assert(requests == 1 && !atomic_load(&g_loaded));
    assert(dm_custom_get("touch_margin_left") == 0 && dm_custom_get("perf_overlay") == 0);
    dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustom_touch_margin_left_next");
    atomic_store(&g_pend[K_PERF], 1);
    int before[NK]; for (int i = 0; i < NK; ++i) before[i] = atomic_load(&g_pend[i]);
    open_driver(); assert(requests == 2);
    for (int i = 0; i < NK; ++i) assert(atomic_load(&g_pend[i]) == before[i]);
    int out[3]; assert(dm_custom_android_read_applied(out) == 1 && out[0] == 0 && out[2] == 0);
    char path[1200]; snprintf(path, sizeof path, "%s/sw_custom.txt", argv[1]);
    FILE *f = fopen(path, "r"); assert(!f); /* Opening manager has not persisted anything. */
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", "");
    assert(dm_custom_android_read_applied(out) == 1 && out[0] == 1 && out[2] == 1);
    f = fopen(path, "r"); assert(f); char text[4096]; size_t n = fread(text, 1, sizeof text - 1, f); fclose(f); text[n] = 0;
    assert(strstr(text, "touch_margin_left=1\n") && strstr(text, "perf_overlay=1\n"));
    assert(!strstr(text, "driver") && !strstr(text, "TouchDriverSettings"));
    open_driver(); assert(requests == 3);
    assert(dm_custom_android_read_applied(out) == 1 && out[0] == 1 && out[2] == 1);
    atomic_store(&g_loaded, 0); assert(dm_custom_get("touch_margin_left") == 1 && dm_custom_get("perf_overlay") == 1);
    puts("PASS Android driver manager entry exact message, one request per click, unchanged pending/applied values and canonical persistence");
    return 0;
}
