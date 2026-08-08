#ifndef TWINKLE_HPP
#define TWINKLE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "color_utils.hpp"
#include "esp_random_max.h"
#include "freertos/task.h"
#include <math.h>
#include <vector>

class Twinkle : public Animation {
public:
  Twinkle(led_strip_handle_t& strip)
    : Animation(strip), backgroundHue(0) {
      // Initialize the twinkle durations vector with 0s
      twinkleDurations.resize(NUM_PIXELS, 0);
    }

  void setup() override {
    backgroundHue = esp_random_max(HUE_VIOLET);
  }

  int steps() override {
    return 216;
  }

  int minIterations() override { return 2; }
  int maxIterations() override { return 4; }
  int tag() override { return 1010; }

  void loop() override {
    // Clear twinkled LEDs that have completed their duration
    int twinkleCount = 0;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      if (twinkleDurations[i] > 0) {
        twinkleDurations[i]--;

        if (twinkleDurations[i] > 0) {
          twinkleCount++;
        }
      }
    }

    // Density rather than a fixed count: 36 of the original 432 pixels. A fixed
    // 36 on a short strip lights most of it at once and reads as noise.
    const int maxTwinkles = NUM_PIXELS / 12 > 0 ? NUM_PIXELS / 12 : 1;
    while (twinkleCount < maxTwinkles) {
      int index = pickTwinkleIndex();
      if (index < 0) {
        break;
      }

      // Set a random duration for the twinkle (between 3 and 15 loops)
      twinkleDurations[index] = esp_random_max(12) + 3;
      twinkleCount++; // Increment the count of active twinkles
    }

    // set leds
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i, twinkleDurations[i] > 0 ? drift(COMPLEMENT(backgroundHue), esp_random_max(5)) : backgroundHue);
    }
  }

  int getDelay() override {
    return 100;
  }

private:
  bool canTwinkle(uint16_t index) const {
    if (twinkleDurations[index] > 0) {
      return false;
    }
    if (index > 0 && twinkleDurations[index - 1] > 0) {
      return false;
    }
    if (index < NUM_PIXELS - 1 && twinkleDurations[index + 1] > 0) {
      return false;
    }
    return true;
  }

  // Random start then walk the strip, so a strip with no legal slot left
  // reports -1 instead of rerolling forever.
  int pickTwinkleIndex() const {
    uint16_t start = esp_random_max(NUM_PIXELS - 1);
    for (uint16_t offset = 0; offset < NUM_PIXELS; offset++) {
      uint16_t index = (start + offset) % NUM_PIXELS;
      if (canTwinkle(index)) {
        return index;
      }
    }
    return -1;
  }

  uint16_t backgroundHue;
  std::vector<int> twinkleDurations; // Stores the remaining duration for each twinkling LED
};

#endif
