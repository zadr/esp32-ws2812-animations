#ifndef DROPIN_HPP
#define DROPIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "color_utils.hpp"
#include "esp_random.h"
#include "actual_led_strip_set_pixel_hsv.h"

class DropIn : public Animation {
public:
  DropIn(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), forward(forward), seed(0) {
  }
  ~DropIn() {}

  void setup() override { seed = esp_random(); }

  int duration() override { return positions() * MS_PER_POSITION; }

  // Sweeps shorten by one position as the pile grows, so a band is a triangular
  // number of positions and the sweep a position falls in is found by peeling
  // those lengths off in turn. That is one pass over the sweeps rather than over
  // the positions, so the whole band costs what a single sweep of it would.
  void render(uint16_t t) override {
    const int n = stepsAt(t, positions() - 1);
    const int band = n / positionsPerBand();

    int within = n % positionsPerBand();
    int sweep = 0;
    for (int length = sweeps(); within >= length; length--) {
      within -= length;
      sweep++;
    }

    draw(sweep * chunk(), NUM_PIXELS - 1 - within * chunk(), bandHue(band));
  }

  int tag() override { return 1004; }

private:
    static const int BANDS = 7;

    // What the block holds at each place it passes, so the sweep keeps its pace
    // on a strip long enough to widen the chunk.
    static const int MS_PER_POSITION = 25;

    static constexpr uint16_t ANCHORS[BANDS] = {
      HUE_RED, HUE_ORANGE, HUE_YELLOW, HUE_GREEN, HUE_BLUE, HUE_INDIGO, HUE_VIOLET,
    };

    // Chunk scales with strip length but never reaches zero, which on a short
    // strip would make the sweep stop advancing.
    static int chunk() {
      int amount = NUM_PIXELS / 72;
      return amount < 1 ? 1 : amount;
    }

    // Each sweep starts at the far end and stops on the pile, and the pile takes
    // one more place each time, so the sweeps run the full height, one short of
    // it, two short, down to one.
    static int sweeps() { return (NUM_PIXELS - 1) / chunk() + 1; }

    static int positionsPerBand() { return sweeps() * (sweeps() + 1) / 2; }

    static int positions() { return positionsPerBand() * BANDS; }

    // A sweep leaves its trail lit, and the trail is everything the block has
    // already passed: the pile below landed, and the strip above the block's
    // leading edge. That is the whole lit set, so it is rebuilt here rather
    // than accumulated in the buffer.
    void draw(int landed, int falling, uint16_t hue) const {
      led_strip_clear(strip);

      for (int m = 0; m < landed; m++) {
        actual_led_strip_set_pixel_hsv(strip, m, hue);
      }

      const int leading = falling - chunk() + 1;
      for (int i = leading < 0 ? 0 : leading; i < NUM_PIXELS; i++) {
        actual_led_strip_set_pixel_hsv(strip, i, hue);
      }
    }

    // Drift is hashed from the band rather than drawn when the band begins, so
    // a render that lands mid palette knows the colour of a band it never
    // walked through. A backward run takes the palette from the far end.
    uint16_t bandHue(int band) const {
      const uint32_t index = forward ? band : BANDS - 1 - band;
      return driftBy(ANCHORS[index], noiseMax(seed, index, 0, 30), noise(seed, index, 1));
    }

    const bool forward;
    uint32_t seed;
};

#endif
