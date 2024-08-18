#ifndef ESP_RANDOM_MAX
#define ESP_RANDOM_MAX

#include "esp_random.h"

static uint16_t esp_random_max(uint32_t max) {
  return esp_random() % max;
}

#endif
