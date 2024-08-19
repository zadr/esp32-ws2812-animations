#ifndef DROPIN_HPP
#define DROPIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "hue_to_rgb.h"

class DropIn : public Animation {
public:
  DropIn(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), currentStep(0), hueIndex(0), forward(forward) {
  }
  ~DropIn() {}

  void setup() {
    currentStep = forward ? 0 : NUM_PIXELS;
    hueIndex = forward ? 0 : 6;
  }

  int steps() {
    return NUM_PIXELS * 7;
  }

  void loop() {
    if (forward) {
      currentStep += 1;
      if (currentStep >= NUM_PIXELS) {
        currentStep = 0;
        hueIndex += 1;
      }
    } else {
      currentStep -= 1;
      if (currentStep < 0) {
        currentStep = NUM_PIXELS;
        hueIndex -= 1;
      }
    }

    switch (hueIndex) {
    case 0:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_RED, RANDOM_PERCENT(30)));
      break;
    case 1:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_ORANGE, RANDOM_PERCENT(30)));
      break;
    case 2:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_YELLOW, RANDOM_PERCENT(30)));
      break;
    case 3:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_GREEN, RANDOM_PERCENT(30)));
      break;
    case 4:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_BLUE, RANDOM_PERCENT(30)));
      break;
    case 5:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_INDIGO, RANDOM_PERCENT(30)));
      break;
    case 6:
      actual_led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_VIOLET, RANDOM_PERCENT(30)));
      break;
    }
  }

  int getDelay() {
    return 100;
  }

private:
    int currentStep;
    int hueIndex;
    bool forward;
};

#endif
