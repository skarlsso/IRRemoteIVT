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

// Pin mapping
#define TX_PIN     0
#define RX_PIN     1
#define DEBUG_PIN  7
#define DEBUG2_PIN 8

// The IR pin must match the Output Compare pin for OC1A.
#define IR_PIN     9

// Setup what serial ports to use.
//
// The Pro Micro uses:
//  Serial1 to communicate over the TX_PIN and RX_PIN pins.
//  Serial communicate to the Serial Monitor, through the USB connection.

// #define USE_ONLY_HW_SERIAL
#define USE_ONLY_SW_SERIAL

#ifdef USE_ONLY_SW_SERIAL
# define SerialUI Serial
#else
# define SerialUI Serial1
#endif

#ifdef USE_ONLY_HW_SERIAL
# define SerialDebug Serial1
#else
# define SerialDebug Serial
#endif

// FIXME: Need to send both '\r' and '\n' for the current code to work.
// Setup to expect '\r' as the end-of-line marker.
#define is_eol(data) ((char)data == '\r')


// Bytes per IR package
const int NUM_IR_BYTES = 13;

// The BitSegment describe segments of the IR package.

struct BitSegment {
  byte index;     // byte position in the IR package
  byte offset;    // bit offset in the byte
  byte bits;      // bits in field
  byte inverted;  // if the bits should reversed before used as a number
};

const BitSegment bs_abs_temp          = { 4, 4, 4, 1};
const BitSegment bs_rel_temp          = { 4, 2, 2, 1};
const BitSegment bs_unknown0_0        = { 4, 1, 1, 0};
const BitSegment bs_rel_temp_sign     = { 4, 0, 1, 0};

const BitSegment bs_unknown1_1000     = { 5, 4, 4, 0};
const BitSegment bs_state             = { 5, 1, 3, 0};
const BitSegment bs_unknown2_0        = { 5, 0, 1, 0};

const BitSegment bs_mode              = { 6, 6, 2, 0};
const BitSegment bs_clean             = { 6, 4, 2, 0};
const BitSegment bs_fan_strength      = { 6, 1, 3, 0};
const BitSegment bs_unknown3_0        = { 6, 0, 1, 0};

const BitSegment bs_time_hours        = { 7, 3, 5, 1};
const BitSegment bs_set_clear_state   = { 7, 0, 3, 0};

const BitSegment bs_rotate            = { 8, 5, 3, 0};
const BitSegment bs_unknown4_10000    = { 8, 0, 5, 0};

const BitSegment bs_unknown5_00000001 = { 9, 0, 8, 0};

const BitSegment bs_time_minutes      = {10, 2, 6, 1};
const BitSegment bs_set_clear         = {10, 1, 1, 0};
const BitSegment bs_unknown6_0        = {10, 0, 1, 0};

const BitSegment bs_unknown7_00       = {11, 6, 2, 0};
const BitSegment bs_ion               = {11, 5, 1, 0};
const BitSegment bs_unknown8_01111    = {11, 0, 5, 0};

const BitSegment bs_unknown9_1000     = {12, 4, 4, 0};
const BitSegment bs_parity            = {12, 0, 4, 0};

// Values for bs_mode
#define MODE_FAN  0b00
#define MODE_HEAT 0b10
#define MODE_COOL 0b01
#define MODE_DRY  0b11

// Values for bs_state
#define STATE_ON              0b100
#define STATE_OFF             0b010
#define STATE_CMD             0b110
#define STATE_FULL_EFFECT_ON  0b011
#define STATE_FULL_EFFECT_OFF 0b111

// Values for bs_rotate
#define ROTATE_ON    0b011
#define ROTATE_OFF   0b101
#define ROTATE_SWING 0b111

// Values for bs_clean
#define CLEAN_ON   0b01
#define CLEAN_OFF  0b10

// Values for bs_fan_strength
#define STRENGTH_SLOW   0b110
#define STRENGTH_MEDIUM 0b101
#define STRENGTH_FAST   0b111
#define STRENGTH_AUTO   0b010

// Values for bs_ion
#define ION_ON  0b1
#define ION_OFF 0b0

// String helpers
#define str_equals(str, str_literal, len) (len == (sizeof(str_literal) - 1 /* '\0' */) && strncmp(str, str_literal, len) == 0)
#define str_begins(str, str_literal, len) (strncmp(str, str_literal, len) == 0)

