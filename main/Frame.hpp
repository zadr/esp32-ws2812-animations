#ifndef FRAME_HPP
#define FRAME_HPP

#include <stdint.h>
#include <string.h>
#include "Constants.h"

// A whole strip as duties, standing apart from the wire. What a render produces
// is a value that can be held, compared or mixed, rather than an effect on the
// driver's transmit buffer that is gone as soon as the next write lands.
//
// Channels are in RGB order whatever the strip takes on the wire; the driver
// puts them in its own order on the way out.
struct Frame {
  uint8_t rgb[NUM_PIXELS][3];

  // Widths are led_strip_set_pixel's, so a caller handing in arithmetic wider
  // than a duty step loses the same bits here that it lost there.
  void set(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    rgb[index][0] = (uint8_t)red;
    rgb[index][1] = (uint8_t)green;
    rgb[index][2] = (uint8_t)blue;
  }

  void clear() {
    memset(rgb, 0, sizeof(rgb));
  }
};

#endif
