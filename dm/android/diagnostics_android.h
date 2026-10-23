#ifndef DM_ANDROID_DIAGNOSTICS_H
#define DM_ANDROID_DIAGNOSTICS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void dm_android_diagnostics_configure(const char *runtime, const char *overlay, const char *revision);
const char *dm_android_diagnostics_runtime(void);
void dm_android_diagnostics_gpu(const char *name, uint32_t api, uint32_t driver, int custom,
                                const char *digest, const char *library);
void dm_android_diagnostics_snapshot(char *out, size_t capacity, uint64_t now_ns);
void dm_android_diagnostics_request(void);
#ifdef __cplusplus
}
#endif
#endif
