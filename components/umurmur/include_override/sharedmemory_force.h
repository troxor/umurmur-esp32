/* Pre-include: satisfy src/sharedmemory.h include guard and provide no-op API. */
#ifndef SHAREDMEMORY_H_777736932196
#define SHAREDMEMORY_H_777736932196
static inline void Sharedmemory_init(int bindport) { (void)bindport; }
static inline void Sharedmemory_update(void) { }
static inline void Sharedmemory_alivetick(void) { }
static inline void Sharedmemory_deinit(void) { }
#endif
