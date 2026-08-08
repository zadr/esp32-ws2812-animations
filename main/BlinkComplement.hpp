#ifndef Blink_HPP
#define Blink_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "color_utils.hpp"
#include "esp_log.h"
#include "esp_random_max.h"
#include "actual_led_strip_set_pixel_hsv.h"

class BlinkComplement : public Animation {
public:
  BlinkComplement(led_strip_handle_t& strip, bool fullRandom, bool evolves)
    : Animation(strip), fullRandom(fullRandom), primaryHue(0), secondaryHue(0), evolves(evolves) {
  }
  ~BlinkComplement() {}

  void setup() {
    if (fullRandom) {
      primaryHue = esp_random_max(HUE_VIOLET);
      secondaryHue = drift(COMPLEMENT(primaryHue), 10);
      return;
    }

    switch (esp_random_max(6)) {
    case 0:
      primaryHue = HUE_RED;
      break;
    case 1:
      primaryHue = HUE_ORANGE;
      break;
    case 2:
      primaryHue = HUE_YELLOW;
      break;
    case 3:
      primaryHue = HUE_GREEN;
      break;
    case 4:
      primaryHue = HUE_BLUE;
      break;
    case 5:
      primaryHue = HUE_INDIGO;
      break;
    case 6:
      primaryHue = HUE_VIOLET;
      break;
    }

    primaryHue = primaryHue;
    secondaryHue = drift(COMPLEMENT(primaryHue), 10);
  }

  int steps() {
    return 24;
  }

  void loop() {
    // Nominally 4 secondary then 4 primary, but the count of alternations is
    // taken from the strip and the bands stretch to fill it, so a strip that 8
    // does not divide gets slightly wider bands rather than a short one at the
    // end. Whole pairs keep the two hues on equal footing across the swap.
    const int pairs = NUM_PIXELS / 8 > 0 ? NUM_PIXELS / 8 : 1;
    const int bands = pairs * 2;

    for (int band = 0; band < bands; band++) {
      const int begin = (band * NUM_PIXELS) / bands;
      const int end = ((band + 1) * NUM_PIXELS) / bands;
      const uint16_t hue = band % 2 == 0 ? secondaryHue : primaryHue;

      for (int j = begin; j < end; j++) {
        actual_led_strip_set_pixel_hsv(strip, j, hue);
      }
    }

    // the exchange is the blink: each frame lands the opposite hue on each block
    uint16_t temp = primaryHue;
    primaryHue = secondaryHue;
    secondaryHue = temp;

    zeroIsPrimaryHue = !zeroIsPrimaryHue;

    if (evolves) {
      if (esp_random() % 2 == 0) {
        int was = primaryHue;
        primaryHue = drift(primaryHue, 3);
      } else {
        int was = secondaryHue;
        secondaryHue = drift(secondaryHue, 3);
      }
    }
  }

  int getDelay() {
    return 333;
  }

  int tag() override { return 1000; }
  int minIterations() override { return 2; }
  int maxIterations() override { return 3; }

private:
  bool zeroIsPrimaryHue;
  bool fullRandom;
  uint16_t primaryHue;
  uint16_t secondaryHue;
  bool evolves;
};

#endif
