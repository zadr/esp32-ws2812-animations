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
        // the delay macro floors to whole 10ms ticks, so 10 is the shortest
        // interval that actually blocks
        return 10;
    }

private:
    bool adding;
    uint16_t hues[NUM_PIXELS];
};

#endif
