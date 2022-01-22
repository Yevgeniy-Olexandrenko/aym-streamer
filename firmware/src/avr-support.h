#pragma once

// libc includes
#include <stdint.h>

// avr includes
#include <avr/pgmspace.h>  // reading data from PROGMEM
#include <avr/eeprom.h>    // reading/writing data to EEPROM
#include <avr/interrupt.h> // mcu interrupts
#include <avr/power.h>     // mcu power management
#include <avr/sleep.h>     // mcu sleeping
#include <util/delay.h>

// support for strings in PROGMEM
class __FlashStringHelper;
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(string_literal) (FPSTR(PSTR(string_literal)))
