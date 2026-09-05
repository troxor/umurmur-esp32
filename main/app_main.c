#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "heap_log.h"
#include "nvs_config.h"
#include "cert_gen.h"
#include "wifi_net.h"
#include "umurmur_task.h"
#include "conf_esp.h"
#include "sdkconfig.h"

static const char *TAG = "app";

void app_main(void)
{
	heap_log_snapshot("boot");

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	umurmur_nvs_t cfg;
	nvs_config_load(&cfg);

	if (!umurmur_certs_ensure(&cfg)) {
		ESP_LOGE(TAG, "TLS cert/key unavailable; not starting umurmur");
		nvs_config_free(&cfg);
		return;
	}

	umurmur_conf_apply(cfg.murmur_password, cfg.admin_password,
			   cfg.cert_pem, cfg.key_pem);

	if (!wifi_net_start(&cfg)) {
		ESP_LOGE(TAG, "WiFi failed");
		nvs_config_free(&cfg);
		return;
	}
	heap_log_snapshot("wifi-up");
	nvs_config_free(&cfg); /* passwords/certs already copied into conf_esp */

	BaseType_t ok = xTaskCreate(
		umurmur_task,
		"umurmur",
		CONFIG_UMURMUR_TASK_STACK,
		NULL,
		CONFIG_UMURMUR_TASK_PRIORITY,
		NULL);
	if (ok != pdPASS) {
		ESP_LOGE(TAG, "Failed to create umurmur task");
		return;
	}

	ESP_LOGI(TAG, "umurmur task started (port %d, max_users %d)",
		CONFIG_UMURMUR_BIND_PORT, CONFIG_UMURMUR_MAX_USERS);
}
