#ifndef RAINBOWSINGLECOLORSLICE_HPP
#define RAINBOWSINGLECOLORSLICE_HPP

#include "led_strip.h"
#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

class RainbowSingleColorSlice : public Animation {
public:
    RainbowSingleColorSlice(led_strip_handle_t& strip, bool direction)
        : Animation(strip), adding(direction), hues(new uint16_t[NUM_PIXELS]) {}

    ~RainbowSingleColorSlice() {
        delete[] hues;
    }

    void setup() override {
        for (int i = 0; i < NUM_PIXELS; i++) {
            hues[i] = 0;
        }
    }

    int steps() override {
        return 650;
    }

    int minIterations() override { return 1; }
    int maxIterations() override { return 2; }
    int tag() override { return 1009; }

    void loop() override {
        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, hues[i]);
            hues[i] += adding ? 100 : -100;
        }
    }

    int getDelay() {
        return 50;
    }

private:
    bool adding;
    uint16_t* hues;
};

#endif
