#ifndef STRIP_HPP
#define STRIP_HPP

#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "soc/soc_caps.h"

#include "Constants.h"
#include "Frame.hpp"

// The RMT channel and the encoder that put a Frame on the wire. Held here
// rather than reached through a driver that keeps pixels of its own, so the
// bytes an animation rendered are the bytes the strip is sent.

// One tick per 100ns. Every interval below is a whole number of ticks at this
// rate, so nothing in the timing depends on how a fraction rounds.
static const uint32_t STRIP_RESOLUTION_HZ = 10 * 1000 * 1000;

static constexpr uint16_t strip_ticks(uint32_t nanoseconds) {
  return (uint16_t)((uint64_t)STRIP_RESOLUTION_HZ * nanoseconds / 1000000000);
}

// A bit is 1.2us whichever it is; which one it is is where the edge inside it
// falls.
static const uint16_t STRIP_T0H_TICKS = strip_ticks(300);
static const uint16_t STRIP_T0L_TICKS = strip_ticks(900);
static const uint16_t STRIP_T1H_TICKS = strip_ticks(900);
static const uint16_t STRIP_T1L_TICKS = strip_ticks(300);

// The low period the strip reads as end of frame. 280us is 5.6x the 50us the
// datasheet asks and is what this strip has been driven at. One symbol carries
// two levels, so each half holds 140us.
static const uint16_t STRIP_LATCH_TICKS = strip_ticks(140000);

// Pixel bytes and the latch behind them are one transaction, so the strip
// cannot see a gap where the frame ends.
struct StripEncoder {
  rmt_encoder_t base;
  rmt_encoder_t* bytes;
  rmt_encoder_t* copy;
  rmt_symbol_word_t latch;
  bool latching;
};

// Runs from the transmit ISR as the channel's symbol memory drains, which is
// why it is in IRAM: 48 symbols of channel memory against a frame's 1200 bits
// means most of a frame is encoded from the interrupt.
RMT_ENCODER_FUNC_ATTR
static size_t strip_encode(rmt_encoder_t* encoder, rmt_channel_handle_t channel,
                           const void* data, size_t size, rmt_encode_state_t* ret_state) {
  StripEncoder* strip = (StripEncoder*)encoder;
  rmt_encode_state_t session = RMT_ENCODING_RESET;
  uint32_t state = RMT_ENCODING_RESET;
  size_t symbols = 0;

  // Which of the two a resumed call is in the middle of.
  if (!strip->latching) {
    symbols += strip->bytes->encode(strip->bytes, channel, data, size, &session);
    if (session & RMT_ENCODING_COMPLETE) {
      strip->latching = true;
    }
    if (session & RMT_ENCODING_MEM_FULL) {
      *ret_state = RMT_ENCODING_MEM_FULL;
      return symbols;
    }
  }

  symbols += strip->copy->encode(strip->copy, channel, &strip->latch, sizeof(strip->latch), &session);
  if (session & RMT_ENCODING_COMPLETE) {
    strip->latching = false;
    state |= RMT_ENCODING_COMPLETE;
  }
  if (session & RMT_ENCODING_MEM_FULL) {
    state |= RMT_ENCODING_MEM_FULL;
  }

  *ret_state = (rmt_encode_state_t)state;
  return symbols;
}

RMT_ENCODER_FUNC_ATTR
static esp_err_t strip_encoder_reset(rmt_encoder_t* encoder) {
  StripEncoder* strip = (StripEncoder*)encoder;
  rmt_encoder_reset(strip->bytes);
  rmt_encoder_reset(strip->copy);
  strip->latching = false;
  return ESP_OK;
}

// One strip, for as long as the program runs, so neither is handed around and
// neither has a delete to reach.
static rmt_channel_handle_t stripChannel;
static StripEncoder stripEncoder;

static void strip_start(void) {
  rmt_tx_channel_config_t channelConfig = {};
  channelConfig.gpio_num = (gpio_num_t)PIN_WS2812B;
  channelConfig.clk_src = RMT_CLK_SRC_DEFAULT;
  channelConfig.resolution_hz = STRIP_RESOLUTION_HZ;
  channelConfig.mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL;
  // A frame is waited on before the next is rendered, so one descriptor is
  // every transaction there will ever be in flight.
  channelConfig.trans_queue_depth = 1;
  // The C6's RMT has no DMA, so the ISR is what refills the channel.
  channelConfig.flags.with_dma = false;
  ESP_ERROR_CHECK(rmt_new_tx_channel(&channelConfig, &stripChannel));

  rmt_bytes_encoder_config_t bytesConfig = {};
  bytesConfig.bit0.level0 = 1;
  bytesConfig.bit0.duration0 = STRIP_T0H_TICKS;
  bytesConfig.bit0.level1 = 0;
  bytesConfig.bit0.duration1 = STRIP_T0L_TICKS;
  bytesConfig.bit1.level0 = 1;
  bytesConfig.bit1.duration0 = STRIP_T1H_TICKS;
  bytesConfig.bit1.level1 = 0;
  bytesConfig.bit1.duration1 = STRIP_T1L_TICKS;
  bytesConfig.flags.msb_first = 1;
  ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytesConfig, &stripEncoder.bytes));

  rmt_copy_encoder_config_t copyConfig = {};
  ESP_ERROR_CHECK(rmt_new_copy_encoder(&copyConfig, &stripEncoder.copy));

  stripEncoder.base.encode = strip_encode;
  stripEncoder.base.reset = strip_encoder_reset;
  stripEncoder.latch.level0 = 0;
  stripEncoder.latch.duration0 = STRIP_LATCH_TICKS;
  stripEncoder.latch.level1 = 0;
  stripEncoder.latch.duration1 = STRIP_LATCH_TICKS;

  // Enabled once and left enabled. Enabling around each transmit resets the
  // channel every frame and leaves the line exactly where the latch already
  // left it, so it buys nothing that is visible on the wire.
  ESP_ERROR_CHECK(rmt_enable(stripChannel));
}

// Waited on rather than queued, so the frame the caller holds is not being read
// by the encoder once this returns and the next render can write over it.
static void strip_transmit(const Frame& frame) {
  rmt_transmit_config_t transmitConfig = {};
  transmitConfig.loop_count = 0;

  ESP_ERROR_CHECK(rmt_transmit(stripChannel, &stripEncoder.base, frame.wire, sizeof(frame.wire), &transmitConfig));
  ESP_ERROR_CHECK(rmt_tx_wait_all_done(stripChannel, -1));
}

#endif
