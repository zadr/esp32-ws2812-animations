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

    // Red to red the long way, HUE_PER_STEP at a time: STEPS of them is 65000 of
    // the wheel, so the run stops just short of coming back round.
    int duration() override { return STEPS * MS_PER_STEP; }

    void render(uint16_t t) override {
        const uint32_t travelled = ((uint32_t)t * STEPS) / 65535;
        const uint32_t turned = travelled * HUE_PER_STEP;

        // Whole wheel here, so the hue wraps in its uint16 rather than at
        // HUE_MAX, and backwards is the same walk taken the other way.
        const uint16_t hue = (uint16_t)(adding ? turned : -turned);

        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, hue);
        }
    }

    int tag() override { return 1009; }

private:
    static const int STEPS = 650;
    static const int HUE_PER_STEP = 100;
    static const int MS_PER_STEP = 50;

    bool adding;
};

#endif
