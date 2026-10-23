#ifdef NDEBUG
#error Driver cache regression requires active assertions
#endif
#include <cassert>
#include <cstdint>
#include "../driver_cache.h"

int main() {
    const uint8_t uuid[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    const char *a = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const char *b = "1123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    auto ka = dm_driver_cache_key(a, 0x5143, 730, 615, uuid);
    assert(!ka.empty() && ka == dm_driver_cache_key(a, 0x5143, 730, 615, uuid));
    assert(ka != dm_driver_cache_key(b, 0x5143, 730, 615, uuid));
    assert(ka != dm_driver_cache_key(a, 0x5143, 730, 616, uuid));
    assert(dm_driver_cache_key(nullptr, 0x5143, 730, 615, uuid).empty());
}
