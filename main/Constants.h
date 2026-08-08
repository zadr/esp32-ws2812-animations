#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PIN_WS2812B 0 // D0 on the XIAO ESP32C6
#define NUM_PIXELS 50
#define HUE_MAX 65535

// The palette above was tuned by eye at this level. WS2812B dies do not dim
// evenly, so lowering it shifts perceived hue rather than just output.
#define BRIGHTNESS_SCALE 0.33

// Spaced by eye, not by angle. WS2812B primaries do not sit where the math says
// and the green die dominates, so the value that reads as orange is well below
// the arithmetic 30 degrees.
#define HUE_RED 0
#define HUE_ORANGE 1967 // 3% of 65535
#define HUE_YELLOW 8847 // 13.5% of 65535
#define HUE_GREEN 21845 // 33.333% of 65535
#define HUE_BLUE 42270 // 64.5% of 65535
#define HUE_INDIGO 45711 // 69.75% of 65535
#define HUE_VIOLET 49151 // 75% of 65535

// constrain complementary colors to the range of hues we have available
#define COMPLEMENT(hue) (((hue) + 24576) % 49152)
#define delay(time) vTaskDelay(time / portTICK_PERIOD_MS);

#endif
