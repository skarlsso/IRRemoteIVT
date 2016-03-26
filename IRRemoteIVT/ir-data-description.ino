#include "globals.h"

#include "ir-data-description.h"
#include "ir-data.h"

// Helper functions to extract and set values in the BitSegments.

// Invert the 'bits' number of bits in the 'value' uint8_t.
static uint8_t invert(uint8_t value, uint8_t bits) {
  uint8_t inverted = 0;
  for (int i = 0; i < bits; i++) {
    inverted <<= 1;
    inverted |= value & 1;
    value >>= 1;
  }

  return inverted;
}

static uint8_t maybe_invert(const BitSegment& bs, uint8_t value) {
  return (bs.inverted == 1) ? invert(value, bs.bits) : value;
}

static uint8_t bs_mask(const BitSegment& bs) {
  return ((1 << bs.bits) - 1) << bs.offset;
}

static void ir_data_clear_bits(const BitSegment& bs) {
  ir_data[bs.index] &= ~bs_mask(bs);
}

static void ir_data_set_bits(const BitSegment& bs, uint8_t value) {
  ir_data[bs.index] |= ((value << bs.offset) & bs_mask(bs));
}

void set_ir_data(const BitSegment& bs, uint8_t value) {
  ir_data_clear_bits(bs);
  ir_data_set_bits(bs, maybe_invert(bs, value));
}

uint8_t get_ir_data(const BitSegment& bs) {
  return maybe_invert(bs, ((ir_data[bs.index] & bs_mask(bs)) >> bs.offset));
}
