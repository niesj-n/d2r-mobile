"""Compile and exercise Sort DOWN page-token policy without game memory."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
CASES = r'''
#include <assert.h>
#include "dm/host/dm_product_ui.h"
int main(void) {
    dm_product_ui_scene(0,1);
    dm_product_ui_touch(0);
    assert(dm_product_ui_sort(0,1)==0);
    assert(dm_product_ui_sort(0,1)==-1);
    for(int page=4;page<=8;page++) {
        dm_product_ui_scene(page,2);dm_product_ui_touch(0);
        assert(dm_product_ui_sort(page,2)==page);
    }
    dm_product_ui_scene(4,3);dm_product_ui_touch(0);
    assert(dm_product_ui_sort(5,3)==-1);
    dm_product_ui_scene(4,3);dm_product_ui_touch(0);
    dm_product_ui_scene(5,3);dm_product_ui_scene(4,3);
    assert(dm_product_ui_sort(4,3)==-1);
    dm_product_ui_touch(0);assert(dm_product_ui_sort(4,4)==-1);
    dm_product_ui_scene(4,3);dm_product_ui_touch(0);dm_product_ui_touch(3);
    assert(dm_product_ui_sort(4,3)==-1);
    dm_product_ui_touch(0);dm_product_ui_touch(2);
    assert(dm_product_ui_sort(4,3)==4);
    for(int bad=-1;bad<12;bad++) if(bad!=0 && !(bad>=4 && bad<=8)) {
        dm_product_ui_scene(bad,4);dm_product_ui_touch(0);
        assert(dm_product_ui_sort(bad,4)==-1);
    }
    return 0;
}
'''

class ProductInput(unittest.TestCase):
    def test_main_thread_pump_is_wired(self):
        source = (ROOT / 'dm/host/dm_cursor.c').read_text(encoding='utf-8')
        self.assertTrue('ui_product_pump(pm, proot);' in source, 'native Sort pump must run each UI frame')
        self.assertIn('"TouchProductSort"', source)

    def test_production_widget_visibility_and_message_dispatch(self):
        source = (ROOT / 'dm/host/dm_cursor.c').read_text(encoding='utf-8')
        start = source.index('static void ui_product_pump(')
        pump = source[start:source.index('#endif', start)]
        start = source.index('void dm_ui_on_uimsg(')
        start = source.index('#ifdef __ANDROID__', start)
        message = source[start:source.index('    sel_keep(cat, act);', start)]
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "dm/host/dm_product_ui.h"
typedef struct { uintptr_t base; } module_t;
static unsigned char inventory[128], bank[128];
static int context, ingame=1, calls, last=-1;
static uint64_t generation=7;
static uint64_t ui_panel(uint64_t root,const char *name) { (void)root;return !strcmp(name,"BankExpansionLayout")?20:10; }
static uint64_t ui_find(uint64_t panel,const char *name,int depth) {
    (void)depth;assert(!strcmp(name,"TouchProductSort"));return (uintptr_t)(panel==20?bank:inventory);
}
static int dm_uiscreen_ingame(void) { return ingame; }
static int dm_ui_sort_context(void) { return context; }
static uint64_t dm_ui_gen(void) { return generation; }
static void dm_inv_sort_touch(int token) { calls++;last=token; }
''' + pump + '\nstatic void message(const char *cat,const char *act,const char *arg) {\n' + message + '\n}\n' + r'''
int main(void) {
    module_t m={.base=0x1000};
    *(uint64_t *)inventory=*(uint64_t *)bank=m.base+0x20b7b00;
    context=0;ui_product_pump(&m,1);assert(inventory[0x50] && !bank[0x50]);
    dm_product_ui_touch(0);message("PanelManager","ClosePanel","TouchProductSort");
    assert(calls==1 && last==0);
    for(context=4;context<=8;context++) {
        ui_product_pump(&m,1);assert(!inventory[0x50] && bank[0x50]);
        dm_product_ui_touch(0);message("PanelManager","ClosePanel","TouchProductSort");
        assert(last==context);
    }
    int before=calls;
    context=4;ui_product_pump(&m,1);dm_product_ui_touch(0);
    context=5;ui_product_pump(&m,1);message("PanelManager","ClosePanel","TouchProductSort");
    assert(calls==before);
    dm_product_ui_touch(0);message("PanelManager","OpenPanel","PlayerInventoryExpansionLayout");
    message("PanelManager","ClosePanel","TouchProductSort");assert(calls==before);
    context=2;ui_product_pump(&m,1);assert(!inventory[0x50] && !bank[0x50]);
    context=4;ingame=0;ui_product_pump(&m,1);assert(!inventory[0x50] && !bank[0x50]);
    return 0;
}
'''
        self.run_native(harness)

    def run_native(self, harness):
        cc = os.environ['DM_TEST_CC']
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp)/'sort.c'; exe = Path(tmp)/'sort.exe'
            src.write_text(harness, encoding='utf-8')
            cmd = [cc]
            if Path(cc).stem == 'zig': cmd += ['cc', '-target', 'x86_64-windows-gnu']
            cmd += ['-D__ANDROID__=1','-std=c11','-UNDEBUG','-O1','-pthread','-I',str(ROOT),str(src),str(ROOT/'dm/host/dm_product_ui.c'),'-o',str(exe)]
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_sort_token_cancel_and_dispatch_revalidation(self):
        source = ROOT / 'dm/host/dm_product_ui.c'
        self.assertTrue(source.exists(), 'native game widget capture is missing')
        self.run_native(CASES)

if __name__ == '__main__': unittest.main()
