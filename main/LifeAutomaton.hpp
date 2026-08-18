#ifndef LIFEAUTOMATON_HPP
#define LIFEAUTOMATON_HPP

#include "esp_log.h"
#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random.h"
#include "esp_random_max.h"
#include "HueDrift.hpp"
#include "LifeRules.hpp"

// Neighbour counts as the notation writes them, so a rule written here reads as
// B/S does. At file scope because a class cannot use its own member function in
// the initialiser of its own member.
static constexpr uint16_t lifeCounts(const char* digits) {
  uint16_t mask = 0;
  for (const char* digit = digits; *digit; digit++) {
    mask = (uint16_t)(mask | (1u << (*digit - '0')));
  }
  return mask;
}

// A Life-like automaton on a board ten times the strip in each direction, seen
// through a window the size of the strip that wanders over it.
//
// The family is the outer-totalistic Moore-neighbourhood rules, written B/S: a
// dead cell comes alive on the neighbour counts in birth, a live one stays on
// the counts in survive. It is the two-dimensional counterpart of the elementary
// family in the sense that the next state is a function of the neighbourhood
// alone, but the counting is what makes it two-dimensional and is the whole
// reason it is worth the second animation.
//
// A rule is drawn per run, uniformly over the members of the family that hold
// two colours on the strip for at least half a run. The two tones are how a
// cell's age is read here, so a rule that never settles and a rule that freezes
// are each a single colour for most of the time they are up, and between them
// that is a quarter of the family: a sweep of all 262144 kept 194431. The set is
// carried as the membership blob in LifeRuleSet.hpp and drawn from by rejection,
// which is a draw and a third on average. The pin is for watching one rule on
// its own.
class LifeAutomaton : public Animation {
public:
  // Bit n set is n neighbours: birth for a dead cell, survive for a live one.
  struct Rule {
    uint16_t birth;
    uint16_t survive;
  };

  // Nothing in either mask kills every cell on the first generation, so it can
  // stand for no pin at all.
  LifeAutomaton(Frame& strip, Rule pinned = Rule{0, 0})
    : Animation(strip), pinned(pinned), rule{0, 0}, seed(0), liveHue{}, settledHue{},
      bornCount(0), heldCount(0), walkedTo(UNWALKED) {}

  void setup() override {
    // A new rule and a new board, so what was walked belongs to the last run.
    walkedTo = UNWALKED;

    rule = pinned.birth || pinned.survive ? pinned : drawn();
    decode();

    char birth[10];
    char survive[10];
    notate(birth, rule.birth);
    notate(survive, rule.survive);
    ESP_LOGI("animation", "Life automaton B%s/S%s", birth, survive);

    seed = esp_random();

    // Drawn apart, so the two are colours travelling at their own rates rather
    // than one colour and an offset. They close up and pull away over a run,
    // either may be the still one, and which of the two tones is which colour is
    // not settled for the whole of it.
    liveHue = pickHueDrift(RUN_MS);
    settledHue = pickHueDrift(RUN_MS);
  }

  int duration() override { return RUN_MS; }

  // A hue drifting at a rate cannot be handed a shaped t and still arrive where
  // it was drawn to. Nothing else here wants easing either: a generation is a
  // discrete event with no rate to ramp, and the window steps in whole cells, so
  // shaping progress would only make the board run fast through the middle of
  // the run.
  Curve curve() const override { return CurveLinear; }

  void render(uint16_t t) override {
    const int generations = stepsAt(t, GENERATIONS);

    // A generation follows from the one before it and from its own index, so a
    // walk that stops can be picked up where it stopped and lands where a walk
    // from the opening would have. Only a t behind what is held has to start
    // over, since a generation cannot be run backwards.
    if (walkedTo > generations || walkedTo == UNWALKED) {
      open(walked);
      walkedTo = 0;
    }
    while (walkedTo < generations) {
      advance(walked, walkedTo);
      walkedTo++;
    }

    // Neither hue has state to walk, so both come straight off t.
    draw(walked, liveHue.at(t), settledHue.at(t));
  }

  int tag() override { return 1018; }

private:
  // The window is the strip, laid out below as rows along it.
  static constexpr int VIEW = 10;
  static_assert(NUM_PIXELS == VIEW * VIEW, "the window is not the strip");

