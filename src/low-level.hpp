#ifndef IRREMOTEIVT_LOW_LEVEL_HPP
#define IRREMOTEIVT_LOW_LEVEL_HPP

#include <stdint.h>

// Bytes per IR package
#define NUM_IR_BYTES 13

void setup_low_level();

extern volatile byte ir_data[NUM_IR_BYTES];

// The BitSegment describe segments of the IR package.
struct BitSegment {
  byte index;     // byte position in the IR package
  byte offset;    // bit offset in the byte
  byte bits;      // bits in field
  byte inverted;  // if the bits should reversed before used as a number
};

const BitSegment bs_abs_temp          = { 4, 4, 4, 1};
const BitSegment bs_rel_temp          = { 4, 2, 2, 1};
const BitSegment bs_unknown0_0        = { 4, 1, 1, 0};
const BitSegment bs_rel_temp_sign     = { 4, 0, 1, 0};

const BitSegment bs_unknown1_1000     = { 5, 4, 4, 0};
const BitSegment bs_state             = { 5, 1, 3, 0};
const BitSegment bs_unknown2_0        = { 5, 0, 1, 0};

const BitSegment bs_mode              = { 6, 6, 2, 0};
const BitSegment bs_clean             = { 6, 4, 2, 0};
const BitSegment bs_fan_strength      = { 6, 1, 3, 0};
const BitSegment bs_unknown3_0        = { 6, 0, 1, 0};

const BitSegment bs_time_hours        = { 7, 3, 5, 1};
const BitSegment bs_set_clear_state   = { 7, 0, 3, 0};

const BitSegment bs_rotate            = { 8, 5, 3, 0};
const BitSegment bs_unknown4_10000    = { 8, 0, 5, 0};

const BitSegment bs_unknown5_00000001 = { 9, 0, 8, 0};

const BitSegment bs_time_minutes      = {10, 2, 6, 1};
const BitSegment bs_set_clear         = {10, 1, 1, 0};
const BitSegment bs_unknown6_0        = {10, 0, 1, 0};

const BitSegment bs_unknown7_00       = {11, 6, 2, 0};
const BitSegment bs_ion               = {11, 5, 1, 0};
const BitSegment bs_unknown8_01111    = {11, 0, 5, 0};

const BitSegment bs_unknown9_1000     = {12, 4, 4, 0};
const BitSegment bs_parity            = {12, 0, 4, 0};


// Values for bs_mode
#define MODE_FAN  0b00
#define MODE_HEAT 0b10
#define MODE_COOL 0b01
#define MODE_DRY  0b11
inline const char* mode_string(byte mode) {
  switch (mode) {
    case MODE_FAN:  return "fan";
    case MODE_HEAT: return "heat";
    case MODE_COOL: return "cool";
    case MODE_DRY:  return "dry";
    default:        return "unknown";
  }
}

// Values for bs_state
#define STATE_ON              0b100
#define STATE_OFF             0b010
#define STATE_CMD             0b110
#define STATE_FULL_EFFECT_ON  0b011
#define STATE_FULL_EFFECT_OFF 0b111
inline const char* state_string(byte value) {
  switch (value) {
    case STATE_ON:              return "on";
    case STATE_OFF:             return "off";
    case STATE_CMD:             return "command";
    case STATE_FULL_EFFECT_ON:  return "full effect on";
    case STATE_FULL_EFFECT_OFF: return "full effect off";
    default:                    return "unknown";
  }
}

// Values for bs_rotate
#define ROTATE_ON    0b011
#define ROTATE_OFF   0b101
#define ROTATE_SWING 0b111
inline const char* rotate_string(byte value) {
  switch (value) {
    case ROTATE_ON:    return "on";
    case ROTATE_OFF:   return "off";
    case ROTATE_SWING: return "swing";
    default:           return "unknown";
  }
}

// Values for bs_clean
#define CLEAN_ON   0b01
#define CLEAN_OFF  0b10
inline const char* clean_string(byte value) {
  switch (value) {
    case CLEAN_ON:  return "on";
    case CLEAN_OFF: return "off";
    default:        return "unknown";
  }
}

// Values for bs_fan_strength
#define STRENGTH_SLOW   0b110
#define STRENGTH_MEDIUM 0b101
#define STRENGTH_FAST   0b111
#define STRENGTH_AUTO   0b010
inline const char* strength_string(byte value) {
  switch (value) {
    case STRENGTH_SLOW:   return "slow";
    case STRENGTH_MEDIUM: return "medium";
    case STRENGTH_FAST:   return "fast";
    case STRENGTH_AUTO:   return "auto";
    default:              return "unknown";
  }
}

// Values for bs_ion
#define ION_ON  0b1
#define ION_OFF 0b0
inline const char* ion_string(byte value) {
  switch (value) {
    case ION_ON:  return "on";
    case ION_OFF: return "off";
    default:      return "unknown";
  }
}

// Transmit the ir_data bits.
void ir_data_send();

byte calculate_parity(byte* ir_buffer);

void replace_ir_data(byte data[NUM_IR_BYTES]);

// Update the IR data package with the correct 4 bit parity.
void ir_data_update_parity();

// Invert the 'bits' number of bits in the 'value' byte.
byte invert(byte value, byte bits);

// Helper macros to extract and set values in the BitSegments.

#define MAYBE_INVERT(bs, value) ((bs.inverted == 1) ? invert(value, bs.bits) : value)

#define BS_MASK(bs) (((1 << bs.bits) - 1) << bs.offset)
#define IR_DATA_CLEAR_BITS(bs)      ir_data[bs.index] &= ~BS_MASK(bs)
#define IR_DATA_SET_BITS(bs, value) ir_data[bs.index] |= (((value) << bs.offset) & BS_MASK(bs))

#define set_ir_data(bf, value) IR_DATA_CLEAR_BITS(bf); IR_DATA_SET_BITS(bf, MAYBE_INVERT(bf, value));
#define get_ir_data(bf) MAYBE_INVERT(bf, ((ir_data[bf.index] & BS_MASK(bf)) >> bf.offset))

#endif // IRREMOTEIVT_LOW_LEVEL_HPP
