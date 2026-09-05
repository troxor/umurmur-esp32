/* ESP stub: sharedmemory API disabled; avoid Linux sys/mman.h from src/sharedmemory.h */
#ifndef SHAREDMEMORY_H_ESP_STUB
#define SHAREDMEMORY_H_ESP_STUB

static inline void Sharedmemory_init(int bindport) { (void)bindport; }
static inline void Sharedmemory_update(void) { }
static inline void Sharedmemory_alivetick(void) { }
static inline void Sharedmemory_deinit(void) { }

#endif
