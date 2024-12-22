#pragma once

#include <avr/pgmspace.h>

class __FlashStringHelper;
namespace psg { class AdvancedAccess; }

class uart_stream
{
public:
    using Handler = void (*)(const char* str);

    void Start(psg::AdvancedAccess& psg);
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
