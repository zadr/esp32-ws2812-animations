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
      twinkleCount = 0;
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

    // Twinkle random LEDs with complementary hues, ensuring no more than 12 are active
    while (twinkleCount < 36) {
      uint16_t index = esp_random_max(NUM_PIXELS);
      while (
        twinkleDurations[index] > 0 && // make sure we don't turn a light thats on back on again
        (index > 0 && twinkleDurations[index - 1] > 0) && // make sure we don't turn a light on next to another light that's on
        (index < NUM_PIXELS - 1 && twinkleDurations[index + 1] > 0) // make sure we don't turn a light on next to another light that's on
      ) {
        index = esp_random_max(NUM_PIXELS);
      }

      // Set a random duration for the twinkle (between 5 and 25 loops)
      twinkleDurations[index] = esp_random_max(10) + 3;
      twinkleCount++; // Increment the count of active twinkles
    }

    // set leds
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i, twinkleDurations[i] > 0 ? drift(COMPLEMENT(backgroundHue), esp_random_max(5)) : backgroundHue);
    }
  }

  int getDelay() override {
    return 45;
  }

private:
  uint16_t backgroundHue;
  std::vector<int> twinkleDurations; // Stores the remaining duration for each twinkling LED
  std::vector<int> activeTwinkles; // what lights are twinkling
  int twinkleCount;
};

#endif
