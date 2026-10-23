#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

/* Empty key keeps the existing system-driver cache location. */
static inline std::string dm_driver_cache_key(const char *digest, uint32_t vendor,
                                               uint32_t device, uint32_t version,
                                               const uint8_t uuid[16]) {
    if (!digest || std::strlen(digest) != 64 || !uuid) return {};
    for (int i = 0; i < 64; ++i)
        if (!((digest[i] >= '0' && digest[i] <= '9') ||
              (digest[i] >= 'a' && digest[i] <= 'f'))) return {};
    char tail[64];
    int at = std::snprintf(tail, sizeof tail, "-%08x-%08x-%08x-", vendor, device, version);
    for (int i = 0; i < 16; ++i) at += std::snprintf(tail + at, sizeof tail - at, "%02x", uuid[i]);
    return std::string("custom-") + digest + tail;
}
