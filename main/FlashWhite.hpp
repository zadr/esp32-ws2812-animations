#ifndef FLASHWHITE_HPP
#define FLASHWHITE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

class FlashWhite : public Animation {
public:
    FlashWhite(led_strip_handle_t& strip) : Animation(strip), lit(false) {}

    void setup() override { lit = false; }
    int steps() override { return 100; }

    // White has no hue, so it cannot go through actual_led_strip_set_pixel_hsv
    // and the scales that function applies are repeated here. Unscaled white on
    // 50 pixels draws about 3A, well past what the board's USB input carries.
    void loop() override {
      const uint32_t red = lit ? 255 * RED_SCALE : 0;
      const uint32_t greenBlue = lit ? 255 * BRIGHTNESS_SCALE : 0;

      for (int i = 0; i < NUM_PIXELS; i++) {
        led_strip_set_pixel(strip, i, red, greenBlue, greenBlue);
      }

      lit = !lit;
    }

    int getDelay() override { return 25; }
    int minIterations() override { return 1; }
    int maxIterations() override { return 2; }
    int tag() override { return 1006; }

private:
    bool lit;
};

#endif