  // Ten windows on a side. Larger and most of the board is somewhere the window
  // will never reach in a run; smaller and there is nowhere for it to go.
  //
  // Wrapped in both directions. A bounded board has corners that collect and
  // edges that behave unlike the middle, and the window would find them; a torus
  // has neither, and the window has no boundary to be stopped at either.
  static constexpr int BOARD = VIEW * 10;

  // A row of the board packed into words, the columns past the width unused. The
  // target is 32-bit, so the words are too: a 64-bit shift here would be two
  // instructions pretending to be one.
  static constexpr int WORDS = (BOARD + 31) / 32;
  static constexpr int SPARE = WORDS * 32 - BOARD;
  static constexpr uint32_t TOP = ~0u >> SPARE;

  struct Board {
    uint32_t row[BOARD][WORDS];
  };

  // Generations kept behind the current one, which is what the value ramp reads
  // and what the stall check is judged against. Named for what it holds rather
  // than for a window, since the window here is the viewport.
  //
  // Even, so a cell alive on alternate generations is alive in exactly half of
  // it whichever generation it is read on, and holds a level instead of swinging
  // a step either side of one.
  //
  // Eight of them is two seconds at the rate below, which is about as far back
  // as the eye still has the strip; further and a cell's brightness is reporting
  // a board that is no longer there. Eight also puts three steps
  // between a birth and an alternation and four more between an alternation and
  // a cell that has not moved, so a glider, a blinker and a block are three
  // readings rather than adjacent ones.
  static constexpr int RECENT = 8;
  static_assert(RECENT % 2 == 0, "an alternating cell would swing between two levels");

  // Generations back a repeat is looked for. Two, since a board that has come
  // back to either of them is still lifes and blinkers and nothing else, and a
  // longer reach would call a board spent while something in it is still turning
  // over.
  static constexpr int SPENT = 2;
  static_assert(SPENT < RECENT, "the check reaches past what is kept");

  // Eighty thousand cells of board and where the window is standing in them. The
  // whole of what a run carries, so resuming is this and the count of
  // generations standing in it and nothing else.
  struct Field {
    Board gen[RECENT];
    int at;
    int viewX;
    int viewY;
    int heading;    // the direction of the bout, and what momentum is owed to
    int travelling; // steps left in the bout, zero while the window is standing
    int holding;    // generations left before the next step of the bout
    int dwelling;   // generations left to stand, spent only while standing
  };

  // A generation is a discrete event, so this is its dwell rather than a frame
  // interval. Four a second is where Life stops being a slideshow and has not
  // yet become a boil; it is also the ceiling on how fast the window travels,
  // since the window takes at most one pixel of the strip per generation.
  //
  // Well above the driver's tick, so no frame ever advances more than one
  // generation and the worst a frame costs is one generation plus a draw.
  //
  // Measured on the part at 160MHz and at the -Og the firmware is built at: the
  // ten thousand cells of a generation are 122993 cycles, 769us, and the same
  // figure every time, since word parallel work does not depend on what is on
  // the board. The steering and the stall check bring the worst generation to
  // 151578 cycles, 947us. A draw is a hundred pixels, each of them counting
  // itself through the generations kept, and is a fraction of that again. Just
  // over a millisecond of the ten the tick is, on the one frame in twenty-five
  // that has a generation to run.
  static constexpr int MS_PER_GENERATION = 250;

  // Five minutes, the length an automaton is worth watching for, which is 1200
  // generations at the rate above. A soup settles in a few hundred, so a run is
  // several rounds of settling and resowing, and the window has time to cross
  // the board and come back many times over.
  static constexpr int RUN_MS = 300000;
  static constexpr int GENERATIONS = RUN_MS / MS_PER_GENERATION;
  static_assert(GENERATIONS >= BOARD * 8, "the run is short of eight crossings of the board");

  // The opening, and what a stalled board is given. Soup in patches rather than
  // over the whole board: soup everywhere looks the same wherever the window
  // goes, and a board ten times the window is only worth having if it has
  // somewhere else to be. Patches give it dense middles, edges the pattern grows
  // out of, and empty ground between.
  //
  // A patch is wider than the window, so the window can sit inside one and see
  // no edge, or sit on the edge and watch it move. Seven of them cover about a
  // fifth of the board.
  static constexpr int PATCHES = 7;
  static constexpr int PATCH = 16;
  static_assert(PATCH > VIEW, "the window cannot fit inside a patch");
  static_assert(PATCH <= 32, "a patch row is drawn from one word of noise");

