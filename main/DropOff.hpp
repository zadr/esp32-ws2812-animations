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
    : Animation(ws2812b), currentStep(0), currentTarget(0), hueIndex(0), forward(forward),
      falling(0), activeHue(0) {
  }
  ~DropOff() {}

  void setup() {
    hueIndex = forward ? 0 : 6;
    // Start on the top pixel so the far end is covered by the first chunk.
    falling = NUM_PIXELS - 1;
    activeHue = pickHue();
  }

  int steps() {
    return ((NUM_PIXELS - 1) / chunk() + 1) * 7;
  }

  void loop() {
    led_strip_clear(strip);

    // Light the current dropping LED
    actual_led_strip_set_pixel_hsv(strip, falling, activeHue);

    // Keep fully dropped LEDs on
    for (int16_t k = 0; k < chunk(); k++) {
      if (falling - k >= 0) {
        actual_led_strip_set_pixel_hsv(strip, falling - k, activeHue);
      }
    }

    falling -= chunk();
    if (falling < 0) {
      falling = NUM_PIXELS - 1;
      hueIndex += forward ? 1 : -1;
      activeHue = pickHue();
    }
  }

  int getDelay() {
    return 50;
  }

  int minIterations() override { return 1; }
  int maxIterations() override { return 1; }
  int tag() override { return 1004; }

private:
    // Chunk scales with strip length but never reaches zero, which on a short
    // strip would make the descent stop advancing.
    int chunk() {
      int amount = NUM_PIXELS / 72;
      return amount < 1 ? 1 : amount;
    }

    int pickHue() {
      switch (hueIndex) {
      case 0:
        return drift(HUE_RED, esp_random_max(30));
      case 1:
        return drift(HUE_ORANGE, esp_random_max(30));
      case 2:
        return drift(HUE_YELLOW, esp_random_max(30));
      case 3:
        return drift(HUE_GREEN, esp_random_max(30));
      case 4:
        return drift(HUE_BLUE, esp_random_max(30));
      case 5:
        return drift(HUE_INDIGO, esp_random_max(30));
      case 6:
        return drift(HUE_VIOLET, esp_random_max(30));
      }
      return 0;
    }

    int currentStep;
    int currentTarget;
    int hueIndex;
    bool forward;
    int16_t falling;
    int activeHue;
};

#endif
