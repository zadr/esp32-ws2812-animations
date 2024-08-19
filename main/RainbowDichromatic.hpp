#ifndef RAINBOWDICHROMATIC_HPP
#define RAINBOWDICHROMATIC_HPP

#include "led_strip.h"
#include "Constants.h"
#include "Animation.hpp"

class RainbowDichromatic : public Animation {
public:
    RainbowDichromatic(led_strip_handle_t& strip, bool forward)
        : Animation(strip), currentHueIndex(0), currentHuePosition(NUM_PIXELS - 1), forward(forward) {}

    ~RainbowDichromatic() {}

    void setup() override {
        currentHueIndex = 0;
        currentHuePosition = NUM_PIXELS - 1;

        if (forward) {
            hues[0] = HUE_RED;
            hues[NUM_PIXELS - 1] = HUE_ORANGE;
        } else {
            hues[0] = HUE_VIOLET;
            hues[NUM_PIXELS - 1] = HUE_INDIGO;
        }

        interpolateColors(hues[0], hues[NUM_PIXELS - 1], 0, NUM_PIXELS - 1);

        if (forward) {
            hues[NUM_PIXELS * 2 - 1] = HUE_YELLOW;
        } else {
            hues[NUM_PIXELS * 2 - 1] = HUE_BLUE;
        }

        interpolateColors(hues[NUM_PIXELS - 1], hues[NUM_PIXELS * 2 - 1], NUM_PIXELS, NUM_PIXELS * 2 - 1);
    }

    int steps() override {
        return 720;
    }

    void loop() override {
        animate();
    }

    int getDelay() {
        return 25;
    }

private:
    uint16_t hues[NUM_PIXELS * 2];
    int currentHueIndex; // 0 - 6 (for the 7 color pairs)
    int currentHuePosition; // 0 - NUM_PIXELS
    bool forward;

    // Quadratic interpolation
    void interpolateColors(uint16_t startHue, uint16_t endHue, int startIndex, int endIndex) {
        int stepsPerColor = (endIndex - startIndex);
        for (int i = 0; i <= stepsPerColor; i++) {
            float t = (float)i / stepsPerColor;
            hues[startIndex + i] = startHue + (endHue - startHue) * t * t;
        }
    }

    void animate() {
        for (int i = 0; i < NUM_PIXELS; i++) {
            led_strip_set_pixel_hsv(strip, i, hues[i], 255, 255);
        }

        shiftHues();
    }

    void shiftHues() {
        uint16_t temp = hues[0];
        for (int i = 0; i < NUM_PIXELS * 2 - 1; i++) {
            hues[i] = hues[i + 1];
        }
        hues[NUM_PIXELS * 2 - 1] = temp;

        currentHuePosition--;

        if (currentHuePosition == 0) {
            currentHuePosition = NUM_PIXELS - 1;
            currentHueIndex = (currentHueIndex + 1) % 7;

            if (forward) {
                switch(currentHueIndex) {
                    case 0:
                        hues[NUM_PIXELS - 1] = HUE_ORANGE;
                        hues[NUM_PIXELS * 2 - 1] = HUE_YELLOW;
                        break;
                    case 1:
                        hues[NUM_PIXELS - 1] = HUE_YELLOW;
                        hues[NUM_PIXELS * 2 - 1] = HUE_GREEN;
                        break;
                    case 2:
                        hues[NUM_PIXELS - 1] = HUE_GREEN;
                        hues[NUM_PIXELS * 2 - 1] = HUE_BLUE;
                        break;
                    case 3:
                        hues[NUM_PIXELS - 1] = HUE_BLUE;
                        hues[NUM_PIXELS * 2 - 1] = HUE_INDIGO;
                        break;
                    case 4:
                        hues[NUM_PIXELS - 1] = HUE_INDIGO;
                        hues[NUM_PIXELS * 2 - 1] = HUE_VIOLET;
                        break;
                    case 5:
                        hues[NUM_PIXELS - 1] = HUE_VIOLET;
                        hues[NUM_PIXELS * 2 - 1] = HUE_MAX;
                        break;
                    case 6:
                        hues[NUM_PIXELS - 1] = HUE_RED;
                        hues[NUM_PIXELS * 2 - 1] = HUE_ORANGE;
                        break;
                }
            } else {
                switch(currentHueIndex) {
                    case 0:
                        hues[NUM_PIXELS - 1] = HUE_INDIGO;
                        hues[NUM_PIXELS * 2 - 1] = HUE_BLUE;
                        break;
                    case 1:
                        hues[NUM_PIXELS - 1] = HUE_BLUE;
                        hues[NUM_PIXELS * 2 - 1] = HUE_GREEN;
                        break;
                    case 2:
                        hues[NUM_PIXELS - 1] = HUE_GREEN;
                        hues[NUM_PIXELS * 2 - 1] = HUE_YELLOW;
                        break;
                    case 3:
                        hues[NUM_PIXELS - 1] = HUE_YELLOW;
                        hues[NUM_PIXELS * 2 - 1] = HUE_ORANGE;
                        break;
                    case 4:
                        hues[NUM_PIXELS - 1] = HUE_ORANGE;
                        hues[NUM_PIXELS * 2 - 1] = HUE_RED;
                        break;
                    case 5:
                        hues[NUM_PIXELS - 1] = HUE_RED;
                        hues[NUM_PIXELS * 2 - 1] = HUE_VIOLET;
                        break;
                    case 6:
                        hues[NUM_PIXELS - 1] = HUE_VIOLET;
                        hues[NUM_PIXELS * 2 - 1] = HUE_INDIGO;
                        break;
                }
            }

            interpolateColors(hues[NUM_PIXELS - 1], hues[NUM_PIXELS * 2 - 1], NUM_PIXELS, NUM_PIXELS * 2 - 1);
        }
    }
};

#endif
