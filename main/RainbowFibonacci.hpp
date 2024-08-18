#ifndef RAINBOWFIBONACCI_HPP
#define RAINBOWFIBONACCI_HPP

#include "led_strip.h"
#include "Constants.h"
#include "Animation.hpp"

class RainbowFibonacci : public Animation {
public:
    RainbowFibonacci(led_strip_handle_t& strip, bool direction)
        : Animation(strip), adding(direction), hues(new uint16_t[NUM_PIXELS]) {}

    ~RainbowFibonacci() {
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
            led_strip_set_pixel_hsv(strip, i, hues[i], 255, 255);
            hues[i] = adding ? (hues[i] + 100) % HUE_MAX : (hues[i] - 100 + HUE_MAX) % HUE_MAX;
        }
    }

private:
    bool adding;
    uint16_t* hues;
};

#endif
