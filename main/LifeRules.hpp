#ifndef LIFERULES_HPP
#define LIFERULES_HPP

#include <stdint.h>

#include "LifeRuleSet.hpp"

// Whether a Life-like rule is one of the ones worth drawing, answered out of the
// compressed membership set in LifeRuleSet.hpp.
//
// One bit per 18-bit rule code is 262144 bits laid flat, which is not a table
// this part should be carrying. The blob it is packed into stays in flash and is
// read where it sits: a query decodes only the segments on the way to its own
// answer and keeps nothing afterwards, so there is no cache, no unpacked copy
// and nothing to initialise before the first call. A rule is drawn once a run,
// so what the decode costs is spent where nothing is waiting on it.
namespace LifeRules {

static_assert(LifeRuleSet::SEGMENTS * LifeRuleSet::SEGBITS == 1 << 18,
              "the segments do not cover the rule codes");

// Rice gaps are the only position coding read here, and a blob built with one of
// the others decodes to nothing that would look wrong.
static_assert(LifeRuleSet::POSCODE == 0, "the positions are not Rice coded");

// Numbered from the top of a byte, the order the blob was written in.
constexpr bool bit(uint32_t at) {
  return (LifeRuleSet::BLOB[at >> 3] >> (7 - (at & 7))) & 1;
}

constexpr uint32_t bits(uint32_t at, int width) {
  uint32_t value = 0;
  for (int i = 0; i < width; i++) {
    value = (value << 1) | bit(at + i);
  }
  return value;
}

// How many positions a row differs at, canonical Huffman. The table is in the
// order the codes were assigned, by length and then by symbol, so each length's
// first code and first symbol follow from the length below it and no per length
// table has to be carried.
constexpr uint32_t readCount(uint32_t& at) {
  // A single symbol is the count of every row and is not worth a bit.
  if (LifeRuleSet::COUNT_SYMBOLS == 1) return LifeRuleSet::COUNT_SYMBOL[0];

  uint32_t code = 0;
  uint32_t first = 0;
  uint32_t index = 0;
  for (int length = 1;; length++) {
    code = (code << 1) | bit(at++);

    uint32_t span = 0;
    while (index + span < (uint32_t)LifeRuleSet::COUNT_SYMBOLS &&
           LifeRuleSet::COUNT_LENGTH[index + span] == length) {
      span++;
    }

    const uint32_t rank = code - first;
    if (rank < span) return LifeRuleSet::COUNT_SYMBOL[index + rank];

    index += span;
    first = (first + span) << 1;
  }
}

// Whether a row's diffs include the cell. They arrive in order, as Rice coded
// gaps, so one that reaches or passes the cell settles the row; the parent is
// read ahead of them, so the rest of the positions can be left in the stream.
constexpr bool differsAt(uint32_t at, uint32_t count, uint32_t cell) {
  uint32_t least = 0; // the lowest position the next gap can land on
  for (uint32_t i = 0; i < count; i++) {
    uint32_t zeros = 0;
    while (!bit(at)) {
      zeros++;
      at++;
    }
    at++;

    const uint32_t gap = (zeros << LifeRuleSet::RICE_K) | bits(at, LifeRuleSet::RICE_K);
    at += LifeRuleSet::RICE_K;

    const uint32_t position = least + gap;
    if (position >= cell) return position == cell;
    least = position + 1;
  }
  return false;
}

// A segment is either an anchor, holding its cells raw, or the positions where
// it differs from another segment. So membership is the anchor's bit turned over
// once for every segment along the way that differs at the cell. Each chain ends
// at an anchor by construction, and that is the whole of what ends the walk:
// depth is a property of how the blob was packed, and nothing here is sized to
// it.
constexpr bool contains(uint32_t code) {
  uint32_t row = code / LifeRuleSet::SEGBITS;
  const uint32_t cell = code % LifeRuleSet::SEGBITS;

  bool parity = false;
  while (true) {
    const uint32_t record =
      LifeRuleSet::BLOB_AT +
      bits(LifeRuleSet::OFFSETS_AT + row * LifeRuleSet::OFFBITS, LifeRuleSet::OFFBITS);
    if (bit(record)) return parity ^ bit(record + 1 + cell);

    const uint32_t parent = bits(record + 1, LifeRuleSet::REFBITS);
    uint32_t at = record + 1 + LifeRuleSet::REFBITS;
    const uint32_t count = readCount(at);

    parity ^= differsAt(at, count, cell);
    row = parent;
  }
}

}

#endif
