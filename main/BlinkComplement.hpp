#ifndef Blink_HPP
#define Blink_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "color_utils.hpp"
#include "esp_random.h"
#include "esp_random_max.h"
#include "actual_led_strip_set_pixel_hsv.h"

class BlinkComplement : public Animation {
public:
  BlinkComplement(Frame& strip, bool fullRandom, bool evolves)
    : Animation(strip), fullRandom(fullRandom), evolves(evolves), primaryHue(0), secondaryHue(0), seed(0) {
  }
  ~BlinkComplement() {}

  void setup() override {
    primaryHue = fullRandom ? esp_random_max(HUE_VIOLET) : PALETTE[esp_random_max(PALETTE_SIZE - 1)];
    secondaryHue = drift(COMPLEMENT(primaryHue), 10);
    seed = esp_random();
  }

  int duration() override { return RUN_MS; }

  // The swap alone would be the parity of the blink count, but where a drift has
  // got to is reachable only through the drifts before it, so both variants walk.
  void render(uint16_t t) override {
    Hues hues = opening();

    const int blinks = stepsAt(t, BLINKS);
    for (int blink = 0; blink < blinks; blink++) {
      advance(hues, blink);
    }

    draw(hues);
  }

  int tag() override { return 1000; }

private:
  // Two hues trading places is one idea, and the evolving variants are that idea
  // going somewhere. Twelve seconds is thirty swaps, which is enough of a walk
  // to have left the complement it started as behind.
  static const int RUN_MS = 12000;

  // A dwell rather than a frame interval: long enough that each pair of hues is
  // read before it swaps. Taken against the cruise, which the trapezoid holds a
  // seventh above the mean rate.
  static const int MS_PER_BLINK = 333;
  static const int BLINKS = RUN_MS * 6 / (MS_PER_BLINK * 7);

  static constexpr uint16_t PALETTE[] = {HUE_RED, HUE_ORANGE, HUE_YELLOW, HUE_GREEN, HUE_BLUE, HUE_INDIGO, HUE_VIOLET};
  static constexpr int PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);

  // Which hue drifts and which way it drifts are two draws, so they take two
  // lanes: one lane read twice would tie the direction to the choice.
  static constexpr uint32_t LANE_WHICH = 0;
  static constexpr uint32_t LANE_DIRECTION = 1;

  // Where the two hues have got to, held as a local of render() so that walking
  // to one blink leaves nothing behind for the next call to find.
  struct Hues {
    uint16_t primary;
    uint16_t secondary;
  };

  Hues opening() const {
    return Hues{primaryHue, secondaryHue};
  }

  void advance(Hues& hues, int blink) const {
    // the exchange is the blink: each step lands the opposite hue on each block
    const uint16_t held = hues.primary;
    hues.primary = hues.secondary;
    hues.secondary = held;

    if (!evolves) {
      return;
    }

    // One hue at a time, so the pair walks away from being complements rather
    // than sliding around the wheel together.
    const uint32_t direction = noise(seed, blink, LANE_DIRECTION);
    if (noise(seed, blink, LANE_WHICH) % 2 == 0) {
      hues.primary = driftBy(hues.primary, 3, direction);
    } else {
      hues.secondary = driftBy(hues.secondary, 3, direction);
    }
  }

  void draw(const Hues& hues) const {
    // Nominally 4 secondary then 4 primary, but the count of alternations is
    // taken from the strip and the bands stretch to fill it, so a strip that 8
    // does not divide gets slightly wider bands rather than a short one at the
    // end. Whole pairs keep the two hues on equal footing across the swap.
    const int pairs = NUM_PIXELS / 8 > 0 ? NUM_PIXELS / 8 : 1;
    const int bands = pairs * 2;

    for (int band = 0; band < bands; band++) {
      const int begin = (band * NUM_PIXELS) / bands;
      const int end = ((band + 1) * NUM_PIXELS) / bands;
      const uint16_t hue = band % 2 == 0 ? hues.secondary : hues.primary;

      for (int j = begin; j < end; j++) {
        actual_led_strip_set_pixel_hsv(strip, j, hue);
      }
    }
  }

  const bool fullRandom;
  const bool evolves;
  uint16_t primaryHue;
  uint16_t secondaryHue;
  uint32_t seed;
};

#endif
