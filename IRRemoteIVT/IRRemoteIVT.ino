// Control an IVT Nordic Inverter heat pump by sending commands over
// the Serial port to an Arduino board.
//
// Main entry point - dispatches to either the full version or the minimal
// version depending on the compiler flags used.
//
// See: main-full.ino and main-minimal.ino.
//
// @author Stefan Karlsson (skarlsso@github)

#include "globals.h"

#include "commands.h"
#include "low-level.h"
#include "serial.h"

// Default to build the full target.
#ifndef BUILD_FULL
#define BUILD_FULL 1
#endif

void setup() {
  setup_low_level();
  setup_serial();
}

void loop()  {
#if BUILD_FULL
  // Execute the full version, with the entire remote controller state machine implemented in the Arduino.
  bool execute_commands_full(char *buffer, int length, bool send_ir);
  commands_loop(&execute_commands_full);
#else
  // Execute the minimal version, which only sends raw uint8_ts to the heat pump.
  // The remote controller state machine is implemented externally.
  bool execute_commands_minimal(char *buffer, int length, bool send_ir);
  commands_loop(&execute_commands_minimal);
#endif
}