  // How often a single patch is sown on top of whatever is there, apart from
  // any stall. Left alone, Life settles to a few hundred cells over ten thousand
  // and a window is dark most of the time; a patch every ten seconds keeps
  // somewhere on the board young, which is both something to look at and
  // somewhere for the window to go. Ten seconds is long enough that a patch has
  // finished being an explosion before the next lands.
  static constexpr int SOW_EVERY = 40;

  // Patch numbers within a round of sowing, which is what separates one draw
  // from another. The steady patch sits above the ones a stall sows so the two
  // never land in the same place.
  static constexpr int SOW_KEYS = 32;
  static constexpr int STEADY_PATCH = 16;
  static_assert(PATCHES <= STEADY_PATCH && STEADY_PATCH < SOW_KEYS, "sowings share a draw");

  // What the window is drawn toward. Change is what is worth watching, so it
  // carries the weight; live cells count for something so that a window with
  // still lifes in it is not the same as empty board, but not for enough to hold
  // the window on a pattern that has stopped.
  static constexpr int CHANGE_WORTH = 4;
  static constexpr int LIVE_WORTH = 1;

  // A run is bouts rather than a pan: the window stands and watches, then
  // crosses to somewhere else, then stands again. Which of the two it is doing
  // is a clock, and the board only decides which way a bout goes. Comparing the
  // window against its neighbours to decide whether to move at all was the other
  // way, and it does not work at either end: a margin small enough that the
  // window ever leaves the best place in reach is small enough that it leaves
  // every place, since between two ordinary windows a hundred cell count differs
  // by more than any margin worth naming.
  //
  // A bout is a window's width, whichever axis it is on. On the strip that is
  // ten pixels of slide for a column bout and the whole hundred for a row one,
  // because the rows are laid end to end along it: the two axes are a drift and
  // a scroll, and which one a bout is depends only on where the board is
  // busiest. The two take the same pixels a second and so different lengths of
  // time, which `hold` below is the whole of.
  static constexpr int TRAVEL = VIEW;

  // Generations the window stands before it goes, the least and the spread drawn
  // on top of it. Four to twelve seconds at the generation rate, which is long
  // enough to watch a patch settle and short enough that the board is not one
  // picture for the length of a run.
  static constexpr int DWELL = 16;
  static constexpr int DWELL_SPREAD = 32;

  // Weight given to the direction of the last bout, so the window carries on
  // rather than turning back into ground it has just crossed.
  static constexpr int MOMENTUM = 12;

  // What separates two directions with nothing between them. Without it a flat
  // board is always crossed the same way, since the first candidate wins every
  // tie for the whole of a run.
  static constexpr int JITTER = 8;

  // A live cell counts itself, so depth runs 1 to RECENT: just born at the
  // bottom, alive throughout at the top, alternating at half.
  //
  // The bottom of the duty range is where a step in value stops being a step in
  // brightness, so the dim end stands well above it rather than near the floor.
  // The two ends then sit the same distance either side of VALUE_DEFAULT, which
  // places the ramp against the level the rest of the strip runs at rather than
  // against the top of what the driver will take.
  static constexpr int VALUE_ARRIVED = 84;
  static constexpr int VALUE_HELD = 252;
  static constexpr int VALUE_STEP = (VALUE_HELD - VALUE_ARRIVED) / (RECENT - 1);
  static_assert(VALUE_ARRIVED + VALUE_HELD == 2 * VALUE_DEFAULT, "the ramp is off centre");
  static_assert((VALUE_HELD - VALUE_ARRIVED) % (RECENT - 1) == 0, "the ramp has an uneven step");

  static constexpr int UNWALKED = -1;

  static constexpr int STEPS = 4;
  static constexpr int STEP_X[STEPS] = {1, -1, 0, 0};
  static constexpr int STEP_Y[STEPS] = {0, 0, 1, -1};

