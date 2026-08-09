#ifndef BOUNCE_HPP
#define BOUNCE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "color_utils.hpp"
#include "esp_random.h"
#include "esp_random_max.h"

class Bounce : public Animation {
public:
  Bounce(Frame& strip) : Animation(strip), hue(0), seed(0) {}

  void setup() override {
    hue = esp_random_max(HUE_VIOLET);
    seed = esp_random();
  }

  // Three trips up the strip and back, which puts the head at 44ms to the pixel
  // through the middle of the run: travelling rather than sliding, and slow
  // enough at the turns that the far end is a stop rather than a blur. Neither
  // end is counted twice, so the run closes on the pixel it opened at, and five
  // bounces are enough for the hue to have gone somewhere by then.
  int duration() override { return 15000; }

  void render(uint16_t t) override {
    const int step = stepsAt(t, STEPS);

    // A leg is one length of the strip, so the count of legs behind the head is
    // also the count of bounces behind it, and the parity of that count is which
    // way the head is pointing.
    const int leg = step / LEG;
    const int along = step % LEG;
    const bool ascending = leg % 2 == 0;

    draw(ascending ? along : LEG - along, ascending ? 1 : -1, hueAfter(leg));
  }

  int tag() override { return 1002; }

private:
  static constexpr int LEG = NUM_PIXELS - 1;
  static constexpr int TRIPS = 3;
  static constexpr int STEPS = 2 * LEG * TRIPS;

  static constexpr uint8_t DRIFT_MAX = 30;

  // The amount is itself drawn, so it needs a lane of its own: taken from the
  // same one as the sign it would decide the direction it moves in.
  static constexpr uint32_t LANE_AMOUNT = 0;
  static constexpr uint32_t LANE_SIGN = 1;

  // A bounce drifts the hue and the drift stays, so the hue at a step is the
  // setup hue carried through every bounce already behind it. A run holds two,
  // so the carry is walked rather than folded into a formula.
  uint16_t hueAfter(int bounces) const {
    uint16_t drifted = hue;
    for (int bounce = 0; bounce < bounces; bounce++) {
      drifted = driftBy(drifted,
                        (uint8_t)noiseMax(seed, bounce, LANE_AMOUNT, DRIFT_MAX),
                        noise(seed, bounce, LANE_SIGN));
    }
    return drifted;
  }

  // The turn lands before the head is drawn at the end pixel, so the tail there
  // is already on the side the head is leaving for and truncates off the end.
  // Each end is the one place the head is drawn without a tail behind it.
  void draw(int pos, int direction, uint16_t headHue) const {
    const uint16_t compHue = COMPLEMENT(headHue);

    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i, compHue);
    }

    // Proportion rather than a fixed length: 16 of the original 432 pixels. Two
    // is the shortest that still fades, and below that the head reads as a dot.
    const int tailLength = NUM_PIXELS / 27 > 2 ? NUM_PIXELS / 27 : 2;

    // The tail truncates at the ends rather than wrapping: it trails a head that
    // is about to turn around, so wrapping would draw it at the far end of the
    // strip.
    for (int tail = 0; tail < tailLength; tail++) {
      const int index = (direction > 0) ? (pos - tail) : (pos + tail);

      if (index < 0 || index >= NUM_PIXELS) {
        break;
      }

      const uint16_t tailHue = headHue + ((compHue - headHue) * tail / tailLength);
      actual_led_strip_set_pixel_hsv(strip, index, tailHue);
    }
  }

  uint16_t hue;
  uint32_t seed;
};

#endif
