#ifndef HUE_TO_RGB
#define HUE_TO_RGB_MAX

#include "led_strip.h"
#include "esp_log.h"

static void hue_to_rgb(uint16_t h, uint32_t &r, uint32_t &g, uint32_t &b) {
    float p, q, t, ff;
    int i;
    float hue = (h / 65535.0f) * 360.0f; // Normalize hue to range [0, 360]
    float saturation = 1.0f; // Full saturation (255)
    float value = 1.0f; // Full value (255)

    hue /= 60.0f; // Sector 0 to 5
    i = (int)hue; // Integer part of hue
    ff = hue - i; // Fractional part of hue
    p = value * (1.0f - saturation);
    q = value * (1.0f - (saturation * ff));
    t = value * (1.0f - (saturation * (1.0f - ff)));

    if (i == 0) {
        r = value * 255.0f;
        g = t * 255.0f;
        b = p * 255.0f;
    } else if (i == 1) {
        r = q * 255.0f;
        g = value * 255.0f;
        b = p * 255.0f;
    } else if (i == 2) {
        r = p * 255.0f;
        g = value * 255.0f;
        b = t * 255.0f;
    } else if (i == 3) {
        r = p * 255.0f;
        g = q * 255.0f;
        b = value * 255.0f;
    } else if (i == 4) {
        r = t * 255.0f;
        g = p * 255.0f;
        b = value * 255.0f;
    } else {
        r = value * 255.0f;
        g = p * 255.0f;
        b = q * 255.0f;
    }
}

static void actual_led_strip_set_pixel_hsv(led_strip_handle_t strip, uint32_t index, uint16_t hue) {
  uint32_t r = 0; uint32_t g = 0; uint32_t b = 0;
  hue_to_rgb(hue, r, g, b);
  led_strip_set_pixel(strip, index, r, g, b);
}

#endif
