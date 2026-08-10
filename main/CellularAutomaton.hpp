#ifndef CELLULARAUTOMATON_HPP
#define CELLULARAUTOMATON_HPP

#include "esp_log.h"
#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random.h"
#include "esp_random_max.h"
#include "HueDrift.hpp"

// One bit per pixel. The target has no integer this wide, so the ring is two
// words with the high one masked back to the pixels that exist; a rotation is
// then compared word by word, high word first, for the smallest-rotation key to
// order states the way it does on a single word.
struct Cells {
  uint64_t word[2] = {0, 0};

  constexpr Cells() {}
  constexpr Cells(uint64_t low, uint64_t high) : word{low, high} {}

  static constexpr Cells mask(int bits) {
    return bits >= 128  ? Cells(~0ULL, ~0ULL)
           : bits > 64  ? Cells(~0ULL, (1ULL << (bits - 64)) - 1)
           : bits == 64 ? Cells(~0ULL, 0)
                        : Cells((1ULL << bits) - 1, 0);
  }

  constexpr bool bit(int i) const { return (word[i / 64] >> (i % 64)) & 1; }
  constexpr void set(int i) { word[i / 64] |= 1ULL << (i % 64); }

  // Taken by less than the width, so neither side handles everything falling
  // out.
  constexpr Cells operator<<(int by) const {
    if (by == 0) return *this;
    if (by >= 64) return Cells(0, word[0] << (by - 64));
    return Cells(word[0] << by, (word[1] << by) | (word[0] >> (64 - by)));
  }

  constexpr Cells operator>>(int by) const {
    if (by == 0) return *this;
    if (by >= 64) return Cells(word[1] >> (by - 64), 0);
    return Cells((word[0] >> by) | (word[1] << (64 - by)), word[1] >> by);
  }

  constexpr Cells operator&(const Cells& other) const {
    return Cells(word[0] & other.word[0], word[1] & other.word[1]);
  }

  constexpr Cells operator|(const Cells& other) const {
    return Cells(word[0] | other.word[0], word[1] | other.word[1]);
  }

  constexpr bool operator==(const Cells& other) const {
    return word[0] == other.word[0] && word[1] == other.word[1];
  }

  constexpr bool operator<(const Cells& other) const {
    return word[1] != other.word[1] ? word[1] < other.word[1] : word[0] < other.word[0];
  }
};

// An elementary automaton, one generation at a time. The rule is the Wolfram
// byte: bit (left << 2 | self << 1 | right) is what that neighbourhood becomes.
// The strip is a ring, so a pattern running off one end arrives at the other
// rather than dying against a dead edge, which 50 cells cannot spare.
//
// Constructed without a rule it draws one of the 256 per run and logs it, so a
// rule worth keeping can be found by watching and then pinned by number.
class CellularAutomaton : public Animation {
public:
  // Rule 0 leaves nothing alive, so it can stand for no pin at all.
  CellularAutomaton(Frame& strip, uint8_t pinned = 0)
    : Animation(strip), pinned(pinned), rule(0), seed(0), seedCell(0), liveHue{}, settledHue{},
      walkedTo(UNWALKED) {}

  void setup() override {
    // A new rule and a new opening, so what was walked belongs to the last run.
    walkedTo = UNWALKED;

    rule = pinned ? pinned : pick();
    ESP_LOGI("animation", "Cellular automaton rule %d", rule);

    seed = esp_random();

    // Drawn apart, so the pair has no fixed interval and no shared direction.
    // They cross, close up and pull away over a run, and which of the two tones
    // is which colour is not settled for the whole of it.
    liveHue = pickHueDrift(RUN_MS);
    settledHue = pickHueDrift(RUN_MS);

    // The single live cell is the seed these rules are known by. A ring has no
    // centre, so the index only decides where the strip cuts the pattern.
    seedCell = esp_random_max(NUM_PIXELS - 1);
  }

  int duration() override { return RUN_MS; }

