#ifndef Blinkvolution_HPP
#define Blinkvolution_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "esp_random_max.h"

class Blinkvolution : public Animation {
public:
  Blinkvolution(led_strip_handle_t& ws2812b) : Animation(ws2812b),
      alternatedPrimaryColorCounter(0),
      alternatedSecondaryColorCounter(0)
  {}
  ~Blinkvolution() {}

  void setup() {
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
    secondaryHue = COMPLEMENT(primaryHue);
  }

  int steps() {
    return NUM_PIXELS;
  }

  void loop() {
    for (int i = 0; i < NUM_PIXELS; i += 8) {
      for (int j = i; j <= i + 4; j++) {
        led_strip_set_pixel_hsv(strip, j, secondaryHue, 255, 255);
      }
      for (int j = i + 4; j <= i + 8; j++) {
        led_strip_set_pixel_hsv(strip, j, primaryHue, 255, 255);
      }
    }

    bool shouldAlternatePrimaryHue = false;
    if (alternatedPrimaryColorCounter > 0) {
      shouldAlternatePrimaryHue = esp_random_max(100) > (50 - (alternatedPrimaryColorCounter * 2));
    } else  if (alternatedSecondaryColorCounter > 0) {
      shouldAlternatePrimaryHue = esp_random_max(100) > (50 + (alternatedSecondaryColorCounter * 2));
    } else {
      shouldAlternatePrimaryHue = esp_random_max(100) % 2 == 0;
    }

    if (shouldAlternatePrimaryHue) {
      primaryHue = DRIFT(primaryHue, RANDOM_PERCENT(120));
      alternatedPrimaryColorCounter++;
      alternatedSecondaryColorCounter = 0;
    } else {
      secondaryHue = DRIFT(secondaryHue, RANDOM_PERCENT(120));
      alternatedPrimaryColorCounter = 0;
      alternatedSecondaryColorCounter++;
    }

    // swap values
    primaryHue = primaryHue ^ secondaryHue;
    secondaryHue = primaryHue ^ secondaryHue;
    primaryHue = primaryHue ^ secondaryHue;

    led_strip_refresh(strip);
    vTaskDelay(450 / portTICK_PERIOD_MS);
  }

private:
  uint16_t primaryHue;
  uint16_t secondaryHue;
  int alternatedPrimaryColorCounter;
  int alternatedSecondaryColorCounter;
};

#endif
