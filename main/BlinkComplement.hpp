#ifndef Blink_HPP
#define Blink_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "esp_random_max.h"

class BlinkComplement : public Animation {
public:
  BlinkComplement(led_strip_handle_t& strip, bool zeroIsPrimaryHue, bool changeHueEachLoop, bool fullRandom)
    : Animation(strip), zeroIsPrimaryHue(true), changeHueEachLoop(changeHueEachLoop), fullRandom(fullRandom) {
  }
  ~BlinkComplement() {}

  void setup() {
    if (fullRandom) {
      primaryHue = esp_random_max(HUE_VIOLET);
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
  }

  int steps() {
    return NUM_PIXELS * 3;
  }

  void loop() {
    if (changeHueEachLoop) {
      setup();
    }

    uint16_t primaryColor = zeroIsPrimaryHue ? primaryHue : drift(primaryHue, 90);
    uint16_t secondaryColor = zeroIsPrimaryHue ?  drift(primaryHue, 90) : primaryHue;
    for (int i = 0; i <= 4; i++) {
      for (int i = 0; i < NUM_PIXELS; i += 8) {
        for (int j = i; j <= i + 4; j++) {
          led_strip_set_pixel_hsv(strip, j, secondaryColor, 255, 255);
        }
        for (int j = i + 4; j <= i + 8; j++) {
          led_strip_set_pixel_hsv(strip, j, primaryColor, 255, 255);
        }
      }
      uint16_t temp = primaryColor;
      primaryColor = secondaryColor;
      secondaryColor = temp;
      led_strip_refresh(strip);
      delay(333);
    }

    zeroIsPrimaryHue = !zeroIsPrimaryHue;
  }

  int getDelay() {
    return 25;
  }

  int tag() override { return 1000; }
  int minIterations() override { return 1; }
  int maxIterations() override { return 2; }

private:
  bool zeroIsPrimaryHue;
  bool changeHueEachLoop;
  bool fullRandom;
  uint16_t primaryHue;
};

#endif