  // Nine neighbour counts to a mask and two masks to a rule, so a rule is the
  // eighteen bit code the membership set is indexed by.
  static constexpr int COUNTS = 9;
  static constexpr uint32_t CODES = 1u << (2 * COUNTS);
  static constexpr uint16_t MASK = (1u << COUNTS) - 1;

  // A power of two of codes, so the draw is a mask and not a modulo, and every
  // rule is as likely as every other. The cap is there only so that an RNG stuck
  // on a rejected code cannot take the run with it, and Life is what it lands on
  // if it ever comes to that.
  static constexpr int ATTEMPTS = 32;

  static Rule drawn() {
    for (int attempt = 0; attempt < ATTEMPTS; attempt++) {
      const uint32_t code = esp_random() & (CODES - 1);
      if (LifeRules::contains(code)) {
        return Rule{(uint16_t)(code >> COUNTS), (uint16_t)(code & MASK)};
      }
    }
    return Rule{lifeCounts("3"), lifeCounts("23")};
  }

  static void notate(char* into, uint16_t mask) {
    int at = 0;
    for (int n = 0; n <= 8; n++) {
      if (mask & (1u << n)) {
        into[at++] = (char)('0' + n);
      }
    }
    into[at] = 0;
  }

  // The sum below counts the cell along with its neighbours, since that is what
  // three row sums add up to and taking the cell back out again would be a
  // borrow chain for nothing. One decode then answers both halves of the rule: a
  // birth reads the count as it stands, a survival reads it one higher.
  void decode() {
    bornCount = 0;
    heldCount = 0;
    for (int total = 0; total <= 9; total++) {
      if (total <= 8 && (rule.birth & (1u << total))) {
        bornLine[bornCount++] = (uint8_t)total;
      }
      if (total >= 1 && (rule.survive & (1u << (total - 1)))) {
        heldLine[heldCount++] = (uint8_t)total;
      }
    }
  }

  // The board wraps, so a shift along a row is a rotation of it: the column that
  // falls off one end arrives at the other. The last word carries only the top
  // few columns, which is where both wraps are.
  static void east(const uint32_t from[WORDS], uint32_t into[WORDS]) {
    uint32_t carried = (from[WORDS - 1] >> (31 - SPARE)) & 1u;
    for (int w = 0; w < WORDS; w++) {
      const uint32_t leaving = from[w] >> 31;
      into[w] = (from[w] << 1) | carried;
      carried = leaving;
    }
    into[WORDS - 1] &= TOP;
  }

  static void west(const uint32_t from[WORDS], uint32_t into[WORDS]) {
    uint32_t carried = (from[0] & 1u) << (31 - SPARE);
    for (int w = WORDS - 1; w >= 0; w--) {
      const uint32_t leaving = (from[w] & 1u) << 31;
      into[w] = (from[w] >> 1) | carried;
      carried = leaving;
    }
  }

  // A row's cells and the two beside each of them, as a two bit count per
  // column. Three of these added together is the nine cell neighbourhood, so a
  // row of the board reads its neighbours in two shifts and a full adder rather
  // than in eight loads per cell.
  struct Sum {
    uint32_t low[WORDS];
    uint32_t high[WORDS];
  };

  static void triple(const uint32_t from[WORDS], Sum& sum) {
    uint32_t left[WORDS];
    uint32_t right[WORDS];
    west(from, left);
    east(from, right);
    for (int w = 0; w < WORDS; w++) {
      const uint32_t pair = left[w] ^ from[w];
      sum.low[w] = pair ^ right[w];
      sum.high[w] = (left[w] & from[w]) | (right[w] & pair);
    }
  }

