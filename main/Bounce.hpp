#ifndef BOUNCE_HPP
#define BOUNCE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "esp_random_max.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "freertos/task.h"

class Bounce : public Animation {
public:
  Bounce(led_strip_handle_t& strip) : Animation(strip), pos(0), direction(1), hue(0) {}

  void setup() override {
    hue = esp_random_max(HUE_VIOLET);
  }

  int steps() override {
    return (2 * NUM_PIXELS - 2); // The number of steps needed for seven full bounces
  }

  void loop() override {
    uint16_t compHue = COMPLEMENT(hue);

    // Clear the strip and set all LEDs to the complementary hue
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i, compHue);
    }

    // Proportion rather than a fixed length: 16 of the original 432 pixels. Two
    // is the shortest that still fades, and below that the head reads as a dot.
    const int16_t tailLength = NUM_PIXELS / 27 > 2 ? NUM_PIXELS / 27 : 2;

    // Light the current LED and create a tail effect with hue fading. The tail
    // truncates at the ends rather than wrapping: it trails a head that is about
    // to turn around, so wrapping would draw it at the far end of the strip.
    for (int16_t tail = 0; tail < tailLength; tail++) {
      int16_t index = (direction > 0) ? (pos - tail) : (pos + tail);

      if (index < 0 || index >= NUM_PIXELS) {
        break;
      }

      uint16_t tailHue = hue + ((compHue - hue) * tail / tailLength);
      actual_led_strip_set_pixel_hsv(strip, index, tailHue);
    }

    // Update position and direction
    pos += direction;
    if (pos == NUM_PIXELS - 1 || pos == 0) {
      direction = -direction;

      // Drift the hue slightly after each bounce, wrapping around the wheel
      int32_t drift = (esp_random_max(30) / 100.0) * HUE_MAX;
      if (esp_random() % 2 == 0) {
        drift = -drift;
      }
      hue = (uint16_t)(hue + drift);
    }
  }

  int getDelay() override {
    return 10;
  }

  int minIterations() override { return 2; }
  int maxIterations() override { return 5; }
  int tag() override { return 1002; }

private:
  int16_t pos;
  int8_t direction;
  uint16_t hue;
};

#endif
