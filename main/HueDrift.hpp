#ifndef HUE_DRIFT_HPP
#define HUE_DRIFT_HPP

#include <stdint.h>

#include "Constants.h"
#include "esp_random.h"
#include "esp_random_max.h"

// A hue moving at its own steady rate for the length of a run: where it starts,
// and how far and which way it has travelled by the end.
//
// Two of these are two colours rather than one colour and an offset. Nothing
// here relates one drift to another, so a pair closing up, crossing and pulling
// apart again is what independent rates do rather than something arranged.
//
// Wrapped modulo the palette span, so a hue leaving violet arrives at red
// instead of wandering into the wedge above violet, which has never been judged
// against this strip.
//
// The rate is against t, which means an animation that drifts a hue has to be
// handed a t that has not been shaped. Through a curve a steady rate is not
// steady, and the drift picks up whatever easing the pattern was given. Such an
// animation returns CurveLinear from curve() and applies the easing it wants
// inside render(), to the part that wants it.
struct HueDrift {
  uint16_t from = 0;  // hue at t == 0
  int32_t across = 0; // signed hue units by t == 65535, zero for a hue that stays put

  uint16_t at(uint16_t t) const {
    const int32_t travelled = (int32_t)(((int64_t)across * t) / 65536);
    const int32_t hue = ((int32_t)from + travelled) % HUE_SPAN;
    return (uint16_t)(hue < 0 ? hue + HUE_SPAN : hue);
  }
};

// The ends of the range a rate is drawn from, in hue units a second, each named
// by how long it takes to cross the palette span.
//
// Motion is read against what the eye was holding a moment ago, not against the
// opening of a run. A hue that has arrived somewhere else by the end without
// ever being caught moving is a colour that is different later, not a colour
// that moves, and the whole span is available to a rate slow enough to do that.
// A lap in 96 seconds is about three degrees a second, which is a few degrees
// against what was there a couple of seconds ago and about the least that still
// reads as travel; below it the strip is only somewhere else again.
//
// A lap in 16 seconds is the far end. Faster and the cycling is the animation
// rather than something the animation is doing, and on a pattern that also
// moves it starts competing with the pattern for what the eye follows.
static const int32_t DRIFT_SLOWEST = HUE_SPAN / 96;
static const int32_t DRIFT_FASTEST = HUE_SPAN / 16;

// A hue that stays put is its own reading, one colour fixed while another moves
// through it, and it only happens if it is drawn for: no rate this side of the
// slowest is worth spending a run on. One draw in eight, so a pair has a still
// member about one run in four.
static const int DRIFT_STILL_ONE_IN = 8;

// Rate is drawn against real time and then expressed across the run, so the
// same range means the same speed to the eye whatever length the animation
// asked for. Direction is drawn after it and is not tied to anything.
static HueDrift pickHueDrift(int runMs) {
  HueDrift drift;
  drift.from = esp_random_max(HUE_SPAN - 1);

  if (esp_random_max(DRIFT_STILL_ONE_IN - 1)) {
    const int32_t perSecond = DRIFT_SLOWEST + (int32_t)esp_random_max(DRIFT_FASTEST - DRIFT_SLOWEST);
    const int32_t across = (int32_t)((int64_t)perSecond * runMs / 1000);
    drift.across = esp_random_max(1) ? across : -across;
  }

  return drift;
}

#endif
