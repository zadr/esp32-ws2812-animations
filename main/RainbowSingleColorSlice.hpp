#ifndef RAINBOWSINGLECOLORSLICE_HPP
#define RAINBOWSINGLECOLORSLICE_HPP

#include "led_strip.h"
#include "Animation.hpp"
#include "Constants.h"

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

    void loop() override {
        for (int i = 0; i < NUM_PIXELS; i++) {
            hues[i] += adding ? 100 : -100;
            led_strip_set_pixel_hsv(strip, i, hues[i], 255, 255);
        }
    }

private:
    bool adding;
    uint16_t* hues;
};

#endif
