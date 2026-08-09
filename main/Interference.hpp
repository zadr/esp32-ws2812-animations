#ifndef INTERFERENCE_HPP
#define INTERFERENCE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random_max.h"
#include <math.h>
#include <stdint.h>

// Phase is a 16 bit angle, so a wavelength converts to phase per pixel by
// division and both phases wrap for free. The two wavelengths are 20.0 and
// 12.4 pixels, a ratio of 1.618: near enough to irrational that the beat does
// not land back on itself while anyone is watching, and both coarse enough that
// 50 pixels still resolves the shorter one at six pixels per half cycle.
static const uint16_t INTERFERENCE_PIXEL_STEP_LONG = 3277;
static const uint16_t INTERFERENCE_PIXEL_STEP_SHORT = 5303;

// Phase per step of time, as the pair above is phase per pixel. Deliberately not
// in the ratio of the wavelengths: matching ratios give both waves the same
// phase velocity and the whole figure, envelope included, slides along rigidly.
// These have the carriers running down the strip at 8 and 3.3 pixels per second
// while the envelope crawls up it at 4.4, all three as the middle of a run holds
// them and slower at either end of it.
static const uint16_t INTERFERENCE_TIME_STEP_LONG = 1049;
static const uint16_t INTERFERENCE_TIME_STEP_SHORT = 691;

// Amplitude is value, and hue holds still for the whole run. Amplitude spent on
// hue instead reads as a colour gradient travelling along the strip, which is a
// rainbow: the nodes and antinodes are all still in it, and not one of them
// reads as loud or quiet.
class Interference : public Animation {
public:
  Interference(led_strip_handle_t& strip)
    : Animation(strip), hue(HUE_GREEN), setupPhaseLong(0), setupPhaseShort(0) {
  }
  ~Interference() {}

  // Red is compensated by a square root of the level, so it decays more slowly
  // than green and blue and any hue holding red turns toward it on the way down:
  // at a tenth of full output red stands 3.1 times higher against them than it
  // does at full. From the green anchor to the blue one the red channel is zero
  // outright, which is the only arc a fade to black crosses without turning, and
  // the nodes here are for fading to black.
  void setup() override {
    hue = (uint16_t)(HUE_GREEN + esp_random_max(HUE_BLUE - HUE_GREEN));
    setupPhaseLong = esp_random_max(HUE_MAX);
    setupPhaseShort = esp_random_max(HUE_MAX);
  }

  int duration() override { return RUN_MS; }

  // Both phases are the opening phase plus a fixed step times the step count, so
  // there is nothing to integrate and no accumulator to hold. The uint16_t cast
  // is where the wrap happens, exactly where the accumulators used to put it.
  void render(uint16_t t) override {
    const int step = stepsAt(t, STEPS);

    draw((uint16_t)(setupPhaseLong + INTERFERENCE_TIME_STEP_LONG * step),
         (uint16_t)(setupPhaseShort + INTERFERENCE_TIME_STEP_SHORT * step));
  }

  int tag() override { return 1016; }

private:
  // The envelope crosses the strip in about eleven seconds and is the slowest
  // thing in the figure, so it is what the length is set by: a run is a little
  // short of two crossings of it.
  static const int RUN_MS = 24000;

  // The two time steps above are phase per step at this dwell, so it is a rate
  // the figure is drawn against rather than a frame interval: change it and the
  // velocities quoted there are no longer the ones on the strip. Taken against
  // the cruise, which the trapezoid holds a seventh above the mean rate.
  static const int MS_PER_STEP = 40;
  static const int STEPS = RUN_MS * 6 / (MS_PER_STEP * 7);

  // Two waves are summed where they are amplitudes and nowhere else. Two hue
  // angles cannot be added at all: both wrap, their sum wraps twice as often,
  // and a seam lands wherever either one rolls over.
  void draw(uint16_t phaseLong, uint16_t phaseShort) const {
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      const float sum = wave((uint16_t)(INTERFERENCE_PIXEL_STEP_LONG * i + phaseLong))
                      + wave((uint16_t)(INTERFERENCE_PIXEL_STEP_SHORT * i + phaseShort));

      actual_led_strip_set_pixel_hsv(strip, i, hue, level(sum));
    }
  }

  // Rectified, which is what an exposure of two waves records and what leaves a
  // node dark: the sign belongs to the carrier, and the envelope survives the
  // fold whole.
  //
  // The bottom of the duty range is coarse, stepping visibly under 32 and
  // reading as discrete stops under 8, and a node reaching black has to cross
  // it. It is crossed at the carrier's zero, where a pixel is moving through the
  // range at its fastest and holds no level long enough to show a step. A floor
  // would keep every pixel clear of the coarse end at the cost of the dark that
  // the nodes are.
  //
  // Value is absolute, so the antinodes stand above the strip's default rather
  // than being fitted under it. A rectified pair of sines averages 8/pi squared
  // of unit amplitude, which puts the strip at a mean 103 of 255 against the 168
  // an animation at the default holds everywhere.
  static uint8_t level(float sum) {
    const float amplitude = sum < 0.0f ? -sum : sum;
    return (uint8_t)(amplitude * (255.0f / 2.0f) + 0.5f);
  }

  static float wave(uint16_t phase) {
    return sinf(phase * (6.28318531f / 65536.0f));
  }

  uint16_t hue;
  uint16_t setupPhaseLong;
  uint16_t setupPhaseShort;
};

#endif
