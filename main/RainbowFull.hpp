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

    void loop() override {
        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, hues[i]);
            hues[i] = adding ? (hues[i] + 100) % HUE_MAX : (hues[i] - 100 + HUE_MAX) % HUE_MAX;
        }
    }

    int steps() override {
        return 1950;
    }

    int minIterations() override { return 2; }
    int maxIterations() override { return 4; }
    int tag() override { return 1008; }

    int getDelay() {
        return 15;
    }

private:
    bool adding;
    uint16_t* hues;
};

#endif
