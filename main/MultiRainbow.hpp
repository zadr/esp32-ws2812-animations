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
        : Animation(strip), adding(direction), hues(new uint16_t[NUM_PIXELS]) {}

    ~MultiRainbow() {
        delete[] hues;
    }

    void setup() override {
        int numberOfRainbows = esp_random_max(4) + 2; // max 7 min 2
        int numberOfLEDsPerRainbow = NUM_PIXELS / numberOfRainbows;
        uint16_t slice = HUE_MAX / numberOfLEDsPerRainbow;
        for (int i = 0; i < numberOfRainbows; i++) {
            int offset = i * numberOfLEDsPerRainbow;
            for (int j = 0; j < numberOfLEDsPerRainbow; j++) {
                hues[j + offset] = slice * j;
            }
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
    int tag() override { return 1011; }

    int getDelay() {
        return 5;
    }

private:
    bool adding;
    uint16_t* hues;
};

#endif
