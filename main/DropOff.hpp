#ifndef DROP_OFF_HPP
#define DROP_OFF_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "color_utils.hpp"
#include "esp_random_max.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "freertos/task.h"

class DropOff : public Animation {
public:
  DropOff(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), currentStep(0), currentTarget(0), hueIndex(0), forward(forward) {
  }
  ~DropOff() {}

  void setup() {
    hueIndex = forward ? 0 : 6;
  }

  int steps() {
    return 7;
  }

  void loop() {
    led_strip_clear(strip);

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
      int16_t j = NUM_PIXELS - 1 - i;
      if (j % 4 == 0) {
        led_strip_clear(strip);

        // Light the current dropping LED
        actual_led_strip_set_pixel_hsv(strip, j, hueToDrop);

        // Keep fully dropped LEDs on
        for (int16_t k = 0; k < 4; k++) {
          if (j - k >= 0) {
            actual_led_strip_set_pixel_hsv(strip, j - k, hueToDrop);
          }
        }

        led_strip_refresh(strip);
        delay(getDelay()); // Wait before moving to the next LED
      }
    }

    hueIndex += forward ? 1 : -1;
  }

  int getDelay() {
    return 50;
  }

  int minIterations() override { return 1; }
  int maxIterations() override { return 1; }
  int tag() override { return 1004; }

private:
    int currentStep;
    int currentTarget;
    int hueIndex;
    bool forward;
};

#endif
