#include "globals.h"

#include "serial.h"
#include "ir-data.h"

void dump_ir_data() {
  for (int i = 0; i < NUM_IR_BYTES; i++) {
    // For all bytes
    int byte_value = ir_data[i];
    for (int j = 0; j < 8; j++) {
      // For all bits
      int bit_value = (byte_value & (1 << 7)) == 0 ? 0 : 1;
      byte_value <<= 1;

      SerialUI.print(bit_value);
    }
    SerialUI.print(' ');
  }
  SerialUI.println("");
}