// The IR data package.
//
// Bytes containing the state which will be send upon request.
// This following values are a reasonable default.
volatile byte ir_data[NUM_IR_BYTES] =
   { 0x55,    0x5A,    0xF3,    0x08,    0x00,    0x88,    0x04,    0x00,    0x10,    0x01,    0x00,    0x0F,    0x88 };
 //  Always the same____________________ TEMP  0  1000XXX  MO  FAN  x60m_    RRX10000 00000001 MINUTS 0 00N01111 1000CRC_
 //  01010101 01011010 11110011 00001000 00000000 10001000 00000100 00000000 00010000 00000001 00000000 00001111 10001000

// Debugging aid to dump all IR data bits.
const byte dump_all_ir_data = 0;

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

// Invert the 'bits' number of bits in the 'value' byte.
inline byte invert(byte value, byte bits) {
  byte inverted = 0;
  for (int i = 0; i < bits; i++) {
    inverted <<= 1;
    inverted |= value & 1;
    value >>= 1;
  }

  return inverted;
}

// Helper macros to extract and set values in the BitSegments.

#define MAYBE_INVERT(bs, value) ((bs.inverted == 1) ? invert(value, bs.bits) : value)

#define BS_MASK(bs) (((1 << bs.bits) - 1) << bs.offset)
#define IR_DATA_CLEAR_BITS(bs)      ir_data[bs.index] &= ~BS_MASK(bs)
#define IR_DATA_SET_BITS(bs, value) ir_data[bs.index] |= (((value) << bs.offset) & BS_MASK(bs))

#define set_ir_data(bf, value) IR_DATA_CLEAR_BITS(bf); IR_DATA_SET_BITS(bf, MAYBE_INVERT(bf, value));
#define get_ir_data(bf) MAYBE_INVERT(bf, ((ir_data[bf.index] & BS_MASK(bf)) >> bf.offset))


// Update the IR data package with the correct 4 bit parity.
void ir_data_update_parity() {
  byte parity = 0;

  // Calculate 8 bytes parity, including the previous parity.
  for (int i = 0; i < NUM_IR_BYTES; i++) {
    parity ^= ir_data[i];
  }

  // 8 bytes parity => 4 bytes parity
  parity ^= (parity >> 4);
  parity &= 0x0F;

  // xor:ing will get rid of previous parity.
  ir_data[NUM_IR_BYTES - 1] ^= parity;
}

#if defined(__AVR_ATmega32U4__)
# define TIME_MULTIPLIER  2
#elif defined(__AVR_ATmega328P__)
# define TIME_MULTIPLIER  1
#endif

// Timer functions
// ===============

void setup_modulation_timer() {
 // Timer 1 is setup as a 38 kHz modulation timer with toggling Output Compare.
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1A = 0; /* _BV(COM1A0) */ // Don't enable the OC yet.

  TCCR1B = _BV(WGM12)  // Clear Timer On Compare Match (CTC) mode.
         | _BV(CS10);  // Pre-scaler

  // 38kHz with 8MHz or 16Mhz /wo prescaler
  OCR1A = 105 * TIME_MULTIPLIER;
}

void setup_IR_tick_timer() {
  // Timer 0 is time the ticks in the IR signal train of one IR data package.
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0  = 0;

  // Clear Timer on Compare Match (CTC) Mode.
  TCCR0A = _BV(WGM01);

 // Don't enable the clock source yet.
# define CLOCK_SELECT (_BV(CS01) | _BV(CS00))  // clk1/8

  // 0.45 ms - 8MHz or 16Mhz /w 8 prescaler
  OCR0A = 58 * TIME_MULTIPLIER;

  TIMSK0 = _BV(OCIE0A); // Enable interrupt.
}


// Turn off the 38 kHz modulation.
void turn_off_modulation() {
  // Enable Timer1 interrupt and let the ISR turn
  // off the modulation at an appropriate point.
  TIMSK1 |= _BV(OCIE1A); // Enable interrupt.
}

void turn_off_modulation_from_isr() {
  TCCR1A &= ~(_BV(COM1A0));
  TIMSK1 &= ~(_BV(OCIE1A)); // Disable interrupt
}

// Turn on the 38 kHz modulation.
void turn_on_modulation() {
  TCNT1 = 0;
  TCCR1A |= _BV(COM1A0);
  TCCR1C |= _BV(FOC1A); // Force output compare
}

