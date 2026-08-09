#ifndef FILLIN_HPP
#define FILLIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "color_utils.hpp"
#include "esp_random.h"

class FillIn : public Animation {
public:
  FillIn(led_strip_handle_t& strip, bool forward)
    : Animation(strip), forward(forward), seed(0) {
  }
  ~FillIn() {}

  void setup() override { seed = esp_random(); }

  int duration() override { return NUM_PIXELS * BANDS * MS_PER_PIXEL; }

  // Step n lays the nth pixel of band n / NUM_PIXELS over what the band before
  // left there, so the strip is two runs: the part this band has reached, and
  // the part still standing from its predecessor. That is the whole lit set at
  // n, which is why nothing has to accumulate in the buffer to arrive at it.
  void render(uint16_t t) override {
    const int n = stepsAt(t, NUM_PIXELS * BANDS - 1);
    const int band = n / NUM_PIXELS;
    const int reached = n % NUM_PIXELS;

    const uint16_t filling = bandHue(band);
    for (int i = 0; i <= reached; i++) {
      actual_led_strip_set_pixel_hsv(strip, at(i), filling);
    }

    // The first band paints onto a dark strip, so ahead of it there is nothing
    // to stand rather than a predecessor's colour.
    if (band > 0) {
      const uint16_t standing = bandHue(band - 1);
      for (int i = reached + 1; i < NUM_PIXELS; i++) {
        actual_led_strip_set_pixel_hsv(strip, at(i), standing);
      }
    }
  }

  int tag() override { return 1005; }

private:
  static const int BANDS = 7;
  static const int MS_PER_PIXEL = 10;

  static constexpr uint16_t ANCHORS[BANDS] = {
    HUE_RED, HUE_ORANGE, HUE_YELLOW, HUE_GREEN, HUE_BLUE, HUE_INDIGO, HUE_VIOLET,
  };

  // Distance from the end the fill starts at, so a backward run is the same
  // arithmetic read off the other end of the strip.
  int at(int distance) const { return forward ? distance : NUM_PIXELS - 1 - distance; }

  // Drift is hashed from the band rather than drawn when the band begins, so a
  // render that lands mid palette knows the colour of a band it never walked
  // through. A backward run takes the palette from the far end.
  uint16_t bandHue(int band) const {
    const uint32_t index = forward ? band : BANDS - 1 - band;
    return driftBy(ANCHORS[index], noiseMax(seed, index, 0, 30), noise(seed, index, 1));
  }

  const bool forward;
  uint32_t seed;
};

#endif
