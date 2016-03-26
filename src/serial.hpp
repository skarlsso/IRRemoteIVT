#ifndef IRREMOTEIVT_SERIAL_HPP
#define IRREMOTEIVT_SERIAL_HPP

#include "globals.hpp"

// Setup what serial ports to use.
//
// The Pro Micro uses:U
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

inline void setup_serial() {
  // Arduino Serial Monitor - extra debugging.
  SerialDebug.begin(9600);

  // HW Serial Port - all commands are recieved from this port.
  SerialUI.begin(9600);
}

#endif // IRREMOTEIVT_SERIAL_HPP
