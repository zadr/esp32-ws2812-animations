#ifndef HUE_TO_RGB
#define HUE_TO_RGB

#include <math.h>
#include "led_strip.h"
#include "esp_log.h"
#include "Constants.h"

// Kept for the one animation that renders white directly and so cannot reach the
// red compensation through the hue path.
static const float RED_SCALE = powf(BRIGHTNESS_SCALE, RED_RESPONSE);

// Saturation and value are pinned at full, which leaves one channel at 255, one
// at 0, and one ramping across each sixth of the wheel.
//
// A sixth of 65536 is not an integer, so the hue is scaled by 6 and the sector
// read off the high bits rather than divided by a truncated sector width. h * 6
// tops out at 393210, below 6 * 65536, so the sector never reaches 6 and the top
// of the wheel stays in the magenta to red sector instead of running off the end.
//
// Channels come out full scale at 65536 rather than 255, which is what the
// position within the sector already measures, so the ramp needs no arithmetic
// of its own. color_utils weighs the dies against each other at that width; the
// renderer brings it down to duty steps itself, where the quantisation is part
// of what was tuned.
static void hue_to_rgb16(uint16_t h, uint32_t &r, uint32_t &g, uint32_t &b) {
    const uint32_t scaled = (uint32_t)h * 6;
    const uint32_t rising = scaled % 65536;
    const uint32_t falling = 65536 - rising;

    switch (scaled / 65536) {
        case 0:  r = 65536;   g = rising;  b = 0;       break;
        case 1:  r = falling; g = 65536;   b = 0;       break;
        case 2:  r = 0;       g = 65536;   b = rising;  break;
        case 3:  r = 0;       g = falling; b = 65536;   break;
        case 4:  r = rising;  g = 0;       b = 65536;   break;
        default: r = 65536;   g = 0;       b = falling; break;
    }
}

// Red's decay across value, at RED_RESPONSE rather than in step with the other
// two dies.
//
// Eight fractional bits rather than a whole duty step, so the compensation still
// resolves between adjacent values where an animation sweeps one. Full output is
// 255 << 8, which is also the scale color_utils weighs the three dies on.
static uint32_t red_level(uint8_t value) {
    return (uint32_t)(255.0f * 256.0f * powf(value / 255.0f, RED_RESPONSE) + 0.5f);
}

// Several animations step in fixed blocks that overrun a strip length not
// divisible by the block size, so the bound is enforced here rather than in
// each of them.
//
// Value is absolute rather than a fraction of the default, so an animation can
// ask for more output than the default as well as less.
//
// The wheel comes to duty steps before value is applied, and both stages
// truncate. That is load-bearing rather than incidental: of the four ways to
// arrange those two roundings it is the only one that reproduces all seven tuned
// hues, and three of them sit a step below what exact arithmetic gives, so no
// single rounding at full width reaches them however it is taken. Violet's red
// is exactly 174.996 and was approved at 174, which leaves the final stage no
// room to be biased either.
//
// Divided by 255 rather than shifted by 8, since a channel at full has to come
// through value unchanged.
static void actual_led_strip_set_pixel_hsv(led_strip_handle_t strip, uint32_t index, uint16_t hue, uint8_t value = VALUE_DEFAULT) {
  if (index >= NUM_PIXELS) return;

  uint32_t r = 0; uint32_t g = 0; uint32_t b = 0;
  hue_to_rgb16(hue, r, g, b);

  r = (r * 255) >> 16;
  g = (g * 255) >> 16;
  b = (b * 255) >> 16;

  led_strip_set_pixel(strip, index,
                      r * red_level(value) / (255 * 256),
                      g * value / 255,
                      b * value / 255);
}

#endif
