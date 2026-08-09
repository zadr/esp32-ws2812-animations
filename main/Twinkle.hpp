#ifndef TWINKLE_HPP
#define TWINKLE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "color_utils.hpp"
#include "esp_random.h"
#include "esp_random_max.h"

class Twinkle : public Animation {
public:
  Twinkle(Frame& strip) : Animation(strip), backgroundHue(0), seed(0) {}

  void setup() override {
    backgroundHue = esp_random_max(HUE_VIOLET);
    seed = esp_random();
  }

  int duration() override { return RUN_MS; }

  // A twinkle outlives the step that lit it, so there is no place to jump to and
  // the run is walked from its opening on every render. Whole strip, whole walk,
  // in locals: 171 steps of fifty pixels is well inside the tick.
  void render(uint16_t t) override {
    State state = opening();

    const int steps = stepsAt(t, STEPS);
    for (int step = 1; step <= steps; step++) {
      advance(state, (uint32_t)step);
    }

    draw(state, (uint32_t)steps);
  }

  int tag() override { return 1010; }

private:
  // Nothing accumulates here and nothing arrives: the strip at the end of a run
  // is the strip at the start of one with different pixels lit. So the length is
  // how long a state the room is in wants to last, not how long anything takes.
  static constexpr int RUN_MS = 20000;

  // A twinkle's life is counted in steps, so this is the dwell of one twinkle
  // tick rather than a frame interval: the lifetimes below are 300 to 1500ms of
  // it, which is what was tuned. Taken against the cruise, which the trapezoid
  // holds a seventh above the mean rate.
  static constexpr int MS_PER_STEP = 100;
  static constexpr int STEPS = RUN_MS * 6 / (MS_PER_STEP * 7);

  // Density rather than a fixed count: 36 of the original 432 pixels. A fixed
  // 36 on a short strip lights most of it at once and reads as noise.
  static constexpr int MAX_TWINKLES = NUM_PIXELS / 12 > 0 ? NUM_PIXELS / 12 : 1;

  static constexpr int LIFETIME_MIN = 3;
  static constexpr int LIFETIME_SPAN = 12;

  // Lanes below SHIMMER are handed out in order by the pick loop, two to a pick:
  // the start index, then the lifetime. The loop stops at MAX_TWINKLES picks, so
  // the counter cannot climb into the range above. From SHIMMER up each pixel
  // owns an adjacent pair, amount then sign, indexed by the pixel, so one
  // pixel's shimmer can neither repeat another's nor land on a pick.
  static constexpr uint32_t SHIMMER = NUM_PIXELS;
  static_assert(2u * MAX_TWINKLES <= SHIMMER, "the pick loop reaches into the shimmer lanes");

  // Everything a step changes, and so a local of render() rather than a member.
  struct State {
    uint8_t remaining[NUM_PIXELS];
  };

  // The first pick belongs to the opening state, not to the first step: a bare
  // background is a state the strip is never meant to be seen in.
  State opening() const {
    State state = {};
    advance(state, 0);
    return state;
  }

  void advance(State& state, uint32_t step) const {
    int twinkleCount = 0;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      if (state.remaining[i] > 0) {
        state.remaining[i]--;

        if (state.remaining[i] > 0) {
          twinkleCount++;
        }
      }
    }

    uint32_t lane = 0;
    while (twinkleCount < MAX_TWINKLES) {
      const int index = pickTwinkleIndex(state, step, lane);
      if (index < 0) {
        break;
      }

      state.remaining[index] = (uint8_t)(noiseMax(seed, step, lane++, LIFETIME_SPAN) + LIFETIME_MIN);
      twinkleCount++;
    }
  }

  void draw(const State& state, uint32_t step) const {
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      actual_led_strip_set_pixel_hsv(strip, i,
                                     state.remaining[i] > 0 ? shimmerHue(step, i) : backgroundHue);
    }
  }

  // A fresh hue per lit pixel per step. It depends on nothing the walk carries,
  // so it is rolled at the step that is drawn rather than at every step along
  // the way; rolling it per render instead would run the shimmer at the driver's
  // tick rather than at the twinkle's.
  uint16_t shimmerHue(uint32_t step, uint16_t index) const {
    const uint32_t lane = SHIMMER + 2 * index;
    return driftBy(COMPLEMENT(backgroundHue),
                   (uint8_t)noiseMax(seed, step, lane, 5),
                   noise(seed, step, lane + 1));
  }

  static bool canTwinkle(const State& state, uint16_t index) {
    if (state.remaining[index] > 0) {
      return false;
    }
    if (index > 0 && state.remaining[index - 1] > 0) {
      return false;
    }
    if (index < NUM_PIXELS - 1 && state.remaining[index + 1] > 0) {
      return false;
    }
    return true;
  }

  // Random start then walk the strip, so a strip with no legal slot left
  // reports -1 instead of rerolling forever.
  int pickTwinkleIndex(const State& state, uint32_t step, uint32_t& lane) const {
    const uint16_t start = (uint16_t)noiseMax(seed, step, lane++, NUM_PIXELS - 1);
    for (uint16_t offset = 0; offset < NUM_PIXELS; offset++) {
      const uint16_t index = (start + offset) % NUM_PIXELS;
      if (canTwinkle(state, index)) {
        return index;
      }
    }
    return -1;
  }

  uint16_t backgroundHue;
  uint32_t seed;
};

#endif
