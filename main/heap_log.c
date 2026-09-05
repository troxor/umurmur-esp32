#include "heap_log.h"

#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "heap";

void heap_log_snapshot(const char *tag)
{
	size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	size_t min_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
	size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
	ESP_LOGI(TAG, "[%s] free=%u min_free=%u largest_block=%u",
		tag ? tag : "?",
		(unsigned)free_heap,
		(unsigned)min_heap,
		(unsigned)largest);
}
