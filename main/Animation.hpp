#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "Curve.hpp"
#include "led_strip.h"
#include <stdint.h>

class Animation {
public:
    Animation(led_strip_handle_t& strip) : strip(strip) {}
    virtual ~Animation() {}

    // Everything a run is decided by, and the only place entropy is drawn. What
    // is chosen here is what duration() and render() are then answering for, so
    // nothing that varies per run may be settled in a constructor.
    virtual void setup() = 0;

    // How long the animation occupies, in milliseconds. Frame rate is the
    // driver's business, so nothing below here counts frames or knows the tick.
    //
    // Asked after setup() and re-asked every run, so it may be computed from
    // what setup() decided and may come back different each time: a Sort knows
    // its length once it has recorded, and an animation that draws a random
    // number of colours runs for as long as the colours it drew.
    virtual int duration() = 0;

    // The strip at progress t across duration(), 0 to 65535 and through curve().
    //
    // A function of t and of what setup() decided, of nothing else. There is no
    // previous t and no direction: two calls at the same t do identical work and
    // draw an identical strip, and any two values of t in any order are both
    // correct. A curve is therefore free to hold, double back, settle or
    // oscillate rather than only ramp.
    //
    // An animation with no closed form for its state walks to it from the
    // opening state on every call, in locals. Nothing render() writes may
    // outlive the call that wrote it.
    virtual void render(uint16_t t) = 0;

    virtual Curve curve() const { return CurveQuadraticInOut; }

    virtual int tag() = 0;

protected:
    // Steps taken by t, out of a run of steps. Zero is the opening state, before
    // anything has moved, and 65535 stands past the last step.
    static int stepsAt(uint16_t t, int steps) {
      return (int)(((uint32_t)t * steps) / 65535);
    }

    // Entropy inside a run would make the strip depend on how often it was asked
    // rather than on t, so a step wanting a random number hashes its own index
    // and gets the same answer however many times it is walked through. The seed
    // is drawn once in setup(), so runs still differ from one another. Lane
    // separates draws made within one step.
    static uint32_t noise(uint32_t seed, uint32_t step, uint32_t lane = 0) {
      uint32_t x = seed + step * 0x9E3779B9u + lane * 0x85EBCA6Bu;
      x ^= x >> 16;
      x *= 0x7FEB352Du;
      x ^= x >> 15;
      x *= 0x846CA68Bu;
      x ^= x >> 16;
      return x;
    }

    // Inclusive of max, as esp_random_max is.
    static uint32_t noiseMax(uint32_t seed, uint32_t step, uint32_t lane, uint32_t max) {
      return noise(seed, step, lane) % (max + 1);
    }

    led_strip_handle_t& strip;
};

#endif
