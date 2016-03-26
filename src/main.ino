#include "globals.hpp"

#include "commands.hpp"
#include "low-level.hpp"
#include "serial.hpp"

#ifndef BUILD_FULL
#define BUILD_FULL 1
#endif

// The conditional includes need to be prepended with # include instead of #include.
// Otherwise, the includes will be unconditionally hoisted by some arduino pre-processing.

void setup() {
  setup_low_level();
  setup_serial();
}

boolean execute_commands_full(char *buffer, int length, boolean send_ir);
boolean execute_commands_minimal(char *buffer, int length, boolean send_ir);

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
