"""Exercise the real Custom pump and unchanged native horizontal drag routing.

The native game boundary is a rectangular slider hit-test and mouse receiver;
the host's production drag code discovers that track and sends all inputs.
No guest, phone, save, or Android overlay is started.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]

BOUNDARY = r'''
static int down_events, up_events;
int dm_ui_panel_state(void) { return 0; }
int dm_vault_ui_drag(int phase,int x,int y) { (void)phase;(void)x;(void)y;return 0; }
int dm_vault_ui_over(int x,int y) { (void)x;(void)y;return 0; }
static uint64_t ui_hash(const char *s) {
    return !strcmp(s,"MouseMove")?1:!strcmp(s,"LeftMouseButtonDown")?2:!strcmp(s,"LeftMouseButtonUp")?3:4;
}
static int game_hit(uint64_t w,uint64_t xy) {
    int x=(int)(uint32_t)xy,y=(int)(xy>>32);
    return w==(uintptr_t)sliders[0] && x>=200 && x<=800 && y>=100 && y<=172;
}
static void ui_send_mouse(module_t *m,uint64_t root,uint64_t hash,int x,int y) {
    (void)m;(void)root; assert(y==136);
    if(hash==2) { assert(x>=200 && x<=800); ++down_events; *(int *)(sliders[0]+0x5c4)=(x-200)*96/600; }
    if(hash==3) ++up_events;
}
static void ui_send_mouse_arg(module_t *m,uint64_t root,uint64_t hash,int x,int y,int amount) {
    (void)m;(void)root;(void)hash;(void)x;(void)y;(void)amount;assert(0);
}
'''

SCENARIO = r'''
    *(uint64_t *)(b+0x20bad70+0x48)=(uintptr_t)game_hit;
    dm_ui_drag_begin(2,.2f,.136f,0);
    dm_ui_drag_move(.8f,.136f);
    for(int f=0;f<3;f++) { ui_drag_pump(&g_mod[0],(uintptr_t)root_mem,1001,1001); pump(); }
    assert(down_events==1 && up_events==1 && atomic_load(&g_pend[K_MARGIN_LEFT])==96);
    assert(!strcmp(chars[0],"96") && dm_custom_get("touch_margin_left")==0);
    dm_ui_drag_move(-.5f,.136f);
    for(int f=0;f<3;f++) { ui_drag_pump(&g_mod[0],(uintptr_t)root_mem,1001,1001); pump(); }
    assert(down_events==2 && up_events==2 && atomic_load(&g_pend[K_MARGIN_LEFT])==0);
    assert(!strcmp(chars[0],"0") && dm_custom_get("touch_margin_left")==0);
    dm_ui_drag_end(); ui_drag_pump(&g_mod[0],(uintptr_t)root_mem,1001,1001);
    /* A horizontal gesture originating outside the native track sends no clicks. */
    dm_ui_drag_begin(2,.05f,.136f,0); dm_ui_drag_move(.8f,.136f);
    ui_drag_pump(&g_mod[0],(uintptr_t)root_mem,1001,1001); dm_ui_drag_end();
    ui_drag_pump(&g_mod[0],(uintptr_t)root_mem,1001,1001); assert(down_events==2 && up_events==2);
'''


class NativeMarginSlider(unittest.TestCase):
    def test_drag_track_clamp_pending_and_settings_semantics(self):
        cc = os.environ['DM_TEST_CC']
        if os.name != 'nt':
            self.skipTest('x86-64 Windows host thunks')
        cursor = (ROOT/'dm/host/dm_cursor.c').read_text(encoding='utf-8')
        drag = cursor[cursor.index('#define UI_SCROLL_STEP'):cursor.index('static void ui_auto_key(uint64_t mask);',cursor.index('#define UI_SCROLL_STEP'))]
        source = (ROOT/'dm/host/tests/t_android_margin_slider.c').read_text(encoding='utf-8')
        source = source.replace('#include "t_android_render_settings.c"', '#include "'+(ROOT/'dm/host/tests/t_android_render_settings.c').as_posix()+'"')
        self.assertIn('int main(int argc,char **argv) {', source)
        source = source.replace('int main(int argc,char **argv) {', BOUNDARY+'\n'+drag+'\nint main(int argc,char **argv) {')
        marker = 'assert(*(int *)(sliders[0]+0x830)==0 && *(int *)(sliders[1]+0x830)==0);'
        self.assertIn(marker, source)
        source = source.replace(marker,marker+'\n'+SCENARIO)
        with tempfile.TemporaryDirectory() as tmp:
            src=Path(tmp)/'slider.c'; exe=Path(tmp)/'slider.exe'; src.write_text(source,encoding='utf-8')
            cmd=[cc]
            if Path(cc).stem=='zig': cmd+=['cc','-target','x86_64-windows-gnu']
            cmd+=['-std=c11','-UNDEBUG','-O1','-ffunction-sections','-fdata-sections',str(src),'-Wl,--gc-sections','-o',str(exe)]
            result=subprocess.run(cmd,capture_output=True,text=True,encoding='utf-8',errors='replace')
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(exe),str(Path(tmp)/'settings')],capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=30)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertIn('PASS native Custom slider',result.stdout)


if __name__=='__main__': unittest.main()
