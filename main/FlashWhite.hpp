#ifndef FLASHWHITE_HPP
#define FLASHWHITE_HPP

#include "Animation.hpp"

class FlashWhite : public Animation {
public:
    FlashWhite(led_strip_handle_t& strip) : Animation(strip) {}

    void setup() override {}
    int steps() override { return 100; }
    void loop() override {
      for (int i = 0; i < NUM_PIXELS; i++) {
        led_strip_set_pixel(strip, i, 255, 255, 255);
      }
    }
};

#endif
