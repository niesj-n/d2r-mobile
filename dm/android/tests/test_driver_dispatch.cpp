/* The production dispatch must use one supplied loader for instance and device calls. */
#ifdef NDEBUG
#error Vulkan dispatch regression requires active assertions
#endif
#include <cassert>
#include <cstring>
#include <volk.h>
#include "../driver_dispatch.h"

static int instance_calls, enumerate_calls, submit_calls;
static VKAPI_ATTR VkResult VKAPI_CALL fake_create_instance(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *out) {
    ++instance_calls; *out = reinterpret_cast<VkInstance>(0x1000); return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL fake_enumerate(VkInstance, uint32_t *count, VkPhysicalDevice *) {
    ++enumerate_calls; *count = 1; return VK_SUCCESS;
}
static VKAPI_ATTR VkResult VKAPI_CALL fake_submit(VkQueue, uint32_t, const VkSubmitInfo *, VkFence) {
    ++submit_calls; return VK_SUCCESS;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_gdpa(VkDevice, const char *name) {
    return std::strcmp(name, "vkQueueSubmit") == 0 ? reinterpret_cast<PFN_vkVoidFunction>(fake_submit) : nullptr;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_gipa(VkInstance, const char *name) {
    if (std::strcmp(name, "vkCreateInstance") == 0) return reinterpret_cast<PFN_vkVoidFunction>(fake_create_instance);
    if (std::strcmp(name, "vkEnumeratePhysicalDevices") == 0) return reinterpret_cast<PFN_vkVoidFunction>(fake_enumerate);
    if (std::strcmp(name, "vkGetDeviceProcAddr") == 0) return reinterpret_cast<PFN_vkVoidFunction>(fake_gdpa);
    return nullptr;
}
int main() {
    assert(dm_vk_dispatch_bind(fake_gipa));
    VkInstance instance = VK_NULL_HANDLE;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    assert(vkCreateInstance(&create, nullptr, &instance) == VK_SUCCESS && instance_calls == 1);
    dm_vk_dispatch_load_instance(instance);
    uint32_t count = 0;
    assert(vkEnumeratePhysicalDevices(instance, &count, nullptr) == VK_SUCCESS && count == 1 && enumerate_calls == 1);
    dm_vk_dispatch_load_device(reinterpret_cast<VkDevice>(0x2000));
    assert(vkQueueSubmit(reinterpret_cast<VkQueue>(0x3000), 0, nullptr, VK_NULL_HANDLE) == VK_SUCCESS && submit_calls == 1);
}
