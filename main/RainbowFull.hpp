#ifndef FULLRAINBOW_HPP
#define FULLRAINBOW_HPP

#include "led_strip.h"
#include "Constants.h"
#include "Animation.hpp"
#include "actual_led_strip_set_pixel_hsv.h"

class FullRainbow : public Animation {
public:
    FullRainbow(led_strip_handle_t& strip, bool direction)
        : Animation(strip), adding(direction) {}

    // The wheel laid once across the strip. These are the hues the run turns
    // from; they are not touched again.
    void setup() override {
        uint16_t slice = HUE_MAX / NUM_PIXELS;
        for (int i = 0; i < NUM_PIXELS; i++) {
            hues[i] = slice * i;
        }
    }

    // Every hue turns by the same amount each step, so the pattern holds its
    // shape and travels through the wheel. HUE_MAX / HUE_PER_STEP steps is one
    // turn of it, and the run is just under three.
    int duration() override { return STEPS * MS_PER_STEP; }

    // The offset follows from t alone, so there is nothing to carry between
    // renders and two renders at the same t draw the same strip.
    void render(uint16_t t) override {
        const uint32_t travelled = ((uint32_t)t * STEPS) / 65535;
        const uint16_t turned = (uint16_t)((travelled * HUE_PER_STEP) % HUE_MAX);

        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, turn(hues[i], turned));
        }
    }

    int tag() override { return 1008; }

private:
    static const int STEPS = 1950;
    static const int HUE_PER_STEP = 100;
    static const int MS_PER_STEP = 10;

    // The wheel closes at HUE_MAX, one short of the uint16 it is carried in, so
    // this is not a uint16 wrap: 65535 is never reached, and a hue crossing the
    // top comes back one value further round than a wrap would put it.
    uint16_t turn(uint16_t hue, uint16_t by) const {
        return adding ? (hue + by) % HUE_MAX : (hue + HUE_MAX - by) % HUE_MAX;
    }

    bool adding;
    uint16_t hues[NUM_PIXELS];
};

#endif
