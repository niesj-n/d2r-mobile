#include "driver_loader.h"
#include <adrenotools/driver.h>
#include <dlfcn.h>
#include <link.h>
#include <sys/stat.h>
#include <cstring>
#include <string>

static void *open_system() {
    return dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
}

static void *open_custom(const char *hooks, const char *directory, const char *name) {
    std::string path = std::string(directory) + name;
    struct stat info{};
    if (stat(path.c_str(), &info) || !S_ISREG(info.st_mode)) return nullptr;
    return adrenotools_open_libvulkan(RTLD_NOW | RTLD_LOCAL,
                                     ADRENOTOOLS_DRIVER_CUSTOM,
                                     nullptr, hooks, directory, name, nullptr, nullptr);
}

static PFN_vkGetInstanceProcAddr find_gipa(void *handle) {
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(handle, "vkGetInstanceProcAddr"));
}

struct loaded_search { dev_t device; ino_t inode; bool found; };
static int match_library(struct dl_phdr_info *info, size_t, void *opaque) {
    auto *search = static_cast<loaded_search *>(opaque);
    if (!info->dlpi_name || !*info->dlpi_name) return 0;
    struct stat current{};
    if (!stat(info->dlpi_name, &current) &&
        current.st_dev == search->device && current.st_ino == search->inode) {
        search->found = true;
        return 1;
    }
    return 0;
}

static int path_loaded(const char *path) {
    struct stat expected{};
    if (!path || stat(path, &expected) || !S_ISREG(expected.st_mode)) return 0;
    loaded_search search{expected.st_dev, expected.st_ino, false};
    dl_iterate_phdr(match_library, &search);
    return search.found;
}

const dm_driver_ops *dm_android_driver_ops() {
    static const dm_driver_ops ops{open_system, open_custom, find_gipa, path_loaded};
    return &ops;
}
