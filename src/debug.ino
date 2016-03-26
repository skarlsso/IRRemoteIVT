#include "globals.hpp"

#include "serial.hpp"
#include "low-level.hpp"

void dump_ir_data() {
  for (int i = 0; i < NUM_IR_BYTES; i++) {
    // For all uint8_ts
    int uint8_t_value = ir_data[i];
    for (int j = 0; j < 8; j++) {
      // For all bits
      int bit_value = (uint8_t_value & (1 << 7)) == 0 ? 0 : 1;
      uint8_t_value <<= 1;

      SerialUI.print(bit_value);
    }
    SerialUI.print(' ');
  }
  SerialUI.println("");
}
