// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#pragma once

#include "avr-support.h"

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
#define PIN_BC1  PB0   // Arduino pin D8
#define PIN_BDIR PB1   // Arduino pin D9

// -----------------------------------------------------------------------------
// PSG Definitions
// -----------------------------------------------------------------------------

typedef uint8_t psg_reg;
typedef uint8_t psg_data;

enum psg_clk
{
    PSG_CLK_1_00_MHZ = 0x10,
    PSG_CLK_1_77_MHZ = 0x09,
    PSG_CLK_2_00_MHZ = 0x08,
};

// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------

void PSG_Address(psg_reg reg);
void PSG_Write(psg_data data);
void PSG_Read(psg_data& data);

// -----------------------------------------------------------------------------
// High Level Access
// -----------------------------------------------------------------------------

void PSG_Init();
void PSG_Reset();
void PSG_Clock(psg_clk clk);
void PSG_Send(psg_reg reg, psg_data data);
psg_data PSG_Receive(psg_reg reg);
