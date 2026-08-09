#ifndef DROPIN_HPP
#define DROPIN_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "color_utils.hpp"
#include "esp_random.h"
#include "esp_random_max.h"
#include "actual_led_strip_set_pixel_hsv.h"

class DropIn : public Animation {
public:
  DropIn(led_strip_handle_t& ws2812b, bool forward)
    : Animation(ws2812b), forward(forward), shades{}, bands(0) {
  }
  ~DropIn() {}

  void setup() override { bands = chooseShades(shades); }

  // As long as the colours drawn for it, since a band is a fill of the whole
  // strip and setup() decides how many there are.
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

    draw(sweep * chunk(), NUM_PIXELS - 1 - within * chunk(), bandShade(band));
  }

  int tag() override { return 1004; }

private:
    static const int ANCHOR_COUNT = 7;

    // Colours a run may draw. A band is a fill of the whole strip, so walking the
    // palette end to end holds the strip for minutes, and a few colours picked
    // together carry the fill as well as seven unrelated ones do.
    static const int MAX_BANDS = 3;

    // What the block holds at each place it passes, so the sweep keeps its pace
    // on a strip long enough to widen the chunk.
    static const int MS_PER_POSITION = 25;

    // A third of the wheel, as complementHue takes half of it. The palette's red,
    // green and blue anchors sit within a hundred or so of these thirds, so a
    // computed triad lands where this strip's own already is.
    static constexpr uint16_t WHEEL_THIRD = (HUE_MAX + 1) / 3;

    static constexpr uint16_t ANCHORS[ANCHOR_COUNT] = {
      HUE_RED, HUE_ORANGE, HUE_YELLOW, HUE_GREEN, HUE_BLUE, HUE_INDIGO, HUE_VIOLET,
    };

    // One relationship for the whole run, so the colours that arrive have a
    // reason to be seen together and the run is as long as what it drew. All of
    // it is the shared harmony helper's business once that exists: what it
    // replaces is the body, not the shape, which is shades out and a count back.
    //
    // The rotation is drawn once and applied to every anchor rather than per
    // colour, since a drift of up to a third of the wheel taken separately would
    // leave nothing of the relationship. A partner computed from a rotated anchor
    // is as exact as one computed from the anchor, and a run of anchors rotated
    // together keeps the spacing it was tuned with.
    int chooseShades(Shade out[MAX_BANDS]) const {
      const uint8_t amount = esp_random_max(30);
      const uint32_t direction = esp_random();
      const uint16_t anchor = driftBy(ANCHORS[esp_random_max(ANCHOR_COUNT - 1)], amount, direction);

      switch (esp_random_max(3)) {
      case 0:
        out[0] = { anchor, VALUE_DEFAULT };
        return 1;

      case 1: {
        const ShadePair pair = complementPair(anchor);
        out[0] = pair.base;
        out[1] = pair.partner;
        return 2;
      }

      case 2:
        out[0] = { anchor, VALUE_DEFAULT };
        out[1] = { (uint16_t)(anchor + WHEEL_THIRD), VALUE_DEFAULT };
        out[2] = { (uint16_t)(anchor + 2 * WHEEL_THIRD), VALUE_DEFAULT };
        return 3;

      default: {
        const int length = esp_random_max(1) + 2;
        const int first = esp_random_max(ANCHOR_COUNT - length);
        for (int i = 0; i < length; i++) {
          out[i] = { driftBy(ANCHORS[first + i], amount, direction), VALUE_DEFAULT };
        }
        return length;
      }
      }
    }

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

    int positions() const { return positionsPerBand() * bands; }

    // A sweep leaves its trail lit, and the trail is everything the block has
    // already passed: the pile below landed, and the strip above the block's
    // leading edge. That is the whole lit set, so it is rebuilt here rather
    // than accumulated in the buffer.
    void draw(int landed, int falling, Shade shade) const {
      for (int m = 0; m < landed; m++) {
        actual_led_strip_set_pixel_hsv(strip, m, shade.hue, shade.value);
      }

      const int leading = falling - chunk() + 1;
      for (int i = leading < 0 ? 0 : leading; i < NUM_PIXELS; i++) {
        actual_led_strip_set_pixel_hsv(strip, i, shade.hue, shade.value);
      }
    }

    // A backward run takes the set from the far end.
    Shade bandShade(int band) const {
      return shades[forward ? band : bands - 1 - band];
    }

    const bool forward;
    Shade shades[MAX_BANDS];
    int bands;
};

#endif
