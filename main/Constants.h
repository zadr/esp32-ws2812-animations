#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PIN_WS2812B 12
#define NUM_PIXELS 144
#define HUE_MAX 65535

#define HUE_RED 0
#define HUE_ORANGE 1967 // 3% of 65535
#define HUE_YELLOW 8847 // 13.5% of 65535
#define HUE_GREEN 21845 // 33.333% of 65535
#define HUE_BLUE 42270 // 64.5% of 65535
#define HUE_INDIGO 45711 // 69.75% of 65535
#define HUE_VIOLET 49151 // 75% of 65535

// constrain complementary colors to the range of hues we have available
#define COMPLEMENT(HUE) (HUE + ((HUE_VIOLET - HUE_RED) / 2)) % (HUE_VIOLET - HUE_RED)
#define DRIFT(HUE, PERCENT) (HUE + ((HUE_VIOLET - HUE_RED) * ((PERCENT / 100) * 360)))
#define RANDOM_PERCENT(MAX) esp_random_max(MAX) * ((esp_random_max(100) % 2 == 0) ? 1 : -1)
#define delay(time) vTaskDelay(time / portTICK_PERIOD_MS);

#endif
