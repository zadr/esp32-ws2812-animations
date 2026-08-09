#ifndef HUE_TO_RGB
#define HUE_TO_RGB

#include <math.h>
#include "led_strip.h"
#include "esp_log.h"
#include "Constants.h"

// Kept for the one animation that renders white directly and so cannot reach the
// red compensation through the hue path.
static const float RED_SCALE = sqrtf(BRIGHTNESS_SCALE);

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
// and carries no error into the level below.
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

// Red reads dimmer than green and blue as output falls, so it has to decay more
// slowly than they do. Refitting that decay against each level the palette was
// tuned at gives exponents scattered from 0.33 to 0.92, which does not resolve
// finely enough to justify a tunable, so it is fixed at a square root. That costs
// no pow, no float and no table: sqrt(value / 255) is isqrt(value * 255) / 255,
// the geometric mean of the pixel's level and full output.
//
// The root is taken over the level shifted up by 16, so it comes back with eight
// fractional bits rather than snapped to a whole duty step. The shifted level
// peaks at 65025 << 16, just inside 32 bits, and its root at 255 << 8.
static uint32_t red_level(uint8_t value) {
    uint32_t remainder = ((uint32_t)value * 255) << 16;
    uint32_t root = 0;

    for (uint32_t bit = 1u << 30; bit != 0; bit >>= 2) {
        if (remainder >= root + bit) {
            remainder -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
    }

    return root;
}

// Several animations step in fixed blocks that overrun a strip length not
// divisible by the block size, so the bound is enforced here rather than in
// each of them.
//
// Value is absolute rather than a fraction of the default, so an animation can
// ask for more output than the default as well as less.
//
// Hue, level and root all stay at their full width to here and round to a duty
// step once, at the end. Rounding at each stage instead would floor three times
// over, and the three floors are independent rather than offsetting. The red
// product peaks at 65536 * (255 << 8), which is inside 32 bits with room for the
// rounding term.
static void actual_led_strip_set_pixel_hsv(led_strip_handle_t strip, uint32_t index, uint16_t hue, uint8_t value = VALUE_DEFAULT) {
  if (index >= NUM_PIXELS) return;

  uint32_t r = 0; uint32_t g = 0; uint32_t b = 0;
  hue_to_rgb16(hue, r, g, b);

  const uint32_t red = red_level(value);

  led_strip_set_pixel(strip, index,
                      (r * red + (1u << 23)) >> 24,
                      (g * value + (1u << 15)) >> 16,
                      (b * value + (1u << 15)) >> 16);
}

#endif
