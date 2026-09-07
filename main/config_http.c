#include "config_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_config.h"
#include "sdkconfig.h"

#include "dns_server.h"

static const char *TAG = "cfg_http";

// Since umurmur isn't running yet, we can accept large data, e.g. certificates
#define BODY_MAX (16 * 1024)

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

static bool url_decode(const char *src, size_t srclen, char *dst, size_t dstlen)
{
	size_t di = 0;
	for (size_t si = 0; si < srclen && di + 1 < dstlen; ) {
		char c = src[si++];
		if (c == '+') {
			dst[di++] = ' ';
		} else if (c == '%' && si + 1 < srclen) {
			int hi = hex_nibble(src[si]);
			int lo = hex_nibble(src[si + 1]);
			if (hi < 0 || lo < 0)
				return false;
			dst[di++] = (char)((hi << 4) | lo);
			si += 2;
		} else {
			dst[di++] = c;
		}
	}
	dst[di] = '\0';
	return true;
}

static bool form_get(const char *body, size_t bodylen, const char *key, char *out, size_t outlen)
{
	size_t klen = strlen(key);
	const char *p = body;
	const char *end = body + bodylen;
	while (p < end) {
		const char *amp = memchr(p, '&', (size_t)(end - p));
		const char *pair_end = amp ? amp : end;
		const char *eq = memchr(p, '=', (size_t)(pair_end - p));
		if (eq && (size_t)(eq - p) == klen && memcmp(p, key, klen) == 0) {
			return url_decode(eq + 1, (size_t)(pair_end - eq - 1), out, outlen);
		}
		p = amp ? amp + 1 : end;
	}
	out[0] = '\0';
	return true;
}

static char *form_get_alloc(const char *body, size_t bodylen, const char *key)
{
	size_t klen = strlen(key);
	const char *p = body;
	const char *end = body + bodylen;
	while (p < end) {
		const char *amp = memchr(p, '&', (size_t)(end - p));
		const char *pair_end = amp ? amp : end;
		const char *eq = memchr(p, '=', (size_t)(pair_end - p));
		if (eq && (size_t)(eq - p) == klen && memcmp(p, key, klen) == 0) {
			size_t elen = (size_t)(pair_end - eq - 1);
			char *out = malloc(elen + 1);
			if (!out)
				return NULL;
			if (!url_decode(eq + 1, elen, out, elen + 1)) {
				free(out);
				return NULL;
			}
			return out;
		}
		p = amp ? amp + 1 : end;
	}
	char *empty = malloc(1);
	if (empty)
		empty[0] = '\0';
	return empty;
}

static bool looks_like_pem(const char *s)
{
	return s && strncmp(s, "-----BEGIN", 10) == 0;
}

static void reboot_task(void *arg)
{
	(void)arg;
	vTaskDelay(pdMS_TO_TICKS(800));
	esp_restart();
}

static const char PAGE_CSS[] =
	"*{box-sizing:border-box}"
	"body{margin:0;min-height:100vh;color:#f0e7d5;"
	"font:16px/1.5 \"Helvetica Neue\",Helvetica,Arial,sans-serif;"
	"background:linear-gradient(#2a2a29,#1c1c1c);background-attachment:fixed}"
	"main{max-width:36rem;margin:0 auto;padding:1.5rem 1.25rem 3rem}"
	"header h1{margin:0;font-weight:300;font-size:2.25rem;letter-spacing:-.02em;color:#f0e7d5}"
	"header .tag{margin:.35rem 0 0;color:#b6b6b6;font-size:.95rem}"
	"hr{border:0;height:1px;margin:1rem 0 1.25rem;"
	"background:linear-gradient(90deg,transparent,#ffcc00,transparent)}"
	".hint{color:#b6b6b6;font-size:.9rem;margin:0 0 1.25rem}"
	"label{display:block;margin:0 0 1rem;color:#e8e8e8;font-size:.9rem}"
	"input,textarea{display:block;width:100%;margin-top:.35rem;padding:.55rem .65rem;"
	"color:#efefef;background:#191919;border:1px solid #3a3a3a;border-radius:3px;"
	"font:inherit}"
	"textarea{font-family:ui-monospace,Menlo,Consolas,monospace;font-size:.8rem;resize:vertical}"
	"input:focus,textarea:focus{outline:0;border-color:#93bd20}"
	"button{margin-top:.5rem;padding:.65rem 1.25rem;border:0;border-radius:3px;"
	"color:#fff;font:600 .95rem/1 \"Helvetica Neue\",Helvetica,Arial,sans-serif;"
	"background:linear-gradient(#93bd20,#659e10);cursor:pointer}"
	"button:hover{background:linear-gradient(#749619,#527f0e)}"
	".ok{margin-top:2rem;text-align:center}"
	".ok h1{font-weight:300;color:#ffcc00}";

