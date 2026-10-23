"""Compile actual Android target/pacing functions with synthetic settings.

Only graphics/guest dependencies are excluded; the production implementations of
mtl_fps_target, dm_pace_hz and present_auto_hz are selected verbatim from sources.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[3]

def function(source, signature):
    start=source.index(signature);opening=source.index('{',start)
    while ';' in source[start:opening]:
        start=source.index(signature,start+len(signature));opening=source.index('{',start)
    depth=1;i=opening+1
    while depth:
        if source[i]=='{':depth+=1
        elif source[i]=='}':depth-=1
        i+=1
    return source[start:i]

class AndroidTargetPacing(unittest.TestCase):
    def test_actual_target_and_present_pacing(self):
        vk=(ROOT/'dm/gfx/vulkan/nvn_vk.c').read_text(encoding='utf-8')
        hle=(ROOT/'dm/gfx/nvn/hle_nvn.c').read_text(encoding='utf-8')
        selected='\n'.join((function(vk,'int mtl_fps_target(void)'),function(vk,'double dm_pace_hz('),function(hle,'static double present_auto_hz('),
            'static int g_hz=-1,g_display_mode,g_hz_auto;',function(hle,'static void present_cfg('),function(hle,'static uint64_t present_period(')))
        code='''
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define LOG(...) ((void)0)
typedef struct { int interval; } win_t;
static double screen=120;
static int mtl_display_hz(void){return (int)screen;}
static double mtl_display_refresh(void){return screen;}
static double front_display_refresh(char *name,size_t n){if(n)name[0]=0;return screen;}
#include "nvn_pace.h"
#include "nvn_adaptive_pace.h"
static int setting=60,g_pa_k;
static double g_pa_screen_hz;
static int dm_fps_setting(void){return setting;}
static int pace_adapt_enabled(void){return 0;}
'''+selected+'''
static int failures;
#define CHECK(c) do {if(!(c)){fprintf(stderr,"FAIL %d %s\\n",__LINE__,#c);failures++;}}while(0)
static int near(double a,double b){return fabs(a-b)<.001;}
int main(void){
#ifdef __ANDROID__
    win_t window={.interval=1};
    setting=30;CHECK(present_period(&window)==33333333);
    setting=60;CHECK(present_period(&window)==16666666);
    g_hz=-1;screen=0;setting=30;CHECK(present_period(&window)==33333333);screen=120;
    setting=30;CHECK(mtl_fps_target()==30);CHECK(near(present_auto_hz(120),30));
    CHECK(near(present_auto_hz(60),30));CHECK(near(present_auto_hz(0),30));
    setting=60;CHECK(mtl_fps_target()==60);CHECK(near(present_auto_hz(120),60));
    CHECK(near(present_auto_hz(144),48));CHECK(near(present_auto_hz(90),45));
    CHECK(near(present_auto_hz(59.94),59.94));CHECK(near(dm_pace_hz(119.88,30),29.97));
    CHECK(near(dm_pace_hz(NAN,30),30));CHECK(near(dm_pace_hz(INFINITY,60),60));
    CHECK(near(dm_pace_hz(24,30),24));CHECK(near(dm_pace_hz(60.01,60),60));
    setting=999;CHECK(mtl_fps_target()==60);
#else
    setting=30;CHECK(near(present_auto_hz(144),72));CHECK(near(dm_pace_hz(144,30),36));
#endif
    if(!failures)puts("PASS actual target/pacing path; GPU present NOT TESTED");return failures!=0;
}
'''
        cc=os.environ.get('DM_TEST_CC') or shutil.which('cc') or shutil.which('clang')
        if not cc:self.fail('Set DM_TEST_CC to a host compiler path')
        with tempfile.TemporaryDirectory() as temp:
            folder=Path(temp);src=folder/'pace.c';src.write_text(code,encoding='utf-8')
            failures=[]
            for platform in ('android','legacy'):
                exe=folder/(platform+('.exe' if os.name=='nt' else ''))
                command=[cc]
                if Path(cc).stem=='zig':
                    command+=['cc']
                    if os.name=='nt':command+=['-target','x86_64-windows-gnu']
                command+=['-std=c11','-O1','-I',str(ROOT/'dm/gfx/nvn'),str(src),'-o',str(exe)]
                if platform=='android':command+=['-D__ANDROID__=1']
                if os.name!='nt':command+=['-lm']
                built=subprocess.run(command,text=True,capture_output=True)
                self.assertEqual(built.returncode,0,built.stdout+built.stderr)
                environment=os.environ.copy();environment.pop('DM_PRESENT_HZ',None)
                ran=subprocess.run([str(exe)],text=True,capture_output=True,env=environment)
                if ran.returncode:failures.append(platform+': '+ran.stdout+ran.stderr)
                if platform=='android':
                    environment['DM_PRESENT_HZ']='0'
                    override=subprocess.run([str(exe)],text=True,capture_output=True,env=environment)
                    if override.returncode:failures.append('android stale env: '+override.stdout+override.stderr)
            self.assertFalse(failures,'\n'.join(failures))

if __name__=='__main__':unittest.main()