void turn_on_IR_ticks_timer() {
  //digitalWrite(IR_PIN, HIGH);
  // Start Timer 3 to clock out all IR data.
  // Set the time to one before OC to immediately trigger a Output Compare event.
  TCNT0   = OCR0A - 1;
  // Turn on clock
  TCCR0B |= CLOCK_SELECT;
}


void turn_off_IR_ticks_timer() {
  //digitalWrite(IR_PIN, LOW);
  TCCR0B &= ~CLOCK_SELECT  ; // Turn off clock
}

// The Timer 1 Output Capture is turned on at the end of a 'HIGH' tick in the IR pulse train,
// and only used to safely shut down the Timer 1 generated 38 kHz modulation.
ISR(TIMER1_COMPA_vect) {
  if (digitalRead(IR_PIN) == LOW) {
    // Can only turn off while the IR modulation is off;
    turn_off_modulation_from_isr(); 
  }
}

// Interrupt routine for the Timer 3 Output Compare.
// Handles the timing needed to send the IR data package.
ISR(TIMER0_COMPA_vect) {
  // ISR local variables
  static byte ir_send_state    = 0;
  static byte ir_send_byte     = 0;
  static byte ir_send_bit      = 0;
  static byte ir_send_bit_tick = 0;
  static byte previous_output  = false;

  // IR data package state machine.
  //
  // The timing is divided into 'ticks' of a fixed duration.
  //  H: A tick with the output enabled (held high)
  //  L: A tick with the output disabled (held low)
  //
  // The ticks for the IR data package are:
  //  HHHHHHHHLLLLH<data specific ticks>
  //
  // The 'data specific ticks' encode the 0's and 1's.
  //  0: LH (one L tick followed by a H tick)
  //  1: LLLH (three L ticks folled by a H tick)
  //
  // Example:
  //  IR data package: 01010110
  //                   Preamble     0 1   0 1   0 1   1
  //  Ticks & output:  HHHHHHHHLLLLHLHLLLHLHLLLHLHLLLHLLLH

  byte enable_output;

  if (ir_send_state < 8) {
    // The start of the send sequence is 8 ticks with the output held high ...
    ir_send_state++;
    enable_output = true;

  } else if (ir_send_state < 12) {
    // ... followed by 4 ticks with the output held low ...
    ir_send_state++;
    enable_output = false;

  } else if (ir_send_state == 12) {
    // ... follwed by 1 tick with the output held high.
    ir_send_state++;
    enable_output = true;

    // Prepare first bit.
    ir_send_byte     = 0;
    ir_send_bit      = 8;
    ir_send_bit_tick = 1; // Set to 1, since pre-decremented below.

  } else if (ir_send_state == 13) {
    // After the start sequence the bits are sent.

    // 1 is sent as 3 ticks with the output held low, followed by 1 tick with the output held high.
    // 0 is sent as 1 tick  with the output held low, followed by 1 tick with the output held high.
    ir_send_bit_tick--;
    if (ir_send_bit_tick == 0) {
      // All ticks done for this bit; fetch new bit.

      if (ir_send_bit > 0) {
        // More bits to send in the current byte.
        ir_send_bit--;
      } else {
        // All bits sent. Fetch a new byte.
        ir_send_bit = 7;
        ir_send_byte++;
      }

      if (ir_send_byte < NUM_IR_BYTES) {
        byte ir_byte = ir_data[ir_send_byte];
        byte ir_bit  = ir_byte & (1 << ir_send_bit);
        // 0 bit => 1 tick  low + 1 tick high
        // 1 bit => 3 ticks low + 1 tick high
        ir_send_bit_tick = ((ir_bit == 0) ? 1 : 3) + 1;

        // First tick: low
        enable_output = false;

      } else {
        // This was the last byte. Got to end state.
        ir_send_state++;

        // Last bit. Pull the line low.
        enable_output = false;
      }

    } else if (ir_send_bit_tick == 1) {
      // Last tick: high
      enable_output = true;

    } else { // ir_send_bit_tick == 2 or 3
      // Ticks 3 and 2 for 1 bit case: low
      enable_output = false;
    }

  } else if (ir_send_state < 250) {
    // Wait aribitrarily long before restarting;
    ir_send_state++;
    enable_output = false;

  } else {
    // All bits have been sent. Time to turn off the ticks timer.
    turn_off_IR_ticks_timer();
    ir_send_state = 0;
    enable_output = false;
  }

  // Output the state.
  if (previous_output != enable_output) {
    digitalWrite(DEBUG_PIN, enable_output);
    if (enable_output) {
      turn_on_modulation();
    } else {
      turn_off_modulation();
    }

    previous_output = enable_output;
  }
}

