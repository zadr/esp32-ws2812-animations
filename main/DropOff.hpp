#ifndef DROP_OFF_HPP
#define DROP_OFF_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "color_utils.hpp"
#include "esp_random.h"
#include "actual_led_strip_set_pixel_hsv.h"

class DropOff : public Animation {
public:
  DropOff(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), forward(forward), seed(0) {
  }
  ~DropOff() {}

  void setup() override { seed = esp_random(); }

  int duration() override { return positions() * MS_PER_POSITION; }

  // One block falls per hue and nothing is left behind it, so the strip at step
  // n is the block alone: which band it belongs to and how far down it has come
  // both divide out of n. DropIn is the counterpart that keeps what it drops.
  void render(uint16_t t) override {
    const int n = stepsAt(t, positions() - 1);
    const int band = n / restsPerBand();
    const int falling = NUM_PIXELS - 1 - (n % restsPerBand()) * chunk();

    const uint16_t hue = bandHue(band);
    for (int k = 0; k < chunk(); k++) {
      if (falling - k >= 0) {
        actual_led_strip_set_pixel_hsv(strip, falling - k, hue);
      }
    }
  }

  int tag() override { return 1012; }

private:
    // The whole palette, a band each. A band here is one block falling rather
    // than a fill of sweeps, so seven of them run for seconds where seven of
    // DropIn's run for minutes, which is why that one draws a related few
    // instead.
    static const int BANDS = ANCHOR_COUNT;

    // What the block holds at each place it lands, so the descent keeps its
    // pace on a strip long enough to widen the chunk.
    static const int MS_PER_POSITION = 50;

    // Chunk scales with strip length but never reaches zero, which on a short
    // strip would make the descent stop advancing.
    static int chunk() {
      int amount = NUM_PIXELS / 72;
      return amount < 1 ? 1 : amount;
    }

    // Places the block comes to rest on the way down. A strip the chunk does
    // not divide evenly ends on a short chunk rather than overshooting the
    // bottom.
    static int restsPerBand() { return (NUM_PIXELS - 1) / chunk() + 1; }

    static int positions() { return restsPerBand() * BANDS; }

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
