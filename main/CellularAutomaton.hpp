#ifndef CELLULARAUTOMATON_HPP
#define CELLULARAUTOMATON_HPP

#include "led_strip.h"
#include "esp_log.h"
#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random.h"
#include "esp_random_max.h"

// An elementary automaton, one generation per frame. The rule is the Wolfram
// byte: bit (left << 2 | self << 1 | right) is what that neighbourhood becomes.
// The strip is a ring, so a pattern running off one end arrives at the other
// rather than dying against a dead edge, which 50 cells cannot spare.
//
// Constructed without a rule it draws one per run from the table below and logs
// it, so a rule worth keeping can be found by watching and then pinned by
// number.
class CellularAutomaton : public Animation {
public:
  // Rule 0 leaves nothing alive, so it can stand for no pin at all.
  CellularAutomaton(led_strip_handle_t& strip, uint8_t pinned = 0)
    : Animation(strip), pinned(pinned), rule(0), cells(0), previous(0), historyNext(0), baseHue(0) {}

  void setup() override {
    rule = pinned ? pinned : pick();
    ESP_LOGI("animation", "Cellular automaton rule %d", rule);

    baseHue = esp_random_max(HUE_VIOLET);
    previous = 0;

    // The single live cell is the seed these rules are known by, and the one the
    // pool was measured from. A ring has no centre, so the index only decides
    // where the strip cuts the pattern.
    cells = 1ULL << esp_random_max(NUM_PIXELS - 1);
    forget();
  }

  // Five laps at the one cell per generation speed limit. Rule 30 closes the
  // ring on itself in half a lap and boils for the rest; rule 110 needs a full
  // lap before its left edge catches its own right one.
  int steps() override { return NUM_PIXELS * 5; }

  void loop() override {
    led_strip_clear(strip);

    uint64_t held = cells & previous;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      if ((cells >> i) & 1) {
        actual_led_strip_set_pixel_hsv(strip, i, ((held >> i) & 1) ? heldHue() : baseHue);
      }
    }

    previous = cells;
    cells = generation();
    if (recurs(cells)) {
      reseed();
    } else {
      remember(cells);
    }

    baseHue = (baseHue + HUE_SPAN / steps()) % HUE_SPAN;
  }

  int getDelay() override { return 80; }

  int minIterations() override { return 1; }
  int maxIterations() override { return 2; }
  int tag() override { return 1013; }

private:
  static_assert(NUM_PIXELS < 64, "the ring is carried in one word");

  static constexpr uint64_t RING = (1ULL << NUM_PIXELS) - 1;
  static constexpr int HISTORY = 32;
  static constexpr uint64_t VACANT = ~0ULL; // sits outside RING, so it matches nothing

  // Measured on this ring from this seed rather than taken on reputation. Every
  // rule left out either dies, freezes, or trips the cycle detector often enough
  // that the run becomes a slideshow of reseeds. Two families go early: a rule
  // that maps 000 to 1 floods all fifty pixels at full brightness on the second
  // generation, and with no per pixel value to soften it that is a flash rather
  // than a pattern; a rule that maps 000, 001, 010 and 100 all to 0 has nothing
  // left after the seed. Eight of the survivors share rule 90's evolution
  // exactly from a single cell, so rule 90 stands for all of them.
  static constexpr uint8_t POOL[] = {22, 30, 54, 60, 86, 90, 102, 106, 110, 120, 122, 124, 126, 150, 182};

  // Drawn from half the time, so roughly three times as likely each. The mirrors
  // 86, 102 and 124 stay in the pool alone: a mirrored rule is the same
  // behaviour running the other way along the strip, worth seeing but not worth
  // favouring twice.
  static constexpr uint8_t FEATURED[] = {22, 30, 54, 60, 90, 110, 126, 150, 182};

  static uint8_t pick() {
    if (esp_random_max(1)) {
      return FEATURED[esp_random_max(sizeof(FEATURED) / sizeof(FEATURED[0]) - 1)];
    }
    return POOL[esp_random_max(sizeof(POOL) / sizeof(POOL[0]) - 1)];
  }

  uint64_t generation() const {
    uint64_t next = 0;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      uint8_t left = (cells >> ((i + NUM_PIXELS - 1) % NUM_PIXELS)) & 1;
      uint8_t self = (cells >> i) & 1;
      uint8_t right = (cells >> ((i + 1) % NUM_PIXELS)) & 1;
      next |= (uint64_t)((rule >> ((left << 2) | (self << 1) | right)) & 1) << i;
    }
    return next;
  }

  static uint64_t turn(uint64_t state, uint16_t by) {
    return ((state << by) | (state >> (NUM_PIXELS - by))) & RING;
  }

  // A rotation of a past generation is as spent as an exact repeat: the rule
  // reads the same at every cell, so the ring would replay that stretch shifted
  // along the strip forever. Rule 110 settles into a drifting background this
  // way, which an exact comparison would never catch. A rule that is itself a
  // shift trips this on its first generation, which is why those are kept out of
  // the pool rather than the detector being loosened to admit them.
  bool recurs(uint64_t state) const {
    for (uint16_t by = 0; by < NUM_PIXELS; by++) {
      uint64_t turned = turn(state, by);
      for (int h = 0; h < HISTORY; h++) {
        if (history[h] == turned) return true;
      }
    }
    return false;
  }

  // Random, not another single cell: a stalled ring would otherwise replay the
  // opening it just stalled from.
  void reseed() {
    cells = (((uint64_t)esp_random() << 32) | esp_random()) & RING;
    previous = 0;
    forget();
  }

  void remember(uint64_t state) {
    history[historyNext] = state;
    historyNext = (historyNext + 1) % HISTORY;
  }

  void forget() {
    for (int h = 0; h < HISTORY; h++) {
      history[h] = VACANT;
    }
    historyNext = 0;
    remember(cells);
  }

  // Cells that also lived last generation take a neighbouring hue. Age counted
  // any further would be a per pixel gradient, which at this width is a mush of
  // single lit pixels in different colours; two tones stay coarse enough to read
  // as the boundary between the settled side of a pattern and the churning side.
  uint16_t heldHue() const { return (baseHue + HUE_SPAN / 8) % HUE_SPAN; }

  const uint8_t pinned;
  uint8_t rule;
  uint64_t cells;
  uint64_t previous;
  uint64_t history[HISTORY];
  int historyNext;
  uint16_t baseHue;
};

#endif