// Send the IR data package.
void ir_data_send() {
  turn_on_IR_ticks_timer();
  // The timer will be turned off when the entire IR data package has been sent out.
}

// Make the IR data package complete and send it out to the IR port.
void ir_data_finalize_and_send(byte state, byte debug = 0) {
  // Must always set state.
  set_ir_data(bs_state, state);
  ir_data_update_parity();

  if (debug || dump_all_ir_data) {
    dump_ir_data();
  }

  ir_data_send();
}


// Commands
// ========

void execute_on(char *buffer, int length) {
  // Ignore parameters.
  ir_data_finalize_and_send(STATE_ON);
}

void execute_off(char *buffer, int length) {
  // Ignore parameters.
  ir_data_finalize_and_send(STATE_OFF);
}

#define TRIM(buffer, length)                 \
  do {                                        \
    while (length > 0 && buffer[0] == ' ') {  \
      buffer++;                               \
      length--;                               \
    }                                         \
  } while (0)


// Turn on/off the "Plasma Cluster"/ion mode.
void execute_ion(char *buffer, int length) {
  TRIM(buffer, length);

  byte old_value = get_ir_data(bs_ion);
  byte new_value;

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

  SerialUI.print("Old ion value: "); SerialUI.println(old_value);
  SerialUI.print("New ion value: "); SerialUI.println(new_value);

  set_ir_data(bs_ion, new_value);
  ir_data_finalize_and_send(STATE_CMD);
}


// Helper functions to get/set the temperature.

