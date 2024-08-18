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
#include "DropIn.hpp"
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
DropIn dropInForward(led_strip, true);
DropIn dropInBackwards(led_strip, false);
// BlinkComplement blinkComplementRandomHueConsistently(led_strip, false, true);
// BlinkComplement blinkComplementRandomHueDifferently(led_strip, true, true);
// Blinkvolution blinkvolution(led_strip);

Animation* animations[] = {
  &fullRainbowForward,
  &fullRainbowBackward,
  &dropInForward,
  &dropInBackwards,
  &rainbowSliceForward,
  &rainbowSliceBackward,
  &rainbowDichromaticForwards,
  &rainbowDichromaticBackwards,
  // &blinkComplementRandomHueConsistently,
  // &blinkComplementRandomHueDifferently,
  // &blinkvolution,
};

Animation* currentAnimation = nullptr;

static void configure_led(void) {
  ESP_LOGI("config", "Example configured to blink addressable LED!");
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = 23;
  strip_config.max_leds = NUM_PIXELS;

  led_strip_rmt_config_t rmt_config = {};
  // rmt_config.resolution_hz = 1000 * 100 * 10; // 1000 * 800; // 800KHz
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
  fullRainbowForward.setup();

  for (int i = 0; i < fullRainbowForward.steps(); i++) {
    ESP_LOGI("animation", "Looping %s step %d", __FUNCTION__, i);
    fullRainbowForward.loop();
    led_strip_refresh(led_strip);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }
}

// void inOrder(void) {
//   ESP_LOGI("animation", "Starting %s!", __FUNCTION__);

//   while (1) {
//     int numberOfAnimations = (sizeof(animations) / sizeof(animations[0]));
//     for (int i = 0; i < numberOfAnimations; i++) {
//       animations[i]->setup();

//       int numberOfSteps = animations[i]->steps();
//       for (int step = 0; step < numberOfSteps; step++) {
//         animations[i]->loop();
//         led_strip_refresh(led_strip);
//         delay(25);
//       }
//     }
//   }
// }

// void randomlySelect(void) {
//   ESP_LOGI("animation", "Starting %s!", __FUNCTION__);

//   int numberOfAnimations = (sizeof(animations) / sizeof(animations[0]));
//   int actualAnimationIndex =  esp_random_max(numberOfAnimations);

//   animations[actualAnimationIndex]->setup();

  // if (currentAnimation != nullptr) {
  //   AnimatedPrepareForAnimation animatedPrepareForAnimation(ws2812b, currentAnimation, animations[actualAnimationIndex]);
  //   animatedPrepareForAnimation.setup();
  //   for (int step = 0; step < animatedPrepareForAnimation.steps(); step++) {
  //     animatedPrepareForAnimation.loop();
  //     led_strip_refresh(led_strip);
  //     delay(25);
  //   }
  // }

  // currentAnimation = animations[actualAnimationIndex];

  // repeat a few times to look good
  // for (int i = 0; i < esp_random_max(6); i++) {
  //   int numberOfSteps = animations[actualAnimationIndex]->steps();
  //   for (int step = 0; step < numberOfSteps; step++) {
  //     animations[actualAnimationIndex]->loop();
  //     led_strip_refresh(led_strip);
  //     delay(25);
  //   }
  // }

  // repeat a few times to look good
//   for (int animationIndex = 0 ;; animationIndex++) {
//     for (int i = 0; i < esp_random_max(8); i++) {
//         int actualAnimationIndex = animationIndex % (sizeof(animations) / sizeof(animations[0]));

//         animations[actualAnimationIndex]->setup();
//         int numberOfSteps = animations[actualAnimationIndex]->steps();
//         for (int step = 0; step < numberOfSteps; step++) {
//           animations[actualAnimationIndex]->loop();
//           led_strip_refresh(led_strip);
//           delay(25);
//         }
//       }
//     }
// }

extern "C" void app_main(void) {
  configure_led();
  // basic_blink();
  rainbow();
  // inOrder();
  // randomlySelect();
}