  // A hue drifting at a rate cannot be handed a shaped t and still arrive where
  // it was drawn to, and a generation has nothing to ease into. Both run at the
  // rate they were sized at, from the first frame to the last.
  Curve curve() const override { return CurveLinear; }

  void render(uint16_t t) override {
    const int generations = stepsAt(t, GENERATIONS);

    // A generation follows from the one before it and from its own index, so a
    // walk that stops can be picked up where it stopped and lands where a walk
    // from the opening would have. Only a t behind what is held has to start
    // over, since a generation cannot be run backwards.
    if (walkedTo > generations || walkedTo == UNWALKED) {
      walked = opening();
      walkedTo = 0;
    }
    while (walkedTo < generations) {
      advance(walked, walkedTo);
      walkedTo++;
    }

    // Neither hue has state to walk, so both come straight off t.
    draw(walked, liveHue.at(t), settledHue.at(t));
  }

  int tag() override { return 1013; }

private:
  static_assert(NUM_PIXELS <= 128, "the ring is carried in two words");

  // A rule needs room to get past what it does first. Rule 30 closes the ring on
  // itself in half a lap and boils for the rest; rule 110 needs a full lap
  // before its left edge catches its own right one, and a pattern that has only
  // just closed has not been watched doing anything with it. Five laps at the
  // one cell per generation speed limit is what that comes to, which the assert
  // below holds the run to; the length itself is set by how long a rule is worth
  // watching, well past that floor.
  static constexpr int RUN_MS = 300000;

  // A generation is a discrete event, so this is its dwell rather than a frame
  // interval, and it is the whole of how fast the ring moves. The count falls
  // out of the run, so this is the only figure to move.
  static constexpr int MS_PER_GENERATION = 190;
  static constexpr int GENERATIONS = RUN_MS / MS_PER_GENERATION;
  static_assert(GENERATIONS >= NUM_PIXELS * 5, "the run is short of five laps of the ring");

  static constexpr Cells RING = Cells::mask(NUM_PIXELS);
  static constexpr int HISTORY = 32;
  static constexpr Cells VACANT = Cells(~0ULL, ~0ULL); // sits outside RING, so it matches nothing

  // Everything that moves during a run. Whole, so that a Ring plus the number
  // of generations standing in it is the entire walk and nothing else has to be
  // carried alongside it to resume.
  struct Ring {
    Cells cells;
    Cells previous;
    Cells history[HISTORY];
    int historyNext;
  };

  // The rules that carry a name outside their number. 30 is the chaos Wolfram
  // built Mathematica's random numbers on; 60, 90, 102 and 150 are the additive
  // ones that draw Sierpinski's triangle; 110 is the one Cook proved universal;
  // 184 is the traffic model; 54 is the class four candidate; 18, 22, 45, 73,
  // 105, 122, 126 and 182 are the class three rules the classification is
  // usually shown with.
  static constexpr uint8_t FAMOUS[] = {18, 22, 30, 45, 54, 60, 73, 90, 102, 105, 110, 122, 126, 150, 182, 184};

  // Half the draws are uniform over all 256 and half land on the table, which
  // is seventeen to one on a named rule against an unnamed one and leaves every
  // rule reachable, including the ones that die on the first generation and the
  // ones that only shift.
  static uint8_t pick() {
    if (esp_random_max(1)) {
      return FAMOUS[esp_random_max(sizeof(FAMOUS) / sizeof(FAMOUS[0]) - 1)];
    }
    return (uint8_t)esp_random_max(255);
  }

  Ring opening() const {
    Ring ring;
    ring.cells = Cells();
    ring.cells.set(seedCell);
    ring.previous = Cells();
    forget(ring);
    return ring;
  }

  void advance(Ring& ring, int generation) const {
    ring.previous = ring.cells;
    ring.cells = successor(ring.cells);
    if (recurs(ring, ring.cells)) {
      reseed(ring, generation);
    } else {
      remember(ring, ring.cells);
    }
  }

