#include "globals.hpp"

#include "commands.hpp"
#include "serial.hpp"
#include "utils.hpp"

// Main loop - Reads and acts on commands sent to the SerialUI port.
//
// @param execute_command_function Implements the command handling logic.
void commands_loop(bool (*execute_command_function)(char*, int, bool)) {
  const int BUFFER_SIZE = 64;
  // +1 to always be able to end the command with '\0'
  static char buffer[BUFFER_SIZE + 1];
  static int length = 0;
  static int saved_length = 0;

  if (SerialUI.available() > 0) {
    uint8_t data = SerialUI.read();

    if (length >= 0) {
      if (is_eol(data)) {
        // Found end-of-line. Execute the command.
        if (length > 0) {
          buffer[length] = '\0';

          execute_command_function(buffer, length, true);

          saved_length = length;
          length = 0;
        } else {
          execute_command_function(buffer, saved_length, true);
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
      SerialUI.println("Overflow handling");
      // Overflow handling. Drain incomming data until end-of-line is found.
      if (is_eol(data)) {
        SerialUI.println("Overflow done");
        length = 0;
      }
    }
  }
}