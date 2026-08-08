#ifndef COLOR_UTILS_HPP
#define COLOR_UTILS_HPP

#include "esp_random_max.h"
#include "Constants.h"

// Hue is a full 16-bit wheel, so the narrowing conversion is the wrap: an
// offset past either end comes back around the other side.
static uint16_t drift(uint16_t hue, uint8_t amount) {
    int32_t offset = (amount / 100.0) * HUE_MAX;
    if (esp_random() % 2 == 0) {
        offset = -offset;
    }
    return (uint16_t)(hue + offset);
}

#endif