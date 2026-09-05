#include "umurmur_task.h"

#include <stdio.h>
#include <string.h>

#include "ban.h"
#include "channel.h"
#include "client.h"
#include "conf.h"
#include "config.h"
#include "esp_log.h"
#include "log.h"
#include "server.h"
#include "ssl.h"

/* Globals expected by server.c / client.c (normally defined in main.c). */
char *bindaddr;
char *bindaddr6;
int bindport;
int bindport6;
char system_string[64];
char version_string[64];

static const char *TAG = "umurmur_task";

void umurmur_task(void *arg)
{
	(void)arg;

	bindaddr = NULL;
	bindaddr6 = NULL;
	bindport = 0;
	bindport6 = 0;
	snprintf(system_string, sizeof(system_string), "ESP32 FreeRTOS");
	snprintf(version_string, sizeof(version_string), "%s", UMURMUR_VERSION);

	Log_init(true);
	Conf_init(NULL);

	ESP_LOGI(TAG, "Initializing SSL / channels / clients");
	SSLi_init();
	Chan_init();
	Client_init();
	Ban_init();

	ESP_LOGI(TAG, "Entering Server_run()");
	Server_run();

	Ban_deinit();
	SSLi_deinit();
	Chan_free();
	Log_free();
	Conf_deinit();

	ESP_LOGW(TAG, "Server_run returned; deleting task");
	vTaskDelete(NULL);
}
