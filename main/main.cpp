// esp32 stuff
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bootloader_random.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

// idf libraries
#include "led_strip.h"

// my animations
#include "Constants.h"
#include "BlinkComplement.hpp"
#include "Bounce.hpp"
#include "CellularAutomaton.hpp"
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
CellularAutomaton rule30(led_strip, 30);
CellularAutomaton rule110(led_strip, 110);

// Named here rather than on Animation, since direction and the BlinkComplement
// flags distinguish entries that share a class.
struct Entry {
  Animation* animation;
  const char* name;
};

Entry animations[] = {
  {&fullRainbowForward, "full rainbow fwd"},
  {&fullRainbowBackward, "full rainbow rev"},
  {&rainbowSliceForward, "rainbow slice fwd"},
  {&rainbowSliceBackward, "rainbow slice rev"},
  {&dropInForward, "drop in"},
  {&dropOffForward, "drop off"},
  {&fillInForward, "fill in fwd"},
  {&fillInBackwards, "fill in rev"},
  {&blinkComplementDefinedColors, "blink complement, palette"},
  {&blinkComplementAllHues, "blink complement, all hues"},
  {&bounce, "bounce"},
  {&twinkle, "twinkle"},
  {&multiRainbowForwards, "multi rainbow fwd"},
  {&multiRainbowBackwards, "multi rainbow rev"},
  {&blinkComplementDefinedColorsEvolution, "blink complement, palette, evolving"},
  {&blinkComplementAllHuesEvolution, "blink complement, all hues, evolving"},
  {&rule30, "rule 30"},
  {&rule110, "rule 110"},
};

static void configure_led(void) {
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = PIN_WS2812B;
  strip_config.max_leds = NUM_PIXELS;
  strip_config.led_model = LED_MODEL_WS2812;
  strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.clk_src = RMT_CLK_SRC_DEFAULT;
  rmt_config.resolution_hz = 10 * 1000 * 1000;
  rmt_config.flags.with_dma = false;

  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  led_strip_clear(led_strip);
}

void single(void) {
  // Bound by reference so that swapping the animation above cannot slice it and
  // so the object under inspection is the one the other drivers run.
  auto& animation = blinkComplementAllHuesEvolution;
  ESP_LOGI("animation", "Starting %s!", __FUNCTION__);
  animation.setup();

  int numberOfSteps = animation.steps();
  for (int step = 0; step < numberOfSteps; step++) {
    ESP_LOGI("animation", "Looping %s step %d of %d", __FUNCTION__, step, numberOfSteps);
    animation.loop();
    led_strip_refresh(led_strip);
    delay(animation.getDelay());
  }
}

void inOrder(void) {
  ESP_LOGI("animation", "Starting %s!", __FUNCTION__);

  int numberOfAnimations = (sizeof(animations) / sizeof(animations[0]));
  for (int i = 0; i < numberOfAnimations; i++) {
    ESP_LOGI("animation", "Starting %s", animations[i].name);
    animations[i].animation->setup();

    int numberOfSteps = animations[i].animation->steps();
    for (int step = 0; step < numberOfSteps; step++) {
      animations[i].animation->loop();
      led_strip_refresh(led_strip);
      delay(animations[i].animation->getDelay());
    }
  }
}

void randomlySelect(void) {
  ESP_LOGI("animation", "Starting %s", __FUNCTION__);

  int numberOfAnimations = (sizeof(animations) / sizeof(animations[0]));
  int actualAnimationIndex = esp_random_max(numberOfAnimations - 1);
  Animation* animation = animations[actualAnimationIndex].animation;
  const char* name = animations[actualAnimationIndex].name;

  ESP_LOGI("animation", "Picking %s, %d of %d", name, actualAnimationIndex, numberOfAnimations);
  animation->setup();

  int numberOfSteps = animation->steps();

  // repeat a few times to look good
  int range = animation->maxIterations() - animation->minIterations();
  int repetitionCount = esp_random_max(range) + animation->minIterations();
  if (repetitionCount > 6) { repetitionCount = 6; }
  // for (int i = 0; i < repetitionCount; i++) {
    for (int step = 0; step < numberOfSteps; step++) {
      ESP_LOGI("animation", "Looping %s step %d of %d", name, step, numberOfSteps);
      animation->loop();
      led_strip_refresh(led_strip);
      delay(animation->getDelay());
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
    // single();
    // inOrder();
    randomlySelect();
    delay(100);
  }
}
