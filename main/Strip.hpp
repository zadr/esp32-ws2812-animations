#ifndef STRIP_HPP
#define STRIP_HPP

#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "Constants.h"
#include "Frame.hpp"

// The SPI channel and the expansion that put a Frame on the wire. Held here
// rather than reached through a driver that keeps pixels of its own, so the
// bytes an animation rendered are the bytes the strip is sent.
//
// The C6's RMT has no DMA, so the CPU built every pulse descriptor and refilled
// 48 words of channel memory across the whole 3ms send. SPI2 is served by GDMA,
// so the waveform is written once into memory and clocked out with the CPU
// asleep. What SPI cannot do is vary a symbol's length, so the shape of a bit
// has to be spelled out in fixed slots instead.

// 80MHz / 24 is exactly 3.3333MHz, so a slot is 300ns with nothing to round.
// That is the coarsest slot that divides both halves of both bit shapes, and so
// the shortest expansion: four slots to a colour bit, 1.2us, as before.
static const int STRIP_SLOT_HZ = 3333333;
static const uint32_t STRIP_SLOTS_PER_BIT = 4;

// A bit is 1.2us whichever it is; which one it is is where the edge inside it
// falls. 1000 is 0.3 high then 0.9 low, 1110 is 0.9 then 0.3.
static constexpr uint32_t strip_shape(uint32_t bit) {
  return bit ? 0xE : 0x8;
}

// The low period the strip reads as end of frame. 280us is 5.6x the 50us the
// datasheet asks and is what this strip has been driven at; 934 slots is the
// first value at or above it that a 300ns slot can express, 280.2us.
static const uint32_t STRIP_LATCH_SLOTS = 934;

static const uint32_t STRIP_PIXEL_SLOTS = NUM_PIXELS * 3 * 8 * STRIP_SLOTS_PER_BIT;
static const uint32_t STRIP_WAVE_BITS = STRIP_PIXEL_SLOTS + STRIP_LATCH_SLOTS;

// A colour byte is four wire bytes, so a whole word of waveform comes of one
// lookup and one store. Expanding a bit at a time would only move the work RMT
// was doing onto this side of the wire.
//
// Wire order is the order a store leaves in memory, which the low byte of a
// word reaches first.
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "wave words are stored in wire order");

struct StripExpansion {
  uint32_t word[256];
};

static constexpr StripExpansion strip_expansion() {
  StripExpansion expansion = {};

  for (uint32_t value = 0; value < 256; value++) {
    uint32_t word = 0;
    // Most significant colour bit first, two to a wire byte.
    for (uint32_t index = 0; index < 4; index++) {
      const uint32_t high = strip_shape((value >> (7 - 2 * index)) & 1);
      const uint32_t low = strip_shape((value >> (6 - 2 * index)) & 1);
      word |= ((high << 4) | low) << (8 * index);
    }
    expansion.word[value] = word;
  }

  return expansion;
}

static constexpr StripExpansion STRIP_EXPANSION = strip_expansion();

// One strip, for as long as the program runs, so neither is handed around and
// neither has a delete to reach.
//
// The waveform, not a second copy of the picture: a Frame is still the only
// place a pixel's duty is written down, and this is what that duty looks like
// on the wire. Words past the pixels are the latch, which is zero from load and
// is never written again.
static spi_device_handle_t stripDevice;
static uint32_t stripWave[(STRIP_WAVE_BITS + 31) / 32];

static void strip_start(void) {
  spi_bus_config_t busConfig = {};
  busConfig.mosi_io_num = PIN_WS2812B;
  // Data alone drives the strip; a zeroed config would name GPIO 0 for all of
  // these and hand the pin to the clock as well.
  busConfig.miso_io_num = -1;
  busConfig.sclk_io_num = -1;
  busConfig.quadwp_io_num = -1;
  busConfig.quadhd_io_num = -1;
  // Where the line sits between frames, which is where the latch already left
  // it, so an idle strip sees one continuous low and not an edge.
  busConfig.data_io_default_level = false;
  busConfig.max_transfer_sz = sizeof(stripWave);
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO));

  spi_device_interface_config_t deviceConfig = {};
  // PLL_F80M on this part, the 80MHz the slot rate is a whole division of.
  deviceConfig.clock_source = SPI_CLK_SRC_DEFAULT;
  deviceConfig.clock_speed_hz = STRIP_SLOT_HZ;
  deviceConfig.mode = 0;
  deviceConfig.spics_io_num = -1;
  // A frame is waited on before the next is rendered, so one is every
  // transaction there will ever be in flight.
  deviceConfig.queue_size = 1;
  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &deviceConfig, &stripDevice));

  // The bit shapes are only what they claim to be if the divider landed where
  // it was asked to, and nothing downstream would show that it had not.
  int slotKhz = 0;
  ESP_ERROR_CHECK(spi_device_get_actual_freq(stripDevice, &slotKhz));
  ESP_LOGI("strip", "%dkHz slot, %.1fns", slotKhz, 1000000.0 / slotKhz);
}

// Waited on rather than queued, so the frame the caller holds is not being read
// by the DMA once this returns and the next render can write over it.
static void strip_transmit(const Frame& frame) {
  for (uint32_t i = 0; i < sizeof(frame.wire); i++) {
    stripWave[i] = STRIP_EXPANSION.word[frame.wire[i]];
  }

  // Pixel bytes and the latch behind them are one transaction, so the strip
  // cannot see a gap where the frame ends. Length is in slots rather than
  // bytes, which is how the latch comes out at 934 of them.
  spi_transaction_t transaction = {};
  transaction.length = STRIP_WAVE_BITS;
  transaction.tx_buffer = stripWave;

  ESP_ERROR_CHECK(spi_device_transmit(stripDevice, &transaction));
}

#endif
