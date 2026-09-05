#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Apply runtime overrides before Conf_init / Server_run.
 * password/admin may be empty strings.
 * cert_pem/key_pem must be non-empty PEMs (app_main ensures via umurmur_certs_ensure).
 * Pointers are copied; caller may free after this returns.
 */
void umurmur_conf_apply(const char *password, const char *admin_password,
			const char *cert_pem, const char *key_pem);

#ifdef __cplusplus
}
#endif
