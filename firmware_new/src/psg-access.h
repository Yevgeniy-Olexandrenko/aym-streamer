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

// -----------------------------------------------------------------------------
// PSG Definitions
// -----------------------------------------------------------------------------

enum psg_clk
{
    PSG_CLK_1_00_MHZ = 0x10,
    PSG_CLK_1_77_MHZ = 0x09,
    PSG_CLK_2_00_MHZ = 0x08,
};

// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------

void PSG_Address(uint8_t reg);
void PSG_Write(uint8_t data);
void PSG_Read(uint8_t& data);

// -----------------------------------------------------------------------------
// High Level Access
// -----------------------------------------------------------------------------

void PSG_Init();
void PSG_Reset();
void PSG_Clock(psg_clk clk);
void PSG_Send(uint8_t reg, uint8_t data);
uint8_t PSG_Receive(uint8_t reg);
