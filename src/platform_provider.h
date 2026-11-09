/* Private build-selected platform-provider contract. */
#ifndef HAMMER_PLATFORM_PROVIDER__H
#define HAMMER_PLATFORM_PROVIDER__H

#include "platform.h"

/*
 * Providers with H_PLATFORM_THREADS must make these operations safe for
 * concurrent graph construction/destruction. current_graph_slot() returns
 * storage for the calling thread's active HParserGraph pointer. Providers
 * without threaded support may return one process-wide slot and use no-op
 * locking; their graph API is then serial-only.
 */
void h_platform_graph_lock(void);
void h_platform_graph_unlock(void);
void **h_platform_current_graph_slot(void);

#endif /* HAMMER_PLATFORM_PROVIDER__H */
