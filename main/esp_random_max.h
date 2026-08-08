#ifndef ESP_RANDOM_MAX
#define ESP_RANDOM_MAX

#include "esp_random.h"

// Inclusive of max. The former trailing -1 underflowed to 65535 whenever the
// modulo landed on zero, which reached callers as an array index.
static uint16_t esp_random_max(uint32_t max) {
  return esp_random() % (max + 1);
}

#endif
