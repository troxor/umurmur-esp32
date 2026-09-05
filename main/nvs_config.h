#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char wifi_sta_ssid[33];
	char wifi_sta_pass[65];
	char murmur_password[65];
	char admin_password[65];
	/* PEM blobs from NVS (or generated); NULL if not yet present. */
	char *cert_pem; /* heap-allocated if from NVS blob; NULL if unused */
	char *key_pem;
} umurmur_nvs_t;

/** Load NVS namespace "umurmur"; fill missing fields from Kconfig defaults. */
void nvs_config_load(umurmur_nvs_t *out);

/** Free any heap PEM buffers from nvs_config_load. */
void nvs_config_free(umurmur_nvs_t *cfg);

/**
 * Write string keys (passwords / WiFi). PEMs use set_blob helpers below.
 * Keys: wifi_ssid, wifi_pass, password, admin_password
 */
bool nvs_config_set_str(const char *key, const char *value);

bool nvs_config_set_blob(const char *key, const void *data, size_t len);

#ifdef __cplusplus
}
#endif
