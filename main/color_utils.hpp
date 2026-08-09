#ifndef COLOR_UTILS_HPP
#define COLOR_UTILS_HPP

#include "esp_random.h"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"

// Relative luminance of the three dies at equal duty, as the WS2812B datasheet's
// typical luminous intensities: 550-700, 1100-1400 and 200-400 mcd, so 1 : 2 :
// 0.5. Millicandela is photometric, so the eye is already in these; what is not
// is that a saturated blue reads brighter than its luminance, which pushes the
// true figures flatter still. Equal thirds is the far end of that, and the two
// ends differ by a factor of nine on the dimmed side of a blue pair, so this
// wants measuring against the strip rather than reasoning about.
static const uint32_t LUMA_RED = 2;
static const uint32_t LUMA_GREEN = 4;
static const uint32_t LUMA_BLUE = 1;

struct Shade {
    uint16_t hue;
    uint8_t value;
};

// Hue is a full 16-bit wheel, so the narrowing conversion is the wrap: an
// offset past either end comes back around the other side.
//
// The caller supplies which way it goes, so a step inside a run can drift the
// same way every time it is walked through by handing in a hash of its own index
// rather than fresh entropy.
static uint16_t driftBy(uint16_t hue, uint8_t amount, uint32_t entropy) {
    int32_t offset = (amount / 100.0) * HUE_MAX;
    if (entropy % 2 == 0) {
        offset = -offset;
    }
    return (uint16_t)(hue + offset);
}

static uint16_t drift(uint16_t hue, uint8_t amount) {
    return driftBy(hue, amount, esp_random());
}

// Half the wheel, which is exactly the channel-wise inverse of the colour: three
// of the six sectors along, so the channels at full and at nothing swap and the
// ramp between them reverses about the same sector position. Exact rather than
// near, since the ramp is that position rather than a value divided out of it.
static uint16_t complementHue(uint16_t hue) {
    return (uint16_t)(hue + (HUE_MAX + 1) / 2);
}

// The duties the renderer will emit for this pixel, weighted by die. Red's
// square root is part of what is emitted, so it belongs here too.
//
// Both products are carried at the renderer's own width and brought to a common
// scale of one duty step per 256, which is where red's root runs out of bits
// anyway. Rounding to duty steps here instead would sum three floors and then
// compare the result against a target built from three more.
static uint32_t weightedOutput(uint16_t hue, uint8_t value) {
    uint32_t r = 0; uint32_t g = 0; uint32_t b = 0;
    hue_to_rgb16(hue, r, g, b);

    return LUMA_RED * ((r * red_level(value)) >> 16)
         + LUMA_GREEN * ((g * value) >> 8)
         + LUMA_BLUE * ((b * value) >> 8);
}

// Output climbs with value, so the nearest match is a bisection. Red's root
// leaves no closed form to solve, and 8 probes is cheaper than fitting one.
static uint8_t valueFor(uint16_t hue, uint32_t target) {
    uint32_t low = 0;
    uint32_t high = 255;

    while (low < high) {
        uint32_t mid = (low + high) / 2;
        if (weightedOutput(hue, (uint8_t)mid) < target) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    const uint32_t reached = weightedOutput(hue, (uint8_t)low);
    if (reached < target) {
        return (uint8_t)low; // full output is still short of it
    }
    if (low > 0 && target - weightedOutput(hue, (uint8_t)(low - 1)) < reached - target) {
        return (uint8_t)(low - 1);
    }
    return (uint8_t)low;
}

// Bring a set to one output, which the dimmest member at the asked-for value
// sets, since dimming is always available where brightening runs into the top of
// the range. Hues go in, values come out.
//
// The level is returned because a set is rarely the end of it: a hue arrived at
// afterwards, a drifted member or two members mixed, sits at the same weight
// only if it is matched against the same figure.
static uint32_t level(Shade* shades, int count, uint8_t value) {
    uint32_t target = 0;

    for (int i = 0; i < count; i++) {
        const uint32_t output = weightedOutput(shades[i].hue, value);
        if (i == 0 || output < target) {
            target = output;
        }
    }

    for (int i = 0; i < count; i++) {
        shades[i].value = valueFor(shades[i].hue, target);
    }

    return target;
}

// Hues spaced evenly around the wheel and levelled. Two is the complement and
// three the triad, which are one rule rather than two: any division lands its
// members on a different channel mix from each other, so all of them need the
// same levelling before they read as one set.
//
// One is a set too, and answers with the hue it was given, so a caller drawing
// between one and three colours has no case to write.
static uint32_t spread(uint16_t hue, int count, Shade* out, uint8_t value = VALUE_DEFAULT) {
    if (count < 1) {
        return 0;
    }

    for (int i = 0; i < count; i++) {
        out[i].hue = (uint16_t)(hue + (uint32_t)i * (HUE_MAX + 1) / count);
    }

    return level(out, count, value);
}

#endif