  // Three two-bit counts into four bit planes, by two full adders and two half
  // adders. That is the carry-save a per cell loop would do one cell at a time,
  // except that a word of it is thirty-two columns and the whole board goes
  // through it in four hundred passes rather than ten thousand.
  void apply(const uint32_t self[WORDS], const Sum& above, const Sum& here,
             const Sum& below, uint32_t into[WORDS]) const {
    for (int w = 0; w < WORDS; w++) {
      const uint32_t lowPair = above.low[w] ^ here.low[w];
      const uint32_t n0 = lowPair ^ below.low[w];
      const uint32_t lowCarry = (above.low[w] & here.low[w]) | (below.low[w] & lowPair);

      const uint32_t highPair = above.high[w] ^ here.high[w];
      const uint32_t highSum = highPair ^ below.high[w];
      const uint32_t highCarry = (above.high[w] & here.high[w]) | (below.high[w] & highPair);

      const uint32_t n1 = highSum ^ lowCarry;
      const uint32_t n2 = highCarry ^ (highSum & lowCarry);
      const uint32_t n3 = highCarry & (highSum & lowCarry);

      // Nine is the most three row sums can come to, so a set eight plane has
      // the four plane clear and the top of the decode is two lines wide.
      const uint32_t low0 = ~n1 & ~n0;
      const uint32_t low1 = ~n1 & n0;
      const uint32_t low2 = n1 & ~n0;
      const uint32_t low3 = n1 & n0;
      const uint32_t high0 = ~n3 & ~n2;
      const uint32_t high1 = ~n3 & n2;

      const uint32_t is[10] = {
        high0 & low0, high0 & low1, high0 & low2, high0 & low3,
        high1 & low0, high1 & low1, high1 & low2, high1 & low3,
        n3 & low0,    n3 & low1,
      };

      uint32_t born = 0;
      for (int line = 0; line < bornCount; line++) {
        born |= is[bornLine[line]];
      }
      uint32_t held = 0;
      for (int line = 0; line < heldCount; line++) {
        held |= is[heldLine[line]];
      }

      into[w] = (born & ~self[w]) | (held & self[w]);
    }

    // The decode sets every column, including the ones past the width, and a
    // rule with a birth on no neighbours would light all of them.
    into[WORDS - 1] &= TOP;
  }

  // Three row sums are live at a time and each is used by three rows, so they
  // are computed once and rotated rather than three times each.
  void successor(const Board& from, Board& into) const {
    Sum sums[3];
    int above = 0;
    int here = 1;
    int below = 2;

    triple(from.row[BOARD - 1], sums[above]);
    triple(from.row[0], sums[here]);

    for (int y = 0; y < BOARD; y++) {
      triple(from.row[y + 1 == BOARD ? 0 : y + 1], sums[below]);
      apply(from.row[y], sums[above], sums[here], sums[below], into.row[y]);
      const int spent = above;
      above = here;
      here = below;
      below = spent;
    }
  }

  static void blank(Board& board) {
    memset(board.row, 0, sizeof(board.row));
  }

  static bool alive(const Board& board, int x, int y) {
    return (board.row[y][x >> 5] >> (x & 31)) & 1u;
  }

  static const Board& behind(const Field& field, int generations) {
    return field.gen[(field.at + RECENT - generations) % RECENT];
  }

  // Offsets stay under the board's width, so one step brings anything back into
  // range and there is no division on a path a generation takes a thousand
  // times.
  static int wrapped(int at) {
    return at < 0 ? at + BOARD : (at >= BOARD ? at - BOARD : at);
  }

  // Written rather than added, so a round of sowing is fresh soup whether the
  // board it lands on was empty or full. A patch row is one word of noise, drawn
  // against the round and the row so that sowing lands the same way however
  // often the run is walked through.
  void sowPatch(Board& board, int round, int patch) const {
    const int left = corner(round, patch, 0);
    const int top = corner(round, patch, 1);
    for (int r = 0; r < PATCH; r++) {
      const int y = wrapped(top + r);
      const uint32_t drawn = noise(seed, key(round, patch), (uint32_t)r);
      for (int c = 0; c < PATCH; c++) {
        const int x = wrapped(left + c);
        const uint32_t bit = 1u << (x & 31);
        if ((drawn >> c) & 1u) {
          board.row[y][x >> 5] |= bit;
        } else {
          board.row[y][x >> 5] &= ~bit;
        }
      }
    }
  }

  void sow(Board& board, int round) const {
    for (int patch = 0; patch < PATCHES; patch++) {
      sowPatch(board, round, patch);
    }
  }

  static uint32_t key(int round, int patch) {
    return (uint32_t)(round * SOW_KEYS + patch);
  }

  int corner(int round, int patch, int lane) const {
    return (int)noiseMax(seed, key(round, patch), (uint32_t)(64 + lane), BOARD - 1);
  }

