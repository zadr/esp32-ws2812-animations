#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "color_utils.hpp"
#include "esp_random.h"

// Two travellers sharing a track. Positions and velocities are fixed point in
// 1/256 of a pixel: the chip has no FPU, and a fractional position has nothing
// to render with anyway, since hue is the only per pixel variable and a pixel is
// either lit or it is not.
class Collision : public Animation {
public:
  Collision(led_strip_handle_t& strip) : Animation(strip), seed(0) {}

  void setup() override { seed = esp_random(); }

  int duration() override { return RUN_MS; }

  void render(uint16_t t) override {
    Travellers pair = opening();

    const int steps = stepsAt(t, STEPS);
    for (int step = 0; step < steps; step++) {
      integrate(pair, step);
    }

    draw(pair);
  }

  int tag() override { return 1017; }

private:
  // Long enough for a throw to be spent and the pair to be thrown again several
  // times over, which is what there is to see: one collision says less about the
  // pair than the run of them a single throw ends in.
  static const int RUN_MS = 20000;

  // What the physics is tuned against. Not a frame interval the driver owes the
  // animation, but the meaning of a unit of velocity: 1/256 of a pixel per this.
  static const int MS_PER_STEP = 10;

  // Against the cruise rather than the mean, which the trapezoid's ramps put a
  // seventh below it. A step is the tick at that rate, so the middle of a run is
  // the finest the strip is shown at and the ends hold each step twice over.
  static const int STEPS = RUN_MS * 6 / (MS_PER_STEP * 7);

  static const int TRAVELLERS = 2;
  static const int32_t SUBPIXEL = 256;
  static const int32_t SPEED_MIN = 128;
  static const int32_t SPEED_MAX = 384;
  static const int32_t FRICTION = 1;
  static const int32_t FRICTION_PERIOD = 6;
  static const int32_t RESTITUTION_ONE = 256;

  // Breakaway speed. Coulomb friction takes the same bite whatever the speed, so
  // the last stretch before zero is a crawl of one pixel every several steps
  // that reads as nothing happening and is too slow to reach the other
  // traveller. Static friction is the honest way to cut it: below this the body
  // is at rest, and at eight steps to the pixel there is nothing to see lost.
  static const int32_t STOP_SPEED = 32;

  // Traveller steps of stillness a single throw is allowed before the pair is
  // thrown again.
  static const int32_t REST_LIMIT = 90;

  // Speed difference at which a collision goes fully inelastic. Below it the two
  // deform each other and rebound; above it the impact goes into hauling the
  // slower body up to speed and the pair leaves together.
  static const int32_t INELASTIC_MISMATCH = 128;

  // Draws made within one step, kept apart so a throw and a wall bounce landing
  // on the same step cannot take the same number twice.
  enum Lane : uint32_t { LaneSpeedFirst, LaneSpeedSecond, LaneHue, LaneComplement, LaneBounce };

  // Everything that moves during a run, held as a local of render() so that
  // integrating to one step leaves nothing behind for the next call to find.
  struct Travellers {
    int32_t position[TRAVELLERS];
    int32_t velocity[TRAVELLERS];
    uint16_t hue[TRAVELLERS];
    int32_t rest;
  };

  Travellers opening() const {
    Travellers pair;
    launch(pair, 0);
    return pair;
  }

  void integrate(Travellers& pair, int step) const {
    int32_t before = pair.position[0] - pair.position[1];

    for (int i = 0; i < TRAVELLERS; i++) {
      pair.position[i] += pair.velocity[i];
      reflect(pair, i, step);
    }

    // A pass through in one step is a hit that was missed, so the crossing
    // counts as well as the touch.
    int32_t gap = pair.position[0] - pair.position[1];
    bool crossed = (before < 0) != (gap < 0);
    if (crossed || (magnitude(gap) <= SUBPIXEL && closing(pair, gap))) {
      collide(pair, before);
    }

    // A whole unit of friction every step would strand the slowest traveller
    // within a few pixels of its launch, so it bites on a period instead. The
    // period is long enough that friction never stops a traveller that is still
    // finding collisions; what stops one is the run of collisions itself, and
    // friction only closes out what they have already emptied.
    if ((step + 1) % FRICTION_PERIOD == 0) {
      for (int i = 0; i < TRAVELLERS; i++) {
        applyFriction(pair, i);
      }
    }

    // One traveller at rest is an obstacle for the other to run into, which is
    // worth keeping, so rest is budgeted rather than ended on sight. Counting
    // traveller steps rather than steps spends a pair that stopped together
    // twice as fast as a lone obstacle waiting to be hit, and the budget is per
    // throw, so no throw can go quiet for longer than REST_LIMIT however the
    // stillness arrives.
    pair.rest += (pair.velocity[0] == 0) + (pair.velocity[1] == 0);
    if (pair.rest >= REST_LIMIT) {
      launch(pair, step + 1);
    }
  }

