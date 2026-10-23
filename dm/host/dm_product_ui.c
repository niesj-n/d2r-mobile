#include "dm_product_ui.h"
#include <pthread.h>

static pthread_mutex_t sort_lock=PTHREAD_MUTEX_INITIALIZER;
static int latest_context=-1, down_context=-1, down_active;
static uint64_t latest_generation, down_generation;
static int supported(int context) { return context==0 || (context>=4 && context<=8); }
void dm_product_ui_scene(int context,uint64_t generation) {
    pthread_mutex_lock(&sort_lock);
    if(context!=latest_context || generation!=latest_generation) down_active=0;
    latest_context=context;latest_generation=generation;
    pthread_mutex_unlock(&sort_lock);
}
void dm_product_ui_touch(int phase) {
    pthread_mutex_lock(&sort_lock);
    if(phase==0) {
        down_context=latest_context;down_generation=latest_generation;
        down_active=supported(down_context);
    } else if(phase==3) down_active=0;
    pthread_mutex_unlock(&sort_lock);
}
int dm_product_ui_sort(int context,uint64_t generation) {
    pthread_mutex_lock(&sort_lock);
    int token=down_active && supported(context) && context==down_context && generation==down_generation ? context : -1;
    down_active=0;
    pthread_mutex_unlock(&sort_lock);
    return token;
}
