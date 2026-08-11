// esp32 stuff
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bootloader_random.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

// my animations
#include "Constants.h"
#include "Curve.hpp"
#include "Frame.hpp"
#include "Strip.hpp"
#include "Transition.hpp"
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

// One between all of them, since a single animation runs at a time and it is
// handed a blank one.
static Frame buffer;

// The frame the last animation finished on, kept for the next one to come up
// against. A finished animation is never called again, so what it left is all
// there is of it.
static Frame outgoing;
static bool haveOutgoing = false;

// animation inits
// Array of animation objects
// FlashWhite flashWhite(ws2812b);
FullRainbow fullRainbowForward(buffer, true);
FullRainbow fullRainbowBackward(buffer, false);
RainbowSingleColorSlice rainbowSliceForward(buffer, true);
RainbowSingleColorSlice rainbowSliceBackward(buffer, false);
RainbowDichromatic rainbowDichromaticForward(buffer, true);
RainbowDichromatic rainbowDichromaticBackward(buffer, false);
MultiRainbow multiRainbowForwards(buffer, true);
MultiRainbow multiRainbowBackwards(buffer, false);
DropIn dropInForward(buffer, true);
DropIn dropInBackwards(buffer, false);
DropOff dropOffForward(buffer, true);
DropOff dropOffBackwards(buffer, false);
FillIn fillInForward(buffer, true);
FillIn fillInBackwards(buffer, false);
BlinkComplement blinkComplementDefinedColors(buffer, false, false);
BlinkComplement blinkComplementAllHues(buffer, true, false);
BlinkComplement blinkComplementDefinedColorsEvolution(buffer, false, true);
BlinkComplement blinkComplementAllHuesEvolution(buffer, true, true);
Bounce bounce(buffer);
Twinkle twinkle(buffer);
CellularAutomaton automatonAny(buffer);
TheaterChase theaterChaseForward(buffer, 3, true);
TheaterChase theaterChaseBackward(buffer, 3, false);
Interference interference(buffer);
Collision collision(buffer);
Ripple ripple(buffer);
Sort sortBitonicRainbow(buffer, SortBitonic, SortRainbow);
Sort sortBitonicSegment(buffer, SortBitonic, SortSegment);
Sort sortQuickRainbow(buffer, SortQuick, SortRainbow);
Sort sortQuickSegment(buffer, SortQuick, SortSegment);
Sort sortRadixRainbow(buffer, SortRadix, SortRainbow);
Sort sortRadixSegment(buffer, SortRadix, SortSegment);
Sort sortMergeRainbow(buffer, SortMerge, SortRainbow);
Sort sortMergeSegment(buffer, SortMerge, SortSegment);
Sort sortInsertionRainbow(buffer, SortInsertion, SortRainbow);
Sort sortInsertionSegment(buffer, SortInsertion, SortSegment);
Sort sortSelectionRainbow(buffer, SortSelection, SortRainbow);
Sort sortSelectionSegment(buffer, SortSelection, SortSegment);
Sort sortHeapRainbow(buffer, SortHeap, SortRainbow);
Sort sortHeapSegment(buffer, SortHeap, SortSegment);

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

// Out of the rotation on the owner's read of the strip. A pixel is the shimmer
// hue or the background and nothing between, with a step counter for its life,
// from before set_pixel_hsv carried a value to fade one out on.
// Entry twinkleVariants[] = {
//   {&twinkle, "twinkle"},
// };

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
// variant was a ticket: sort holds fourteen of the thirty-three entries and so
// took 42% of the strip, and bounce, which registers one, took 3%. Fourteen
// animations is 7.1% each.
Group groups[] = {
  // grouped(fullRainbowVariants),
  // grouped(rainbowSliceVariants),
  // grouped(rainbowDichromaticVariants),
  // grouped(dropInVariants),
  // grouped(dropOffVariants),
  // grouped(fillInVariants),
  // grouped(bounceVariants),
  // grouped(twinkleVariants),
  // grouped(multiRainbowVariants),
  grouped(automatonVariants),
  // grouped(theaterChaseVariants),
  // grouped(interferenceVariants),
  // grouped(collisionVariants),
  // grouped(rippleVariants),
  // grouped(sortVariants),
};

static const int GROUP_COUNT = sizeof(groups) / sizeof(groups[0]);

// What goes between runs, drawn the way the rotation is drawn: an animation
// first, then which of its variants. Separate from groups, so what sits between
// runs and what the rotation is made of are two lists and neither reads off
// the other. A sort therefore reaches its fourteen the same way it would from
// the rotation, algorithm and dataset settled by the second draw.
Group interludes[] = {
  grouped(fullRainbowVariants),
  grouped(rainbowSliceVariants),
  grouped(rainbowDichromaticVariants),
  grouped(multiRainbowVariants),
  grouped(sortVariants),
};

