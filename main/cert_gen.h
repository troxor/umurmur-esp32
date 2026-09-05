#pragma once

#include "nvs_config.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ensure cfg has cert_pem and key_pem.
 * If either is missing, generate ECDSA P-256 self-signed PEMs, persist to NVS,
 * and allocate cfg->cert_pem / cfg->key_pem (caller frees via nvs_config_free).
 * Returns false on generation or NVS failure.
 */
bool umurmur_certs_ensure(umurmur_nvs_t *cfg);

#ifdef __cplusplus
}
#endif
