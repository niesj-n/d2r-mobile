"""Real Android event queue -> real dm_cursor touch/drag pump regression.

Only guest widgets and delivered mouse messages are faked. Production gesture
routing, touch cancellation, modal gates, track discovery, clamping and scroll
steps are compiled directly from their actual C sources, without rewriting them.
"""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
CURSOR = ROOT / 'dm/host/dm_cursor.c'
PRELUDE = r'''
#include <stdatomic.h>
typedef struct { uintptr_t base; } module_t;
static _Atomic int g_ut_phase, g_fe_opt_open, g_fe_confirm_open, g_fe_back, g_fe_opt_side;
static _Atomic int g_fe_name_open, g_fe_del_open, g_del_q, g_del_hold;
static _Atomic uint64_t g_ut_xy;
#define BLISS_Y0 1290
#define BLISS_Y1 1470
#define DM_LAT(...) ((void)0)
#define LOG(...) ((void)0)
int dm_restart_busy(void) { return 0; }
int dm_vault_ui_drag(int phase,int x,int y) { (void)phase;(void)x;(void)y; unexpected_host_call(__func__); return 0; }
int dm_vault_ui_over(int x,int y) { (void)x;(void)y; unexpected_host_call(__func__); return 0; }
static uint64_t ui_hash(const char *name);
static int delivered_down, delivered_up, delivered_wheel, delivered_moves, last_down_x, last_down_y, last_wheel_arg;
static void ui_send_mouse_arg(module_t *m,uint64_t root,uint64_t act,int x,int y,int arg) {
    (void)m;(void)root;
    if (act==ui_hash("LeftMouseButtonDown")) {
        REQUIRE(x>=300 && x<=600 && y==200);
        ++delivered_down; last_down_x=x; last_down_y=y;
    } else if (act==ui_hash("LeftMouseButtonUp")) ++delivered_up;
    else if (act==ui_hash("MouseScroll")) { REQUIRE(x==49 && y==179); ++delivered_wheel; last_wheel_arg=arg; }
    else { REQUIRE(act==ui_hash("MouseMove")); ++delivered_moves; }
}
static void ui_send_mouse(module_t *m,uint64_t root,uint64_t act,int x,int y) { ui_send_mouse_arg(m,root,act,x,y,0); }
'''
CASES = r'''
/* Widget memory matches the offsets read by the actual dg_sliders/dg_hit.
   Only the virtual guest hit callback is simulated; recognition is real code. */
static uint64_t root_widget[14], slider_widget[14], child_list[1], slider_vtable[10];
static module_t test_module;
static int slider_track_hit(uint64_t widget,uint64_t packed) {
    (void)widget; int x=(int32_t)packed, y=(int32_t)(packed>>32);
    return x>=300 && x<=600 && y>=190 && y<=210;
}
static void pump(void) { ui_drag_pump(&test_module,(uint64_t)(uintptr_t)root_widget,800,375); }
static void sample(int phase,int slot,float x,float y) {
    dm_android_touch_event(phase,slot,x,y);
    REQUIRE(queue_sample()==0);
    REQUIRE(tc.lx==0 && tc.ly==0 && tc.rx==0 && tc.ry==0);
    REQUIRE(!tc.world_tap_seq && !tc.mouse_click_seq);
}
int main(int argc,char **argv) {
    REQUIRE(argc==2); int which=atoi(argv[1]);
    fixture(1); frontend=1;
    root_widget[0]=1; /* A non-slider root vtable, never invoked. */
    *(uint8_t *)((char *)root_widget+0x50)=1;
    root_widget[0x58/8]=(uint64_t)(uintptr_t)child_list; root_widget[0x60/8]=1;
    child_list[0]=(uint64_t)(uintptr_t)slider_widget;
    slider_widget[0]=(uint64_t)(uintptr_t)slider_vtable;
    *(uint8_t *)((char *)slider_widget+0x50)=1;
    slider_vtable[0x48/8]=(uint64_t)(uintptr_t)slider_track_hit;
    test_module.base=(uintptr_t)slider_vtable-0x20bad70;
    if (which==0) {
        sample(0,0,400,200); REQUIRE(atomic_load(&g_ut_phase)==1);
        sample(1,0,450,202); REQUIRE(atomic_load(&g_dg_kind)==2 && atomic_load(&g_ut_phase)==0);
        pump(); REQUIRE(delivered_down==1 && last_down_x>=448 && last_down_x<=450 && last_down_y==200);
        pump(); REQUIRE(delivered_up==1);
        sample(1,0,900,203); pump(); pump(); REQUIRE(last_down_x==600);
        sample(1,0,-30,203); pump(); pump(); REQUIRE(last_down_x==300);
        sample(2,0,-30,203); pump(); pump();
        REQUIRE(!atomic_load(&g_dg_kind) && fe_slot<0 && !fe_drag);
        REQUIRE(delivered_down==delivered_up && delivered_wheel==0 && atomic_load(&g_ut_phase)==0);
    } else if (which==1) {
        /* A sweep crosses the real track but starts outside it: no clicks. */
        sample(0,0,100,200); sample(1,0,450,200); pump();
        REQUIRE(atomic_load(&g_dg_kind)==2 && atomic_load(&g_ut_phase)==0);
        sample(1,0,650,200); pump(); sample(2,0,650,200); pump();
        REQUIRE(delivered_down==0 && delivered_up==0 && delivered_moves==0 && delivered_wheel==0);
    } else if (which==2) {
        sample(0,0,50,180); sample(1,0,52,250); pump();
        REQUIRE(atomic_load(&g_dg_kind)==1 && delivered_wheel>0 && last_wheel_arg>0);
        REQUIRE(atomic_load(&g_ut_phase)==0 && delivered_down==0);
        sample(1,0,400,260); pump(); REQUIRE(atomic_load(&g_dg_kind)==1); /* Axis stays captured. */
        sample(2,0,400,260); pump(); REQUIRE(!atomic_load(&g_dg_kind) && fe_slot<0);
    } else if (which==3) {
        /* A confirmation dialog keeps the original tap route. */
        atomic_store(&g_fe_confirm_open,1);
        sample(0,0,400,200); sample(1,0,480,201); pump(); sample(2,0,480,201); pump();
        REQUIRE(!fe_drag && !atomic_load(&g_dg_kind) && atomic_load(&g_ut_phase)==2);
        REQUIRE(delivered_down==0 && delivered_wheel==0);
    } else if (which==4) {
        sample(0,0,400,200); sample(1,0,450,200); pump(); REQUIRE(delivered_down==1);
        dm_android_touch_uimsg("PanelManager","UnloadPanel","LoadScreenPanel");
        REQUIRE(!frontend && fe_slot<0 && !fe_drag && !atomic_load(&g_dg_kind));
        pump(); pump(); REQUIRE(delivered_down==1 && delivered_up==1 && atomic_load(&g_ut_phase)==0);
    } else if (which==5) {
        /* Sub-threshold movement stays a tap; another finger cannot capture it. */
        sample(0,0,400,200); sample(1,1,600,200); sample(1,0,409,201);
        REQUIRE(!fe_drag && !atomic_load(&g_dg_kind));
        sample(2,0,409,201); REQUIRE(atomic_load(&g_ut_phase)==2 && fe_slot<0);
        REQUIRE(delivered_down==0 && delivered_wheel==0);
    } else REQUIRE(0);
    printf("PASS frontend_real_drag case=%d assertions=%u\n",which,checks); return 0;
}
'''

