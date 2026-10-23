#include "driver_dispatch.h"

static bool bound;

int dm_vk_dispatch_bind(PFN_vkGetInstanceProcAddr get_proc) {
    if (bound || !get_proc) return 0;
    volkInitializeCustom(get_proc);
    if (!vkCreateInstance) { volkFinalize(); return 0; }
    bound = true;
    return 1;
}

void dm_vk_dispatch_load_instance(VkInstance instance) {
    if (bound && instance != VK_NULL_HANDLE) volkLoadInstanceOnly(instance);
}

void dm_vk_dispatch_load_device(VkDevice device) {
    if (bound && device != VK_NULL_HANDLE) volkLoadDevice(device);
}

void dm_vk_dispatch_reset_early(void) {
    if (!bound || volkGetLoadedInstance() != VK_NULL_HANDLE) return;
    volkFinalize();
    bound = false;
}
