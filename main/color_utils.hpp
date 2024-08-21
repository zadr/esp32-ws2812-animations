#ifndef COLOR_UTILS_HPP
#define COLOR_UTILS_HPP

#include "esp_random_max.h"
#include "Constants.h"
#include <math.h>

static uint16_t drift(uint16_t hue, uint8_t drift) {
    int8_t _drift = drift;
    if (esp_random() % 2 == 0) {
    _drift *= -1;
    }
    return fmod((double)(hue + ((_drift / 100.0) * HUE_MAX)), HUE_MAX);
}

#endif