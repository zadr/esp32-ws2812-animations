#ifndef FILLIN_HPP
#define FILLIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

class FillIn : public Animation {
public:
  FillIn(led_strip_handle_t& strip, bool forward)
    : Animation(strip), currentStep(0), hueIndex(0), activeHue(0), forward(forward) {
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
    int oldHueIndex = hueIndex;
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
    int newHueIndex = hueIndex;

    if (newHueIndex != oldHueIndex) {
      switch (hueIndex) {
      case 0:
        activeHue = drift(HUE_RED, esp_random_max(30));
        break;
      case 1:
        activeHue = drift(HUE_ORANGE, esp_random_max(30));
        break;
      case 2:
        activeHue = drift(HUE_YELLOW, esp_random_max(30));
        break;
      case 3:
        activeHue = drift(HUE_GREEN, esp_random_max(30));
        break;
      case 4:
        activeHue = drift(HUE_BLUE, esp_random_max(30));
        break;
      case 5:
        activeHue = drift(HUE_INDIGO, esp_random_max(30));
        break;
      case 6:
        activeHue = drift(HUE_VIOLET, esp_random_max(30));
        break;
      }
    }
    actual_led_strip_set_pixel_hsv(strip, currentStep, activeHue);
  }

  int getDelay() {
    return 10;
  }

  int minIterations() override { return 1; }
  int maxIterations() override { return 2; }
  int tag() override { return 1005; }

private:
    int currentStep;
    int hueIndex;
    int activeHue;
    bool forward;
};

#endif
