#!/bin/bash

# Uses PlatformIO to build

# Build the minimal version, that only supports sendin raw bytes.
# The correct byte sequence is built by some external program.
pio run -e minimal
