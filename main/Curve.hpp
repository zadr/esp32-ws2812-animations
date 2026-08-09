#ifndef CURVE_HPP
#define CURVE_HPP

#include <stdint.h>

// Progress shaping, 0 to 65535 in and out. The chip has no FPU, so this is all
// integer, and every curve is pinned at both ends: 0 maps to 0 and 65535 maps to
// 65535 exactly, or an animation never reaches its final state.
//
// Division is by 65535 rather than 65536 for that reason. 65536 would be a shift
// but would land the top of the range one short.
//
// A curve need not rise. render() answers for the t it is given without regard
// to the t before it, so holding, doubling back, settling and oscillating are
// all open. What the type forecloses is leaving the range: a curve that
// overshoots one or anticipates below zero has nowhere to put the excess, and
// wants a wider or signed progress before it can be written.
enum Curve : uint8_t {
  CurveLinear,
  CurveQuadraticIn,
  CurveQuadraticOut,
  CurveQuadraticInOut,
  CurveTrapezoidInOut,
};

// The two numbers a trapezoid is: how long the ends take to reach the rate the
// middle holds, and what rate they run at while they get there. Neither settles
// the other, and together they fix the cruise at
// floor + (65535 - floor) * 65535 / (65535 - ramp), over 65535, which is 7/6 as
// they stand.
//
// Duration fixes the mean rate at 1, so the cruise is above it by however much
// the ends are below. A duration that wants its animation cruising at the rate
// it was tuned at is therefore 7/6 of the one a constant rate would have asked
// for; the extra is what the ramps occupy.

// In t units, 0 to 32767, and zero is linear. Half the run ramps as this stands,
// a quarter at each end.
static const uint16_t TRAPEZOID_RAMP = 65535 / 4;

// As a fraction of the average rate, 1 to 65535, 65535 being linear. Arriving at
// rest is what reads as a pause rather than as an ease: progress resolves to one
// of a few hundred steps, so a rate approaching zero holds the opening step
// across many frames and the strip is genuinely still, for longer the more steps
// the animation has. A floor bounds that hold at 65535 / floor times an average
// step, whatever the animation and however long the ramp.
static const uint16_t TRAPEZOID_FLOOR = 65535 / 2;

// t squared needs 32 bits, and the rounding term has to fit alongside it:
// 65535 * 65535 + 32767 is 4294868992, inside a uint32_t with room to spare.
static uint16_t quadraticIn(uint16_t t) {
  return (uint16_t)(((uint32_t)t * t + 32767) / 65535);
}

static uint16_t quadraticOut(uint16_t t) {
  return (uint16_t)(65535 - quadraticIn(65535 - t));
}

// Half a curve at each end, each doubled in t and halved in output. The halves
// are built from the doubled remainder rather than from quadraticIn so the seam
// lands exactly: 32767 arrives from below and 32768 from above.
static uint16_t quadraticInOut(uint16_t t) {
  const uint32_t doubled = (uint32_t)(t < 32768 ? t : 65535 - t) * 2;
  const uint32_t half = (doubled * doubled + 65535) / (2 * 65535);
  return (uint16_t)(t < 32768 ? half : 65535 - half);
}

// Constant rate through the middle, ramped to and from a floor at the ends.
//
// Built from the folded remainder like quadraticInOut, so the midpoint seam
// neither gaps nor repeats. The two pieces of a half meet in one expression at
// u == ramp, since both reduce to 65535 * ramp over the plateau divisor there,
// so the ramp seams land exactly as well.
//
// The floor is blended in at the position level rather than written into the
// profile. A constant added to a rate across the whole run adds a multiple of t
// to the position and leaves the total unchanged, so the shaped part carries
// what is left of the run and both ends stay exact.
//
// Widths: the plateau numerator peaks at 65535 * 65534 plus a rounding term
// under 65535, which is inside a uint32_t. The ramp squares a value already
// normalised to the full range, so it borrows quadraticIn's headroom rather than
// squaring u against 65535 a second time. The blend weights two values no larger
// than 32767 by weights summing to 65535, so it lands under 65535 * 32768 too.
static uint16_t trapezoidInOut(uint16_t t, uint16_t ramp, uint16_t floorRate) {
  const uint32_t u = t < 32768 ? t : 65535 - t;
  const uint32_t plateau = 2 * (65535 - (uint32_t)ramp);

  uint32_t shaped;
  if (u < ramp) {
    const uint16_t within = (uint16_t)((u * 65535 + ramp / 2) / ramp);
    shaped = ((uint32_t)quadraticIn(within) * ramp + plateau / 2) / plateau;
  } else {
    shaped = (65535 * (2 * u - ramp) + plateau / 2) / plateau;
  }

  const uint32_t half = ((uint32_t)floorRate * u + (65535 - floorRate) * shaped + 32767) / 65535;
  return (uint16_t)(t < 32768 ? half : 65535 - half);
}

// Adding a curve is a value above and a case here. The driver reads neither.
static uint16_t curved(Curve curve, uint16_t t) {
  switch (curve) {
    case CurveQuadraticIn:    return quadraticIn(t);
    case CurveQuadraticOut:   return quadraticOut(t);
    case CurveQuadraticInOut: return quadraticInOut(t);
    case CurveTrapezoidInOut: return trapezoidInOut(t, TRAPEZOID_RAMP, TRAPEZOID_FLOOR);
    default:                  return t;
  }
}

#endif
