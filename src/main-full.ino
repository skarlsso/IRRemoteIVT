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

#include "globals.hpp"

#include "commands.hpp"
#include "debug.hpp"
#include "low-level.hpp"
#include "serial.hpp"
#include "utils.hpp"

// Make the IR data package complete and send it out to the IR port.
void ir_data_finalize_and_send(uint8_t state, boolean send_ir, uint8_t debug = 0) {
  // Must always set state.
  set_ir_data(bs_state, state);
  ir_data_update_parity();

  if (debug || dump_all_ir_data) {
    dump_ir_data();
  }

  if (send_ir) {
    ir_data_send();
  }
}


// Commands
// ========

void execute_on(char *buffer, int length, boolean send_ir) {
  // Ignore parameters.
  ir_data_finalize_and_send(STATE_ON, send_ir);
}

void execute_off(char *buffer, int length, boolean send_ir) {
  // Ignore parameters.
  ir_data_finalize_and_send(STATE_OFF, send_ir);
}

#define print_change(str, old_value, new_value)                                        \
  SerialUI.print("Old " #str " value: "); SerialUI.println(str ## _string(old_value)); \
  SerialUI.print("New " #str " value: "); SerialUI.println(str ## _string(new_value))

// Turn on/off the "Plasma Cluster"/ion mode.
void execute_ion(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t old_value = get_ir_data(bs_ion);
  uint8_t new_value;

  if (length == 0) {
    // Toggle the value.
    new_value = (old_value == ION_ON) ? ION_OFF : ION_ON;

  } else if (str_equals(buffer, "on", length)) {
    new_value = ION_ON;

  } else if (str_equals(buffer, "off", length)) {
    new_value = ION_OFF;

  } else {
    // Illegal argument value
    return;
  }

  print_change(ion, old_value, new_value);

  set_ir_data(bs_ion, new_value);
  ir_data_finalize_and_send(STATE_CMD, send_ir);
}


// Helper functions to get/set the temperature.

uint8_t mode_uses_absolute_time() {
   uint8_t mode = get_ir_data(bs_mode);
  return mode == MODE_HEAT || mode == MODE_COOL;
}
int max_temp_for_mode() {
  return mode_uses_absolute_time() ? 32 : 2;
}

int min_temp_for_mode() {
  return mode_uses_absolute_time() ? 18 : -2;
}

int get_temp_for_mode() {
  if (mode_uses_absolute_time()) {
    // The temp bits are relative +17C.
    return 17 + get_ir_data(bs_abs_temp);
  } else {
   return (get_ir_data(bs_rel_temp_sign) == 0 ? 1 : -1) * get_ir_data(bs_rel_temp);
  }
}

void set_temp_for_mode(int temp) {
  if (mode_uses_absolute_time()) {
    set_ir_data(bs_abs_temp, temp - 17);
  } else {
    set_ir_data(bs_rel_temp, (temp < 0 ? -temp : temp));
    set_ir_data(bs_rel_temp_sign, (temp < 0 ? 1 : 0));
  }
}

// Request a given temperature.
void execute_temp(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  if (length <= 0) {
     // Nothing to do.
     return;
  }

  int old_temp = get_temp_for_mode();
  int new_temp = 0;

  if (length == 1 && buffer[0] == '+') {
    new_temp = old_temp + ((old_temp < max_temp_for_mode()) ? 1 : 0);

  } else if (length == 1 && buffer[0] == '-') {
    new_temp = old_temp - ((old_temp > min_temp_for_mode()) ? 1 : 0);

  } else {
    uint8_t negative = buffer[0] == '-';

    int value = a_to_positive_number(buffer + (negative ? 1 : 0), length - (negative ? 1 : 0));
    if (value == -1) {
      return; // Error
    }

    value = negative ? -value : value;

    SerialUI.print("Requesting temp: "); SerialUI.println(value);

    // Ignore special 10C value, for now.
    int min_temp = min_temp_for_mode();
    int max_temp = max_temp_for_mode();

    SerialUI.print("Min temp: "); SerialUI.println(min_temp);
    SerialUI.print("Max temp: "); SerialUI.println(max_temp);

    if (value > max_temp) {
      value = max_temp;
    }
    if (value < min_temp) {
      value = min_temp;
    }

    new_temp = value;
  }

  SerialUI.print("Old temp: "); SerialUI.println(old_temp);
  SerialUI.print("New temp: "); SerialUI.println(new_temp);

  set_temp_for_mode(new_temp);
  ir_data_finalize_and_send(STATE_CMD, send_ir);
}

// Turn on/off the Cleaning program.
void execute_clean(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t state     = get_ir_data(bs_state);
  uint8_t old_value = get_ir_data(bs_clean);
  uint8_t new_value = old_value;

  if (length == 0) {
    // Toggle.
    if (old_value == CLEAN_ON) {
       new_value = CLEAN_OFF;
    } else if (old_value == CLEAN_OFF) {
      new_value = CLEAN_ON;
    } else {
      // Don't know.
    }
  } else if (str_equals(buffer, "on", length)) {
    new_value = CLEAN_ON;
  } else if (str_equals(buffer, "off", length)) {
    new_value = CLEAN_OFF;
  } else {
    // Illegal argument
    return;
  }

  if (new_value == CLEAN_ON && state != STATE_OFF) {
     // Only start if turned off.
    SerialUI.println("Can't clean when device is on");
    return;
  }

  if (new_value == CLEAN_ON) {
    SerialUI.println("Request start cleaning");
  } else if (new_value == CLEAN_OFF) {
    SerialUI.println("Request stop cleaning");
  } else {
    // Don't know.
  }

  set_ir_data(bs_clean, new_value);
  ir_data_finalize_and_send(STATE_CMD, send_ir);
}

// FIXME: Implement the timer features.
void execute_timer(char *buffer, int length, boolean send_ir) {
  SerialUI.println("Command not implemented: timer");
}

// Turn on the selected mode.
void execute_mode_selected(uint8_t mode, boolean send_ir) {
  uint8_t old_value = get_ir_data(bs_mode);

  print_change(mode, old_value, mode);

  set_ir_data(bs_mode, mode);

  // The remote sets the temp when changing modes. Is this needed/wanted?.
  if (mode == MODE_HEAT) {
    set_ir_data(bs_abs_temp, 23 - 17);
  } else if (mode == MODE_COOL) {
    set_ir_data(bs_abs_temp, 26 - 17);
  } else if (mode == MODE_FAN) {
    set_ir_data(bs_abs_temp, 0);
  } else if (mode == MODE_DRY) {
    set_ir_data(bs_abs_temp, 0);
  }
  set_ir_data(bs_rel_temp, 0);
  set_ir_data(bs_rel_temp_sign, 0);

  ir_data_finalize_and_send(STATE_CMD, send_ir);
}

// Turn on Heat mode.
void execute_heat(char *buffer, int length, boolean send_ir) {
  execute_mode_selected(MODE_HEAT, send_ir);
}

// Turn on Cool mode.
void execute_cool(char *buffer, int length, boolean send_ir) {
  execute_mode_selected(MODE_COOL, send_ir);
}

// Turn on Fan mode.
void execute_fan(char *buffer, int length, boolean send_ir) {
  execute_mode_selected(MODE_FAN, send_ir);
}

// Turn on Dry mode.
void execute_dry(char *buffer, int length, boolean send_ir) {
  execute_mode_selected(MODE_DRY, send_ir);
}

// Cycle through the modes.
void execute_mode(char* buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t old_value = get_ir_data(bs_mode);
  uint8_t new_value;

  if (length == 0) {
    // Cycle through the modes.
    new_value = (old_value + 1) & 0b11;

  } else if (str_equals(buffer, "fan", length)) {
    new_value = MODE_FAN;

  } else if (str_equals(buffer, "heat", length)) {
    new_value = MODE_HEAT;

  } else if (str_equals(buffer, "cool", length)) {
    new_value = MODE_COOL;

  } else if (str_equals(buffer, "dry", length)) {
    new_value = MODE_DRY;

  } else {
    // Illegal argument.
    return;
  }

  execute_mode_selected(new_value, send_ir);
}

// Start/stop swing the air outlet.
void execute_swing(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t new_value = ROTATE_SWING;

  // Ignore arguments.
  set_ir_data(bs_rotate, new_value);
  ir_data_finalize_and_send(STATE_CMD, send_ir);
}

// Turn on/off the Rotate feature.
// FIXME: Figure out what this is.
void execute_rotate(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t old_value = get_ir_data(bs_rotate);
  uint8_t new_value = old_value;

  if (length == 0) {
    // Toggle.
    if (old_value == ROTATE_ON) {
      new_value = ROTATE_OFF;
    } else if (old_value == ROTATE_OFF) {
      new_value = ROTATE_ON;
    } else if (old_value == ROTATE_SWING) {
      new_value = ROTATE_ON;
    }
  } else if (str_equals(buffer, "on", length)) {
    new_value = ROTATE_ON;
  } else if (str_equals(buffer, "off", length)) {
    new_value = ROTATE_OFF;
  }

  print_change(rotate, old_value, new_value);

  set_ir_data(bs_rotate, new_value);
  ir_data_finalize_and_send(STATE_CMD, send_ir);
}

// Turn on/off the Full Effect feature.
void execute_full_effect(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t old_value = get_ir_data(bs_state);
  uint8_t new_value = old_value;

  if (length == 0) {
    // Toggle.
    if (old_value == STATE_FULL_EFFECT_OFF) {
      new_value = STATE_FULL_EFFECT_ON;

    } else if (old_value == STATE_FULL_EFFECT_ON) {
      new_value = STATE_FULL_EFFECT_OFF;

    } else if (old_value != STATE_OFF) {
      // Turn full effect on.
      new_value = STATE_FULL_EFFECT_ON;
    } else {
      // Do nothing when the IVT is off.
    }

  } else if (str_equals(buffer, "on", length)) {
    if (old_value != STATE_OFF) {
      new_value = STATE_FULL_EFFECT_ON;
    }

  } else if (str_equals(buffer, "off", length)) {
    if (old_value == STATE_FULL_EFFECT_ON) {
      new_value = STATE_FULL_EFFECT_OFF;
    }
  } else {
    // Illegal argument.
    return;
  }

  print_change(state, old_value, new_value);


  if (new_value == STATE_FULL_EFFECT_ON || new_value == STATE_FULL_EFFECT_OFF) {
    // The remote sets this time.
    // FIXME: Need to save the old timer value, and restore it later when STATE_CMD events are sent.
    set_ir_data(bs_time_hours, 0);
    set_ir_data(bs_time_minutes, 1);
  }

  ir_data_finalize_and_send(new_value, send_ir);
}

// Set fan strength.
void execute_strength(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);

  uint8_t old_value = get_ir_data(bs_fan_strength);
  uint8_t new_value;

  if (length == 0) {
    // Cycle through the values.
    new_value = old_value == STRENGTH_AUTO   ? STRENGTH_SLOW   :
                old_value == STRENGTH_SLOW   ? STRENGTH_MEDIUM :
                old_value == STRENGTH_MEDIUM ? STRENGTH_FAST   :
                old_value == STRENGTH_FAST   ? STRENGTH_AUTO   :
                /* Fallback */                 STRENGTH_AUTO;

  } else if (str_equals(buffer, "slow", length)) {
    new_value = STRENGTH_SLOW;

  } else if (str_equals(buffer, "medium", length)) {
    new_value = STRENGTH_MEDIUM;

  } else if (str_equals(buffer, "fast", length)) {
    new_value = STRENGTH_FAST;

  } else if (str_equals(buffer, "auto", length)) {
    new_value = STRENGTH_AUTO;

  } else {
    // Illegal arguments.
    return;
  }


  print_change(strength, old_value, new_value);

  set_ir_data(bs_fan_strength, new_value);
  ir_data_finalize_and_send(STATE_CMD, send_ir);
}


void execute_dump(char* buffer, int length, boolean send_ir) {
  dump_ir_data();
}


int index_of(const char *buffer, int length, char c) {
  for (int i = 0; i < length; i++) {
    if (buffer[i] == '\0')  {
      return -1;
    }
    if (buffer[i] == c) {
      return i;
    }
  }
  return -1;
}

// Forward declaration
boolean execute_commands_full(char *buffer, int length, boolean send_ir);

// Executes multiple commands, but onlysend one ir command.
// Format: command1 <args...>, command2 <args...>, ..., commandN <args...>,
// The last command determines the CMD_* state to send.
void execute_multi(char *buffer, int length, boolean send_ir) {
  TRIM(buffer, length);
  while (true) {
    int delimiter_index = index_of(buffer, length, ',');
    if (delimiter_index == -1) {
      break;
    }

    execute_commands_full(buffer, delimiter_index, false /* don't send the ir command */);

    buffer += delimiter_index + 1;
    length -= delimiter_index + 1;
  }

  ir_data_send();
}

void execute_help(char *buffer, int length, boolean transmit /* ignored */) {
  SerialUI.println("Commands:");
  SerialUI.println("  help  - Print this help text");
  SerialUI.println("  on    - Turn on the device");
  SerialUI.println("  off   - Turn off the device");
  SerialUI.println("  heat  - Turn on heat mode");
  SerialUI.println("  cool  - Turn on cool mode");
  SerialUI.println("  fan   - Turn on fan mode");
  SerialUI.println("  dry   - Turn on dry mode");
  SerialUI.println("  swing - Start or stop the swing");
  SerialUI.println("  ion      [on|off] - Toggle or turn on/off ion mode");
  SerialUI.println("  clean    [on|off] - Toggle or turn on/off clean mode.");
  SerialUI.println("  full     [on|off] - Toggle or turn on/off full-effect mode");
  SerialUI.println("  rotate   [on|off] - Turn on/off auto rotate mode?");
  SerialUI.println("  mode     [heat|cool|fan|dry]     - Cycle through or select mode");
  SerialUI.println("  strength [slow|medium|fast|auto] - Cycle through or select fan speed");
  SerialUI.println("  temp     <+|-|absolute value (18 to 32)|relative value (-2 to 2)]>");
}

boolean execute_commands_full(char *buffer, int length, boolean send_ir) {
  // All available commands:
  execute_command_cond("on",       on);
  execute_command_cond("off",      off);
  execute_command_cond("temp",     temp);
  execute_command_cond("ion",      ion);
  execute_command_cond("clean",    clean);
  execute_command_cond("TIMER",    timer);
  execute_command_cond("heat",     heat);
  execute_command_cond("cool",     cool);
  execute_command_cond("fan",      fan);
  execute_command_cond("dry",      dry);
  execute_command_cond("mode",     mode);
  execute_command_cond("swing",    swing);
  execute_command_cond("rotate",   rotate);
  execute_command_cond("full",     full_effect);
  execute_command_cond("strength", strength);
  execute_command_cond("dump",     dump);
  execute_command_cond("help",     help);
  execute_command_cond("multi",    multi);

  SerialUI.write("No such command: '");
  SerialUI.write((uint8_t*)buffer, length);
  SerialUI.write("'\r\n");

  return false;
}
