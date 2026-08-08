#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PIN_WS2812B 0 // D0 on the XIAO ESP32C6
#define NUM_PIXELS 50
#define HUE_MAX 65535

#define BRIGHTNESS_SCALE 0.66

// Red reads dimmer than green and blue as overall output falls, so scaling all
// three together pulls the hue out of every mix that leans on red. Exponent
// below 1 decays red more slowly. Full output is a fixed point, so the palette
// is tuned there and this only governs the way down.
#define RED_RESPONSE 0.54

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

// Complements fold into the tuned span rather than around the full circle,
// since nothing above violet has been measured on this strip. Violet is the top
// of that span, so the width follows the palette and does not need revisiting
// when a hue is retuned. Even width, so a complement of a complement is exact.
#define HUE_SPAN (HUE_VIOLET + 1)
#define COMPLEMENT(hue) (((hue) + HUE_SPAN / 2) % HUE_SPAN)
#define delay(time) vTaskDelay(time / portTICK_PERIOD_MS);

#endif