static esp_err_t root_get(httpd_req_t *req)
{
	const umurmur_nvs_t *cfg = req->user_ctx;
	const char *ssid = cfg && cfg->wifi_sta_ssid[0] ? cfg->wifi_sta_ssid : "";

	char *page = NULL;
	int n = asprintf(&page,
		"<!DOCTYPE html><html><head><meta charset=utf-8>"
		"<meta name=viewport content=\"width=device-width,initial-scale=1\">"
		"<title>uMurmur setup</title><style>%s</style></head><body><main>"
		"<h3>uMurmur setup</h3>"
		"<header><p class=tag>Wireless configuration</p><hr></header>"
		"<form method=POST action=/save>"
		"<label>WiFi SSID<input name=wifi_ssid required maxlength=32 value=\"%s\"></label>"
		"<label>WiFi password<input name=wifi_pass type=password maxlength=64></label>"
		"<header><p class=tag>Optional configuration</p><hr></header>"
		"<label>Mumble password<input name=password type=password maxlength=64></label>"
		"<label>Admin password<input name=admin_password type=password maxlength=64></label>"
		"<p class=hint>Leave these fields empty to auto-generate a self-signed certificate.</p>"
		"<label>TLS certificate PEM"
		"<textarea name=cert_pem rows=8 placeholder=\"-----BEGIN CERTIFICATE-----\"></textarea></label>"
		"<label>TLS private key PEM"
		"<textarea name=key_pem rows=8 placeholder=\"-----BEGIN PRIVATE KEY-----\"></textarea></label>"
		"<button type=submit>Save &amp; reboot</button>"
		"</form></main></body></html>",
		PAGE_CSS, ssid);
	if (n < 0 || !page) {
		free(page);
		return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
	}
	httpd_resp_set_type(req, "text/html");
	esp_err_t err = httpd_resp_send(req, page, n);
	free(page);
	return err;
}

static int recv_all(httpd_req_t *req, char *buf, int total)
{
	int got = 0;
	while (got < total) {
		int r = httpd_req_recv(req, buf + got, total - got);
		if (r <= 0)
			return r;
		got += r;
	}
	return got;
}