  void draw(const Travellers& pair) const {
    for (int i = 0; i < TRAVELLERS; i++) {
      actual_led_strip_set_pixel_hsv(strip, pixelOf(pair.position[i]), pair.hue[i]);
    }
  }

  static int32_t magnitude(int32_t value) { return value < 0 ? -value : value; }

  static uint32_t pixelOf(int32_t at) {
    int32_t index = (at + SUBPIXEL / 2) / SUBPIXEL;
    if (index < 0) return 0;
    if (index > NUM_PIXELS - 1) return NUM_PIXELS - 1;
    return (uint32_t)index;
  }

  static bool closing(const Travellers& pair, int32_t gap) {
    int32_t relative = pair.velocity[0] - pair.velocity[1];
    return gap > 0 ? relative < 0 : relative > 0;
  }

  // Thrown against the step it is thrown on, so a run walked straight to step
  // 900 gets the same throws in the same places as one walked there in pieces.
  void launch(Travellers& pair, int step) const {
    pair.rest = 0;
    pair.position[0] = 0;
    pair.position[1] = (NUM_PIXELS - 1) * SUBPIXEL;
    pair.velocity[0] = speed(step, LaneSpeedFirst);
    pair.velocity[1] = -speed(step, LaneSpeedSecond);

    // Complements so the two are told apart on sight, which is what makes the
    // fused hue of an inelastic hit legible as a third colour.
    pair.hue[0] = noiseMax(seed, step, LaneHue, HUE_VIOLET);
    pair.hue[1] = driftBy(COMPLEMENT(pair.hue[0]), 2, noise(seed, step, LaneComplement));
  }

  int32_t speed(int step, uint32_t lane) const {
    return SPEED_MIN + noiseMax(seed, step, lane, SPEED_MAX - SPEED_MIN);
  }

  // Reflecting about the end pixel keeps both travellers on 50 pixels where they
  // meet often. Wrapping would let two heading the same way never meet at all.
  void reflect(Travellers& pair, int i, int step) const {
    const int32_t far = (NUM_PIXELS - 1) * SUBPIXEL;
    if (pair.position[i] < 0) {
      pair.position[i] = -pair.position[i];
    } else if (pair.position[i] > far) {
      pair.position[i] = 2 * far - pair.position[i];
    } else {
      return;
    }
    pair.velocity[i] = -pair.velocity[i];

    // Walls take no speed, so a bounce spends itself on hue. Without it a pair
    // that fused would stay one colour for the rest of the run.
    pair.hue[i] = driftBy(pair.hue[i], 1, noise(seed, step, LaneBounce + i));
  }

  static void applyFriction(Travellers& pair, int i) {
    if (magnitude(pair.velocity[i]) <= STOP_SPEED) {
      pair.velocity[i] = 0;
    } else if (pair.velocity[i] > 0) {
      pair.velocity[i] -= FRICTION;
    } else {
      pair.velocity[i] += FRICTION;
    }
  }

  static void collide(Travellers& pair, int32_t before) {
    int32_t mismatch = magnitude(magnitude(pair.velocity[0]) - magnitude(pair.velocity[1]));
    int32_t restitution = RESTITUTION_ONE - (mismatch * RESTITUTION_ONE) / INELASTIC_MISMATCH;
    if (restitution < 0) {
      restitution = 0;
    }

    // Equal masses, so momentum is the plain sum and the two halves sit
    // symmetrically about it. Symmetry is what matters: at zero restitution both
    // reduce to the same expression, so a fused pair comes out with exactly
    // equal velocities and holds its spacing instead of creeping apart a unit at
    // a time. At full restitution the halves are the untouched velocities,
    // swapped.
    int32_t total = pair.velocity[0] + pair.velocity[1];
    int32_t exchange = restitution * (pair.velocity[1] - pair.velocity[0]);
    pair.velocity[0] = (total * RESTITUTION_ONE + exchange) / (2 * RESTITUTION_ONE);
    pair.velocity[1] = (total * RESTITUTION_ONE - exchange) / (2 * RESTITUTION_ONE);

    // Inelasticity is what fuses the pair, so it is also what fuses the colour.
    int32_t fusion = RESTITUTION_ONE - restitution;
    uint16_t first = pair.hue[0] + ((int32_t)pair.hue[1] - pair.hue[0]) * fusion / (2 * RESTITUTION_ONE);
    uint16_t second = pair.hue[1] + ((int32_t)pair.hue[0] - pair.hue[1]) * fusion / (2 * RESTITUTION_ONE);
    pair.hue[0] = first;
    pair.hue[1] = second;

    // Seated a pixel apart at the contact point in the order they arrived, so a
    // pushed pair reads as pusher behind pushed rather than as one dot.
    int32_t contact = (pair.position[0] + pair.position[1]) / 2;
    int32_t lead = before < 0 ? -SUBPIXEL / 2 : SUBPIXEL / 2;
    pair.position[0] = contact + lead;
    pair.position[1] = contact - lead;
  }

  uint32_t seed;
};

#endif
