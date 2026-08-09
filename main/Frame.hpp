#ifndef FRAME_HPP
#define FRAME_HPP

#include <stdint.h>
#include <string.h>
#include "Constants.h"

// A whole strip as duties, and the only copy of them: what a render produces is
// a value that can be held, compared or mixed, and it is also what the RMT
// channel transmits, so there is nowhere for the two to disagree.
//
// Bytes sit in the order the WS2812B reads them, green then red then blue.
// Nothing between here and the wire permutes them.
struct Frame {
  uint8_t wire[NUM_PIXELS * 3];

  // Arguments stay in reading order whatever the strip's order is. Each is
  // taken at a duty step, so a caller handing in wider arithmetic loses the
  // same bits here that the strip would have ignored.
  void set(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    uint8_t* pixel = wire + index * 3;
    pixel[0] = (uint8_t)green;
    pixel[1] = (uint8_t)red;
    pixel[2] = (uint8_t)blue;
  }

  void clear() {
    memset(wire, 0, sizeof(wire));
  }
};

#endif
