#ifndef TRANSITION_HPP
#define TRANSITION_HPP

#include <stdint.h>

#include "Curve.hpp"
#include "Frame.hpp"

// How one animation gives way to the next. Both pictures arrive as finished
// frames, so a transition composes strips and never asks either animation for
// anything: the outgoing one is a still and the incoming one is whatever it drew
// this tick. A wipe, a dissolve or a dip to black is a value here and a case in
// transitioned(), on the same terms as a Curve.
enum Transition : uint8_t {
  TransitionCrossfade,
};

// 500ms, which is 50 ticks. The tick fixes how many blend levels there are to
// spend, and 50 puts about 5 duty levels between consecutive steps at full
// output, under what reads as a step on a lit pixel. Longer and the two pictures
// superimposed stop reading as a handover and start reading as a picture of
// their own; against runs of 12 to 21 seconds this is a beat.
static const int TRANSITION_MS = 500;

// Both pictures are on the strip at once for the whole fade and each is at half
// strength in the middle, which is where neither is legible. So the curve should
// sit near the ends, where the strip is showing something whole, and cross the
// middle quickly. quadraticInOut leaves and arrives at rest and takes the
// midpoint at twice the average rate, the fastest crossing of the shapes on
// hand, and its ends-at-rest is free here because both pictures are stills and
// there is no motion for it to freeze.
//
// Not the trapezoid: its floor exists to stop a stepped animation holding its
// opening frame, which a fade over 256 levels has no equivalent of, and it
// cruises at 7/6 of average, so it lingers in the half-and-half longer than the
// quadratic does.
static const Curve TRANSITION_CURVE = CurveQuadraticInOut;

// Channel by channel, which is the only blend two unrelated animations have. Hue
// has no midpoint between them worth naming and interpolating it would sweep
// through colours neither one shows.
//
// Bytes are in the strip's GRB order, and a weighted sum of like channels does
// not care which order they sit in.
//
// The ends are exact: t == 0 leaves outgoing untouched and t == 65535 leaves
// incoming untouched, both after rounding.
static void crossfade(const Frame& outgoing, Frame& incoming, uint16_t t) {
  for (uint32_t i = 0; i < sizeof(incoming.wire); i++) {
    const uint32_t mixed = (uint32_t)outgoing.wire[i] * (65535 - t)
                         + (uint32_t)incoming.wire[i] * t;
    incoming.wire[i] = (uint8_t)((mixed + 32767) / 65535);
  }
}

// The strip at t through the transition, written over the incoming frame since
// that is the buffer the driver already owns.
static void transitioned(Transition transition, const Frame& outgoing, Frame& incoming, uint16_t t) {
  switch (transition) {
    default: crossfade(outgoing, incoming, t); break;
  }
}

#endif
