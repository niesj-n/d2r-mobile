/* Canonical live filter shortcut through the real Custom host-frame pump. */
#define main render_settings_main
#define dm_guest_frame test_unused_frame
#include "t_android_render_settings.c"
#undef main
#undef dm_guest_frame
static uint64_t filter_frame=100;
uint64_t dm_guest_frame(void) { return filter_frame; }
static int filter_difficulty=2;
int dm_diag_difficulty(void) { return filter_difficulty; }
void (*g_dm_mount_seed_hook)(const char *);
#define mkdir(path,mode) test_mkdir(path)
#include "../dm_lootfilter.c"
#undef mkdir

static unsigned char root_mem[128], page_mem[1024], boxes[2][0x1000];
static uint64_t root_children[1], page_children[2], page_vtable[96];
static uint64_t root(void) { return (uintptr_t)root_mem; }
static void shortcut(void) { dm_custom_android_request_loot_filter_toggle(); }
static void file_read(char *text,size_t cap) {
    char path[1200];snprintf(path,sizeof path,"%s/sw_custom.txt",g_dir);
    FILE *f=fopen(path,"r");assert(f);size_t n=fread(text,1,cap-1,f);text[n]=0;assert(fclose(f)==0);
}
int main(int argc,char **argv) {
    assert(argc==2 && test_mkdir(argv[1])==0);
    shortcut();dm_custom_ui_pump(0);assert(!atomic_load(&g_loaded)); /* No save directory yet: retain request. */
    dm_custom_init(argv[1]);char path[1200];snprintf(path,sizeof path,"%s/sw_custom.txt",argv[1]);
    shortcut();shortcut(); /* Three total: still one queued toggle. */
    assert(!fopen(path,"r"));assert(!atomic_load(&g_loaded)); /* Input API did not load/write. */
    dm_custom_ui_pump(0);assert(dm_custom_get("loot_filter")==0);
    char saved[4096];file_read(saved,sizeof saved);assert(strstr(saved,"loot_filter=0\n"));
    int other=key_index("auto_buff"),filter=key_index("loot_filter");
    atomic_store(&g_pend[other],0);atomic_store(&g_pend[filter],1);
    atomic_store(&g_pend[K_RENDER],8);atomic_store(&g_pend[K_ECO],1);
    atomic_store(&g_pend[K_MARGIN_LEFT],37);atomic_store(&g_pend[K_PERF],1);
    atomic_store(&g_pend[K_PERF_FONT],14);atomic_store(&g_pend[K_PERF_TRANSPARENCY],80);
    int before_pending[NK],before_applied[NK];
    for(int i=0;i<NK;i++) { before_pending[i]=atomic_load(&g_pend[i]);before_applied[i]=atomic_load(&g_val[i]); }
    shortcut();shortcut();dm_custom_ui_pump(0);assert(dm_custom_get("loot_filter")==0);
    assert(atomic_load(&g_pend[other])==0 && atomic_load(&g_pend[filter])==1); /* Even parity does nothing. */
    shortcut();dm_custom_ui_pump(0);assert(dm_custom_get("loot_filter")==1 && atomic_load(&g_pend[filter])==-1);
    assert(atomic_load(&g_pend[other])==0 && dm_custom_get("auto_buff")==1);
    for(int i=0;i<NK;i++) if(i!=filter) {
        assert(atomic_load(&g_pend[i])==before_pending[i]);assert(atomic_load(&g_val[i])==before_applied[i]);
    }
    file_read(saved,sizeof saved);assert(strstr(saved,"loot_filter=1\n") && strstr(saved,"auto_buff=1\n"));
    atomic_store(&g_loaded,0);assert(dm_custom_get("loot_filter")==1); /* Reload canonical persistence. */
    atomic_store(&g_pend[other],0);
    unsigned char *b=VirtualAlloc(NULL,0x3000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(b);
    g_nmod=1;strcpy(g_mod[0].name,"main");g_mod[0].base=b;
    *(uint64_t *)page_mem=(uintptr_t)page_vtable;page_vtable[0]=(uintptr_t)b+0xae5f28;
    *(char **)(page_mem+8)="VideoOptionsPanel";page_mem[0x50]=1;root_children[0]=(uintptr_t)page_mem;
    *(uint64_t *)(root_mem+0x58)=(uintptr_t)root_children;*(uint64_t *)(root_mem+0x60)=1;
    const char *names[]={"TouchCustom_loot_filter","TouchCustom_auto_buff"};
    for(int i=0;i<2;i++) {
        *(uint64_t *)boxes[i]=(uintptr_t)b+DM_VT_TOGGLE;*(const char **)(boxes[i]+8)=names[i];boxes[i][0x50]=1;
        page_children[i]=(uintptr_t)boxes[i];
    }
    *(uint64_t *)(page_mem+0x58)=(uintptr_t)page_children;*(uint64_t *)(page_mem+0x60)=2;
    dm_custom_ui_pump(root());assert(TOGGLED((uintptr_t)boxes[0]) && !TOGGLED((uintptr_t)boxes[1]));
    shortcut();dm_custom_ui_pump(root());assert(dm_custom_get("loot_filter")==0 && !TOGGLED((uintptr_t)boxes[0]));
    assert(atomic_load(&g_pend[other])==0 && dm_custom_get("auto_buff")==1);
    /* Hide/reopen alongside the shortcut: unrelated pending state survives. */
    page_mem[0x50]=0;shortcut();dm_custom_ui_pump(root());assert(dm_custom_get("loot_filter")==1);
    assert(atomic_load(&g_pend[other])==0);page_mem[0x50]=1;dm_custom_ui_pump(root());
    assert(TOGGLED((uintptr_t)boxes[0]) && !TOGGLED((uintptr_t)boxes[1]));
    assert(atomic_load(&g_pend[other])==0 && atomic_load(&g_pend[filter])==-1);
    file_read(saved,sizeof saved);assert(strstr(saved,"loot_filter=1\n") && strstr(saved,"auto_buff=1\n"));
    /* Actual filter pump runs after Custom in hle_hid.fill: force a same-frame
     * applied change through the old 30-frame deadline, in both directions. */
    unsigned char player[256]={0};char profile[32]="DmHeavy";uint64_t f=(uintptr_t)b+0x2d58520;
    *(uint64_t *)(f+0x10)=1;*(char **)(f+0x20)=profile;*(uint64_t *)(f+0x28)=7;*(uint64_t *)(f+0x30)=31;
    dm_lootfilter_pump(b,(uintptr_t)player);assert(!strcmp(profile,"DmHeavy"));
    shortcut();dm_custom_ui_pump(root());dm_lootfilter_pump(b,(uintptr_t)player);assert(!strcmp(profile,""));
    shortcut();dm_custom_ui_pump(root());dm_lootfilter_pump(b,(uintptr_t)player);assert(!strcmp(profile,"DmHeavy"));
    filter_difficulty=1;filter_frame=101;dm_lootfilter_pump(b,(uintptr_t)player);assert(!strcmp(profile,"DmHeavy"));
    filter_frame=130;dm_lootfilter_pump(b,(uintptr_t)player);assert(!strcmp(profile,"DmLite"));
    VirtualFree(b,0,MEM_RELEASE);
    puts("PASS live canonical filter queue, no-panel apply, parity, persistence, checkbox sync, unrelated pending preservation");return 0;
}
