/* Native style sliders, strict persistence and unchanged applied settings ABI. */
#define main render_settings_main
#include "t_android_render_settings.c"
#undef main
static unsigned char root_mem[128],page_mem[1024],sliders[2][0x2000],texts[2][256];
static uint64_t root_children[1],page_children[4],page_vtable[96];static char chars[2][32];
static void set_value(uint64_t w,int value,int notify,int send) {
    assert(!notify && !send);int min=*(int *)(w+0x5bc),max=*(int *)(w+0x5c0);
    assert((min==8 && max==14)||(min==0 && max==80));assert(*(int *)(w+0x5b8)==max);
    *(int *)(w+0x5c4)=value<min?min:value>max?max:value;
}
static void *set_text(uint64_t s,const char *t,uint64_t n) { assert(n<32);memcpy(*(void **)s,t,n);(*(char **)s)[n]=0;*(uint64_t *)(s+8)=n;return (void *)s; }
static void invalidate(uint64_t w) { (void)w; }
static void thunk(unsigned char *p,void *fn) { p[0]=0x48;p[1]=0xb8;memcpy(p+2,&fn,8);p[10]=0xff;p[11]=0xe0; }
static void widget(unsigned char *w,const char *name,uint64_t vt) { *(uint64_t *)w=vt;*(const char **)(w+8)=name;w[0x50]=1; }
static void pump(void) { dm_custom_ui_pump((uintptr_t)root_mem); }
static void action(const char *act) { dm_custom_on_uimsg("OptionsPanelMessage",act,""); }
static void applied(int font,int transparency) { int out[2];assert(dm_custom_android_read_monitor_style(out) && out[0]==font && out[1]==transparency); }
static _Atomic int run_reader,reader_started,reader_samples;
static DWORD WINAPI read_style(LPVOID unused) {
    (void)unused;atomic_store(&reader_started,1);
    while(atomic_load(&run_reader)) { int tuple[2];assert(dm_custom_android_read_monitor_style(tuple));assert((tuple[0]==8 && tuple[1]==0)||(tuple[0]==14 && tuple[1]==80));atomic_fetch_add(&reader_samples,1); }
    return 0;
}
int main(int argc,char **argv) {
    assert(argc==2 && test_mkdir(argv[1])==0);int out[2]={-7,-7};
    assert(!dm_custom_android_read_monitor_style(out) && out[0]==-7 && !dm_custom_android_read_monitor_style(NULL));
    dm_custom_init(argv[1]);assert(dm_custom_get("perf_font_size")==9 && dm_custom_get("perf_transparency")==45);
    assert(key_index("perf_overlay")==19+VOICE_ROW && key_index("perf_font_size")==20+VOICE_ROW && key_index("perf_transparency")==21+VOICE_ROW);
    applied(9,45);int legacy[3];assert(dm_custom_android_read_applied(legacy) && legacy[0]==0 && legacy[1]==0 && legacy[2]==0);
    unsigned char *b=VirtualAlloc(NULL,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);assert(b);g_nmod=1;strcpy(g_mod[0].name,"main");g_mod[0].base=b;
    thunk(b+0xad7028,set_value);thunk(b+0x136e5c,set_text);thunk(b+0xadc434,invalidate);
    widget(root_mem,"Root",0);widget(page_mem,"VideoOptionsPanel",(uintptr_t)page_vtable);page_vtable[0]=(uintptr_t)b+0xae5f28;
    root_children[0]=(uintptr_t)page_mem;*(uint64_t *)(root_mem+0x58)=(uintptr_t)root_children;*(uint64_t *)(root_mem+0x60)=1;
    const char *names[]={"TouchCustom_perf_font_size","TouchCustom_perf_transparency"},*values[]={"TouchCustom_perf_font_size_value","TouchCustom_perf_transparency_value"};
    for(int i=0;i<2;i++) { widget(sliders[i],names[i],(uintptr_t)b+DM_VT_SETTINGS_SLIDER);widget(texts[i],values[i],(uintptr_t)b+DM_VT_TEXT);*(char **)(texts[i]+0x88)=chars[i];page_children[i]=(uintptr_t)sliders[i];page_children[i+2]=(uintptr_t)texts[i]; }
    *(uint64_t *)(page_mem+0x58)=(uintptr_t)page_children;*(uint64_t *)(page_mem+0x60)=4;
    pump();assert(*(int *)(sliders[0]+0x5bc)==8 && *(int *)(sliders[0]+0x5c0)==14);assert(*(int *)(sliders[1]+0x5bc)==0 && *(int *)(sliders[1]+0x5c0)==80);
    assert(!strcmp(chars[0],"9") && !strcmp(chars[1],"45%"));
    atomic_store(&g_pend[key_index("auto_buff")],0);
    *(int *)(sliders[0]+0x5c4)=14;*(int *)(sliders[1]+0x5c4)=80;pump();applied(9,45);assert(!strcmp(chars[0],"14") && !strcmp(chars[1],"80%"));
    assert(dm_custom_get("auto_buff")==1 && atomic_load(&g_pend[key_index("auto_buff")])==0);
    action("CancelChanges");pump();assert(!strcmp(chars[0],"9") && !strcmp(chars[1],"45%"));
    *(int *)(sliders[0]+0x5c4)=8;*(int *)(sliders[1]+0x5c4)=0;pump();action("AcceptChanges");applied(8,0);
    assert(dm_custom_android_read_applied(legacy) && !legacy[0] && !legacy[1] && !legacy[2]);
    atomic_store(&g_loaded,0);assert(dm_custom_get("perf_font_size")==8 && dm_custom_get("perf_transparency")==0);applied(8,0);
    action("ResetToDefaults");pump();applied(8,0);assert(!strcmp(chars[0],"9") && !strcmp(chars[1],"45%"));action("AcceptChanges");applied(9,45);
    char path[1200];snprintf(path,sizeof path,"%s/sw_custom.txt",argv[1]);
    const char *invalid[]={"-1","99999999999999999999999999","nan","9x","9.0"," 9","+9","","9\rjunk"};
    for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++) { FILE *f=fopen(path,"w");assert(f);fprintf(f,"perf_font_size=%s\nperf_transparency=%s\n",invalid[i],invalid[i]);fclose(f);atomic_store(&g_loaded,0);assert(dm_custom_get("perf_font_size")==9 && dm_custom_get("perf_transparency")==45);applied(9,45); }
    FILE *f=fopen(path,"w");assert(f);fputs("perf_font_size=7\nperf_transparency=81\n",f);fclose(f);atomic_store(&g_loaded,0);assert(dm_custom_get("perf_font_size")==9 && dm_custom_get("perf_transparency")==45);
    f=fopen(path,"w");assert(f);fputs("perf_font_size=14\r\nperf_transparency=80\r\ntouch_margin_left=96\ntouch_margin_right=37\nperf_overlay=1\n",f);fclose(f);atomic_store(&g_loaded,0);assert(dm_custom_get("perf_font_size")==14);applied(14,80);assert(dm_custom_android_read_applied(legacy) && legacy[0]==96 && legacy[1]==37 && legacy[2]==1);
    f=fopen(path,"w");assert(f);fputs("perf_font_size=8\nperf_transparency=0\n",f);fclose(f);applied(14,80); /* Snapshot read cannot reload file. */
    atomic_store(&run_reader,1);HANDLE thread=CreateThread(NULL,0,read_style,NULL,0,NULL);assert(thread);
    while(!atomic_load(&reader_started)) SwitchToThread();
    for(int i=0;i<50000;i++) { atomic_store(&g_val[K_PERF_FONT],i&1?8:14);atomic_store(&g_val[K_PERF_TRANSPARENCY],i&1?0:80);android_publish_applied(); }
    atomic_store(&run_reader,0);assert(WaitForSingleObject(thread,10000)==WAIT_OBJECT_0);CloseHandle(thread);assert(atomic_load(&reader_samples)>0);
    VirtualFree(b,0,MEM_RELEASE);puts("PASS monitor style range, pending sliders, Apply/Discard/Defaults, strict parsing, reload, coherent tuple, unchanged legacy ABI");return 0;
}
