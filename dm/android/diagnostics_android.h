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
/* Three bounded categories; snapshot is an export-worker operation, never disk I/O. */
#define DM_ANDROID_PIPELINE_LOG_CAPACITY (160 * 1024)
void dm_android_diagnostics_pipeline_log(const char *text);
size_t dm_android_diagnostics_pipeline_copy(char *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
