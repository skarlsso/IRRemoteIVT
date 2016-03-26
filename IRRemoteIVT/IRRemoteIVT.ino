// Control an IVT Nordic Inverter heat pump by sending commands over
// the Serial port to a SparkFun Pro Micro Arduino board (ATmega 32u4).
//
// Main entry point - dispatches to either the full version or the minimal
// version depending on the compiler flags used.
//
// @author Stefan Karlsson (skarlsso@github)
#include "globals.h"

#include "commands.h"
#include "low-level.h"
#include "serial.h"

#ifndef BUILD_FULL
#define BUILD_FULL 1
#endif

// The conditional includes need to be prepended with # include instead of #include.
// Otherwise, the includes will be unconditionally hoisted by some arduino pre-processing.

void setup() {
  setup_low_level();
  setup_serial();
}

bool execute_commands_full(char *buffer, int length, bool send_ir);
bool execute_commands_minimal(char *buffer, int length, bool send_ir);

void loop()  {
#if BUILD_FULL
  // Execute the full version, with the entire remote controller state machine implemented in the Arduino.
  commands_loop(&execute_commands_full);
#else
  // Execute the minimal version, which only sends raw uint8_ts to the heat pump.
  // The remote controller state machine is implemented externally.
  commands_loop(&execute_commands_minimal);
#endif
}
