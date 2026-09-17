#include "log.h"
#include "umurmur_heap.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "umurmur";

void umurmur_heap_log(const char *phase)
{
	/* free_size is O(1); largest_free_block walks TLSF and can trip IWDT
	 * with several live TLS sessions (seen on Client_free of a 5th reject). */
	ESP_LOGI(TAG, "heap [%s] clients=%d free=%u",
		phase ? phase : "?",
		Client_count(),
		(unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
}

static void vlog(esp_log_level_t level, const char *fmt, va_list ap)
{
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, ap);
	ESP_LOG_LEVEL(level, TAG, "%s", buf);
}

void logthis(const char *logstring, ...)
{
	va_list ap;
	va_start(ap, logstring);
	vlog(ESP_LOG_INFO, logstring, ap);
	va_end(ap);
}

#ifdef DEBUG
void Log_debug(const char *logstring, ...)
{
	va_list ap;
	va_start(ap, logstring);
	vlog(ESP_LOG_DEBUG, logstring, ap);
	va_end(ap);
}
#endif

void Log_warn(const char *logstring, ...)
{
	va_list ap;
	va_start(ap, logstring);
	vlog(ESP_LOG_WARN, logstring, ap);
	va_end(ap);
}

void Log_info(const char *logstring, ...)
{
	va_list ap;
	va_start(ap, logstring);
	vlog(ESP_LOG_INFO, logstring, ap);
	va_end(ap);
}

void Log_info_client(client_t *client, const char *logstring, ...)
{
	va_list ap;
	char buf[256];
	va_start(ap, logstring);
	vsnprintf(buf, sizeof(buf), logstring, ap);
	va_end(ap);
	if (client && client->username)
		ESP_LOGI(TAG, "<%s> %s", client->username, buf);
	else
		ESP_LOGI(TAG, "%s", buf);

	if (strstr(buf, "authenticated"))
		umurmur_heap_log("authed");
}

void Log_fatal(const char *logstring, ...)
{
	va_list ap;
	char buf[256];
	va_start(ap, logstring);
	vsnprintf(buf, sizeof(buf), logstring, ap);
	va_end(ap);
	ESP_LOGE(TAG, "FATAL: %s", buf);
	/* No process exit on FreeRTOS — halt this task loudly. */
	abort();
}

void Log_init(bool_t terminal)
{
	(void)terminal;
}

bool_t Log_preflight(void)
{
	return true;
}

void Log_reset(void)
{
}

void Log_free(void)
{
}