  // The window opens inside the first patch, so a run starts on soup rather than
  // on whatever ground the draw happened to land on.
  void open(Field& field) const {
    for (int g = 0; g < RECENT; g++) {
      blank(field.gen[g]);
    }
    field.at = 0;
    sow(field.gen[0], 0);
    field.viewX = wrapped(corner(0, 0, 0) + (PATCH - VIEW) / 2);
    field.viewY = wrapped(corner(0, 0, 1) + (PATCH - VIEW) / 2);
    field.heading = (int)noiseMax(seed, 0, 128, STEPS - 1);
    field.travelling = 0;
    field.holding = 0;
    field.dwelling = DWELL;
  }

  void advance(Field& field, int generation) const {
    const int next = field.at + 1 == RECENT ? 0 : field.at + 1;
    successor(field.gen[field.at], field.gen[next]);
    field.at = next;

    if (generation % SOW_EVERY == 0) {
      sowPatch(field.gen[field.at], generation + 1, STEADY_PATCH);
    }

    // A board that has come back to either of the two generations behind it is
    // still lifes and blinkers and nothing else: the rule has finished with the
    // soup it was given. Fresh patches rather than a cleared board, since the
    // window is somewhere and a wipe would be a hand reaching in.
    if (stalled(field)) {
      sow(field.gen[field.at], generation + 1);
    }

    steer(field, generation);
  }

  static bool stalled(const Field& field) {
    for (int back = 1; back <= SPENT; back++) {
      const Board& then = behind(field, back);
      if (memcmp(field.gen[field.at].row, then.row, sizeof(then.row)) == 0) {
        return true;
      }
    }
    return false;
  }

  struct Worth {
    int changed;
    int live;
  };

  // What a window at a place is worth, the cells that changed this generation
  // weighed above the cells merely alive.
  Worth weigh(const Field& field, int x, int y) const {
    const Board& now = field.gen[field.at];
    const Board& before = behind(field, 1);

    Worth worth = {0, 0};
    for (int r = 0; r < VIEW; r++) {
      const int by = wrapped(y + r);
      for (int c = 0; c < VIEW; c++) {
        const int bx = wrapped(x + c);
        const int live = alive(now, bx, by) ? 1 : 0;
        worth.live += live;
        worth.changed += live ^ (alive(before, bx, by) ? 1 : 0);
      }
    }
    return worth;
  }

  static int worthOf(const Worth& worth) {
    return worth.changed * CHANGE_WORTH + worth.live * LIVE_WORTH;
  }

  // Generations a step of a bout waits before the next one. A column step slides
  // the picture one pixel and a row step slides it ten, so a row step waits out
  // the nine it covers at once and the window crosses the strip at one pixel a
  // generation on either axis. That is the elementary automaton's speed limit
  // too, and about where the strip stops reading a slide as a cut. It makes a
  // row bout a hundred generations against a column bout's ten, which is the
  // scroll taking as long to cross the strip as the strip is long.
  static int hold(int step) { return STEP_Y[step] ? VIEW - 1 : 0; }

  static void take(Field& field, int step) {
    field.viewX = wrapped(field.viewX + STEP_X[step]);
    field.viewY = wrapped(field.viewY + STEP_Y[step]);
  }

  // Which way the next bout goes.
  //
  // What the four directions are scored on is the neighbouring window, a whole
  // width away rather than one cell over: a single cell of displacement changes
  // a hundred cell count by almost nothing, so a one cell gradient is noise and
  // the window would only dither in it. The bout then walks the ten cells to
  // that window one at a time, so what the strip shows is the ground between,
  // not a cut to somewhere else.
  int toward(const Field& field, int generation) const {
    int taken = 0;
    int best = -1;
    for (int step = 0; step < STEPS; step++) {
      const Worth ahead = weigh(field, wrapped(field.viewX + STEP_X[step] * VIEW),
                                       wrapped(field.viewY + STEP_Y[step] * VIEW));
      const int worth = worthOf(ahead)
                      + (step == field.heading ? MOMENTUM : 0)
                      + (int)noiseMax(seed, (uint32_t)generation, (uint32_t)(200 + step), JITTER);
      if (worth > best) {
        best = worth;
        taken = step;
      }
    }
    return taken;
  }

