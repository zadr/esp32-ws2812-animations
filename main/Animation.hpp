#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "led_strip.h"

class Animation {
public:
    Animation(led_strip_handle_t& strip) : strip(strip) {}
    virtual ~Animation() {}

    virtual void setup() = 0;
    virtual int steps() = 0;
    virtual void loop() = 0;

    virtual int getDelay() = 0;

protected:
    led_strip_handle_t& strip;
};

#endif
