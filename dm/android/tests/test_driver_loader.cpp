/* The real selection algorithm receives controlled OS/AdrenoTools operations. */
#ifdef NDEBUG
#error Driver-loader regression requires active assertions
#endif
#include <cassert>
#include <cstring>
#include <volk.h>
#include "../driver_loader.h"

static int system_opens, custom_opens, loaded_checks;
static bool custom_available = true, selected_loaded = true;
static VKAPI_ATTR VkResult VKAPI_CALL system_create(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *) { return VK_SUCCESS; }
static VKAPI_ATTR VkResult VKAPI_CALL custom_create(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *) { return VK_ERROR_INCOMPATIBLE_DRIVER; }
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL system_gipa(VkInstance, const char *name) {
    return std::strcmp(name, "vkCreateInstance") == 0 ? reinterpret_cast<PFN_vkVoidFunction>(system_create) : nullptr;
}
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL custom_gipa(VkInstance, const char *name) {
    return std::strcmp(name, "vkCreateInstance") == 0 ? reinterpret_cast<PFN_vkVoidFunction>(custom_create) : nullptr;
}
static void *open_system() { ++system_opens; return reinterpret_cast<void *>(1); }
static void *open_custom(const char *hooks, const char *dir, const char *name) {
    ++custom_opens;
    assert(std::strcmp(hooks, "/data/app/lib/arm64") == 0);
    assert(std::strstr(dir, "/drivers/") && dir[std::strlen(dir) - 1] == '/');
    assert(std::strcmp(name, "vulkan.adreno.so") == 0 || std::strcmp(name, "libvulkan_freedreno.so") == 0);
    return custom_available ? reinterpret_cast<void *>(2) : nullptr;
}
static PFN_vkGetInstanceProcAddr find_gipa(void *handle) { return handle == reinterpret_cast<void *>(2) ? custom_gipa : system_gipa; }
static int driver_loaded(const char *path) { ++loaded_checks; assert(std::strstr(path, "/drivers/")); return selected_loaded; }
static dm_driver_ops ops{open_system, open_custom, find_gipa, driver_loaded};
static const char *digest = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static dm_driver_choice selected{"/data/user/0/example/files", "/data/app/lib/arm64", digest, "vulkan.adreno.so"};

int main() {
    dm_driver_loaded active{};
    assert(dm_driver_open(&ops, &selected, &active) && active.custom && custom_opens == 1 && system_opens == 0);
    assert(vkCreateInstance(nullptr, nullptr, nullptr) == VK_ERROR_INCOMPATIBLE_DRIVER);
    assert(dm_driver_verify(&ops, &active));
    selected_loaded = false;
    assert(!dm_driver_verify(&ops, &active) && loaded_checks == 2); /* silent hook fallback is rejected */
    assert(dm_driver_fallback_system(&ops, &active) && !active.custom && system_opens == 1);
    assert(vkCreateInstance(nullptr, nullptr, nullptr) == VK_SUCCESS); /* system proc replaced custom proc */
    dm_vk_dispatch_reset_early();

    custom_available = false;
    assert(dm_driver_open(&ops, &selected, &active) && !active.custom && system_opens == 2);
    assert(vkCreateInstance(nullptr, nullptr, nullptr) == VK_SUCCESS);
    dm_vk_dispatch_reset_early();

    dm_driver_choice default_choice{selected.private_root, selected.hook_lib_dir, nullptr, nullptr};
    assert(dm_driver_open(&ops, &default_choice, &active) && !active.custom && system_opens == 3);
    dm_vk_dispatch_reset_early();

    dm_driver_choice bad{selected.private_root, selected.hook_lib_dir, "../escape", selected.driver_name};
    assert(dm_driver_open(&ops, &bad, &active) && !active.custom && custom_opens == 2);
    dm_vk_dispatch_reset_early();
    custom_available = true;
    dm_driver_choice mesa{selected.private_root, selected.hook_lib_dir, digest, "libvulkan_freedreno.so"};
    assert(dm_driver_open(&ops, &mesa, &active) && active.custom);
    dm_vk_dispatch_load_instance(reinterpret_cast<VkInstance>(0x1234));
    assert(!dm_driver_fallback_system(&ops, &active));  /* after instance creation, require a clean process */
}
