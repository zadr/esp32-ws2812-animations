#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PIN_WS2812B 0 // D0 on the XIAO ESP32C6
#define NUM_PIXELS 50
#define HUE_MAX 65535

// WS2812B dies do not dim evenly, so this shifts perceived hue rather than just
// output. The palette below was judged at full brightness.
#define BRIGHTNESS_SCALE 0.33

// Spaced by eye against this strip, not by angle; the trailing figure is where
// the arithmetic would have put each. Green is the strong die, so yellow lands
// early and blue wants no green at all mixed in. Red is weak against blue,
// which is why violet sits most of the way to magenta.
#define HUE_RED 0     // 0deg
#define HUE_ORANGE 1967  // 10.8deg, nominal 30
#define HUE_YELLOW 6227  // 34.2deg, nominal 60
#define HUE_GREEN 21911  // 120.4deg, nominal 120
#define HUE_BLUE 43580   // 239.4deg, nominal 240
#define HUE_INDIGO 45056 // 247.5deg, nominal 260
#define HUE_VIOLET 53081 // 291.6deg, nominal 280

// constrain complementary colors to the range of hues we have available
#define COMPLEMENT(hue) (((hue) + 24576) % 49152)
#define delay(time) vTaskDelay(time / portTICK_PERIOD_MS);

#endif
