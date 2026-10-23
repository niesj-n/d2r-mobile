"""Run the production Android map pump against synthetic guest memory.

The selected source block is compiled verbatim except for its two guest-code
call boundaries (mode-change callback and panel scale accessor). No game files,
Android input, saves, or GPU are used. DM_TEST_CC may select clang/cc or zig.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'dm/host/dm_cursor.c'

PRELUDE = r'''
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define LOG(...) ((void)0)
typedef struct { unsigned char *base; } module_t;
typedef struct { int valid; } dm_hud_info;
static module_t mod;
static int in_game=1, callback_count, last_mode=-1;
static _Alignas(8) unsigned char automap[0x1600];
static int rect_y=200, rect_h=900, real_h=1100;
static uint64_t dm_guest_frame(void) { return 1; }
static int dm_uiscreen_ingame(void) { return in_game; }
static int dm_padmode_current(void) { return 0; }
static void dm_hud_get(dm_hud_info *h) { h->valid=in_game; }
static void dm_ui_rect(int *x,int *y,int *w,int *h) { *x=0;*y=rect_y;*w=2000;*h=rect_h; }
static int dm_screen_real(int *w,int *h) { *w=2000;*h=real_h;return 1; }
static uint64_t ui_panel(uint64_t root,const char *name) { (void)root;(void)name;return (uint64_t)(uintptr_t)automap; }
static float test_panel_scale(uint64_t p) { (void)p;return 1; }
static void test_change_mode(int mode) {
    callback_count++;last_mode=mode;
    *(int32_t *)(mod.base+0x2d5c748)=mode!=0;
    *(int32_t *)(mod.base+0x2d5e910)=0;
    *(int32_t *)(mod.base+0x2d5e914)=0;
}
static int failures;
#define CHECK(c) do { if(!(c)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c);failures++; } } while(0)
'''
CASES = r'''
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    mod.base=calloc(1,0x2f05460);if(!mod.base)return 3;
    *(uint64_t *)automap=(uint64_t)(uintptr_t)mod.base+0x20c58a0;
    *(float *)(automap+0x15a8)=1;
    *(float *)(mod.base+0x2d5e91c)=1;
    int32_t *cfg=(int32_t *)(mod.base+0x2d479f4),*mini=(int32_t *)(mod.base+0x2d5c748);
    int32_t *sx=(int32_t *)(mod.base+0x2d5e910),*sy=(int32_t *)(mod.base+0x2d5e914);
    switch(atoi(argv[1])) {
    case 0:
        *cfg=0;*mini=0;ui_map_pump(&mod);
        CHECK(*cfg==0);CHECK(callback_count==0);CHECK(dm_ui_map_is_full());
        dm_ui_map_full(0);ui_map_pump(&mod);
        CHECK(*cfg==0);CHECK(*mini==1);CHECK(last_mode==2);
        dm_ui_map_full(1);ui_map_pump(&mod);
        CHECK(*cfg==0);CHECK(*mini==0);CHECK(dm_ui_map_is_full());
        break;
    case 1: case 2: {
        int side=atoi(argv[1]);*cfg=side;*mini=1;ui_map_pump(&mod);
        CHECK(*cfg==side);CHECK(callback_count==0);CHECK(!dm_ui_map_is_full());
        dm_ui_map_full(1);ui_map_pump(&mod);
        CHECK(*cfg==side);CHECK(last_mode==0);CHECK(dm_ui_map_is_full());
        dm_ui_map_full(0);ui_map_pump(&mod);
        CHECK(*cfg==side);CHECK(last_mode==side);CHECK(!dm_ui_map_is_full());
        /* A later saved fullscreen preference must still return to this side. */
        *cfg=0;*mini=0;ui_map_pump(&mod);dm_ui_map_full(0);ui_map_pump(&mod);
        CHECK(*cfg==0);CHECK(last_mode==side);
        break; }
    case 3:
        *cfg=2;*mini=1;in_game=0;dm_ui_map_full(1);ui_map_pump(&mod);
        CHECK(callback_count==0);CHECK(!dm_ui_map_is_full());
        in_game=1;ui_map_pump(&mod);CHECK(callback_count==0);CHECK(*mini==1);
        break;
    case 4:
        *cfg=0;*mini=0;*sx=31;*sy=19;
        ui_map_center(&mod,1,0);CHECK(*sx==31);CHECK(*sy==19);
#if HAVE_RECENTER
        dm_ui_map_recenter();ui_map_center(&mod,1,0);
        CHECK(*sx==0);CHECK(*sy==100);CHECK(*cfg==0);
        *sx=7;*sy=12;in_game=0;dm_ui_map_recenter();ui_map_pump(&mod);
        in_game=1;ui_map_center(&mod,1,0);CHECK(*sx==7);CHECK(*sy==12);
#else
        CHECK(0 && "missing queued recenter API");
#endif
        break;
    }
    free(mod.base);if(!failures)puts("PASS production map controls");return failures!=0;
}
'''

class AndroidMapControls(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        text=SOURCE.read_text(encoding='utf-8')
        start=text.index('static _Atomic int g_map_req')
        end=text.index('// ---------- 右上角小地图的位置',start)
        block=text[start:end]
        changes={
            '((void (*)(int))(m->base + 0x40f780))': 'test_change_mode',
            '((float (*)(uint64_t))(m->base + 0x390b38))': 'test_panel_scale',
        }
        for needle,replacement in changes.items():
            if needle not in block:raise AssertionError('guest boundary changed; review harness: '+needle)
            block=block.replace(needle,replacement)
        cc=os.environ.get('DM_TEST_CC') or shutil.which('cc') or shutil.which('clang')
        if not cc:raise RuntimeError('DM_TEST_CC must point to a host C compiler')
        cls.temp=tempfile.TemporaryDirectory()
        folder=Path(cls.temp.name)
        src=folder/'map_controls.c';cls.exe=folder/('map_controls.exe' if os.name=='nt' else 'map_controls')
        src.write_text(PRELUDE+'\n#define HAVE_RECENTER '+str(int('void dm_ui_map_recenter(void)' in block))+'\n'+block+CASES,encoding='utf-8')
        command=[cc]
        if Path(cc).stem=='zig':
            command+=['cc']
            if os.name=='nt':command+=['-target','x86_64-windows-gnu']
        command+=['-D__ANDROID__=1','-std=c11','-O1',str(src),'-o',str(cls.exe)]
        if os.name!='nt':command+=['-lm']
        result=subprocess.run(command,text=True,capture_output=True)
        if result.returncode:raise AssertionError(result.stdout+result.stderr)
        print('production_source_sha256='+hashlib.sha256(text.encode()).hexdigest())
    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()
    def check_case(self,case):
        result=subprocess.run([str(self.exe),str(case)],text=True,capture_output=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    def test_saved_fullscreen_is_respected_and_has_return_path(self):self.check_case(0)
    def test_left_minimap_returns_without_rewriting_saved_option(self):self.check_case(1)
    def test_right_minimap_returns_without_rewriting_saved_option(self):self.check_case(2)
    def test_outside_game_requests_are_discarded(self):self.check_case(3)
    def test_recenter_is_explicit_and_keeps_world_alignment(self):self.check_case(4)

if __name__=='__main__':unittest.main()
