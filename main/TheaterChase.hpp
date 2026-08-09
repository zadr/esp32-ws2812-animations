#ifndef THEATERCHASE_HPP
#define THEATERCHASE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random_max.h"
#include "led_strip.h"
#include <stdint.h>

class TheaterChase : public Animation {
public:
  TheaterChase(led_strip_handle_t& strip, int spacing, bool forward)
    : Animation(strip), spacing(spacing < 2 ? 2 : spacing), forward(forward), phase(0), litHue(0), alternateHue(0) {
  }
  ~TheaterChase() {}

  void setup() override {
    phase = 0;
    litHue = esp_random_max(HUE_VIOLET);
    alternateHue = COMPLEMENT(litHue);
  }

  // A dot moves one pixel per frame, so this is six passes along the strip.
  int steps() override {
    return NUM_PIXELS * 6;
  }

  void loop() override {
    // Unlit gaps are the whole trick. Every pixel here is at the same output, so
    // filling the gaps with a contrasting hue would put the strip at full width
    // and leave the motion to read as a hue shimmer rather than running lights.
    led_strip_clear(strip);

    for (int i = 0; i < NUM_PIXELS; i++) {
      int position = i + phase;
      if (position % spacing != 0) {
        continue;
      }

      // Every dot is otherwise identical, and at this spacing a step of one is
      // indistinguishable from a step of spacing - 1 the other way. Alternating
      // the two hues doubles the repeat, which fixes which way the strip is
      // travelling.
      actual_led_strip_set_pixel_hsv(strip, i, (position / spacing) % 2 == 0 ? litHue : alternateHue);
    }

    // Dots sit at i + phase, so the phase counts down to carry them up the strip.
    phase = (phase + (forward ? cycle() - 1 : 1)) % cycle();
  }

  int getDelay() override {
    return 60;
  }

  int minIterations() override { return 2; }
  int maxIterations() override { return 4; }
  int tag() override { return 1015; }

private:
  // Two hues, so the pattern only comes back around after twice the spacing.
  // Wrapping the phase at the spacing alone would flip every dot at the seam.
  int cycle() const { return spacing * 2; }

  int spacing;
  bool forward;
  int phase;
  uint16_t litHue;
  uint16_t alternateHue;
};

#endif
