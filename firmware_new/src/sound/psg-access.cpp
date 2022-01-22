// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#include "../firmware.h"
#include "psg-access.h"

// timing delays (nano seconds)
#define tAS 800 // min 400 ns by datasheet
#define tAH 100 // min 100 ns by datasheet
#define tDW 500 // min 500 ns by datasheet
#define tDH 100 // min 100 ns by datasheet
#define tDA 500 // max 500 ns by datasheet
#define tTS 100 // max 200 ns by datasheet

// Helpers
#define control_bus_delay(ns) _delay_us(0.001f * (ns))
#define INLINE __attribute__ ((inline))

// -----------------------------------------------------------------------------
// Control Bus and Data Bus handling
// -----------------------------------------------------------------------------

#if HARDWARE_VERSION > 211206
    #define fix_lsb
#else
// workaround for D0-D3 on pcb prototype
const uint8_t fix_lsb_array[] PROGMEM = 
{
    0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
    0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F 
};
uint8_t fix_lsb(uint8_t lsb) { return pgm_read_byte(fix_lsb_array + lsb); }
#endif

INLINE void get_data_bus(uint8_t& data)
{
    // get bata bits from input ports
    data = fix_lsb(LSB_PIN & LSB_MASK) | (MSB_PIN & MSB_MASK);
}

INLINE void set_data_bus(uint8_t data)
{
    // set ports to output
    LSB_DDR |= LSB_MASK;
    MSB_DDR |= MSB_MASK;

    // set data bits to output ports
    LSB_PORT = (LSB_PORT & ~LSB_MASK) | fix_lsb(data & LSB_MASK);
    MSB_PORT = (MSB_PORT & ~MSB_MASK) | (data & MSB_MASK);
}

INLINE void release_data_bus()
{
    // setup ports to input
    LSB_DDR &= ~LSB_MASK;
    MSB_DDR &= ~MSB_MASK;

    // enable pull-up resistors
    LSB_PORT |= LSB_MASK;
    MSB_PORT |= MSB_MASK;
}

INLINE void set_control_bus_addr()  { BUS_PORT |=  (1 << PIN_BDIR | 1 << PIN_BC1);  }
INLINE void set_control_bus_write() { BUS_PORT |=  (1 << PIN_BDIR);                 }
INLINE void set_control_bus_read()  { BUS_PORT |=  (1 << PIN_BC1);                  }
INLINE void set_control_bus_inact() { BUS_PORT &= ~(1 << PIN_BDIR | 1 << PIN_BC1);  }

// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------

void PSG_Address(psg_reg reg)
{
    set_data_bus(reg);
    set_control_bus_addr();
    control_bus_delay(tAS);
    set_control_bus_inact();
    control_bus_delay(tAH);
    release_data_bus();
}

void PSG_Write(psg_data data)
{
    set_data_bus(data);
    set_control_bus_write();
    control_bus_delay(tDW);
    set_control_bus_inact();
    control_bus_delay(tDH);
    release_data_bus();
}

void PSG_Read(psg_data& data)
{
    set_control_bus_read();
    control_bus_delay(tDA);
    get_data_bus(data);
    set_control_bus_inact();
    control_bus_delay(tTS);
}

// -----------------------------------------------------------------------------
// High Level Access
// -----------------------------------------------------------------------------

void PSG_Init()
{
    // setup control and data bus
    BUS_DDR |= (1 << PIN_BDIR);
    BUS_DDR |= (1 << PIN_BC1);
    set_control_bus_inact();
    release_data_bus();

    // setup default clock and reset
    PSG_Clock(PSG_CLK_1_77_MHZ);
    PSG_Reset();
}

void PSG_Clock(psg_clk clk)
{
    // clock on pin PD3 (Arduino D3)
    DDRD  |= (1 << PD3);
    TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << WGM22) | (1 << CS20);
    OCR2A  = (clk - 1);
    OCR2B  = (clk / 2); 
}

void PSG_Reset()
{
#if HARDWARE_VERSION > 211206
    // TODO: reset on dedicated MCU pin
#else
    for (psg_reg r = 0; r < 14; ++r) PSG_Send(r, 0x00);
#endif
}

void PSG_Send(psg_reg reg, psg_data data)
{
    PSG_Address(reg);
    PSG_Write(data);
}

psg_data PSG_Receive(psg_reg reg)
{
    psg_data data;
    PSG_Address(reg);
    PSG_Read(data);
    return data;
}
