#ifndef HUE_TO_RGB
#define HUE_TO_RGB

#include <math.h>
#include "led_strip.h"
#include "esp_log.h"
#include "Constants.h"

static const float RED_SCALE = powf(BRIGHTNESS_SCALE, RED_RESPONSE);

// Saturation and value are pinned at full, which leaves one channel at 255, one
// at 0, and one ramping across each sixth of the wheel.
//
// A sixth of 65536 is not an integer, so the hue is scaled by 6 and the sector
// read off the high bits rather than divided by a truncated sector width. h * 6
// tops out at 393210, below 6 * 65536, so the sector never reaches 6 and the top
// of the wheel stays in the magenta to red sector instead of running off the end.
static void hue_to_rgb(uint16_t h, uint32_t &r, uint32_t &g, uint32_t &b) {
    const uint32_t scaled = (uint32_t)h * 6;
    const uint32_t offset = scaled % 65536; // 16 bit fraction into the sector
    const uint32_t rising = (offset * 255) / 65536;
    const uint32_t falling = ((65536 - offset) * 255) / 65536;

    switch (scaled / 65536) {
        case 0:  r = 255;     g = rising;  b = 0;       break;
        case 1:  r = falling; g = 255;     b = 0;       break;
        case 2:  r = 0;       g = 255;     b = rising;  break;
        case 3:  r = 0;       g = falling; b = 255;     break;
        case 4:  r = rising;  g = 0;       b = 255;     break;
        default: r = 255;     g = 0;       b = falling; break;
    }
}

// Several animations step in fixed blocks that overrun a strip length not
// divisible by the block size, so the bound is enforced here rather than in
// each of them.
static void actual_led_strip_set_pixel_hsv(led_strip_handle_t strip, uint32_t index, uint16_t hue) {
  if (index >= NUM_PIXELS) return;

  uint32_t r = 0; uint32_t g = 0; uint32_t b = 0;
  hue_to_rgb(hue, r, g, b);
  led_strip_set_pixel(strip, index, (r * RED_SCALE), (g * BRIGHTNESS_SCALE), (b * BRIGHTNESS_SCALE));
}

#endif
