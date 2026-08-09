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
  int duration() override { return bands * MS_PER_BAND; }

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
    // Colours a run may draw. A band is a fill of the whole strip, so walking the
    // palette end to end holds the strip for minutes, and a few colours picked
    // together carry the fill as well as seven unrelated ones do.
    static const int MAX_BANDS = 3;

    // A band is fifty sweeps and each is a place shorter than the one before, so
    // it is over a thousand places the block passes through and it wants to be
    // falling through them. This is the shortest a band can run and still leave
    // a frame at every one: the block crosses a pixel a frame through the middle
    // of a band and takes two over one at the ends.
    static const int MS_PER_BAND = 16000;

    // One relationship for the whole run, so the colours that arrive have a
    // reason to be seen together and the run is as long as what it drew. Either
    // the palette's neighbours around an anchor or an even division of the wheel
    // from one, and a single colour is the division with one member rather than
    // a case of its own.
    //
    // Drift is taken on the hue handed to spread rather than on the members it
    // answers with. Up to a third of the wheel is further than the members are
    // apart, so taken per member there would be no division left. The palette
    // window takes none at all: off the anchors it is no longer the palette
    // saying which hues are neighbours.
    int chooseShades(Shade out[MAX_BANDS]) const {
      const uint16_t anchor = ANCHORS[esp_random_max(ANCHOR_COUNT - 1)];
      const int count = esp_random_max(MAX_BANDS - 1) + 1;

      // Levelling brings a set down to its dimmest member, and the more members
      // it has the further that reaches, so the ceiling comes up to meet a set
      // of three rather than it running at a fraction of what a pair does.
      const uint8_t ceiling = count == MAX_BANDS ? 255 : VALUE_DEFAULT;

      if (count > 1 && esp_random_max(1) == 0) {
        analogous(anchor, count, out, ceiling);
        return count;
      }

      spread(driftBy(anchor, esp_random_max(30), esp_random()), count, out, ceiling);
      return count;
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
