// Control an IVT Nordic Inverter heat pump by sending commands over
// the Serial port to a SparkFun Pro Micro Arduino board (ATmega 32u4).
//
// Pin 9 => transistor connected to a 940 nm IR LED.
//
// Pin 0 => Serial Port tx
// Pin 1 => Serial Port rx
//
// Example commands:
//  "on"      - Turn on the heat pump.
//  "off"     - Turn off the heat pump.
//  "heat"    - Turn on the heating mode.
//  "fan"     - Turn on the fan with the heating element turned off.
//  "temp +"  - Increase the temperature one degree.
//  "temp -"  - Decrease the temperature one degree.
//  "temp 25" - Set the temperature to 25 degrees.
//  "full on" - Turn on full effect.
//   ... and more
//
// @author Stefan Karlsson (skarlsso@github)

#include "commands.hpp"
#include "debug.hpp"
#include "low-level.hpp"
#include "serial.hpp"
#include "utils.hpp"

static void execute_raw(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  if (length != NUM_IR_BYTES * 2) {
    // Illegal argument.
    SerialUI.print("Incorrect number of digits: "); SerialUI.println(length);
    return;
  }

  byte raw_data[NUM_IR_BYTES];

  for (int i = 0; i < NUM_IR_BYTES; i++) {
     byte high = hex_value(buffer[i * 2 + 0]);
     byte low  = hex_value(buffer[i * 2 + 1]);
     if (high == -1 || low == -1) {
        SerialUI.print("Garbage Input: "); SerialUI.print(buffer[i * 2 + 0]); SerialUI.println(buffer[i * 2 + 1]);
        return; // Garbage input
     }
     raw_data[i] = high * 16 +   low;
  }

  byte parity = calculate_parity(raw_data);
  if (parity != (raw_data[NUM_IR_BYTES - 1] & 0x0F)) {
    SerialUI.print("Invalid parity. Expected: ");
    SerialUI.print(parity);
    SerialUI.print(" got: ");
    SerialUI.println(raw_data[NUM_IR_BYTES - 1] & 0x0F);
    return;
  }

  replace_ir_data(raw_data);

  if (true) {
    dump_ir_data();
  }

  if (send_ir) {
    ir_data_send();
  }
}

boolean execute_commands_minimal(char *buffer, int length, boolean send_ir) {
  execute_command_cond("raw", raw);

  SerialUI.write("No such command: '");
  SerialUI.write((byte*)buffer, length);
  SerialUI.write("'\r\n");

  return false;
}