  // Standing or going, and where.
  //
  // The clock decides which of the two it is doing. What the board decides is
  // the direction, and one exception: an empty window is not being watched, it
  // is being waited in, so the dwell is abandoned there however much of it is
  // left. Empty and not merely still, since a rule that freezes into structure
  // has left something worth looking at, and a window that will not stand on
  // anything static pans for the whole of such a run.
  void steer(Field& field, int generation) const {
    if (field.travelling == 0) {
      if (field.dwelling > 0 && weigh(field, field.viewX, field.viewY).live > 0) {
        field.dwelling--;
        return;
      }
      field.heading = toward(field, generation);
      field.travelling = TRAVEL;
      field.holding = 0;
    }

    if (field.holding > 0) {
      field.holding--;
      return;
    }

    take(field, field.heading);
    field.travelling--;
    if (field.travelling == 0) {
      field.dwelling = DWELL + (int)noiseMax(seed, (uint32_t)generation, 128, DWELL_SPREAD);
    } else {
      field.holding = hold(field.heading);
    }
  }

  static uint8_t level(int depth) {
    return (uint8_t)(VALUE_ARRIVED + (depth - 1) * VALUE_STEP);
  }

  // How many of the generations kept a cell was alive in, the current one
  // included. Asked at the hundred cells under the window rather than kept for
  // the ten thousand on the board, since the other nine thousand nine hundred
  // are not being drawn.
  static int depthAt(const Field& field, int x, int y) {
    int lit = 0;
    for (int g = 0; g < RECENT; g++) {
      lit += alive(field.gen[g], x, y) ? 1 : 0;
    }
    return lit;
  }

  // The window is ten rows of ten laid end to end along the strip, row after
  // row. Rows are a fiction the strip cannot show, but the motion can: laid this
  // way a column step slides the whole picture by one pixel and a row step
  // slides it by ten, and both are rigid.
  //
  // The serpentine order a real matrix is wired in was the other candidate,
  // since it makes every neighbouring pair of pixels a neighbouring pair of
  // cells and leaves no false joins at all. What it costs is the motion: under
  // it a row step reverses every row and a column step shears the odd rows
  // against the even ones, so the wander stops reading as a wander. Nine false
  // joins buy ten legible pans, and on a wrapped board the two cells either side
  // of a join are still both inside the window rather than anywhere at all.
  //
  // A cell that was also alive last generation takes the other tone. Age
  // carried any further in hue would be a per pixel gradient, which at this
  // width is a mush of single lit pixels in different colours; two tones stay
  // coarse enough to read as the boundary between the part of a pattern that is
  // holding and the part that is turning over. Which of them is which colour is
  // whatever the drifts say, so what separates the two is the cells they land on
  // rather than anything in the palette.
  //
  // Hue asks one generation back and no further, so a rule whose live set
  // alternates never reaches the second tone at all. Value asks the length of
  // what is kept, where a glider's cells sit near the bottom of the ramp, a
  // blinker's at half, and a still life at the top. Nothing dead is lit, so a
  // glider is where it is rather than a smear of where it has been.
  void draw(const Field& field, uint16_t live, uint16_t settled) const {
    const Board& now = field.gen[field.at];
    const Board& before = behind(field, 1);

    for (int r = 0; r < VIEW; r++) {
      const int y = wrapped(field.viewY + r);
      for (int c = 0; c < VIEW; c++) {
        const int x = wrapped(field.viewX + c);
        if (!alive(now, x, y)) {
          continue;
        }
        const uint32_t index = (uint32_t)(r * VIEW + c);
        actual_led_strip_set_pixel_hsv(strip, index, alive(before, x, y) ? settled : live,
                                       level(depthAt(field, x, y)));
      }
    }
  }

  const Rule pinned;
  Rule rule;
  uint32_t seed;
  HueDrift liveHue;
  HueDrift settledHue;

  uint8_t bornLine[9];
  uint8_t heldLine[9];
  int bornCount;
  int heldCount;

  // How far the run has been walked, and where it got to. Read only through
  // render(), which either extends the walk or throws it away and starts again,
  // so what the strip shows still depends on t alone.
  Field walked;
  int walkedTo;
};

#endif
