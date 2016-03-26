#include "ir-data.hpp"

// The IR data package.
//
// Bytes containing the state which will be send upon request.
// This following values are a reasonable default.
volatile uint8_t ir_data[NUM_IR_BYTES] =
   { 0x55,    0x5A,    0xF3,    0x08,    0x00,    0x88,    0x04,    0x00,    0x10,    0x01,    0x00,    0x0F,    0x88 };
 //  Always the same____________________ TEMP  0  1000XXX  MO  FAN  x60m_    RRX10000 00000001 MINUTS 0 00N01111 1000CRC_
 //  01010101 01011010 11110011 00001000 00000000 10001000 00000100 00000000 00010000 00000001 00000000 00001111 10001000
