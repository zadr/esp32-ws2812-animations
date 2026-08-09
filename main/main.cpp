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
#include "Curve.hpp"
#include "BlinkComplement.hpp"
#include "Bounce.hpp"
#include "CellularAutomaton.hpp"
#include "Collision.hpp"
#include "DropIn.hpp"
#include "DropOff.hpp"
#include "FillIn.hpp"
#include "FlashWhite.hpp"
#include "Interference.hpp"
#include "MultiRainbow.hpp"
#include "RainbowDichromatic.hpp"
#include "RainbowFull.hpp"
#include "RainbowSingleColorSlice.hpp"
#include "Ripple.hpp"
#include "Sort.hpp"
#include "TheaterChase.hpp"
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
RainbowDichromatic rainbowDichromaticForward(led_strip, true);
RainbowDichromatic rainbowDichromaticBackward(led_strip, false);
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
CellularAutomaton automatonAny(led_strip);
TheaterChase theaterChaseForward(led_strip, 3, true);
TheaterChase theaterChaseBackward(led_strip, 3, false);
Interference interference(led_strip);
Collision collision(led_strip);
Ripple ripple(led_strip);
Sort sortBitonicRainbow(led_strip, SortBitonic, SortRainbow);
Sort sortBitonicSegment(led_strip, SortBitonic, SortSegment);
Sort sortQuickRainbow(led_strip, SortQuick, SortRainbow);
Sort sortQuickSegment(led_strip, SortQuick, SortSegment);
Sort sortRadixRainbow(led_strip, SortRadix, SortRainbow);
Sort sortRadixSegment(led_strip, SortRadix, SortSegment);
Sort sortMergeRainbow(led_strip, SortMerge, SortRainbow);
Sort sortMergeSegment(led_strip, SortMerge, SortSegment);
Sort sortInsertionRainbow(led_strip, SortInsertion, SortRainbow);
Sort sortInsertionSegment(led_strip, SortInsertion, SortSegment);
Sort sortSelectionRainbow(led_strip, SortSelection, SortRainbow);
Sort sortSelectionSegment(led_strip, SortSelection, SortSegment);
Sort sortHeapRainbow(led_strip, SortHeap, SortRainbow);
Sort sortHeapSegment(led_strip, SortHeap, SortSegment);

// Named here rather than on Animation, since direction and the BlinkComplement
// flags distinguish entries that share a class.
struct Entry {
  Animation* animation;
  const char* name;
};

Entry fullRainbowVariants[] = {
  {&fullRainbowForward, "full rainbow fwd"},
  {&fullRainbowBackward, "full rainbow rev"},
};

Entry rainbowSliceVariants[] = {
  {&rainbowSliceForward, "rainbow slice fwd"},
  {&rainbowSliceBackward, "rainbow slice rev"},
};

Entry rainbowDichromaticVariants[] = {
  {&rainbowDichromaticForward, "dichromatic rainbow fwd"},
  {&rainbowDichromaticBackward, "dichromatic rainbow rev"},
};

Entry dropInVariants[] = {
  {&dropInForward, "drop in"},
};

Entry dropOffVariants[] = {
  {&dropOffForward, "drop off"},
};

Entry fillInVariants[] = {
  {&fillInForward, "fill in fwd"},
  {&fillInBackwards, "fill in rev"},
};

// Out of the rotation, and its four entries are one animation whenever they come
// back rather than four claims on the strip.
// Entry blinkComplementVariants[] = {
//   {&blinkComplementDefinedColors, "blink complement, palette"},
//   {&blinkComplementAllHues, "blink complement, all hues"},
//   {&blinkComplementDefinedColorsEvolution, "blink complement, palette, evolving"},
//   {&blinkComplementAllHuesEvolution, "blink complement, all hues, evolving"},
// };

Entry bounceVariants[] = {
  {&bounce, "bounce"},
};

Entry twinkleVariants[] = {
  {&twinkle, "twinkle"},
};

Entry multiRainbowVariants[] = {
  {&multiRainbowForwards, "multi rainbow fwd"},
  {&multiRainbowBackwards, "multi rainbow rev"},
};

Entry automatonVariants[] = {
  {&automatonAny, "cellular automaton"},
};

Entry theaterChaseVariants[] = {
  {&theaterChaseForward, "theater chase fwd"},
  {&theaterChaseBackward, "theater chase rev"},
};

Entry interferenceVariants[] = {
  {&interference, "interference"},
};

Entry collisionVariants[] = {
  {&collision, "collision"},
};

