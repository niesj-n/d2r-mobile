#pragma once
#include <stddef.h>
#include "driver_dispatch.h"

struct dm_driver_choice {
    const char *private_root;  /* getFilesDir(), never a document-picker path */
    const char *hook_lib_dir;  /* ApplicationInfo.nativeLibraryDir */
    const char *digest;        /* 64 lower-case SHA-256 hex characters or null */
    const char *driver_name;   /* validated vulkan.*.so basename or null */
};

struct dm_driver_loaded {
    void *handle;
    bool custom;
    char full_path[1024];
};

struct dm_driver_ops {
    void *(*open_system)();
    void *(*open_custom)(const char *hook_lib_dir, const char *driver_dir, const char *driver_name);
    PFN_vkGetInstanceProcAddr (*find_gipa)(void *handle);
    int (*path_loaded)(const char *absolute_path);
};

bool dm_driver_open(const dm_driver_ops *ops, const dm_driver_choice *choice, dm_driver_loaded *loaded);
bool dm_driver_fallback_system(const dm_driver_ops *ops, dm_driver_loaded *loaded);
bool dm_driver_verify(const dm_driver_ops *ops, const dm_driver_loaded *loaded);
const dm_driver_ops *dm_android_driver_ops();
