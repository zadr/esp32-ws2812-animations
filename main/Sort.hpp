#ifndef SORT_HPP
#define SORT_HPP

#include "Animation.hpp"
#include "Constants.h"
#include "actual_led_strip_set_pixel_hsv.h"
#include "esp_random_max.h"
#include <stdint.h>

enum SortAlgorithm : uint8_t {
  SortBitonic,
  SortQuick,
  SortRadix,
  SortMerge,
  SortInsertion,
  SortSelection,
  SortHeap,
  SortAlgorithmCount
};

enum SortDataset : uint8_t {
  SortRainbow,
  SortSegment,
  SortDatasetCount
};

// The strip holds the array. Each algorithm runs to completion during setup
// against a scratch copy, appending every operation that changes a pixel to a
// queue, and loop() replays one queued operation per frame. The algorithms then
// stay in their natural recursive or iterative form and steps() is the exact
// frame count rather than an estimate.
class Sort : public Animation {
public:
  Sort(led_strip_handle_t& strip, SortAlgorithm algorithm, SortDataset dataset)
    : Animation(strip), algorithm(algorithm), dataset(dataset), delayMs(100), frame(0), cursor(0) {}

  void setup() override {
    buildKeys();
    shuffle();
    record();

    delayMs = pace();
    frame = 0;
    cursor = 0;
  }

  int steps() override { return introFrames() + recordCount + settleFrames(); }

  void loop() override {
    int movedLow = -1;
    int movedHigh = -1;

    if (frame >= introFrames() && cursor < recordCount) {
      const Op& op = operations[cursor];
      cursor++;

      if (op.kind == OpSwap) {
        const uint8_t held = keys[op.a];
        keys[op.a] = keys[op.b];
        keys[op.b] = held;
        movedHigh = op.b;
      } else {
        keys[op.a] = op.b;
      }
      movedLow = op.a;
    }
    frame++;

    // The pixels this frame touched carry full output against the default the
    // rest of the strip sits at. Hue alone cannot mark them: a segment spanning
    // two anchors steps about one output level per pixel, so a moved pixel is
    // indistinguishable from its neighbours by colour.
    for (int i = 0; i < NUM_PIXELS; i++) {
      const uint8_t value = (i == movedLow || i == movedHigh) ? 255 : VALUE_DEFAULT;
      actual_led_strip_set_pixel_hsv(strip, i, keyHues[keys[i]], value);
    }
  }

  // Operation counts differ by an order of magnitude between algorithms, so the
  // pace follows the recorded length and every entry runs about as long.
  int getDelay() override { return delayMs; }

  // A sort ends. A second pass would replay the queue against an already sorted
  // strip and show nothing.
  int minIterations() override { return 1; }
  int maxIterations() override { return 1; }

  int tag() override { return 1100 + (int)algorithm * (int)SortDatasetCount + (int)dataset; }

private:
  enum OpKind : uint8_t { OpSwap, OpSet };

  struct Op {
    uint8_t kind;
    uint8_t a;
    uint8_t b; // partner index for a swap, key value for a set
  };

  // Insertion sort on a reversed strip is the ceiling at 1225 operations.
  // Recording stops at the bound and the replay ends part sorted.
  static const int MAX_OPS = 1400;

  static const int TARGET_MS = 14000;
  static const int SLOWEST_MS = 400;

  // A sixth of the wheel ramps one channel across 255 steps before the pixel
  // value scales it down, so one output level is HUE_MAX / 6 / VALUE_DEFAULT
  // hue units. A run narrower than a level per pixel gap renders with repeats
  // and stops reading as ordered. This admits every anchor run except red to
  // orange and blue to indigo, 1967 and 1476 units wide, which render as 30 and
  // 27 distinct colours across the 50 pixels.
  static const int MIN_SEGMENT_UNITS = (NUM_PIXELS - 1) * (HUE_MAX / 6 / VALUE_DEFAULT);

  // Two passes over six key bits. An array sorted by a low digit is not sorted
  // by value, so no digit width makes the passes before the last look ordered.
  // Narrower digits only add passes that appear to do nothing.
  static const int RADIX_BITS = 3;
  static const int RADIX_BASE = 1 << RADIX_BITS;

