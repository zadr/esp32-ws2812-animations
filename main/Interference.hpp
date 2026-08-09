#ifndef INTERFERENCE_HPP
#define INTERFERENCE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random_max.h"
#include <math.h>
#include <stdint.h>

// Phase is a 16 bit angle, so a wavelength converts to phase per pixel by
// division and both accumulators wrap for free. The two wavelengths are 20.0 and
// 12.4 pixels, a ratio of 1.618: near enough to irrational that the beat does
// not land back on itself while anyone is watching, and both coarse enough that
// 50 pixels still resolves the shorter one at six pixels per half cycle.
static const uint16_t INTERFERENCE_PIXEL_STEP_LONG = 3277;
static const uint16_t INTERFERENCE_PIXEL_STEP_SHORT = 5303;

// Deliberately not in the ratio of the wavelengths. Matching ratios give both
// waves the same phase velocity and the whole figure, envelope included, slides
// along rigidly. These have the carriers running down the strip at 8 and 3.3
// pixels per second while the envelope crawls up it at 4.4.
static const uint16_t INTERFERENCE_FRAME_STEP_LONG = 1049;
static const uint16_t INTERFERENCE_FRAME_STEP_SHORT = 691;

class Interference : public Animation {
public:
  Interference(led_strip_handle_t& strip)
    : Animation(strip), phaseLong(0), phaseShort(0) {
  }
  ~Interference() {}

  void setup() override {
    phaseLong = esp_random_max(HUE_MAX);
    phaseShort = esp_random_max(HUE_MAX);
  }

  // The envelope takes about 280 frames to cross the strip, so this is a little
  // under two passes of it.
  int steps() override {
    return 500;
  }

  void loop() override {
    // Two hue angles cannot be added: both wrap, and their sum wraps twice as
    // often, which puts a hard seam wherever either one rolls over and reads as
    // noise. The waves are summed as amplitudes instead and the result mapped
    // across the palette afterwards, so the only discontinuity is the one at the
    // ends of the span, which nothing reaches.
    //
    // Amplitude then has nowhere to go but hue, since every pixel is at the same
    // output. Spending the whole tuned span on it is what makes the beat legible:
    // the antinodes sweep red to violet, the nodes cancel to a flat green blue,
    // and a narrower window would leave the two indistinguishable at this length.
    const float huePerUnit = HUE_VIOLET / 4.0f; // the sum runs -2 to 2

    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      float sum = wave((uint16_t)(INTERFERENCE_PIXEL_STEP_LONG * i + phaseLong))
                + wave((uint16_t)(INTERFERENCE_PIXEL_STEP_SHORT * i + phaseShort));

      actual_led_strip_set_pixel_hsv(strip, i, (uint16_t)((sum + 2.0f) * huePerUnit));
    }

    phaseLong += INTERFERENCE_FRAME_STEP_LONG;
    phaseShort += INTERFERENCE_FRAME_STEP_SHORT;
  }

  int getDelay() override {
    return 40;
  }

  int minIterations() override { return 1; }
  int maxIterations() override { return 2; }
  int tag() override { return 1016; }

private:
  static float wave(uint16_t phase) {
    return sinf(phase * (6.28318531f / 65536.0f));
  }

  uint16_t phaseLong;
  uint16_t phaseShort;
};

#endif
