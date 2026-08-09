#ifndef RAINBOWDICHROMATIC_HPP
#define RAINBOWDICHROMATIC_HPP

#include "led_strip.h"
#include "Constants.h"
#include "Animation.hpp"
#include "actual_led_strip_set_pixel_hsv.h"

// Two hues on the strip at once, the third arriving at the far end as the first
// leaves. The bands are laid end to end down a tape that the strip is a sliding
// window onto, so where the run has got to is a distance along the tape and the
// hue at a pixel is read from it rather than shifted into place.
class RainbowDichromatic : public Animation {
public:
    RainbowDichromatic(led_strip_handle_t& strip, bool forward)
        : Animation(strip), forward(forward) {}

    ~RainbowDichromatic() {}

    // The tape is the same every run: the palette is fixed and the direction is
    // constructed, so there is nothing here to decide and no entropy to draw.
    void setup() override {}

    // A step moves the queue one pixel, so the run is a distance rather than a
    // count.
    int duration() override { return SHIFTS * MS_PER_PIXEL; }

    void render(uint16_t t) override {
        const uint32_t travelled = (uint32_t)stepsAt(t, SHIFTS);

        for (int i = 0; i < NUM_PIXELS; i++) {
            actual_led_strip_set_pixel_hsv(strip, i, hueAt(travelled + i));
        }
    }

    int tag() override { return 1007; }

private:
    // A pair holds the strip for NUM_PIXELS - 1 shifts and there are seven of
    // them, so the run is a little over two laps of the palette.
    static const int SHIFTS = 720;
    static const int MS_PER_PIXEL = 20;

    // One short of the strip, so a band has just crossed it when the next one
    // starts.
    static constexpr uint32_t BAND_LENGTH = NUM_PIXELS - 1;

    // Both ends stated per band rather than chained through a list of stops,
    // since a lap has to cross the top of the wheel somewhere: forward closes
    // at HUE_MAX, which is red arrived at from below.
    static constexpr uint16_t FORWARD[][2] = {
        {HUE_RED, HUE_ORANGE},
        {HUE_ORANGE, HUE_YELLOW},
        {HUE_YELLOW, HUE_GREEN},
        {HUE_GREEN, HUE_BLUE},
        {HUE_BLUE, HUE_INDIGO},
        {HUE_INDIGO, HUE_VIOLET},
        {HUE_VIOLET, HUE_MAX},
    };

    static constexpr uint16_t BACKWARD[][2] = {
        {HUE_VIOLET, HUE_INDIGO},
        {HUE_INDIGO, HUE_BLUE},
        {HUE_BLUE, HUE_GREEN},
        {HUE_GREEN, HUE_YELLOW},
        {HUE_YELLOW, HUE_ORANGE},
        {HUE_ORANGE, HUE_RED},
        {HUE_RED, HUE_VIOLET},
    };

    static constexpr uint32_t BANDS = sizeof(FORWARD) / sizeof(FORWARD[0]);

    // Which band a position falls in and how far into it are the same division.
    uint16_t hueAt(uint32_t position) const {
        const uint16_t (*bands)[2] = forward ? FORWARD : BACKWARD;
        const uint16_t* band = bands[(position / BAND_LENGTH) % BANDS];

        return ramp(band[0], band[1], position % BAND_LENGTH);
    }

    // Quadratic in the distance across the band, so the arriving hue is crowded
    // into the end of the ramp and the strip holds the hue it is leaving for
    // most of the band's length.
    //
    // Integer throughout: this core has no hardware float, and the widest
    // product is a full wheel against the square of the band, well inside 32
    // bits.
    static uint16_t ramp(uint16_t from, uint16_t to, uint32_t within) {
        const int32_t span = (int32_t)to - (int32_t)from;
        return (uint16_t)(from + span * (int32_t)(within * within) / (int32_t)(BAND_LENGTH * BAND_LENGTH));
    }

    const bool forward;
};

#endif
