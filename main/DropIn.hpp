#ifndef DROPIN_HPP
#define DROPIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "esp_random_max.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "freertos/task.h"

class DropIn : public Animation {
public:
  DropIn(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), currentStep(0), currentTarget(0), hueIndex(0), forward(forward) {
  }
  ~DropIn() {}

  void setup() {
    hueIndex = forward ? 0 : 6;
  }

  int steps() {
    return 7;
  }

  void loop() {
    int hueToDrop = 0;
    switch (hueIndex) {
    case 0:
      hueToDrop = drift(HUE_RED, esp_random_max(30));
      break;
    case 1:
      hueToDrop = drift(HUE_ORANGE, esp_random_max(30));
      break;
    case 2:
      hueToDrop = drift(HUE_YELLOW, esp_random_max(30));
      break;
    case 3:
      hueToDrop = drift(HUE_GREEN, esp_random_max(30));
      break;
    case 4:
      hueToDrop = drift(HUE_BLUE, esp_random_max(30));
      break;
    case 5:
      hueToDrop = drift(HUE_INDIGO, esp_random_max(30));
      break;
    case 6:
      hueToDrop = drift(HUE_VIOLET, esp_random_max(30));
      break;
    }

    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
        led_strip_clear(strip);
        for (int16_t j = NUM_PIXELS - 1; j >= i; j -= 4) {
            actual_led_strip_set_pixel_hsv(strip, j, hueToDrop);

            for (int16_t k = 0; k < 4; k++) {
                if (j - k >= 0) {
                    actual_led_strip_set_pixel_hsv(strip, j - k, hueToDrop);
                }
            }

            for (uint16_t m = 0; m < i; m++) {
                actual_led_strip_set_pixel_hsv(strip, m, hueToDrop);
            }

            led_strip_refresh(strip);
            delay(getDelay());
        }
    }
    hueIndex += forward ? 1 : -1;
  }

  int getDelay() {
    return 25;
  }

private:
    int currentStep;
    int currentTarget;
    int hueIndex;
    bool forward;
};

#endif
