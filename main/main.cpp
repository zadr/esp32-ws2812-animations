// esp32 stuff
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

// my stuff
#include "Constants.h"
#include "Blinkvolution.hpp"
#include "BlinkComplement.hpp"
#include "FillIn.hpp"
#include "FlashWhite.hpp"
#include "RainbowFull.hpp"
#include "RainbowSingleColorSlice.hpp"
#include "RainbowDichromatic.hpp"

#include "esp_random_max.h"

static uint8_t s_led_state = 0;
static led_strip_handle_t led_strip;

// animation inits
// Array of animation objects
// FlashWhite flashWhite(ws2812b);
FullRainbow fullRainbowForward(led_strip, true);
FullRainbow fullRainbowBackward(led_strip, false);
RainbowSingleColorSlice rainbowSliceForward(led_strip, true);
RainbowSingleColorSlice rainbowSliceBackward(led_strip, false);
RainbowDichromatic rainbowDichromaticForwards(led_strip, true);
RainbowDichromatic rainbowDichromaticBackwards(led_strip, false);
FillIn fillInForward(led_strip, true);
FillIn fillInBackwards(led_strip, false);
BlinkComplement blinkComplementRandomHueConsistently(led_strip, false, true, false);
BlinkComplement blinkComplementRandomHueDifferently(led_strip, true, true, false);
Blinkvolution blinkvolution(led_strip);

Animation* animations[] = {
  &fullRainbowForward, // 0
  &fullRainbowBackward, // 1
  &fillInForward, // 2
  &fillInBackwards, // 3
  &rainbowSliceForward, // 4
  &rainbowSliceBackward, // 5
  // &rainbowDichromaticForwards, // lol
  // &rainbowDichromaticBackwards, // also lol
  // &blinkComplementRandomHueConsistently, // needs random
  // &blinkComplementRandomHueDifferently, // needs random
  // &blinkvolution,
};

Animation* currentAnimation = nullptr;

static void configure_led(void) {
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = 23;
  strip_config.max_leds = NUM_PIXELS;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.flags.with_dma = false;

  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  led_strip_clear(led_strip);
}

void basic_blink(void) {
  while (1) {
    if (s_led_state) {
      for (int i = 0; i < NUM_PIXELS; i++) {
        led_strip_set_pixel(led_strip, i, 80, 16, 16);
      }
    } else {
      for (int i = 0; i < NUM_PIXELS; i++) {
        led_strip_set_pixel(led_strip, i, 16, 16, 80);
      }
    }
    led_strip_refresh(led_strip);
    s_led_state = !s_led_state;
    delay(CONFIG_BLINK_PERIOD);
  }
}

void rainbow(void) {
  ESP_LOGI("animation", "Starting %s!", __FUNCTION__);
  dropInForward.setup();

  for (int i = 0; i < dropInForward.steps(); i++) {
    ESP_LOGI("animation", "Looping %s step %d", __FUNCTION__, i);
    dropInForward.loop();
    led_strip_refresh(led_strip);
    vTaskDelay(dropInForward.getDelay());
  }
}

void inOrder(void) {
  ESP_LOGI("animation", "Starting %s!", __FUNCTION__);

  int numberOfAnimations = (sizeof(animations) / sizeof(animations[0]));
  for (int i = 0; i < numberOfAnimations; i++) {
    animations[i]->setup();

    int numberOfSteps = animations[i]->steps();
    for (int step = 0; step < numberOfSteps; step++) {
      animations[i]->loop();
      led_strip_refresh(led_strip);
      delay(animations[i]->getDelay());
    }
  }
}

void randomlySelect(void) {
  ESP_LOGI("animation", "Starting %s", __FUNCTION__);

  int numberOfAnimations = (sizeof(animations) / sizeof(animations[0]));
  int actualAnimationIndex =  esp_random_max(numberOfAnimations);
  ESP_LOGI("animation", "Picking %d of %d", actualAnimationIndex, numberOfAnimations);
  animations[actualAnimationIndex]->setup();

  // if (currentAnimation != nullptr) {
  //   AnimatedPrepareForAnimation animatedPrepareForAnimation(ws2812b, currentAnimation, animations[actualAnimationIndex]);
  //   animatedPrepareForAnimation.setup();
  //   for (int step = 0; step < animatedPrepareForAnimation.steps(); step++) {
  //     animatedPrepareForAnimation.loop();
  //     led_strip_refresh(led_strip);
  //     delay(25);
  //   }
  // }

  currentAnimation = animations[actualAnimationIndex];

  int numberOfSteps = animations[actualAnimationIndex]->steps();

  // repeat a few times to look good
  for (int i = 0; i < esp_random_max(6); i++) {
    for (int step = 0; step < numberOfSteps; step++) {
      animations[actualAnimationIndex]->loop();
      led_strip_refresh(led_strip);
      delay(animations[actualAnimationIndex]->getDelay());
    }
  }

  // // repeat a few times to look good
  // for (int animationIndex = 0 ;; animationIndex++) {
  //   for (int i = 0; i < esp_random_max(8); i++) {
  //       int actualAnimationIndex = animationIndex % (sizeof(animations) / sizeof(animations[0]));

  //       animations[actualAnimationIndex]->setup();
  //       int numberOfSteps = animations[actualAnimationIndex]->steps();
  //       for (int step = 0; step < numberOfSteps; step++) {
  //         animations[actualAnimationIndex]->loop();
  //         led_strip_refresh(led_strip);
  //         delay(animations[actualAnimationIndex]->getDelay());
  //       }
  //     }
  //   }
}

extern "C" void app_main(void) {
  configure_led();
  while (1) {
    // basic_blink();
    rainbow();
    // inOrder();
    // randomlySelect();
  }
}
