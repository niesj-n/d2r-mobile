#include "driver_loader.h"
#include <cstdio>
#include <cstring>

static bool valid_digest(const char *digest) {
    if (!digest || std::strlen(digest) != 64) return false;
    for (int i = 0; i < 64; ++i)
        if (!((digest[i] >= '0' && digest[i] <= '9') ||
              (digest[i] >= 'a' && digest[i] <= 'f'))) return false;
    return true;
}

static bool valid_name(const char *name) {
    if (!name) return false;
    size_t prefix = std::strncmp(name, "vulkan.", 7) == 0 ? 7 :
                    std::strncmp(name, "libvulkan_", 10) == 0 ? 10 : 0;
    if (!prefix) return false;
    size_t n = std::strlen(name);
    if (n <= prefix + 3 || n > 80 || std::strcmp(name + n - 3, ".so")) return false;
    for (size_t i = prefix; i < n - 3; ++i)
        if (!((name[i] >= 'a' && name[i] <= 'z') ||
              (name[i] >= '0' && name[i] <= '9') ||
              name[i] == '_' || name[i] == '-')) return false;
    return true;
}

static bool open_system(const dm_driver_ops *ops, dm_driver_loaded *loaded) {
    void *handle = ops->open_system();
    PFN_vkGetInstanceProcAddr proc = handle ? ops->find_gipa(handle) : nullptr;
    if (!proc || !dm_vk_dispatch_bind(proc)) return false;
    loaded->handle = handle;
    loaded->custom = false;
    loaded->full_path[0] = 0;
    return true;
}

bool dm_driver_open(const dm_driver_ops *ops, const dm_driver_choice *choice, dm_driver_loaded *loaded) {
    if (!ops || !choice || !loaded || !ops->open_system || !ops->open_custom ||
        !ops->find_gipa || !ops->path_loaded) return false;
    *loaded = {};
    if (choice->private_root && choice->private_root[0] == '/' &&
        choice->hook_lib_dir && choice->hook_lib_dir[0] == '/' &&
        valid_digest(choice->digest) && valid_name(choice->driver_name)) {
        char directory[sizeof loaded->full_path];
        int d = std::snprintf(directory, sizeof directory, "%s/drivers/%s/", choice->private_root, choice->digest);
        if (d > 0 && static_cast<size_t>(d) < sizeof directory) {
            int p = std::snprintf(loaded->full_path, sizeof loaded->full_path, "%s%s", directory, choice->driver_name);
            if (p > 0 && static_cast<size_t>(p) < sizeof loaded->full_path) {
                void *handle = ops->open_custom(choice->hook_lib_dir, directory, choice->driver_name);
                PFN_vkGetInstanceProcAddr proc = handle ? ops->find_gipa(handle) : nullptr;
                if (proc && dm_vk_dispatch_bind(proc)) {
                    loaded->handle = handle;
                    loaded->custom = true;
                    return true;
                }
            }
        }
    }
    return open_system(ops, loaded);
}

bool dm_driver_fallback_system(const dm_driver_ops *ops, dm_driver_loaded *loaded) {
    if (!ops || !loaded || !loaded->custom || volkGetLoadedInstance() != VK_NULL_HANDLE) return false;
    dm_vk_dispatch_reset_early();
    return open_system(ops, loaded);
}

bool dm_driver_verify(const dm_driver_ops *ops, const dm_driver_loaded *loaded) {
    return ops && loaded && (!loaded->custom ||
           (ops->path_loaded && ops->path_loaded(loaded->full_path)));
}
