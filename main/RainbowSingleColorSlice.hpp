#ifndef RAINBOWSINGLECOLORSLICE_HPP
#define RAINBOWSINGLECOLORSLICE_HPP

#include "led_strip.h"
#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

class RainbowSingleColorSlice : public Animation {
public:
    RainbowSingleColorSlice(led_strip_handle_t& strip, bool direction)
        : Animation(strip), adding(direction) {}

    // The strip is one hue throughout, so there is nothing to seed.
    void setup() override {}

    // Red to red the long way in twenty seconds. Fifty pixels of one colour is
    // the whole of what is on the strip, so the run is a rate and nothing else:
    // slower than this and there is no motion left in it to see.
    int duration() override { return 20000; }

    void render(uint16_t t) override {
        // Whole wheel here, so the hue wraps in its uint16 rather than at
        // HUE_MAX, and t is the turn outright: it runs to 65535 against a wheel
        // that closes at 65536, so the run stops one unit short of coming back
        // round. Backwards is the same walk taken the other way.
        const uint16_t hue = (uint16_t)(adding ? t : -t);

        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, hue);
        }
    }

    int tag() override { return 1009; }

private:
    bool adding;
};

#endif
