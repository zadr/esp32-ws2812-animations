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

    // Three turns of the wheel, six seconds a turn and a little over five
    // through the middle of the run. Slower and the strip is a colour changing;
    // at this rate the pattern reads as travelling along it.
    int duration() override { return 18000; }

    // Every hue turns by the same amount, so the pattern holds its shape and
    // travels through the wheel. The offset follows from t alone: it runs to
    // 65535 and a turn is HUE_MAX, so TURNS turns across the run is t times
    // TURNS with nothing left to divide out, and there is no step to land on.
    void render(uint16_t t) override {
        const uint16_t turned = (uint16_t)(((uint32_t)t * TURNS) % HUE_MAX);

        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, turn(hues[i], turned));
        }
    }

    int tag() override { return 1008; }

private:
    static const int TURNS = 3;

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
