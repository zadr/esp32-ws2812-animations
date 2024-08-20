#ifndef FULLRAINBOW_HPP
#define FULLRAINBOW_HPP

#include "led_strip.h"
#include "Constants.h"
#include "Animation.hpp"
#include "actual_led_strip_set_pixel_hsv.h"

class FullRainbow : public Animation {
public:
    FullRainbow(led_strip_handle_t& strip, bool direction)
        : Animation(strip), adding(direction), hues(new uint16_t[NUM_PIXELS]) {}

    ~FullRainbow() {
        delete[] hues;
    }

    void setup() override {
        uint16_t slice = HUE_MAX / NUM_PIXELS;
        for (int i = 0; i < NUM_PIXELS; i++) {
            hues[i] = slice * i;
        }
    }

    int steps() override {
        return 650;
    }

    void loop() override {
        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, hues[i]);
            hues[i] = adding ? (hues[i] + 100) % HUE_MAX : (hues[i] - 100 + HUE_MAX) % HUE_MAX;
        }
    }

    int getDelay() {
        return 35;
    }

private:
    bool adding;
    uint16_t* hues;
};

#endif
