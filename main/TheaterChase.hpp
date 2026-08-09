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
    : Animation(strip), spacing(spacing < 2 ? 2 : spacing), forward(forward), litHue(0), alternateHue(0) {
  }
  ~TheaterChase() {}

  void setup() override {
    litHue = esp_random_max(HUE_VIOLET);
    alternateHue = COMPLEMENT(litHue);
  }

  // Six passes along the strip at the speed a dot reads as running rather than
  // sliding. Speed is stated as the time a dot takes to cross a pixel, which is
  // what was tuned; the frames that fall inside it are the driver's affair.
  int duration() override { return NUM_PIXELS * PASSES * MS_PER_PIXEL; }

  // Where the dots sit follows from t alone, so there is no counter and no state
  // between renders. Two renders at the same t draw the same strip.
  void render(uint16_t t) override {
    const uint32_t travelled = ((uint32_t)t * (NUM_PIXELS * PASSES)) / 65535;
    const int within = (int)(travelled % (uint32_t)cycle());

    // Dots sit at i + phase, so the phase runs down to carry them up the strip.
    const int phase = forward ? (cycle() - within) % cycle() : within;

    // Unlit gaps are the whole trick. Every pixel here is at the same output, so
    // filling the gaps with a contrasting hue would put the strip at full width
    // and leave the motion to read as a hue shimmer rather than running lights.
    led_strip_clear(strip);

    for (int i = 0; i < NUM_PIXELS; i++) {
      const int position = i + phase;
      if (position % spacing != 0) {
        continue;
      }

      // Every dot is otherwise identical, and at this spacing a step of one is
      // indistinguishable from a step of spacing - 1 the other way. Alternating
      // the two hues doubles the repeat, which fixes which way the strip is
      // travelling.
      actual_led_strip_set_pixel_hsv(strip, i, (position / spacing) % 2 == 0 ? litHue : alternateHue);
    }
  }

  int tag() override { return 1015; }

private:
  static const int PASSES = 6;
  static const int MS_PER_PIXEL = 60;

  // Two hues, so the pattern only comes back around after twice the spacing.
  // Wrapping the phase at the spacing alone would flip every dot at the seam.
  int cycle() const { return spacing * 2; }

  int spacing;
  bool forward;
  uint16_t litHue;
  uint16_t alternateHue;
};

#endif
