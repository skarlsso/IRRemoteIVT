#ifndef IRREMOTEIVT_IR_DATA_HPP
#define IRREMOTEIVT_IR_DATA_HPP

// Bytes per IR package
#define NUM_IR_BYTES 13

// Reference to the IR data package.
extern volatile uint8_t ir_data[NUM_IR_BYTES];

inline void replace_ir_data(uint8_t data[NUM_IR_BYTES]) {
  for (int i = 0; i < NUM_IR_BYTES; i++) {
    ir_data[i] = data[i];
  }
}

inline uint8_t calculate_parity(uint8_t* ir_buffer) {
  uint8_t parity = 0;

  // Calculate 8 uint8_ts parity, including the previous parity.
  for (int i = 0; i < NUM_IR_BYTES; i++) {
    parity ^= ir_buffer[i];
  }

  // 8 uint8_ts parity => 4 uint8_ts parity
  parity ^= (parity >> 4);
  parity &= 0x0F;

  // xor:ing will get rid of previous parity.
  return (ir_buffer[NUM_IR_BYTES - 1] ^ parity) & 0x0F;
}

// Update the IR data package with the correct 4 bit parity.
inline void ir_data_update_parity() {
  ir_data[NUM_IR_BYTES - 1] &= 0xF0;
  ir_data[NUM_IR_BYTES - 1] |= calculate_parity((uint8_t*)ir_data);
}

#endif // IRREMOTEIVT_IR_DATA_HPP
