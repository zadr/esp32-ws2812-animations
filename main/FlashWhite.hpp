#ifndef FLASHWHITE_HPP
#define FLASHWHITE_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

class FlashWhite : public Animation {
public:
    FlashWhite(led_strip_handle_t& strip) : Animation(strip) {}

    void setup() override {}

    // A burst rather than a run. The strip is at full white and there is nothing
    // in it to develop, so three seconds is already the whole of what it has;
    // held longer it stops being a punctuation mark.
    int duration() override { return RUN_MS; }

    // Which half of which flash the strip is in follows from t alone, so there is
    // no counter and no state between renders.
    void render(uint16_t t) override {
      const uint32_t halves = ((uint32_t)t * (FLASHES * 2)) / 65535;

      // Lit on the back half, so the run opens and closes dark.
      const bool lit = halves % 2 == 1;

      // White has no hue, so it cannot go through actual_led_strip_set_pixel_hsv
      // and the scales that function applies are repeated here. Unscaled white on
      // 50 pixels draws about 3A, well past what the board's USB input carries.
      const uint32_t red = lit ? 255 * RED_SCALE : 0;
      const uint32_t greenBlue = lit ? 255 * BRIGHTNESS_SCALE : 0;

      for (int i = 0; i < NUM_PIXELS; i++) {
        led_strip_set_pixel(strip, i, red, greenBlue, greenBlue);
      }
    }

    int tag() override { return 1006; }

private:
    static const int RUN_MS = 3000;

    // Dark and lit hold half a flash each. At this rate the strip reads as a
    // strobe rather than as something switching on and off, which is a
    // perceptual figure and not a frame interval. Taken against the cruise,
    // which the trapezoid holds a seventh above the mean rate.
    static const int MS_PER_FLASH = 50;
    static const int FLASHES = RUN_MS * 6 / (MS_PER_FLASH * 7);
};

#endif
