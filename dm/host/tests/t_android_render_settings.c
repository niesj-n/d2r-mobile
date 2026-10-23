/* Exercise the real Custom settings load, pending/apply, discard and file save. */
#ifdef NDEBUG
#error Android Custom settings regression requires active assertions
#endif
#ifndef __ANDROID__
#define __ANDROID__ 1
#endif
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
static int test_mkdir(const char *path) { return _mkdir(path); }
#else
#include <sys/stat.h>
static int test_mkdir(const char *path) { return mkdir(path, 0700); }
#endif
static size_t test_strlcpy(char *dest, const char *src, size_t cap) {
    size_t n = strlen(src);
    if (cap) { size_t copied = n < cap - 1 ? n : cap - 1; memcpy(dest, src, copied); dest[copied] = 0; }
    return n;
}
#define strlcpy test_strlcpy
#ifdef _WIN32
static int test_rename(const char *source, const char *target);
#define rename test_rename  /* Windows host emulates Android/POSIX atomic replace. */
#endif
#include "../dm_custom.c"
#undef strlcpy
#ifdef _WIN32
#undef rename
#endif
#include <assert.h>

#ifdef _WIN32
static int test_rename(const char *source, const char *target) {
    return MoveFileExA(source, target, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}
#endif

module_t g_mod[MAX_MOD]; int g_nmod;
int dm_lang_group(void) { return DM_LG_EN; }
const char *dm_save_commitdir(void) { return NULL; }
static int quality_mode;
void dm_3d_quality_mode_set(int mode) { quality_mode = mode; }
int dm_3d_quality_mode_get(void) { return quality_mode; }
float dm_3d_scale_cur(void) { return 1.0f; }
int dm_ui_top_chk_on(void) { return 0; }
void dm_ui_top_chk(uint64_t root, const char *name, uint64_t got) { (void)root; (void)name; (void)got; }
uint32_t dm_ui_gen(void) { return 0; }
uint64_t dm_wref_get(dm_wref *r) { return r->w; }
int dm_padmode_current(void) { return 0; }
uint64_t dm_padmode_page_keys(uint64_t keys) { (void)keys; return 0; }
uint64_t dm_guest_frame(void) { return 0; }
void dm_ui_wheel_on(uint64_t w, int n) { (void)w; (void)n; }

int main(int argc, char **argv) {
    assert(argc == 2); assert(test_mkdir(argv[1]) == 0);
    dm_custom_init(argv[1]);
    assert(fabsf(dm_custom_render_scale() - .85f) < .0001f);
    assert(dm_custom_get("render_dynamic") == 0 && dm_fps_setting() == 60 && quality_mode == 1);
    dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustom_render_scale_next");
    assert(fabsf(dm_custom_render_scale() - .85f) < .0001f);
    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", "");
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", "");
    assert(fabsf(dm_custom_render_scale() - .85f) < .0001f);
    for (int i = 0; i < 3; ++i) dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustom_render_scale_next");
    dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustom_eco_fps_prev");
    atomic_store(&g_pend[K_DYNAMIC], 1);  /* equivalent to tapping the actual Custom checkbox */
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", "");
    assert(fabsf(dm_custom_render_scale() - 1.25f) < .0001f);
    assert(dm_custom_get("render_dynamic") == 1 && dm_fps_setting() == 30 && quality_mode == 0);
    char path[1200]; snprintf(path, sizeof path, "%s/sw_custom.txt", argv[1]);
    FILE *file = fopen(path, "r"); assert(file); char text[4096]; size_t n = fread(text, 1, sizeof text - 1, file); fclose(file); text[n] = 0;
    assert(strstr(text, "render_scale=8\n") && strstr(text, "render_dynamic=1\n") && strstr(text, "eco_fps=1\n"));
    atomic_store(&g_loaded, 0);  /* simulated next launch; parse the saved file again */
    assert(fabsf(dm_custom_render_scale() - 1.25f) < .0001f && dm_custom_get("render_dynamic") == 1 && dm_fps_setting() == 30);
    dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", "");
    assert(fabsf(dm_custom_render_scale() - 1.25f) < .0001f);
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", "");
    assert(fabsf(dm_custom_render_scale() - .85f) < .0001f && dm_custom_get("render_dynamic") == 0 && dm_fps_setting() == 60);
    atomic_store(&g_loaded, 0);
    assert(fabsf(dm_custom_render_scale() - .85f) < .0001f && dm_custom_get("render_dynamic") == 0 && dm_fps_setting() == 60);
    file = fopen(path, "w"); assert(file); fputs("render_scale=nan\nrender_dynamic=nan\neco_fps=0\n", file); fclose(file);
    atomic_store(&g_loaded, 0);
    assert(fabsf(dm_custom_render_scale() - .85f) < .0001f && dm_custom_get("render_dynamic") == 0);
    puts("PASS Android Custom settings default, pending/discard, apply, persistence/reload and reset");
    return 0;
}
