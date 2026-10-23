#pragma once
#include <stdint.h>
/* Main game thread publishes the supported page and UI generation. The touch
 * route latches them on physical DOWN without reading any guest UI pointer. */
void dm_product_ui_scene(int context, uint64_t generation);
void dm_product_ui_touch(int phase);
/* Called by the native Sort ButtonWidget's original click message. Returns
 * the original DOWN page token once, or -1 after cancel/page/scene changes. */
int dm_product_ui_sort(int context, uint64_t generation);
