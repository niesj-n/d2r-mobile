"""Execute the real JNI getter and UTF-16 conversion path with fake ART only."""
from pathlib import Path
import os
import subprocess
import sys

android = Path(__file__).resolve().parents[1]
engine_tests = android.parent / 'gfx/vulkan/engine/tests'
sys.path.insert(0, str(engine_tests))
from texture_cache_regression import balanced

source = (android / 'text_input_jni.c').read_text(encoding='utf-8')
conversion = balanced(source, source.index('static jstring utf8_string('))
getter = balanced(source, source.index('JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_DiagnosticsExporter_nativePipelineDiagnostics('))
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
harness = r'''
#include <jni.h>
#include "diagnostics_android.h"
#include "perf_android.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
void dm_android_perf_read(uint64_t now,dm_android_perf_snapshot *s){(void)now;memset(s,0,sizeof *s);}
int dm_android_perf_enabled(void){return 0;}
size_t swkbd_u8_to_u16(const char *s,size_t n,uint16_t *out,size_t cap){size_t i=0;for(;i<n&&i<cap&&s[i];i++){assert((unsigned char)s[i]<128);out[i]=(unsigned char)s[i];}return i;}
int fail_new_string;
static jstring new_string(JNIEnv *env,const jchar *s,jsize n){(void)env;if(fail_new_string)return NULL;assert(n<DM_ANDROID_PIPELINE_LOG_CAPACITY);jchar *p=calloc(n+1,sizeof *p);assert(p);memcpy(p,s,n*sizeof *p);return (jstring)p;}
'''
main = r'''
int main(){
 static const struct JNINativeInterface_ table={.NewString=new_string};JNIEnv env=&table;
 dm_android_diagnostics_pipeline_log("NVN slow-pso frame=74706 key=123 create_ms=25");
 jstring value=Java_com_xiaoboluo_d2r_DiagnosticsExporter_nativePipelineDiagnostics(&env,NULL);assert(value);
 jchar *s=(jchar*)value;char text[DM_ANDROID_PIPELINE_LOG_CAPACITY];size_t i=0;for(;s[i];i++){assert(s[i]<128);text[i]=(char)s[i];}text[i]=0;
 assert(strstr(text,"frame=74706 key=123") && strstr(text,"pipeline_diagnostics=1"));free(value);
 fail_new_string=1;assert(Java_com_xiaoboluo_d2r_DiagnosticsExporter_nativePipelineDiagnostics(&env,NULL)==NULL);
 puts("PASS extracted production JNI getter with actual ring: bounded ASCII-to-UTF16 snapshot, return and ART-null branch");
}
'''
# The JDK uses this tag spelling; the Android build is checked separately with NDK jni.h.
(out / 'pipeline_jni_harness.c').write_text(harness + conversion + '\n' + getter + '\n' + main, encoding='utf-8')
jdk = Path(os.environ['DM_TEST_JDK'])
subprocess.run([os.environ['DM_TEST_ZIG'], 'cc', '-std=c11', '-I'+str(android), '-I'+str(jdk / 'include'), '-I'+str(jdk / 'include/win32'), str(out / 'pipeline_jni_harness.c'), str(android / 'diagnostics_android.c'), '-o', str(out / 'pipeline_jni.exe')], check=True)
subprocess.run([str(out / 'pipeline_jni.exe')], check=True)
