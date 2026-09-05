#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/** FreeRTOS task entry: init SSL/Chan/Client/Ban then Server_run(). */
void umurmur_task(void *arg);

#ifdef __cplusplus
}
#endif
