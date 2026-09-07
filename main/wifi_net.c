#include "wifi_net.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/inet.h"
#include "sdkconfig.h"

static const char *TAG = "wifi";

#define WIFI_GOT_IP_BIT BIT0
#define WIFI_FAIL_BIT   BIT1

static EventGroupHandle_t s_wifi_events;
static int s_retry;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
	(void)arg;
	(void)data;
	if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry < 5) {
			esp_wifi_connect();
			s_retry++;
			ESP_LOGW(TAG, "STA retry %d", s_retry);
		} else {
			xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
		}
	} else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
		s_retry = 0;
		xEventGroupSetBits(s_wifi_events, WIFI_GOT_IP_BIT);
	}
}

static char s_captive_uri[32];

static void dhcp_set_captiveportal_url(void)
{
	esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
	if (!netif)
		return;

	esp_netif_ip_info_t ip_info;
	if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK)
		return;

	char ip_addr[16];
	inet_ntoa_r(ip_info.ip.addr, ip_addr, sizeof(ip_addr));
	snprintf(s_captive_uri, sizeof(s_captive_uri), "http://%s", ip_addr);

	esp_netif_dhcps_stop(netif);
	esp_err_t err = esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI,
					       s_captive_uri, strlen(s_captive_uri));
	esp_netif_dhcps_start(netif);
	if (err == ESP_OK)
		ESP_LOGI(TAG, "DHCP captive portal URI %s", s_captive_uri);
	else
		ESP_LOGW(TAG, "DHCP Option 114 failed: %s", esp_err_to_name(err));
}

static bool start_softap(void)
{
	esp_netif_create_default_wifi_ap();

	wifi_config_t cfg = {0};
	strncpy((char *)cfg.ap.ssid, CONFIG_UMURMUR_WIFI_AP_SSID, sizeof(cfg.ap.ssid) - 1);
	cfg.ap.ssid_len = strlen((char *)cfg.ap.ssid);
	cfg.ap.channel = CONFIG_UMURMUR_WIFI_AP_CHANNEL;
	cfg.ap.max_connection = 4;
	cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
	if (strlen(CONFIG_UMURMUR_WIFI_AP_PASSWORD) < 8) {
		cfg.ap.authmode = WIFI_AUTH_OPEN;
	} else {
		strncpy((char *)cfg.ap.password, CONFIG_UMURMUR_WIFI_AP_PASSWORD, sizeof(cfg.ap.password) - 1);
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));
	ESP_ERROR_CHECK(esp_wifi_start());
	dhcp_set_captiveportal_url();
	ESP_LOGI(TAG, "SoftAP SSID=%s channel=%d", CONFIG_UMURMUR_WIFI_AP_SSID, CONFIG_UMURMUR_WIFI_AP_CHANNEL);
	return true;
}

static bool try_sta(const umurmur_nvs_t *cfg)
{
	if (!cfg->wifi_sta_ssid[0]) {
		ESP_LOGI(TAG, "No SSID provided, return to config");
		return false;
	}

	esp_netif_create_default_wifi_sta();
	s_wifi_events = xEventGroupCreate();
	s_retry = 0;

	esp_event_handler_instance_t inst_any, inst_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, &inst_any));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, NULL, &inst_ip));

	wifi_config_t wcfg = {0};
	strncpy((char *)wcfg.sta.ssid, cfg->wifi_sta_ssid, sizeof(wcfg.sta.ssid) - 1);
	strncpy((char *)wcfg.sta.password, cfg->wifi_sta_pass, sizeof(wcfg.sta.password) - 1);
	wcfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wcfg));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "STA connecting to '%s'...", cfg->wifi_sta_ssid);

	EventBits_t bits = xEventGroupWaitBits(s_wifi_events, WIFI_GOT_IP_BIT | WIFI_FAIL_BIT,
		pdFALSE, pdFALSE, pdMS_TO_TICKS(CONFIG_UMURMUR_WIFI_STA_TIMEOUT_MS));

	esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, inst_any);
	esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, inst_ip);
	vEventGroupDelete(s_wifi_events);
	s_wifi_events = NULL;

	if (bits & WIFI_GOT_IP_BIT) {
		ESP_LOGI(TAG, "STA got IP");
		return true;
	}

	ESP_LOGW(TAG, "STA connect failed; return to config");
	esp_wifi_stop();
	esp_wifi_deinit();
	return false;
}

wifi_net_result_t wifi_net_start(const umurmur_nvs_t *cfg)
{
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));

	if (try_sta(cfg))
		return WIFI_NET_STA;

	{
		wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
	}
	if (!start_softap())
		return WIFI_NET_FAIL;
	return WIFI_NET_AP_CONFIG;
}
