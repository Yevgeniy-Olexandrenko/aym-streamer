#pragma once

#include <avr/pgmspace.h>

// support for strings in PROGMEM
class __FlashStringHelper;
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(string_literal) (FPSTR(PSTR(string_literal)))

class SinglePSG;

class uart_stream
{
public:
    using Handler = void (*)(const char* str);

    void Start(SinglePSG& psg);
    void SetInputHandler(Handler handler);
    void Stop();

    void Print(char chr, int num = 1);
    void Print(const char* str);
    void Print(const __FlashStringHelper* str);
    void Print(uint8_t  val, bool hex = false);
    void Print(uint16_t val, bool hex = false);
    void Print(uint32_t val, bool hex = false);

    void Println(char chr, int num = 1);
    void Println(const char* str);
    void Println(const __FlashStringHelper* str);
    void Println(uint8_t  val, bool hex = false);
    void Println(uint16_t val, bool hex = false);
    void Println(uint32_t val, bool hex = false);
    void Println();

private:
    void PrintNibble(uint8_t nibble);
    void PrintNumber(int32_t number);
};

extern uart_stream UARTStream;
