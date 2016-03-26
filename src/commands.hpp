#ifndef IRREMOTEIVT_COMMANDS_HPP
#define IRREMOTEIVT_COMMANDS_HPP

#include "globals.hpp"

void commands_loop(boolean (*execute_command_function)(char*, int, boolean));

#define execute_command_cond(command_str, command)                                      \
  do {                                                                                  \
    TRIM(buffer, length);                                                               \
    int command_str_len = strlen(command_str);                                          \
    if (str_begins(buffer, command_str, command_str_len)) {                             \
      SerialUI.write("Executing command: ");                                            \
      SerialUI.write((byte*)buffer, length);                                            \
      SerialUI.write("\r\n");                                                           \
      execute_##command(buffer + command_str_len, length - command_str_len, send_ir);   \
      return true;                                                                      \
    }                                                                                   \
  } while (0)

#endif
