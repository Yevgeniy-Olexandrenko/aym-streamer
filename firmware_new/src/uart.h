#pragma once

#include <stdint.h>
#include <avr/pgmspace.h>

// support for strings in PROGMEM
class __FlashStringHelper;
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(string_literal) (FPSTR(PSTR(string_literal)))

// port control
void UART_Open(uint32_t baud);
void UART_EnableRX(bool yes);
void UART_EnableRXInt(bool yes);
void UART_EnableTX(bool yes);
void UART_EnableTXInt(bool yes);
void UART_Close();

// byte write/read operations
void UART_PutByte(uint8_t data);
uint8_t UART_GetByte();

// write hex presentation of integers
void UART_PutNibbleHex(uint8_t nib);
void UART_PutByteHex(uint8_t data);
void UART_PutWordHex(uint16_t data);
void UART_PutDWordHex(uint32_t data);

// write chars and strings
void UART_PutSP();
void UART_PutLN();
void UART_PutString(const char* str);
void UART_PutString(const __FlashStringHelper* str);
void UART_GetString(char* str, uint8_t len);
