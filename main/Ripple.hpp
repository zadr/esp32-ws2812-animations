#ifndef RIPPLE_HPP
#define RIPPLE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random.h"
#include "esp_random_max.h"
#include <stdint.h>

// Still water at one hue and one low level, and a front is a displacement above
// it. Amplitude is the only thing that varies, which is what a ripple is; the
// hue band this used to spend energy on was a way of saying that without a
// per-pixel value to say it with.
//
// The hue comes from green through blue because that is the only arc of the
// wheel with no red in it: r is zero from 21846 to 43690 and the two anchors sit
// inside. Red's duty is the square root of the level, so a hue carrying any red
// turns toward red as it dims, about three times higher against the other two
// dies at a tenth of full output. A front anywhere else would spend its fade
// changing colour, which is the coupling of hue to energy this was rebuilt to be
// rid of. Water is that arc anyway.
class Ripple : public Animation {
public:
  Ripple(Frame& strip) : Animation(strip), hue(HUE_BLUE), seed(0) {}

  void setup() override {
    hue = (uint16_t)(HUE_GREEN + esp_random_max(HUE_BLUE - HUE_GREEN));
    seed = esp_random();
  }

  int duration() override { return RUN_MS; }

  // Speed belongs to the medium, so the run cannot be eased: a curve that holds
  // at the ends and runs at twice the rate through the middle is a change in how
  // fast water carries a wave. It opens and closes at rest without one, since
  // the first impulse lands on still water and the last is given its whole life
  // before the run is out.
  Curve curve() const override { return CurveLinear; }

  // An impulse is a birth step and a position, and the step index gives both. A
  // front's place and what is left of it then follow from the distance it has
  // run, so nothing here is integrated and there is no walk.
  void render(uint16_t t) override {
    uint16_t canvas[NUM_PIXELS] = {};

    const int step = stepsAt(t, STEPS);
    for (uint32_t impulse = 0; impulse < IMPULSES; impulse++) {
      const int born = (int)impulse * IMPULSE_PERIOD;
      if (born > step) {
        break;
      }

      const int at = (int)noiseMax(seed, impulse, 0, NUM_PIXELS - 1);
      deposit(canvas, at, 1, step - born);
      deposit(canvas, at, -1, step - born);
    }

    draw(canvas);
  }

  int tag() override { return 1014; }

private:
  // Pixels between the two ends, which is what a front crosses and what it
  // reflects between.
  static constexpr int LENGTH = NUM_PIXELS - 1;

  // The two add to full output, so the crest of a fresh impulse is the brightest
  // the strip goes and everything else is measured down from it.
  static constexpr int MEDIUM_VALUE = 16;
  static constexpr int PEAK = 255 - MEDIUM_VALUE;

  // Where a front is retired rather than faded through. A fade steps visibly
  // below about 32 duty and reads as a run of discrete stops below 8, so the
  // stretch from here to nothing is four or five of them however it is written.
  // One disappearance is better than that, and it happens at three times the
  // medium, which reads as the front breaking rather than guttering.
  static constexpr int MIN_AMPLITUDE = 32;

  // A pixel of travel costs one and a reflection costs thirty, which puts about
  // a third of a front's life on the two ends and the rest on the water. One
  // currency because it is one quantity: what reaches the eye is what has not
  // gone somewhere else.
  static constexpr int SPREAD_LOSS = 1;
  static constexpr int REFLECTION_LOSS = 30;

  // The longest a front can last. Its first end is at most LENGTH away and there
  // is one every LENGTH after that, so by three lengths it has reflected at
  // least twice and the peak does not cover that.
  static constexpr int MAX_LIFETIME = 3 * LENGTH + 1;
  static_assert(PEAK - MAX_LIFETIME * SPREAD_LOSS - 2 * REFLECTION_LOSS < MIN_AMPLITUDE,
                "a front outlives the tail the run leaves for it");

  // Fifteen seconds of water. A front lives about three of them, so this is
  // long enough that a stretch of the run is fronts crossing fronts rather than
  // one impulse at a time, and short enough that it is still one disturbance
  // settling rather than weather.
  static constexpr int RUN_MS = 15000;

  // A front covers a pixel a step, so the step is the speed of the medium rather
  // than a frame interval: a length of strip in a second. The run is not eased,
  // so the steps it holds are the plain division.
  static constexpr int MS_PER_PIXEL = 20;
  static constexpr int STEPS = RUN_MS / MS_PER_PIXEL;

  // How many disturbances a run is, spread across it rather than dropped at a
  // fixed interval and however many that leaves room for. The last one is given
  // its whole life inside the run, so the water is still at both ends of it.
  static constexpr uint32_t IMPULSES = 9;
  static constexpr int IMPULSE_PERIOD = (STEPS - MAX_LIFETIME) / ((int)IMPULSES - 1);
  static_assert(((int)IMPULSES - 1) * IMPULSE_PERIOD + MAX_LIFETIME <= STEPS,
                "the last front is still running when the run ends");

  // The reflected path is the triangle the unfolded one traces, and the count of
  // reflections is the count of ends reached, which is the first and then one
  // every length.
  void deposit(uint16_t canvas[], int at, int direction, int travelled) const {
    const int toFirstEnd = direction > 0 ? LENGTH - at : at;
    const int reflections = travelled > toFirstEnd
                          ? 1 + (travelled - toFirstEnd - 1) / LENGTH
                          : 0;

    const int amplitude = PEAK - travelled * SPREAD_LOSS - reflections * REFLECTION_LOSS;
    if (amplitude < MIN_AMPLITUDE) {
      return;
    }

    const int period = 2 * LENGTH;
    const int unfolded = at + direction * travelled;
    const int wrapped = ((unfolded % period) + period) % period;
    const int position = wrapped <= LENGTH ? wrapped : period - wrapped;

    // A crest with shoulders. One pixel has no shape to it and three is as wide
    // as fifty holds while the front still crosses them in a second. The
    // shoulders fold at the ends the way the crest does, so a front arriving at
    // a wall piles onto it rather than losing half of itself over the edge.
    raise(canvas, position, amplitude);
    raise(canvas, position - 1, amplitude / 2);
    raise(canvas, position + 1, amplitude / 2);
  }

  static void raise(uint16_t canvas[], int at, int amplitude) {
    const int folded = at < 0 ? -at : (at > LENGTH ? 2 * LENGTH - at : at);
    canvas[folded] += (uint16_t)amplitude;
  }

  // Crests add and then separate again, which is the whole reason the canvas
  // holds amplitude rather than a level: two fronts crossing make one taller
  // one. The medium saturates at full output, since there is nothing above it.
  void draw(const uint16_t canvas[]) const {
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      const uint16_t displacement = canvas[i] > PEAK ? PEAK : canvas[i];
      actual_led_strip_set_pixel_hsv(strip, i, hue, (uint8_t)(MEDIUM_VALUE + displacement));
    }
  }

  uint16_t hue;
  uint32_t seed;
};

#endif