  // Cells that also lived last generation take the other hue. Age counted any
  // further would be a per pixel gradient, which at this width is a mush of
  // single lit pixels in different colours; two tones stay coarse enough to read
  // as the boundary between the settled side of a pattern and the churning side.
  //
  // The two are whatever the drifts say they are, including near enough to each
  // other to be one colour for a stretch. What separates the tones is which
  // cells they land on, and that boundary is the pattern's, not the palette's.
  void draw(const Ring& ring, uint16_t live, uint16_t settled) const {
    const Cells held = ring.cells & ring.previous;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      if (ring.cells.bit(i)) {
        actual_led_strip_set_pixel_hsv(strip, i, held.bit(i) ? settled : live);
      }
    }
  }

  Cells successor(Cells cells) const {
    Cells next;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      uint8_t left = cells.bit((i + NUM_PIXELS - 1) % NUM_PIXELS);
      uint8_t self = cells.bit(i);
      uint8_t right = cells.bit((i + 1) % NUM_PIXELS);
      if ((rule >> ((left << 2) | (self << 1) | right)) & 1) {
        next.set(i);
      }
    }
    return next;
  }

  static Cells turn(Cells state, uint16_t by) {
    return by == 0 ? state : (((state << by) | (state >> (NUM_PIXELS - by))) & RING);
  }

  // A rotation of a past generation is as spent as an exact repeat: the rule
  // reads the same at every cell, so the ring would replay that stretch shifted
  // along the strip forever. Rule 110 settles into a drifting background this
  // way, which an exact comparison would never catch. A rule that is only a
  // shift is a rotation of the generation before it, so it trips this at once
  // and keeps tripping, and what the strip shows for those is the reseed.
  //
  // Two states are rotations of one another exactly when their smallest
  // rotations are equal, so the history holds smallest rotations and the check
  // costs one comparison per entry rather than fifty. Turning every state
  // against every entry was four fifths of the cost of a generation, which is
  // the difference between a walk that can be repeated every frame and one that
  // cannot.
  static Cells smallestTurn(Cells state) {
    Cells smallest = state;
    for (uint16_t by = 1; by < NUM_PIXELS; by++) {
      const Cells turned = turn(state, by);
      if (turned < smallest) smallest = turned;
    }
    return smallest;
  }

  static bool recurs(const Ring& ring, Cells state) {
    const Cells key = smallestTurn(state);
    for (int h = 0; h < HISTORY; h++) {
      if (ring.history[h] == key) return true;
    }
    return false;
  }

  // Random, not another single cell: a stalled ring would otherwise replay the
  // opening it just stalled from. Drawn against the generation it stalled at, so
  // a stall reseeds the same way however often the run is walked through.
  void reseed(Ring& ring, int generation) const {
    ring.cells = Cells(((uint64_t)noise(seed, generation, 0) << 32) | noise(seed, generation, 1),
                       ((uint64_t)noise(seed, generation, 2) << 32) | noise(seed, generation, 3)) & RING;
    ring.previous = Cells();
    forget(ring);
  }

  static void remember(Ring& ring, Cells state) {
    ring.history[ring.historyNext] = smallestTurn(state);
    ring.historyNext = (ring.historyNext + 1) % HISTORY;
  }

  static void forget(Ring& ring) {
    for (int h = 0; h < HISTORY; h++) {
      ring.history[h] = VACANT;
    }
    ring.historyNext = 0;
    remember(ring, ring.cells);
  }

  static constexpr int UNWALKED = -1;

  const uint8_t pinned;
  uint8_t rule;
  uint32_t seed;
  uint16_t seedCell;
  HueDrift liveHue;
  HueDrift settledHue;

  // How far the run has been walked, and where it got to. Read only through
  // render(), which either extends the walk or throws it away and starts again,
  // so what the strip shows still depends on t alone.
  Ring walked;
  int walkedTo;
};

#endif
