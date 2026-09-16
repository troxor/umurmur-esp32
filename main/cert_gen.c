#include "cert_gen.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "psa/crypto.h"

static const char *TAG = "cert_gen";

#define PEM_BUF_CERT 2048
#define PEM_BUF_KEY  2048

static void mbedtls_fail(const char *what, int rc)
{
	char buf[128];
	mbedtls_strerror(rc, buf, sizeof(buf));
	ESP_LOGE(TAG, "%s: %s (-0x%04x)", what, buf, (unsigned)-rc);
}

/* Allocate NUL-terminated PEM copies; returns false on OOM. */
static bool dup_pems(umurmur_nvs_t *cfg, const char *cert, const char *key)
{
	char *c = strdup(cert);
	char *k = strdup(key);
	if (!c || !k) {
		free(c);
		free(k);
		return false;
	}
	free(cfg->cert_pem);
	free(cfg->key_pem);
	cfg->cert_pem = c;
	cfg->key_pem = k;
	return true;
}

static bool generate_ecdsa_pems(char *cert_pem, size_t cert_sz, char *key_pem, size_t key_sz)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	mbedtls_pk_context key;
	mbedtls_x509write_cert crt;
	unsigned char serial = 1;
	int rc;
	bool ok = false;

	if (psa_crypto_init() != PSA_SUCCESS) {
		ESP_LOGE(TAG, "psa_crypto_init failed");
		return false;
	}

	psa_set_key_usage_flags(&attributes,
				PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH |
				PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_generate_key(&attributes, &key_id);
	psa_reset_key_attributes(&attributes);
	if (status != PSA_SUCCESS) {
		ESP_LOGE(TAG, "psa_generate_key failed: %d", (int)status);
		return false;
	}

	mbedtls_pk_init(&key);
	mbedtls_x509write_crt_init(&crt);

	rc = mbedtls_pk_copy_from_psa(key_id, &key);
	if (rc != 0) {
		mbedtls_fail("pk_copy_from_psa", rc);
		goto out;
	}

	mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
	mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
	mbedtls_x509write_crt_set_subject_key(&crt, &key);
	mbedtls_x509write_crt_set_issuer_key(&crt, &key);

	rc = mbedtls_x509write_crt_set_subject_name(&crt, "CN=umurmur-esp32");
	if (rc != 0) {
		mbedtls_fail("set_subject_name", rc);
		goto out;
	}
	rc = mbedtls_x509write_crt_set_issuer_name(&crt, "CN=umurmur-esp32");
	if (rc != 0) {
		mbedtls_fail("set_issuer_name", rc);
		goto out;
	}

	/* Fixed window: no RTC required on first boot. */
	rc = mbedtls_x509write_crt_set_validity(&crt, "20240101000000", "20440101000000");
	if (rc != 0) {
		mbedtls_fail("set_validity", rc);
		goto out;
	}

	rc = mbedtls_x509write_crt_set_serial_raw(&crt, &serial, 1);
	if (rc != 0) {
		mbedtls_fail("set_serial", rc);
		goto out;
	}

	rc = mbedtls_x509write_crt_set_basic_constraints(&crt, 0, -1);
	if (rc != 0) {
		mbedtls_fail("basic_constraints", rc);
		goto out;
	}

	memset(cert_pem, 0, cert_sz);
	memset(key_pem, 0, key_sz);

	rc = mbedtls_x509write_crt_pem(&crt, (unsigned char *)cert_pem, cert_sz);
	if (rc != 0) {
		mbedtls_fail("crt_pem", rc);
		goto out;
	}

	rc = mbedtls_pk_write_key_pem(&key, (unsigned char *)key_pem, key_sz);
	if (rc != 0) {
		mbedtls_fail("pk_write_key_pem", rc);
		goto out;
	}

	ok = true;
out:
	mbedtls_x509write_crt_free(&crt);
	mbedtls_pk_free(&key);
	if (key_id != PSA_KEY_ID_NULL)
		psa_destroy_key(key_id);
	return ok;
}

bool umurmur_certs_ensure(umurmur_nvs_t *cfg)
{
	char *cert_pem = NULL;
	char *key_pem = NULL;
	bool ok = false;

	if (!cfg)
		return false;

	if (cfg->cert_pem && cfg->cert_pem[0] && cfg->key_pem && cfg->key_pem[0])
		return true;

	ESP_LOGI(TAG, "No TLS material in NVS; generating ECDSA P-256 self-signed cert");

	cert_pem = calloc(1, PEM_BUF_CERT);
	key_pem = calloc(1, PEM_BUF_KEY);
	if (!cert_pem || !key_pem) {
		ESP_LOGE(TAG, "OOM allocating PEM buffers");
		goto out;
	}

	if (!generate_ecdsa_pems(cert_pem, PEM_BUF_CERT, key_pem, PEM_BUF_KEY))
		goto out;

	if (!nvs_config_set_blob("cert_pem", cert_pem, strlen(cert_pem) + 1) ||
	    !nvs_config_set_blob("key_pem", key_pem, strlen(key_pem) + 1)) {
		ESP_LOGE(TAG, "Failed to persist generated cert/key to NVS");
		goto out;
	}

	if (!dup_pems(cfg, cert_pem, key_pem)) {
		ESP_LOGE(TAG, "OOM copying generated PEMs");
		goto out;
	}

	ESP_LOGI(TAG, "Generated and stored TLS cert/key in NVS");
	ok = true;
out:
	free(cert_pem);
	free(key_pem);
	return ok;
}
