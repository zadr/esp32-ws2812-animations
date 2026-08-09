#ifndef RIPPLE_HPP
#define RIPPLE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random.h"

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
    : Animation(strip), backgroundHue(HUE_BLUE), bandSpent(HUE_RED), bandFull(HUE_YELLOW), seed(0) {
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

    seed = esp_random();
  }

  int duration() override { return STEPS * MS_PER_PIXEL; }

  // Impulses land at random positions on a period and a front carries the energy
  // the ends have left it, so there is no expression for the strip at t and it is
  // walked to from the opening state instead. The walk is in a local, so the same
  // t always arrives at the same fronts.
  void render(uint16_t t) override {
    State state = opening();

    const int steps = stepsAt(t, STEPS);
    for (int step = 0; step < steps; step++) {
      advance(state, step);
    }

    draw(state);
  }

  int tag() override { return 1014; }

private:
  static const int MAX_FRONTS = 6;
  static const int IMPULSE_PERIOD = 90;
  static const int8_t ENERGY_FULL = 3;
  static const int8_t MEDIUM = -1;

  // Eight impulses at the period they are spaced by. A front covers a pixel a
  // step, so the step is stated as the speed of the medium, which is what was
  // tuned.
  static const int IMPULSES = 8;
  static const int STEPS = IMPULSE_PERIOD * IMPULSES;
  static const int MS_PER_PIXEL = 20;

  struct Front {
    int16_t position;
    int8_t direction;
    int8_t energy;
    bool alive;
  };

  // Everything a step changes. It lives in render(), so no two renders can see
  // each other's fronts.
  struct State {
    Front fronts[MAX_FRONTS];
  };

  // Still water. The first step is what disturbs it.
  State opening() const { return State{}; }

  void advance(State& state, int step) const {
    if (step % IMPULSE_PERIOD == 0) {
      impulse(state, step);
    }

    for (int i = 0; i < MAX_FRONTS; i++) {
      advance(state.fronts[i]);
    }
  }

  // The step index is what the position is drawn against, so the impulse of step
  // 90 is in the same place however many times the walk passes through it.
  void impulse(State& state, int step) const {
    int first = -1;
    int second = -1;
    for (int i = 0; i < MAX_FRONTS; i++) {
      if (state.fronts[i].alive) {
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

    const int16_t at = (int16_t)noiseMax(seed, step, 0, NUM_PIXELS - 1);
    start(state.fronts[first], at, -1);
    start(state.fronts[second], at, 1);
  }

  static void start(Front& front, int16_t at, int8_t direction) {
    front.position = at;
    front.direction = direction;
    front.energy = ENERGY_FULL;
    front.alive = true;
  }

  // Every front moves one pixel a step. Speed belongs to the medium rather than
  // to whatever disturbed it, which is also why two fronts share a pixel and
  // come out the far side unchanged.
  //
  // Ends reflect, and the boundary is the only thing that takes energy. A front
  // that arrives already spent has nothing left to give it and is done.
  static void advance(Front& front) {
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

  void draw(const State& state) const {
    int8_t canvas[NUM_PIXELS];
    for (int i = 0; i < NUM_PIXELS; i++) {
      canvas[i] = MEDIUM;
    }

    // Two pixels wide: one is a dot with no direction to it, and three smears at
    // a pixel a step. The trailing pixel sits one level down the band, which
    // gives the front a shape without spending a second colour on it.
    for (int i = 0; i < MAX_FRONTS; i++) {
      const Front& front = state.fronts[i];
      if (!front.alive) {
        continue;
      }
      paint(canvas, front.position, front.energy);
      paint(canvas, front.position - front.direction, front.energy > 0 ? front.energy - 1 : 0);
    }

    for (int i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i, canvas[i] == MEDIUM ? backgroundHue : energyHue(canvas[i]));
    }
  }

  // Overlapping fronts add and then separate again. The band is the whole
  // budget, so the sum tops out at a full front rather than running past it into
  // a hue that no longer belongs to the ripple.
  static void paint(int8_t canvas[], int16_t at, int8_t energy) {
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

  uint16_t backgroundHue;
  uint16_t bandSpent;
  uint16_t bandFull;
  uint32_t seed;
};

#endif
