#ifndef IRREMOTEIVT_UTILS_HPP
#define IRREMOTEIVT_UTILS_HPP

#include "globals.h"

inline uint8_t hex_value(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }

  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }

  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }

  return -1;
}

inline int a_to_positive_number(char* buffer, int length) {
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

// String helpers
#define str_equals(str, str_literal, len) (len == (sizeof(str_literal) - 1 /* '\0' */) && strncmp(str, str_literal, len) == 0)
#define str_begins(str, str_literal, len) (strncmp(str, str_literal, len) == 0)

// FIXME: Need to send both '\r' and '\n' for the current code to work.
// Setup to expect '\r' as the end-of-line marker.
#define is_eol(data) ((char)data == '\r')

#define TRIM(buffer, length)                 \
  do {                                        \
    while (length > 0 && buffer[0] == ' ') {  \
      buffer++;                               \
      length--;                               \
    }                                         \
  } while (0)

#endif // IRREMOTEIVT_UTILS_HPP
