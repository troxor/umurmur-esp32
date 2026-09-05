#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "umurmur";

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

	/* Heap breadcrumb on auth / disconnect without coupling to main. */
	if (strstr(buf, "authenticated") || strstr(buf, "Timeout") ||
	    strstr(buf, "Closing connection") || strstr(buf, "Connection closed")) {
		ESP_LOGI(TAG, "heap free=%u min=%u",
			(unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
			(unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
	}
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
