#pragma once

#include <stdbool.h>
#include "nvs_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Try STA using cfg SSID/pass. On empty SSID or failure, start SoftAP.
 * Blocks until an IP is available (STA or AP). Returns true on success.
 */
bool wifi_net_start(const umurmur_nvs_t *cfg);

#ifdef __cplusplus
}
#endif