Entry rippleVariants[] = {
  {&ripple, "ripple"},
};

Entry sortVariants[] = {
  {&sortBitonicRainbow, "sort bitonic, rainbow"},
  {&sortBitonicSegment, "sort bitonic, segment"},
  {&sortQuickRainbow, "sort quick, rainbow"},
  {&sortQuickSegment, "sort quick, segment"},
  {&sortRadixRainbow, "sort radix, rainbow"},
  {&sortRadixSegment, "sort radix, segment"},
  {&sortMergeRainbow, "sort merge, rainbow"},
  {&sortMergeSegment, "sort merge, segment"},
  {&sortInsertionRainbow, "sort insertion, rainbow"},
  {&sortInsertionSegment, "sort insertion, segment"},
  {&sortSelectionRainbow, "sort selection, rainbow"},
  {&sortSelectionSegment, "sort selection, segment"},
  {&sortHeapRainbow, "sort heap, rainbow"},
  {&sortHeapSegment, "sort heap, segment"},
};

// One animation and the entries it is drawn as. The count comes off the array so
// that adding a variant is one line in one place.
struct Group {
  Entry* variants;
  int count;
};

template <int N>
static constexpr Group grouped(Entry (&variants)[N]) {
  return { variants, N };
}

// The draw is over animations, and a variant is picked once one has won. Flat, a
// variant was a ticket: sort holds fourteen of the thirty-four entries and so
// took 41% of the strip, and bounce, which registers one, took 2.9%. Fifteen
// animations is 6.7% each.
Group groups[] = {
  grouped(fullRainbowVariants),
  grouped(rainbowSliceVariants),
  grouped(rainbowDichromaticVariants),
  grouped(dropInVariants),
  grouped(dropOffVariants),
  grouped(fillInVariants),
  grouped(bounceVariants),
  grouped(twinkleVariants),
  grouped(multiRainbowVariants),
  grouped(automatonVariants),
  grouped(theaterChaseVariants),
  grouped(interferenceVariants),
  grouped(collisionVariants),
  grouped(rippleVariants),
  grouped(sortVariants),
};

static const int GROUP_COUNT = sizeof(groups) / sizeof(groups[0]);

// CONFIG_FREERTOS_HZ is 100, so vTaskDelay resolves to whole 10ms ticks and this
// is the shortest interval the driver can actually hold. It is also the interval
// the fastest animations already ran at, so nothing loses smoothness by moving
// to a fixed tick; what the slower ones pay is a redraw and a transmit of a
// frame identical to the one before it, which is 1.5ms of RMT on 50 pixels.
static const int TICK_MS = 10;

// led_strip_clear transmits as well as zeroing, so an animation reaching for it
// puts a dark frame on the strip before every real one. Blanking is a statement
// about the buffer rather than about the wire, so it belongs on this side of
// render() next to the refresh, and an animation that lights part of the strip
// no longer has to ask for the rest.
static void blank(void) {
  for (int i = 0; i < NUM_PIXELS; i++) {
    led_strip_set_pixel(led_strip, i, 0, 0, 0);
  }
}

static void run(Animation* animation, const char* name) {
  // setup() decides the run and duration() reports on what it decided, so the
  // order is load bearing and the answer is only good for this run.
  animation->setup();
  const int durationMs = animation->duration();

  // Two is the floor. The first frame has to deliver 0 and the last has to
  // deliver 65535, and one frame cannot be both.
  int frames = durationMs / TICK_MS;
  if (frames < 2) {
    frames = 2;
  }

  ESP_LOGI("animation", "Running %s for %dms in %d frames", name, durationMs, frames);

  const Curve curve = animation->curve();
  for (int frame = 0; frame < frames; frame++) {
    const uint16_t progress = (uint16_t)(((uint32_t)frame * 65535) / (frames - 1));
    blank();
    animation->render(curved(curve, progress));
    led_strip_refresh(led_strip);
    delay(TICK_MS);
  }
}

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
  run(&animation, __FUNCTION__);
}

void inOrder(void) {
  for (int g = 0; g < GROUP_COUNT; g++) {
    for (int v = 0; v < groups[g].count; v++) {
      run(groups[g].variants[v].animation, groups[g].variants[v].name);
    }
  }
}

void randomlySelect(void) {
  const Group& group = groups[esp_random_max(GROUP_COUNT - 1)];
  const Entry& entry = group.variants[esp_random_max(group.count - 1)];
  run(entry.animation, entry.name);
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
