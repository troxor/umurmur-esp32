#pragma once

#include <stdbool.h>
#include "nvs_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WIFI_NET_FAIL = 0,
	WIFI_NET_STA = 1,       /* STA connected, run umurmur */
	WIFI_NET_AP_CONFIG = 2, /* SoftAP + config portal */
} wifi_net_result_t;

wifi_net_result_t wifi_net_start(const umurmur_nvs_t *cfg);

#ifdef __cplusplus
}
#endif