class FrontendDrag(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text = CURSOR.read_text(encoding='utf-8')
        touch = text[text.index('void dm_ui_touch(int phase,'):text.index('static void ui_send_mouse_arg(')]
        drag = text[text.index('#define UI_SCROLL_STEP(h)'):text.index('static void ui_auto_key(uint64_t mask);')]
        cc = os.environ.get('DM_TEST_CC') or shutil.which('cc') or shutil.which('clang')
        if not cc:
            raise RuntimeError('DM_TEST_CC must name a host C compiler')
        cls.temp = tempfile.TemporaryDirectory()
        folder = Path(cls.temp.name)
        (folder / 'frontend_cursor_drag.inc').write_text(PRELUDE + touch + drag, encoding='utf-8')
        src = folder / 'frontend_drag.c'
        cls.exe = folder / ('frontend_drag.exe' if os.name == 'nt' else 'frontend_drag')
        src.write_text('#define DM_ANDROID_REAL_FRONTEND_DRAG 1\n#define main full_snapshot_main\n#include "dm/android/tests/test_touch_snapshot.c"\n#undef main\n' + CASES, encoding='utf-8')
        command = [cc] + (['cc'] if Path(cc).stem == 'zig' else [])
        command += ['-UNDEBUG', '-std=c11', '-O2', '-I'+str(ROOT), '-I'+str(folder), str(src), '-o', str(cls.exe), '-lm']
        result = subprocess.run(command, text=True, capture_output=True)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        print('production_cursor_source_sha256=' + hashlib.sha256(text.encode()).hexdigest())

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def check_case(self, number):
        result = subprocess.run([str(self.exe), str(number)], text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        print(result.stdout.strip())

    def test_track_horizontal_drag_clamps_and_releases(self): self.check_case(0)
    def test_non_track_sweep_never_clicks_crossed_controls(self): self.check_case(1)
    def test_vertical_still_scrolls(self): self.check_case(2)
    def test_confirmation_preserves_tap(self): self.check_case(3)
    def test_scene_change_cancels_slider(self): self.check_case(4)
    def test_short_tap_and_first_finger_ownership(self): self.check_case(5)

if __name__ == '__main__':
    unittest.main(verbosity=2)
