#ifndef MULTIRAINBOW_HPP
#define MULTIRAINBOW_HPP

#include "led_strip.h"
#include "Constants.h"
#include "Animation.hpp"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random_max.h"
#include "esp_log.h"

class MultiRainbow : public Animation {
public:
    MultiRainbow(led_strip_handle_t& strip, bool direction)
        : Animation(strip), adding(direction) {}

    // The run turns these and never re-seeds, so the whole draw is here.
    void setup() override {
        int numberOfRainbows = esp_random_max(4) + 2; // inclusive, so 2 through 6

        // Pixels that do not divide evenly are spread across the rainbows rather
        // than left trailing off the end, so every pixel belongs to a rainbow.
        for (int i = 0; i < numberOfRainbows; i++) {
            int begin = (i * NUM_PIXELS) / numberOfRainbows;
            int end = ((i + 1) * NUM_PIXELS) / numberOfRainbows;
            uint16_t slice = HUE_MAX / (end - begin);

            for (int j = begin; j < end; j++) {
                hues[j] = slice * (j - begin);
            }
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

    int tag() override { return 1011; }

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