byte mode_uses_absolute_time() {
   byte mode = get_ir_data(bs_mode);
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


int a_to_positive_number(char* buffer, int length) {
  int value = 0;
  for (int i = 0; i < length; i++) {
     if (buffer[i] < '0' || buffer[i] > '9') {
       return -1; // Failure
     }
     int digit = buffer[i] - '0';
     value *= 10;
     value += digit;
  }
  return value;
}

// Request a given temperature.
void execute_temp(char *buffer, int length) {
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
    byte negative = buffer[0] == '-';

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

  SerialUI.print("Setting temp to: "); SerialUI.println(new_temp);

  set_temp_for_mode(new_temp);
  ir_data_finalize_and_send(STATE_CMD);
}

// Turn on/off the Cleaning program.
void execute_clean(char *buffer, int length) {
  TRIM(buffer, length);

  byte state     = get_ir_data(bs_state);
  byte old_value = get_ir_data(bs_clean);
  byte new_value = old_value;

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
  ir_data_finalize_and_send(STATE_CMD);
}

// FIXME: Implement the timer features.
void execute_timer(char *buffer, int length) {
  SerialUI.println("Command not implemented: timer");
}

// Turn on the selected mode.
void execute_mode_selected(byte mode) {
  byte old_value = get_ir_data(bs_mode);

  SerialUI.print("Old mode: "); SerialUI.println(old_value);
  SerialUI.print("New mode: "); SerialUI.println(mode);

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

  ir_data_finalize_and_send(STATE_CMD);
}

// Turn on Heat mode.
void execute_heat(char *buffer, int length) {
  execute_mode_selected(MODE_HEAT);
}

// Turn on Cool mode.
void execute_cool(char *buffer, int length) {
  execute_mode_selected(MODE_COOL);
}

// Turn on Fan mode.
void execute_fan(char *buffer, int length) {
  execute_mode_selected(MODE_FAN);
}

// Turn on Dry mode.
void execute_dry(char *buffer, int length) {
  execute_mode_selected(MODE_DRY);
}

// Cycle through the modes.
void execute_mode(char* buffer, int length) {
  TRIM(buffer, length);

  byte old_value = get_ir_data(bs_mode);
  byte new_value;

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

  execute_mode_selected(new_value);
}

// Start/stop swing the air outlet.
void execute_swing(char *buffer, int length) {
  TRIM(buffer, length);

  byte old_value = get_ir_data(bs_rotate);
  byte new_value = ROTATE_SWING;

  SerialUI.print("Old rotate(swing) value: "); SerialUI.println(old_value);
  SerialUI.print("New rotate(swing) value: "); SerialUI.println(new_value);

  // Ignore arguments.
  set_ir_data(bs_rotate, new_value);
  ir_data_finalize_and_send(STATE_CMD);
}

// Turn on/off the Rotate feature.
// FIXME: Figure out what this is.
void execute_rotate(char *buffer, int length) {
  TRIM(buffer, length);

  byte old_value = get_ir_data(bs_rotate);
  byte new_value = old_value;

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

  SerialUI.print("Old rotate value: "); SerialUI.println(old_value);
  SerialUI.print("New rotate value: "); SerialUI.println(new_value);

  set_ir_data(bs_rotate, new_value);
  ir_data_finalize_and_send(STATE_CMD);
}

// Turn on/off the Full Effect feature.
void execute_full_effect(char *buffer, int length) {
  TRIM(buffer, length);

  byte old_value = get_ir_data(bs_state);
  byte new_value = old_value;

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

  SerialUI.print("Old full mode: "); SerialUI.println(old_value);
  SerialUI.print("New full mode: "); SerialUI.println(new_value);

  if (new_value == STATE_FULL_EFFECT_ON || new_value == STATE_FULL_EFFECT_OFF) {
    // The remote sets this time.
    // FIXME: Need to save the old timer value, and restore it later when STATE_CMD events are sent.
    set_ir_data(bs_time_hours, 0);
    set_ir_data(bs_time_minutes, 1);
  }

  ir_data_finalize_and_send(new_value);
}

// Set fan strength.
void execute_strength(char *buffer, int length) {
  TRIM(buffer, length);

  byte old_value = get_ir_data(bs_fan_strength);
  byte new_value;

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

  SerialUI.print("Old strength: "); SerialUI.println(old_value);
  SerialUI.print("New strength: "); SerialUI.println(new_value);

  set_ir_data(bs_fan_strength, new_value);
  ir_data_finalize_and_send(STATE_CMD);
}

void execute_help(char *buffer, int length) {
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

boolean execute_command(char *buffer, int length) {
  #define execute_command_cond(command_str, command)                            \
    do {                                                                        \
      int command_str_len = strlen(command_str);                                \
      if (str_begins(buffer, command_str, command_str_len)) {                   \
        SerialUI.print("Executing command: "); SerialUI.println(buffer);        \
        execute_##command(buffer + command_str_len, length - command_str_len);  \
        return true;                                                            \
      }                                                                         \
    } while (0)

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
  execute_command_cond("help",     help);

  SerialUI.print("No such command: ");
  SerialUI.println(buffer);

  return false;
}


void setup()  {
  pinMode(TX_PIN,     OUTPUT);
  pinMode(RX_PIN,     INPUT);
  pinMode(IR_PIN,     OUTPUT);
  pinMode(DEBUG_PIN,  OUTPUT);
  pinMode(DEBUG2_PIN, OUTPUT);

  digitalWrite(DEBUG_PIN,  LOW);
  digitalWrite(DEBUG2_PIN, LOW);
  digitalWrite(IR_PIN,     LOW);

  setup_modulation_timer();
  setup_IR_tick_timer();
 
  // Arduino Serial Monitor - extra debugging.
  SerialDebug.begin(9600);

  // HW Serial Port - all commands are recieved from this port.
  SerialUI.begin(9600);

  sei(); // Enable global interrupts
}


// Main loop.
// Reads and acts on commands sent to the SerialUI port.
// The commands manipulate the current IR remote state and is then sent
// out as IR pulses that are sent to the IVT Nordic Inverter heat pump.
void loop()  {
  const int BUFFER_SIZE = 64;
  // +1 to always be able to end the command with '\0'
  static char buffer[BUFFER_SIZE + 1];
  static int length = 0;
  static int saved_length = 0;

  if (SerialUI.available() > 0) {
    byte data = SerialUI.read();

    if (length >= 0) {
      if (is_eol(data)) {
        // Found end-of-line. Execute the command.
        if (length > 0) {
          buffer[length] = '\0';

          execute_command(buffer, length);

          saved_length = length;
          length = 0;
        } else {
          execute_command(buffer, saved_length);
        }
      } else {
        if (length < BUFFER_SIZE) {
          buffer[length] = (char)data;
          length++;
        } else {
          // Overflow.
          buffer[BUFFER_SIZE] = '\0';

          SerialUI.print("Too long command: ");
          SerialUI.println(buffer);

          length = -1;
        }
      }
    } else {
      SerialDebug.println("Overflow handling");
      // Overflow handling. Drain incomming data until end-of-line is found.
      if (is_eol(data)) {
        SerialDebug.println("Overflow done");
        length = 0;
      }
    }
  }
}

