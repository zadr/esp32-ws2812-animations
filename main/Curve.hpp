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
};

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

// Adding a curve is a value above and a case here. The driver reads neither.
static uint16_t curved(Curve curve, uint16_t t) {
  switch (curve) {
    case CurveQuadraticIn:    return quadraticIn(t);
    case CurveQuadraticOut:   return quadraticOut(t);
    case CurveQuadraticInOut: return quadraticInOut(t);
    default:                  return t;
  }
}

#endif