static esp_err_t save_post(httpd_req_t *req)
{
	if (req->content_len <= 0 || req->content_len >= BODY_MAX) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad body");
		return ESP_FAIL;
	}

	char *body = malloc((size_t)req->content_len + 1);
	if (!body) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
		return ESP_FAIL;
	}

	int got = recv_all(req, body, req->content_len);
	if (got != req->content_len) {
		free(body);
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv");
		return ESP_FAIL;
	}
	body[got] = '\0';

	char wifi_ssid[33], wifi_pass[65], password[65], admin_password[65];
	char *cert_pem = NULL;
	char *key_pem = NULL;
	esp_err_t fail = ESP_FAIL;

	if (!form_get(body, (size_t)got, "wifi_ssid", wifi_ssid, sizeof(wifi_ssid)) ||
	    !form_get(body, (size_t)got, "wifi_pass", wifi_pass, sizeof(wifi_pass)) ||
	    !form_get(body, (size_t)got, "password", password, sizeof(password)) ||
	    !form_get(body, (size_t)got, "admin_password", admin_password, sizeof(admin_password))) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "decode");
		goto out;
	}

	cert_pem = form_get_alloc(body, (size_t)got, "cert_pem");
	key_pem = form_get_alloc(body, (size_t)got, "key_pem");
	if (!cert_pem || !key_pem) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
		goto out;
	}

	if (!wifi_ssid[0]) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "wifi_ssid required");
		goto out;
	}

	bool have_cert = cert_pem[0] != '\0';
	bool have_key = key_pem[0] != '\0';
	if (have_cert != have_key) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "cert and key both required");
		goto out;
	}
	if (have_cert && (!looks_like_pem(cert_pem) || !looks_like_pem(key_pem))) {
		httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "PEM must start with -----BEGIN");
		goto out;
	}

	if (!nvs_config_set_str("wifi_ssid", wifi_ssid) ||
	    !nvs_config_set_str("wifi_pass", wifi_pass) ||
	    !nvs_config_set_str("password", password) ||
	    !nvs_config_set_str("admin_password", admin_password)) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "nvs");
		goto out;
	}

	if (have_cert) {
		/* Include trailing NUL like cert_gen. */
		if (!nvs_config_set_blob("cert_pem", cert_pem, strlen(cert_pem) + 1) ||
		    !nvs_config_set_blob("key_pem", key_pem, strlen(key_pem) + 1)) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "nvs pem");
			goto out;
		}
	}

	ESP_LOGI(TAG, "NVS saved%s; rebooting", have_cert ? " (with PEMs)" : "");
	char *ok = NULL;
	int on = asprintf(&ok,
		"<!DOCTYPE html><html><head><meta charset=utf-8>"
		"<meta name=viewport content=\"width=device-width,initial-scale=1\">"
		"<title>Saved</title><style>%s</style></head><body><main>"
		"<div class=ok><h2>Success</h2><p class=hint>Configuration written to NVS, rebooting...</p></div>"
		"</main></body></html>",
		PAGE_CSS);
	httpd_resp_set_type(req, "text/html");
	if (on > 0 && ok)
		httpd_resp_send(req, ok, on);
	else
		httpd_resp_send(req, "Saved. Rebooting", HTTPD_RESP_USE_STRLEN);
	free(ok);
	xTaskCreate(reboot_task, "reboot", 2048, NULL, 5, NULL);
	fail = ESP_OK;

out:
	free(body);
	free(cert_pem);
	free(key_pem);
	return fail;
}

// Redirect everything to the configuration dialog
// iOS needs a non-empty body, not just a Location header
static esp_err_t http_404_redirect(httpd_req_t *req, httpd_err_code_t err)
{
	(void)err;
	httpd_resp_set_status(req, "302 Temporary Redirect");
	httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
	httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

void config_http_run(const umurmur_nvs_t *cfg)
{
	esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
	esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
	esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
	conf.max_open_sockets = 5;
	conf.lru_purge_enable = true;
	conf.stack_size = 6144;
	conf.server_port = 80;

	httpd_handle_t server = NULL;
	esp_err_t err = httpd_start(&server, &conf);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
		return;
	}

	httpd_uri_t root = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = root_get,
		.user_ctx = (void *)cfg,
	};
	httpd_uri_t save = {
		.uri = "/save",
		.method = HTTP_POST,
		.handler = save_post,
		.user_ctx = NULL,
	};
	httpd_register_uri_handler(server, &root);
	httpd_register_uri_handler(server, &save);
	httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_redirect);

	dns_server_config_t dns = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
	if (!start_dns_server(&dns))
		ESP_LOGW(TAG, "DNS captive redirect failed to start");

	ESP_LOGI(TAG, "Config portal http://192.168.4.1/ (SoftAP %s, captive DNS)",
		 CONFIG_UMURMUR_WIFI_AP_SSID);

	for (;;)
		vTaskDelay(pdMS_TO_TICKS(60000));
}
