const int NUM_IR_BYTES = 13;
// One example
//                                      01010101 01011010 11110011 00001000 00000000 10001000 00000100 01100000 00010000 00000001 00000000 00101111 10001100
volatile byte ir_data[NUM_IR_BYTES] = { 0x55,    0x5A,    0xF3,    0x08,    0x00,    0x88,    0x04,    0x60,    0x10,    0x01,    0x00,    0x2F,    0x8C };
//volatile byte ir_data[NUM_IR_BYTES] = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55 };
//volatile byte ir_data[NUM_IR_BYTES] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

#define IR_ON   LOW
#define IR_OFF  HIGH

volatile byte ir_send_state = 0;
volatile byte enable_output = false;
volatile byte previous_output = false;
volatile byte ir_send_byte  = 0;
volatile byte ir_send_bit   = 0;
volatile byte ir_send_bit_tick = 0;
#define DEBUG_PIN 7
#define DEBUG2_PIN 8


volatile byte timer0_enabled = true;

void turn_off_modulation() {
  // Enable Timer1 interrupt and let the ISR turn
  // off the modulation at an appropriate point.
  TIMSK1 |= _BV(OCIE1A); // Enable interrupt.
  //digitalWrite(DEBUG2_PIN, HIGH);
}

void turn_on_modulation() {
  TCNT1 = 0;
  TCCR1A |= _BV(COM1A0);
  TCCR1C |= _BV(FOC1A); // Force output compare
}

ISR(TIMER1_COMPA_vect) {
  if (digitalRead(9) == IR_OFF) {
    // Can only turn off while the IR modulation is off;
    TCCR1A &= ~(_BV(COM1A0));
    TIMSK1 &= ~(_BV(OCIE1A)); // Disable interrupt
    //digitalWrite(DEBUG2_PIN, LOW);
  }
}

volatile uint16_t wait_count = 0;

ISR(TIMER3_COMPA_vect) {
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
    ir_send_bit_tick = 1; // pre-decremented

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
    wait_count++;
    if (wait_count < 10) {
      ir_send_state = 14;
    } else {
      wait_count = 0;
      // Restart.
      ir_send_state = 0;
    }
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

void setup()  { 
  // nothing happens in setup 
  pinMode(9, OUTPUT);
  pinMode(DEBUG_PIN, OUTPUT);
  pinMode(DEBUG2_PIN, OUTPUT);
  
  digitalWrite(DEBUG_PIN, LOW);
  digitalWrite(DEBUG2_PIN, LOW);
  
  digitalWrite(9, HIGH);
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  //  pinMode(10, OUTPUT);
  TCCR1A 
    //= _BV(COM1A0) // Toggle OC1A on Compare Match
    //| 
    = _BV(COM1B1) // Clear OC0B on Compare Match, set OC0B at TOP
      ;//| _BV(WGM10)  // Fast PWM
  //| _BV(WGM11); //    -"-
  TCCR1B = _BV(WGM12)  //    -"-
    | _BV(CS10) /*| _BV(CS11)*/; // 264 prescaler
  //  OCR1A = 180;
  //  OCR1B = 50;
  OCR1A = 210; // 38kHz with 16MHz & no prescaler
  //  OCR1B = 50;
  
  TCCR1A |= _BV(COM1A0);
  TCCR1C |= _BV(FOC1A);
  TCCR1A &= ~(_BV(COM1A0));
    
  digitalWrite(DEBUG2_PIN, HIGH);
  
  
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;            // Initial Count value
  TCCR3B = 0
    | _BV(WGM32)        // Count mode: CTC - Counter To Clear?
    | _BV(CS30);        // Clock Select: clk1/0 no prescaler
  OCR3A = 3750 * 2;          // Set timer duration
  //OCF3A = 0;            // Clear interrupt flag.
  TIMSK3 = _BV(OCIE3A); // Enable interrupt.

  sei(); // Enable global interrupts
} 

void loop()  { 
}

