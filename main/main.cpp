// esp32 stuff
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bootloader_random.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

// idf libraries
#include "led_strip.h"

// my animations
#include "Constants.h"
#include "BlinkComplement.hpp"
#include "Bounce.hpp"
#include "DropIn.hpp"
#include "DropOff.hpp"
#include "FillIn.hpp"
#include "FlashWhite.hpp"
#include "MultiRainbow.hpp"
// #include "RainbowDichromatic.hpp"
#include "RainbowFull.hpp"
#include "RainbowSingleColorSlice.hpp"
#include "RainbowDichromatic.hpp"
#include "Twinkle.hpp"

// my helper functions
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
MultiRainbow multiRainbowForwards(led_strip, true);
MultiRainbow multiRainbowBackwards(led_strip, false);
DropIn dropInForward(led_strip, true);
DropIn dropInBackwards(led_strip, false);
DropOff dropOffForward(led_strip, true);
DropOff dropOffBackwards(led_strip, false);
FillIn fillInForward(led_strip, true);
FillIn fillInBackwards(led_strip, false);
BlinkComplement blinkComplementDefinedColors(led_strip, false, false);
BlinkComplement blinkComplementAllHues(led_strip, true, false);
BlinkComplement blinkComplementDefinedColorsEvolution(led_strip, false, true);
BlinkComplement blinkComplementAllHuesEvolution(led_strip, true, true);
Bounce bounce(led_strip);
Twinkle twinkle(led_strip);

Animation* animations[] = {
  &fullRainbowForward, // 0
  &fullRainbowBackward, // 1
  &rainbowSliceForward, // 2
  &rainbowSliceBackward, // 3
  &dropInForward, // 4
  &dropOffForward, // 5
  &fillInForward, // 6
  &fillInBackwards, // 7
  &blinkComplementDefinedColors, // 8
  &blinkComplementAllHues, // 9
  &bounce, // 10
  &twinkle, // 11
  &multiRainbowForwards, // 12
  &multiRainbowBackwards, // 13
  &blinkComplementDefinedColorsEvolution, // 14
  &blinkComplementAllHuesEvolution, // 15
};

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

void single(void) {
  auto animation = blinkComplementAllHuesEvolution;
  ESP_LOGI("animation", "Starting %s!", __FUNCTION__);
  animation.setup();

  for (int i = 0; i < animation.steps(); i++) {
    ESP_LOGI("animation", "Looping %s step %d", __FUNCTION__, i);
    animation.loop();
    led_strip_refresh(led_strip);
    vTaskDelay(animation.getDelay());
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
  int actualAnimationIndex = esp_random_max(numberOfAnimations + 1) - 1;
  ESP_LOGI("animation", "Picking %d of %d (tag %d)", actualAnimationIndex, numberOfAnimations, animations[actualAnimationIndex]->tag());
  animations[actualAnimationIndex]->setup();

  int numberOfSteps = animations[actualAnimationIndex]->steps();

  // repeat a few times to look good
  int range = animations[actualAnimationIndex]->maxIterations() - animations[actualAnimationIndex]->minIterations();
  int repetitionCount = esp_random_max(range) + animations[actualAnimationIndex]->minIterations();
  if (repetitionCount > 6) { repetitionCount = 6; }
  // for (int i = 0; i < repetitionCount; i++) {
    for (int step = 0; step < numberOfSteps; step++) {
      ESP_LOGI("animation", "Looping %s step %d", __FUNCTION__, step);
      animations[actualAnimationIndex]->loop();
      led_strip_refresh(led_strip);
      delay(animations[actualAnimationIndex]->getDelay());
    }
  // }
}

extern "C" void app_main(void) {
  esp_wifi_stop();

  // wifi / ble not used, so pull rng from other entropy sources
  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/random.html
  bootloader_random_enable();

  configure_led();
  while (1) {
    // basic_blink();
    // single();
    // inOrder();
    randomlySelect();
    delay(100);
  }
}
