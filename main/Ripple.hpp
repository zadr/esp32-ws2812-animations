#ifndef RIPPLE_HPP
#define RIPPLE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random.h"
#include "esp_random_max.h"

// hue_to_rgb pins saturation and value, but not luminance: a hue sitting on a
// primary drives one channel, a hue between two drives two, so the strip holds
// close to a two to one range at full value. Red through yellow crosses that
// range without leaving the warm end of the wheel, and blue through cyan crosses
// it without leaving the cool end. That band is the energy of a front here,
// since there is no brightness for it to give up.
//
// Each band is paired against the primary that shares none of its channels, so
// even a spent front stays unmistakable against the medium it crosses. A
// background near the front on the wheel would be invisible: adjacent hues at
// pinned saturation differ only in the ramp of a single channel.
class Ripple : public Animation {
public:
  Ripple(led_strip_handle_t& strip)
    : Animation(strip), backgroundHue(HUE_BLUE), bandSpent(HUE_RED), bandFull(HUE_YELLOW), frame(0) {
    clearFronts();
  }

  void setup() override {
    if (esp_random() % 2 == 0) {
      backgroundHue = HUE_BLUE;
      bandSpent = HUE_RED;
      bandFull = HUE_YELLOW;
    } else {
      backgroundHue = HUE_RED;
      bandSpent = HUE_BLUE;
      bandFull = (HUE_GREEN + HUE_BLUE) / 2;
    }

    frame = 0;
    clearFronts();
  }

  int steps() override { return 720; }

  void loop() override {
    if (frame % IMPULSE_PERIOD == 0) {
      impulse();
    }
    frame++;

    for (int i = 0; i < MAX_FRONTS; i++) {
      advance(fronts[i]);
    }

    render();
  }

  int getDelay() override { return 20; }

  int minIterations() override { return 1; }
  int maxIterations() override { return 2; }
  int tag() override { return 1014; }

private:
  static const int MAX_FRONTS = 6;
  static const int IMPULSE_PERIOD = 90;
  static const int8_t ENERGY_FULL = 3;
  static const int8_t MEDIUM = -1;

  struct Front {
    int16_t position;
    int8_t direction;
    int8_t energy;
    bool alive;
  };

  void clearFronts() {
    for (int i = 0; i < MAX_FRONTS; i++) {
      fronts[i].alive = false;
    }
  }

  void impulse() {
    int first = -1;
    int second = -1;
    for (int i = 0; i < MAX_FRONTS; i++) {
      if (fronts[i].alive) {
        continue;
      }
      if (first < 0) {
        first = i;
        continue;
      }
      second = i;
      break;
    }

    // An impulse is a pair, so a lone free slot is no use to it.
    if (second < 0) {
      return;
    }

    int16_t at = esp_random_max(NUM_PIXELS - 1);
    start(fronts[first], at, -1);
    start(fronts[second], at, 1);
  }

  static void start(Front& front, int16_t at, int8_t direction) {
    front.position = at;
    front.direction = direction;
    front.energy = ENERGY_FULL;
    front.alive = true;
  }

  // Every front moves one pixel a frame. Speed belongs to the medium rather than
  // to whatever disturbed it, which is also why two fronts share a pixel and
  // come out the far side unchanged.
  //
  // Ends reflect, and the boundary is the only thing that takes energy. A front
  // that arrives already spent has nothing left to give it and is done.
  void advance(Front& front) {
    if (!front.alive) {
      return;
    }

    front.position += front.direction;
    if (front.position < 0) {
      front.position = -front.position;
    } else if (front.position > NUM_PIXELS - 1) {
      front.position = 2 * (NUM_PIXELS - 1) - front.position;
    } else {
      return;
    }

    front.direction = -front.direction;
    if (front.energy == 0) {
      front.alive = false;
    } else {
      front.energy--;
    }
  }

  void render() {
    for (int i = 0; i < NUM_PIXELS; i++) {
      canvas[i] = MEDIUM;
    }

    // Two pixels wide: one is a dot with no direction to it, and three smears at
    // a pixel a frame. The trailing pixel sits one level down the band, which
    // gives the front a shape without spending a second colour on it.
    for (int i = 0; i < MAX_FRONTS; i++) {
      if (!fronts[i].alive) {
        continue;
      }
      int8_t energy = fronts[i].energy;
      paint(fronts[i].position, energy);
      paint(fronts[i].position - fronts[i].direction, energy > 0 ? energy - 1 : 0);
    }

    for (int i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i, canvas[i] == MEDIUM ? backgroundHue : energyHue(canvas[i]));
    }
  }

  // Overlapping fronts add and then separate again. The band is the whole
  // budget, so the sum tops out at a full front rather than running past it into
  // a hue that no longer belongs to the ripple.
  void paint(int16_t at, int8_t energy) {
    if (at < 0 || at >= NUM_PIXELS) {
      return;
    }
    if (canvas[at] == MEDIUM) {
      canvas[at] = energy;
      return;
    }
    int8_t higher = canvas[at] > energy ? canvas[at] : energy;
    canvas[at] = higher < ENERGY_FULL ? higher + 1 : ENERGY_FULL;
  }

  uint16_t energyHue(int8_t energy) const {
    return (uint16_t)(bandSpent + ((int32_t)bandFull - bandSpent) * energy / ENERGY_FULL);
  }

  Front fronts[MAX_FRONTS];
  int8_t canvas[NUM_PIXELS];
  uint16_t backgroundHue;
  uint16_t bandSpent;
  uint16_t bandFull;
  uint32_t frame;
};

#endif
