// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <avr/io.h>

// data bus bits D0-D3 (Arduino pins A0-A3)
#define LSB_DDR  DDRC
#define LSB_PORT PORTC
#define LSB_PIN  PINC
#define LSB_MASK 0x0F

// data bus bits D4-D7 (Arduino pins D4-D7)
#define MSB_DDR  DDRD
#define MSB_PORT PORTD
#define MSB_PIN  PIND
#define MSB_MASK 0xF0

// control bus BC1 and BDIR signals
#define BUS_PORT PORTB
#define BUS_DDR  DDRB
#define BC1_PIN  PB0    // Arduino pin D8
#define BDIR_PIN PB1    // Arduino pin D9

// RESET signal
#define RES_PORT PORTD
#define RES_DDR  DDRD
#define RES_PIN  PD2    // Arduino pin D2

// CLOCK signal
#define CLK_DDR  DDRD
#define CLK_PIN  PD3    // Arduino pin D3

namespace PSG
{
// -----------------------------------------------------------------------------
// PSG Definitions
// -----------------------------------------------------------------------------

    enum CLK
    {
        CLK_1_00_MHZ = 0x10,
        CLK_1_77_MHZ = 0x09,
        CLK_2_00_MHZ = 0x08,
    };

    enum
    {
        A_Fine   = 0x00,
        A_Coarse = 0x01,
        B_Fine   = 0x02,
        B_Coarse = 0x03,
        C_Fine   = 0x04,
        C_Coarse = 0x05,
        N_Period = 0x06,
        Mixer    = 0x07,
        A_Volume = 0x08,
        B_Volume = 0x09,
        C_Volume = 0x0A,
        E_Fine   = 0x0B,
        E_Coarse = 0x0C,
        E_Shape  = 0x0D
    };

// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------

    void Address(uint8_t reg);
    void Write(uint8_t data);
    void Read(uint8_t& data);

// -----------------------------------------------------------------------------
// High Level Access
// -----------------------------------------------------------------------------

    void Init();
    void Reset();
    void Clock(CLK clock);
    void Send(uint8_t reg, uint8_t data);
    uint8_t Receive(uint8_t reg);
}