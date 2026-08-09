#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "color_utils.hpp"
#include "esp_random_max.h"

// Two travellers sharing a track. Positions and velocities are fixed point in
// 1/256 of a pixel: the chip has no FPU, and a fractional position has nothing
// to render with anyway, since hue is the only per pixel variable and a pixel is
// either lit or it is not.
class Collision : public Animation {
public:
  Collision(led_strip_handle_t& strip) : Animation(strip), frame(0) {
    for (int i = 0; i < TRAVELLERS; i++) {
      position[i] = 0;
      velocity[i] = 0;
      hue[i] = 0;
    }
  }

  void setup() override {
    frame = 0;
    launch();
  }

  int steps() override { return 1200; }

  void loop() override {
    int32_t before = position[0] - position[1];

    for (int i = 0; i < TRAVELLERS; i++) {
      position[i] += velocity[i];
      reflect(i);
    }

    // A pass through in one frame is a hit that was missed, so the crossing
    // counts as well as the touch.
    int32_t gap = position[0] - position[1];
    bool crossed = (before < 0) != (gap < 0);
    if (crossed || (magnitude(gap) <= SUBPIXEL && closing(gap))) {
      collide(before);
    }

    // A whole unit of friction every frame would strand the slowest traveller
    // within a few pixels of its launch, so it bites on a period instead.
    if (++frame % FRICTION_PERIOD == 0) {
      for (int i = 0; i < TRAVELLERS; i++) {
        applyFriction(i);
      }
    }

    // One traveller at rest is not an empty strip, it is an obstacle for the
    // other to run into. Only a pair at rest is out of moves.
    if (velocity[0] == 0 && velocity[1] == 0) {
      launch();
    }

    led_strip_clear(strip);
    for (int i = 0; i < TRAVELLERS; i++) {
      actual_led_strip_set_pixel_hsv(strip, pixelOf(position[i]), hue[i]);
    }
  }

  int getDelay() override { return 10; }

  int minIterations() override { return 1; }
  int maxIterations() override { return 2; }
  int tag() override { return 1017; }

private:
  static const int TRAVELLERS = 2;
  static const int32_t SUBPIXEL = 256;
  static const int32_t SPEED_MIN = 96;
  static const int32_t SPEED_MAX = 320;
  static const int32_t FRICTION = 1;
  static const int32_t FRICTION_PERIOD = 3;
  static const int32_t RESTITUTION_ONE = 256;

  // Speed difference at which a collision goes fully inelastic. Below it the two
  // deform each other and rebound; above it the impact goes into hauling the
  // slower body up to speed and the pair leaves together.
  static const int32_t INELASTIC_MISMATCH = 128;

  static int32_t magnitude(int32_t value) { return value < 0 ? -value : value; }

  static uint32_t pixelOf(int32_t at) {
    int32_t index = (at + SUBPIXEL / 2) / SUBPIXEL;
    if (index < 0) return 0;
    if (index > NUM_PIXELS - 1) return NUM_PIXELS - 1;
    return (uint32_t)index;
  }

  bool closing(int32_t gap) const {
    int32_t relative = velocity[0] - velocity[1];
    return gap > 0 ? relative < 0 : relative > 0;
  }

  int32_t randomSpeed() const {
    return SPEED_MIN + esp_random_max(SPEED_MAX - SPEED_MIN);
  }

  void launch() {
    position[0] = 0;
    position[1] = (NUM_PIXELS - 1) * SUBPIXEL;
    velocity[0] = randomSpeed();
    velocity[1] = -randomSpeed();

    // Complements so the two are told apart on sight, which is what makes the
    // fused hue of an inelastic hit legible as a third colour.
    hue[0] = esp_random_max(HUE_VIOLET);
    hue[1] = drift(COMPLEMENT(hue[0]), 2);
  }

  // Reflecting about the end pixel keeps both travellers on 50 pixels where they
  // meet often. Wrapping would let two heading the same way never meet at all.
  void reflect(int i) {
    const int32_t far = (NUM_PIXELS - 1) * SUBPIXEL;
    if (position[i] < 0) {
      position[i] = -position[i];
    } else if (position[i] > far) {
      position[i] = 2 * far - position[i];
    } else {
      return;
    }
    velocity[i] = -velocity[i];

    // Walls take no speed, so a bounce spends itself on hue. Without it a pair
    // that fused would stay one colour for the rest of the run.
    hue[i] = drift(hue[i], 1);
  }

  void applyFriction(int i) {
    if (velocity[i] > FRICTION) {
      velocity[i] -= FRICTION;
    } else if (velocity[i] < -FRICTION) {
      velocity[i] += FRICTION;
    } else {
      velocity[i] = 0;
    }
  }

  void collide(int32_t before) {
    int32_t mismatch = magnitude(magnitude(velocity[0]) - magnitude(velocity[1]));
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
    int32_t total = velocity[0] + velocity[1];
    int32_t exchange = restitution * (velocity[1] - velocity[0]);
    velocity[0] = (total * RESTITUTION_ONE + exchange) / (2 * RESTITUTION_ONE);
    velocity[1] = (total * RESTITUTION_ONE - exchange) / (2 * RESTITUTION_ONE);

    // Inelasticity is what fuses the pair, so it is also what fuses the colour.
    int32_t fusion = RESTITUTION_ONE - restitution;
    uint16_t first = hue[0] + ((int32_t)hue[1] - hue[0]) * fusion / (2 * RESTITUTION_ONE);
    uint16_t second = hue[1] + ((int32_t)hue[0] - hue[1]) * fusion / (2 * RESTITUTION_ONE);
    hue[0] = first;
    hue[1] = second;

    // Seated a pixel apart at the contact point in the order they arrived, so a
    // pushed pair reads as pusher behind pushed rather than as one dot.
    int32_t contact = (position[0] + position[1]) / 2;
    int32_t lead = before < 0 ? -SUBPIXEL / 2 : SUBPIXEL / 2;
    position[0] = contact + lead;
    position[1] = contact - lead;
  }

  int32_t position[TRAVELLERS];
  int32_t velocity[TRAVELLERS];
  uint16_t hue[TRAVELLERS];
  uint32_t frame;
};

#endif
