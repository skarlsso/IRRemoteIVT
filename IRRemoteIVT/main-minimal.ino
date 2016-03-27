// Control an IVT Nordic Inverter heat pump by sending raw bytes over
// the Serial port to a SparkFun Pro Micro Arduino board (ATmega 32u4).
//
// The remote controller state machine is implemented externally.
//
// Pin 9 => transistor connected to a 940 nm IR LED.
//
// Pin 0 => Serial Port tx
// Pin 1 => Serial Port rx
//
// Command:
//  "raw <13 bytes hex number>
//  E.g "raw 555AF308008804001001000F88"
//
// @author Stefan Karlsson (skarlsso@github)

#include "globals.h"

#include "commands.h"
#include "debug.h"
#include "ir-data.h"
#include "low-level.h"
#include "serial.h"
#include "utils.h"

static void execute_raw(char *buffer, int length, bool send_ir) {
  TRIM(buffer, length);

  if (length != NUM_IR_BYTES * 2) {
    // Illegal argument.
    SerialUI.print("Incorrect number of digits: "); SerialUI.println(length);
    return;
  }

  uint8_t raw_data[NUM_IR_BYTES];

  for (int i = 0; i < NUM_IR_BYTES; i++) {
     uint8_t high = hex_value(buffer[i * 2 + 0]);
     uint8_t low  = hex_value(buffer[i * 2 + 1]);
     if (high == -1 || low == -1) {
        SerialUI.print("Garbage Input: "); SerialUI.print(buffer[i * 2 + 0]); SerialUI.println(buffer[i * 2 + 1]);
        return; // Garbage input
     }
     raw_data[i] = high * 16 +   low;
  }

  uint8_t parity = calculate_parity(raw_data);
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

static void execute_minimal_help(char *buffer, int length, bool transmit /* ignored */) {
  SerialUI.println("Commands:");
  SerialUI.println("  help - Print this help text");
  SerialUI.println("  raw  - Send the raw ir data bytes");
  SerialUI.println("         E.g. \"raw 555AF308008804001001000F88\"");
}

bool execute_commands_minimal(char *buffer, int length, bool send_ir) {
  execute_command_cond("raw",  raw);
  execute_command_cond("help", minimal_help);

  SerialUI.write("No such command: '");
  SerialUI.write((uint8_t*)buffer, length);
  SerialUI.write("'\r\n");

  return false;
}
