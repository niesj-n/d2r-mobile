/* Real Custom UI pump; verified ARM routines use x86-64 host test thunks. */
#define main render_settings_main
#include "t_android_render_settings.c"
#undef main

static unsigned char root_mem[128], page_mem[1024], sliders[2][0x2000], texts[2][256];
static uint64_t root_children[1], page_children[4], page_vtable[40];
static char chars[2][32]; static int setter_calls;
static void native_set(uint64_t w, int value, int notify, int send) {
    assert(notify == 0 && send == 0); ++setter_calls;
    assert(*(int *)(w+0x5bc)==0 && *(int *)(w+0x5c0)==96 && *(int *)(w+0x5b8)==96);
    value=value<0?0:value>96?96:value;
    if (*(int *)(w+0x5c4)==value) return; /* Native setter early return. */
    *(int *)(w+0x5c4)=value; *(int *)(w+0x830)=value; /* Test indicator. */
}
static void *native_text(uint64_t str, const char *text, uint64_t n) {
    assert(n<32); memcpy(*(void **)str,text,n); (*(char **)str)[n]=0; *(uint64_t *)(str+8)=n; return (void *)str;
}
static void native_invalidate(uint64_t w) { (void)w; }
static void thunk(unsigned char *dest, void *fn) {
    dest[0]=0x48; dest[1]=0xb8; memcpy(dest+2,&fn,8); dest[10]=0xff; dest[11]=0xe0;
}
static void widget(unsigned char *w,const char *name,uint64_t vt) {
    *(uint64_t *)w=vt; *(const char **)(w+8)=name; w[0x50]=1;
}
static void pump(void) { dm_custom_ui_pump((uintptr_t)root_mem); }
static void change(const char *act) { dm_custom_on_uimsg("OptionsPanelMessage",act,""); }
static void check(int left,int right,const char *ls,const char *rs) {
    assert(*(int *)(sliders[0]+0x5c4)==left && *(int *)(sliders[1]+0x5c4)==right);
    assert(!strcmp(chars[0],ls) && !strcmp(chars[1],rs));
}
int main(int argc,char **argv) {
    assert(argc==2 && test_mkdir(argv[1])==0); dm_custom_init(argv[1]);
    unsigned char *b=VirtualAlloc(NULL,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE); assert(b);
    g_nmod=1; strcpy(g_mod[0].name,"main"); g_mod[0].base=b;
    thunk(b+0xad7028,native_set); thunk(b+0x136e5c,native_text); thunk(b+0xadc434,native_invalidate);
    widget(root_mem,"Root",0); widget(page_mem,"VideoOptionsPanel",(uintptr_t)page_vtable);
    page_vtable[0]=(uintptr_t)b+0xae5f28;
    root_children[0]=(uintptr_t)page_mem; *(uint64_t *)(root_mem+0x58)=(uintptr_t)root_children; *(uint64_t *)(root_mem+0x60)=1;
    const char *names[]={"TouchCustom_touch_margin_left","TouchCustom_touch_margin_right"};
    const char *values[]={"TouchCustom_touch_margin_left_value","TouchCustom_touch_margin_right_value"};
    for(int i=0;i<2;i++) {
        widget(sliders[i],names[i],(uintptr_t)b+0x20bad70); widget(texts[i],values[i],(uintptr_t)b+DM_VT_TEXT);
        *(int *)(sliders[i]+0x830)=48; /* Constructor centers indicator. */
        *(char **)(texts[i]+0x88)=chars[i]; page_children[i]=(uintptr_t)sliders[i]; page_children[i+2]=(uintptr_t)texts[i];
    }
    *(uint64_t *)(page_mem+0x58)=(uintptr_t)page_children; *(uint64_t *)(page_mem+0x60)=4;
    pump(); assert(setter_calls==4); check(0,0,"0","0");
    assert(*(int *)(sliders[0]+0x830)==0 && *(int *)(sliders[1]+0x830)==0);
    /* Native click/drag writes current value; pump reads and displays pending. */
    *(int *)(sliders[0]+0x5c4)=48; *(int *)(sliders[1]+0x5c4)=96; pump(); check(48,96,"48","96");
    assert(atomic_load(&g_pend[K_MARGIN_LEFT])==48 && atomic_load(&g_pend[K_MARGIN_RIGHT])==96);
    int applied[3]; assert(dm_custom_android_read_applied(applied) && applied[0]==0 && applied[1]==0);
    change("AcceptChanges"); pump(); assert(dm_custom_android_read_applied(applied) && applied[0]==48 && applied[1]==96);
    *(int *)(sliders[0]+0x5c4)=95; pump(); change("CancelChanges"); pump(); check(48,96,"48","96");
    change("ResetToDefaults"); pump(); check(0,0,"0","0"); assert(dm_custom_android_read_applied(applied) && applied[0]==48);
    change("CancelChanges"); pump(); check(48,96,"48","96");
    change("ResetToDefaults"); pump(); change("AcceptChanges"); assert(dm_custom_android_read_applied(applied) && applied[0]==0 && applied[1]==0);
    *(int *)(sliders[0]+0x5c4)=37; pump(); page_mem[0x50]=0; pump(); page_mem[0x50]=1; pump(); check(0,0,"0","0");
    assert(atomic_load(&g_pend[K_MARGIN_LEFT])==-1);
    change("CancelChanges"); *(uint64_t *)sliders[0]=(uintptr_t)b+DM_VT_BUTTON; *(int *)(sliders[0]+0x5c4)=1234;
    int before=setter_calls; pump(); assert(setter_calls==before+1 && *(int *)(sliders[0]+0x5c4)==1234);
    puts("PASS native Custom slider range, pending text, Apply, Discard, Defaults, close/reopen, class guard");
    VirtualFree(b,0,MEM_RELEASE); return 0;
}
