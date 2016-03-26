// Handles the low-level IR data and pulses to control the IVT Nordic Heat Pump.
//
// @author Stefan Karlsson (skarlsso@github)

#include "globals.hpp"

#include "ir-data.hpp"
#include "low-level.hpp"

// Pin mapping
#define TX_PIN     0
#define RX_PIN     1
#define DEBUG_PIN  7
#define DEBUG2_PIN 8

// The IR pin must match the Output Compare pin for OC1A.
#define IR_PIN     9

#define FORCE_16MHZ 1

#if FORCE_16MHZ
# define TIME_MULTIPLIER 2
#elif defined(__AVR_ATmega32U4__)
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
  static uint8_t ir_send_state    = 0;
  static uint8_t ir_send_uint8_t     = 0;
  static uint8_t ir_send_bit      = 0;
  static uint8_t ir_send_bit_tick = 0;
  static uint8_t previous_output  = false;

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

  uint8_t enable_output;

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
    ir_send_uint8_t     = 0;
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
        // More bits to send in the current uint8_t.
        ir_send_bit--;
      } else {
        // All bits sent. Fetch a new uint8_t.
        ir_send_bit = 7;
        ir_send_uint8_t++;
      }

      if (ir_send_uint8_t < NUM_IR_BYTES) {
        uint8_t ir_uint8_t = ir_data[ir_send_uint8_t];
        uint8_t ir_bit  = ir_uint8_t & (1 << ir_send_bit);
        // 0 bit => 1 tick  low + 1 tick high
        // 1 bit => 3 ticks low + 1 tick high
        ir_send_bit_tick = ((ir_bit == 0) ? 1 : 3) + 1;

        // First tick: low
        enable_output = false;

      } else {
        // This was the last uint8_t. Got to end state.
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

void setup_serial();

void setup_low_level()  {
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

  sei(); // Enable global interrupts
}