static const int INTERLUDE_COUNT = sizeof(interludes) / sizeof(interludes[0]);

// The slot is the fixed thing here, so an animation is held to it rather than
// to the rate it was sized at: every one of these draws from t alone, so a
// longer run is the same pattern travelling slower and nothing else. Three
// turns of the wheel land at ten seconds a turn instead of six, which across a
// hundred pixels still reads as motion along the strip, and a sort spreads its
// intro, replay and settle over the slot in the proportions its own duration()
// gives them.
static const int INTERLUDE_MS = 30000;

// CONFIG_FREERTOS_HZ is 100, so vTaskDelay resolves to whole 10ms ticks and this
// is the shortest interval the driver can actually hold. It is also the interval
// the fastest animations already ran at, so nothing loses smoothness by moving
// to a fixed tick; what the slower ones pay is a redraw and a transmit of a
// frame identical to the one before it, which is 1.5ms of RMT on 50 pixels.
static const int TICK_MS = 10;

// The one the driver runs. Swapping it is the whole of changing how animations
// hand over.
static const Transition TRANSITION = TransitionCrossfade;

// Blanking is a statement about the buffer and not about the wire, so it sits
// on this side of render() next to the transmit. An animation that lights part
// of the strip does not have to ask for the rest, and clearing never costs a
// dark frame ahead of a real one.
static void blank(void) {
  buffer.clear();
}

// The frame is what an animation drew; the wire gets it here and nowhere else.
static void present(void) {
  strip_transmit(buffer);
}

// The handover from the frame the last animation left to the first frame of
// this one. The outgoing side is a still and the incoming side is held at its
// opening frame, so the tick carries one render, the same one a solo run puts on
// it, whatever either animation costs to draw.
static void transitionTo(Animation* incoming) {
  int steps = TRANSITION_MS / TICK_MS;
  if (steps < 2) {
    steps = 2;
  }

  // Step 0 is the outgoing frame alone, which the run that just ended drew and
  // the strip is still holding, so the fade starts one step in. The last step is
  // the incoming animation alone at t == 0.
  for (int step = 1; step < steps; step++) {
    const uint16_t progress = (uint16_t)(((uint32_t)step * 65535) / (steps - 1));
    blank();
    // Every curve is pinned at zero, so this is the opening frame under the one
    // the incoming animation asks for.
    incoming->render(0);
    transitioned(TRANSITION, outgoing, buffer, curved(TRANSITION_CURVE, progress));
    present();
    delay(TICK_MS);
  }
}

// An animation is asked how long it wants unless the caller is filling a slot
// of its own size, which is the one thing here that is not the animation's to
// decide.
static void run(Animation* animation, const char* name, int heldToMs = 0) {
  // setup() decides the run and duration() reports on what it decided, so the
  // order is load bearing and the answer is only good for this run. A transition
  // renders this animation, so it comes after setup() as well.
  animation->setup();
  const int durationMs = heldToMs ? heldToMs : animation->duration();

  // Two is the floor. The first frame has to deliver 0 and the last has to
  // deliver 65535, and one frame cannot be both.
  int frames = durationMs / TICK_MS;
  if (frames < 2) {
    frames = 2;
  }

  ESP_LOGI("animation", "Running %s for %dms in %d frames", name, durationMs, frames);

  // A transition lands on t == 0, so the run picks up at the step after it and
  // no frame is drawn on both sides of the seam. With nothing to come from, the
  // run opens on t == 0 itself.
  int first = 0;
  if (haveOutgoing) {
    transitionTo(animation);
    first = 1;
  }

  const Curve curve = animation->curve();
  for (int frame = first; frame < frames; frame++) {
    const uint16_t progress = (uint16_t)(((uint32_t)frame * 65535) / (frames - 1));
    blank();
    animation->render(curved(curve, progress));
    present();
    delay(TICK_MS);
  }

  // The last frame drawn was t == 65535 through a curve pinned there, so the
  // buffer already holds this animation at the end of its run and freezing it
  // costs no render.
  outgoing = buffer;
  haveOutgoing = true;
}

static void configure_led(void) {
  strip_start();

  // The strip holds its last frame across a reset of the board, so the first
  // thing it is sent is a dark one.
  blank();
  present();
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

void interlude(void) {
  const Group& group = interludes[esp_random_max(INTERLUDE_COUNT - 1)];
  const Entry& entry = group.variants[esp_random_max(group.count - 1)];
  run(entry.animation, entry.name, INTERLUDE_MS);
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
    interlude();
    delay(100);
  }
}
