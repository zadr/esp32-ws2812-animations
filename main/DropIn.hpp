#ifndef DROPIN_HPP
#define DROPIN_HPP

#include "Animation.hpp"
#include "Constants.h"

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
    int offset = (forward ? 1 : -1);
    if ((forward && currentStep < NUM_PIXELS) || (!forward && currentStep >= 0)) {
      currentStep += offset;
    } else {
      currentStep = forward ? 0 : NUM_PIXELS;
      hueIndex += offset;
    }

    switch (hueIndex) {
    case 0:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_RED, RANDOM_PERCENT(30)), 255, 255);
      break;
    case 1:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_ORANGE, RANDOM_PERCENT(30)), 255, 255);
      break;
    case 2:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_YELLOW, RANDOM_PERCENT(30)), 255, 255);
      break;
    case 3:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_GREEN, RANDOM_PERCENT(30)), 255, 255);
      break;
    case 4:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_BLUE, RANDOM_PERCENT(30)), 255, 255);
      break;
    case 5:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_INDIGO, RANDOM_PERCENT(30)), 255, 255);
      break;
    case 6:
      led_strip_set_pixel_hsv(strip, currentStep, DRIFT(HUE_VIOLET, RANDOM_PERCENT(30)), 255, 255);
      break;
    }
  }
private:
    int currentStep;
    int hueIndex;
    bool forward;
};

#endif
