#ifndef FILLIN_HPP
#define FILLIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

class FillIn : public Animation {
public:
  FillIn(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), currentStep(0), hueIndex(0), forward(forward) {
  }
  ~FillIn() {}

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
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_RED, esp_random_max(30)));
      break;
    case 1:
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_ORANGE, esp_random_max(30)));
      break;
    case 2:
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_YELLOW, esp_random_max(30)));
      break;
    case 3:
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_GREEN, esp_random_max(30)));
      break;
    case 4:
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_BLUE, esp_random_max(30)));
      break;
    case 5:
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_INDIGO, esp_random_max(30)));
      break;
    case 6:
      actual_led_strip_set_pixel_hsv(strip, currentStep, drift(HUE_VIOLET, esp_random_max(30)));
      break;
    }
  }

  int getDelay() {
    return 25;
  }

private:
    int currentStep;
    int hueIndex;
    bool forward;
};

#endif
