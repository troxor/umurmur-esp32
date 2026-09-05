#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Log free / min-free heap with a short tag (boot, wifi-up, client-in, …). */
void heap_log_snapshot(const char *tag);

#ifdef __cplusplus
}
#endif