  // The driver runs one animation to completion before starting the next, so
  // the recording scratch is shared rather than sized once per registration.
  static inline Op operations[MAX_OPS];
  static inline int recordCount = 0;
  static inline uint8_t work[NUM_PIXELS];
  static inline uint8_t scratch[NUM_PIXELS];

  void record() {
    recordCount = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
      work[i] = keys[i];
    }

    switch (algorithm) {
      case SortBitonic:   bitonicSort(0, NUM_PIXELS, true); break;
      case SortQuick:     quickSort(0, NUM_PIXELS - 1);     break;
      case SortRadix:     radixSort();                      break;
      case SortMerge:     mergeSort(0, NUM_PIXELS);         break;
      case SortInsertion: insertionSort();                  break;
      case SortSelection: selectionSort();                  break;
      default:            heapSort();                       break;
    }
  }

  int pace() const {
    const int perFrame = TARGET_MS / (recordCount > 0 ? recordCount : 1);
    const int rounded = ((perFrame + 5) / 10) * 10;
    if (rounded < 10) return 10;
    if (rounded > SLOWEST_MS) return SLOWEST_MS;
    return rounded;
  }

  int introFrames() const { return 1000 / delayMs; }
  int settleFrames() const { return 2000 / delayMs; }

  void buildKeys() {
    static const uint16_t anchors[] = {
      HUE_RED, HUE_ORANGE, HUE_YELLOW, HUE_GREEN, HUE_BLUE, HUE_INDIGO, HUE_VIOLET
    };
    const int anchorCount = sizeof(anchors) / sizeof(anchors[0]);

    int first = 0;
    int span = anchorCount;
    if (dataset == SortSegment) {
      do {
        span = esp_random_max(2) + 2;
        first = esp_random_max(anchorCount - span);
      } while (anchors[first + span - 1] - anchors[first] < MIN_SEGMENT_UNITS);
    }

    // Stepping anchor to anchor rather than straight from the first hue number
    // to the last. The palette is spaced by eye and the numbers between anchors
    // are not, so giving each anchor interval an equal share of the pixels is
    // what makes the ramp read evenly. It is also why the anchor count sets the
    // width of a run: red to orange is 1967 units and yellow to green is 15684,
    // but one anchor apart reads as one step of either.
    const int intervals = span - 1;
    for (int i = 0; i < NUM_PIXELS; i++) {
      const uint32_t reach = (uint32_t)i * intervals;
      int step = reach / (NUM_PIXELS - 1);
      uint32_t within = reach % (NUM_PIXELS - 1);
      if (step >= intervals) {
        step = intervals - 1;
        within = NUM_PIXELS - 1;
      }

      const uint16_t low = anchors[first + step];
      const uint16_t high = anchors[first + step + 1];
      keyHues[i] = (uint16_t)(low + (uint32_t)(high - low) * within / (NUM_PIXELS - 1));
      keys[i] = i;
    }
  }

  void shuffle() {
    for (int i = NUM_PIXELS - 1; i > 0; i--) {
      const int j = esp_random_max(i);
      const uint8_t held = keys[i];
      keys[i] = keys[j];
      keys[j] = held;
    }
  }

  void recordSwap(int i, int j) {
    if (i == j) return;

    const uint8_t held = work[i];
    work[i] = work[j];
    work[j] = held;
    push(OpSwap, i, j);
  }

  // Merge and radix write a whole range back, most of it already holding the
  // value being written. A frame that changes no pixel reads as a stall, so
  // those are dropped rather than queued.
  void recordSet(int i, uint8_t value) {
    if (work[i] == value) return;

    work[i] = value;
    push(OpSet, i, value);
  }

  void push(uint8_t kind, uint8_t a, uint8_t b) {
    if (recordCount >= MAX_OPS) return;

    operations[recordCount].kind = kind;
    operations[recordCount].a = a;
    operations[recordCount].b = b;
    recordCount++;
  }

  void insertionSort() {
    for (int i = 1; i < NUM_PIXELS; i++) {
      for (int j = i; j > 0 && work[j - 1] > work[j]; j--) {
        recordSwap(j - 1, j);
      }
    }
  }

  void selectionSort() {
    for (int i = 0; i < NUM_PIXELS - 1; i++) {
      int least = i;
      for (int j = i + 1; j < NUM_PIXELS; j++) {
        if (work[j] < work[least]) least = j;
      }
      recordSwap(i, least);
    }
  }

  // Hoare partition: the two ends walk inward and exchange across the pivot.
  // Recursion takes the shorter side, which holds the depth to about six frames
  // of the 3.5KB main task stack.
  void quickSort(int lo, int hi) {
    while (lo < hi) {
      const uint8_t pivot = work[(lo + hi) / 2];
      int i = lo - 1;
      int j = hi + 1;

      for (;;) {
        do { i++; } while (work[i] < pivot);
        do { j--; } while (work[j] > pivot);
        if (i >= j) break;

        recordSwap(i, j);
      }

      if (j - lo < hi - j - 1) {
        quickSort(lo, j);
        lo = j + 1;
      } else {
        quickSort(j + 1, hi);
        hi = j;
      }
    }
  }

  void mergeSort(int lo, int hi) {
    if (hi - lo < 2) return;

    const int mid = (lo + hi) / 2;
    mergeSort(lo, mid);
    mergeSort(mid, hi);

    int a = lo;
    int b = mid;
    int k = 0;
    while (a < mid && b < hi) {
      if (work[b] < work[a]) {
        scratch[k] = work[b];
        b++;
      } else {
        scratch[k] = work[a];
        a++;
      }
      k++;
    }
    while (a < mid) { scratch[k] = work[a]; a++; k++; }
    while (b < hi)  { scratch[k] = work[b]; b++; k++; }

    for (int i = 0; i < k; i++) {
      recordSet(lo + i, scratch[i]);
    }
  }

  void heapSort() {
    for (int root = NUM_PIXELS / 2 - 1; root >= 0; root--) {
      siftDown(root, NUM_PIXELS);
    }
    for (int end = NUM_PIXELS - 1; end > 0; end--) {
      recordSwap(0, end);
      siftDown(0, end);
    }
  }

  void siftDown(int root, int end) {
    for (;;) {
      int child = 2 * root + 1;
      if (child >= end) return;
      if (child + 1 < end && work[child] < work[child + 1]) child++;
      if (work[child] <= work[root]) return;

      recordSwap(root, child);
      root = child;
    }
  }

  void radixSort() {
    uint8_t largest = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
      if (work[i] > largest) largest = work[i];
    }

    for (int shift = 0; (largest >> shift) > 0; shift += RADIX_BITS) {
      int offset[RADIX_BASE] = { 0 };
      for (int i = 0; i < NUM_PIXELS; i++) {
        offset[(work[i] >> shift) & (RADIX_BASE - 1)]++;
      }

      int total = 0;
      for (int digit = 0; digit < RADIX_BASE; digit++) {
        const int count = offset[digit];
        offset[digit] = total;
        total += count;
      }

      for (int i = 0; i < NUM_PIXELS; i++) {
        const int digit = (work[i] >> shift) & (RADIX_BASE - 1);
        scratch[offset[digit]] = work[i];
        offset[digit]++;
      }

      for (int i = 0; i < NUM_PIXELS; i++) {
        recordSet(i, scratch[i]);
      }
    }
  }

  void bitonicSort(int lo, int count, bool ascending) {
    if (count <= 1) return;

    const int half = count / 2;
    bitonicSort(lo, half, !ascending);
    bitonicSort(lo + half, count - half, ascending);
    bitonicMerge(lo, count, ascending);
  }

  // The network is defined on powers of two. Splitting at the largest power of
  // two below count extends it to a strip length that is not one, without any
  // padding element that would have to live off the end of the strip.
  void bitonicMerge(int lo, int count, bool ascending) {
    if (count <= 1) return;

    const int span = largestPowerOfTwoBelow(count);
    for (int i = lo; i < lo + count - span; i++) {
      if ((work[i] > work[i + span]) == ascending) {
        recordSwap(i, i + span);
      }
    }

    bitonicMerge(lo, span, ascending);
    bitonicMerge(lo + span, count - span, ascending);
  }

  static int largestPowerOfTwoBelow(int count) {
    int power = 1;
    while (power < count) power <<= 1;
    return power >> 1;
  }

  SortAlgorithm algorithm;
  SortDataset dataset;
  int delayMs;
  int frame;
  int cursor;
  uint8_t keys[NUM_PIXELS];
  uint16_t keyHues[NUM_PIXELS];
};

#endif
