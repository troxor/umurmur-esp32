#include "nvs_config.h"

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h"

static const char *TAG = "nvs_cfg";
static const char *NS = "umurmur";

static void copy_default(char *dst, size_t dstlen, const char *def)
{
	if (!def)
		def = "";
	strncpy(dst, def, dstlen - 1);
	dst[dstlen - 1] = '\0';
}

static esp_err_t open_rw(nvs_handle_t *h)
{
	return nvs_open(NS, NVS_READWRITE, h);
}

static void load_str(nvs_handle_t h, const char *key, char *dst, size_t dstlen, const char *def)
{
	size_t len = dstlen;
	esp_err_t err = nvs_get_str(h, key, dst, &len);
	if (err != ESP_OK)
		copy_default(dst, dstlen, def);
}

static char *load_blob_z(nvs_handle_t h, const char *key)
{
	size_t len = 0;
	esp_err_t err = nvs_get_blob(h, key, NULL, &len);
	if (err != ESP_OK || len == 0)
		return NULL;
	char *buf = malloc(len + 1);
	if (!buf)
		return NULL;
	err = nvs_get_blob(h, key, buf, &len);
	if (err != ESP_OK) {
		free(buf);
		return NULL;
	}
	buf[len] = '\0';
	/* PEM must start with -----BEGIN */
	if (strncmp(buf, "-----BEGIN", 10) != 0) {
		ESP_LOGW(TAG, "NVS blob '%s' is not a PEM; ignoring", key);
		free(buf);
		return NULL;
	}
	return buf;
}

void nvs_config_load(umurmur_nvs_t *out)
{
	memset(out, 0, sizeof(*out));
	nvs_handle_t h;
	esp_err_t err = open_rw(&h);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "nvs_open failed (%s); using Kconfig defaults", esp_err_to_name(err));
		copy_default(out->wifi_sta_ssid, sizeof(out->wifi_sta_ssid), CONFIG_UMURMUR_WIFI_STA_SSID);
		copy_default(out->wifi_sta_pass, sizeof(out->wifi_sta_pass), CONFIG_UMURMUR_WIFI_STA_PASSWORD);
		copy_default(out->murmur_password, sizeof(out->murmur_password), CONFIG_UMURMUR_PASSWORD);
		copy_default(out->admin_password, sizeof(out->admin_password), CONFIG_UMURMUR_ADMIN_PASSWORD);
		return;
	}

	load_str(h, "wifi_ssid", out->wifi_sta_ssid, sizeof(out->wifi_sta_ssid), CONFIG_UMURMUR_WIFI_STA_SSID);
	load_str(h, "wifi_pass", out->wifi_sta_pass, sizeof(out->wifi_sta_pass), CONFIG_UMURMUR_WIFI_STA_PASSWORD);
	load_str(h, "password", out->murmur_password, sizeof(out->murmur_password), CONFIG_UMURMUR_PASSWORD);
	load_str(h, "admin_password", out->admin_password, sizeof(out->admin_password), CONFIG_UMURMUR_ADMIN_PASSWORD);
	out->cert_pem = load_blob_z(h, "cert_pem");
	out->key_pem = load_blob_z(h, "key_pem");
	nvs_close(h);
}

void nvs_config_free(umurmur_nvs_t *cfg)
{
	if (!cfg)
		return;
	free(cfg->cert_pem);
	free(cfg->key_pem);
	cfg->cert_pem = NULL;
	cfg->key_pem = NULL;
}

bool nvs_config_set_str(const char *key, const char *value)
{
	nvs_handle_t h;
	if (open_rw(&h) != ESP_OK)
		return false;
	esp_err_t err = nvs_set_str(h, key, value ? value : "");
	if (err == ESP_OK)
		err = nvs_commit(h);
	nvs_close(h);
	return err == ESP_OK;
}

bool nvs_config_set_blob(const char *key, const void *data, size_t len)
{
	nvs_handle_t h;
	if (open_rw(&h) != ESP_OK)
		return false;
	esp_err_t err = nvs_set_blob(h, key, data, len);
	if (err == ESP_OK)
		err = nvs_commit(h);
	nvs_close(h);
	return err == ESP_OK;
}
