#pragma once
#include "../third_party/volk/volk.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One loader and one VkInstance/VkDevice for presenter and replay engine. */
int dm_vk_dispatch_bind(PFN_vkGetInstanceProcAddr get_proc);
void dm_vk_dispatch_load_instance(VkInstance instance);
void dm_vk_dispatch_load_device(VkDevice device);
void dm_vk_dispatch_reset_early(void);

#ifdef __cplusplus
}
#endif
