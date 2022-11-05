// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#include "psg-access.h"
#include <util/delay.h>

// timing delays (nano seconds)
#define tAS 800 // min 400 ns by datasheet
#define tAH 100 // min 100 ns by datasheet
#define tDW 500 // min 500 ns by datasheet
#define tDH 100 // min 100 ns by datasheet
#define tDA 500 // max 500 ns by datasheet
#define tTS 100 // max 200 ns by datasheet
#define tRW 500 // min 500 ns by datasheet
#define tRB 100 // min 100 ns bt datasheet

// Helpers
#define set_bit(reg, bit ) reg |=  (1 << (bit))
#define res_bit(reg, bit ) reg &= ~(1 << (bit))
#define set_bits(reg, bits) reg |=  (bits)
#define res_bits(reg, bits) reg &= ~(bits)
#define control_bus_delay(ns) _delay_us(0.001f * (ns))

// -----------------------------------------------------------------------------
// Control Bus and Data Bus handling
// -----------------------------------------------------------------------------

static inline void get_data_bus(uint8_t& data)
{
    // get bata bits from input ports
    data = (LSB_PIN & LSB_MASK) | (MSB_PIN & MSB_MASK);
}

static inline void set_data_bus(uint8_t data)
{
    // set ports to output
    set_bits(LSB_DDR, LSB_MASK);
    set_bits(MSB_DDR, MSB_MASK);

    // set data bits to output ports
    LSB_PORT = (LSB_PORT & ~LSB_MASK) | (data & LSB_MASK);
    MSB_PORT = (MSB_PORT & ~MSB_MASK) | (data & MSB_MASK);
}

static inline void release_data_bus()
{
    // setup ports to input
    res_bits(LSB_DDR, LSB_MASK);
    res_bits(MSB_DDR, MSB_MASK);

    // enable pull-up resistors
    set_bits(LSB_PORT, LSB_MASK);
    set_bits(MSB_PORT, MSB_MASK);
}

static inline void set_control_bus_addr()  { set_bits(BUS_PORT, 1 << BDIR_PIN | 1 << BC1_PIN); }
static inline void set_control_bus_write() { set_bits(BUS_PORT, 1 << BDIR_PIN);                }
static inline void set_control_bus_read()  { set_bits(BUS_PORT, 1 << BC1_PIN );                }
static inline void set_control_bus_inact() { res_bits(BUS_PORT, 1 << BDIR_PIN | 1 << BC1_PIN); }

namespace PSG
{
// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------

    void Address(uint8_t reg)
    {
        set_data_bus(reg);
        set_control_bus_addr();
        control_bus_delay(tAS);
        set_control_bus_inact();
        control_bus_delay(tAH);
        release_data_bus();
    }

    void Write(uint8_t data)
    {
        set_data_bus(data);
        set_control_bus_write();
        control_bus_delay(tDW);
        set_control_bus_inact();
        control_bus_delay(tDH);
        release_data_bus();
    }

    void Read(uint8_t& data)
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

    void Init()
    {
        // setup reset
        set_bit(RES_DDR,  RES_PIN);
        set_bit(RES_PORT, RES_PIN);

        // setup control and data bus
        set_bit(BUS_DDR, BDIR_PIN);
        set_bit(BUS_DDR, BC1_PIN);
        set_control_bus_inact();
        release_data_bus();

        // setup default clock and reset
        Clock(CLK_1_77_MHZ);
        Reset();
    }

    void Clock(CLK clock)
    {
        set_bit(CLK_DDR, CLK_PIN);
        TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
        TCCR2B = (1 << WGM22 ) | (1 << CS20 );
        OCR2A  = (clock - 1);
        OCR2B  = (clock / 2);
    }

    void Reset()
    {
        res_bit(RES_PORT, RES_PIN);
        control_bus_delay(tRW);
        set_bit(RES_PORT, RES_PIN);
        control_bus_delay(tRB);
    }

    void Send(uint8_t reg, uint8_t data)
    {
        Address(reg);
        Write(data);
    }

    uint8_t Receive(uint8_t reg)
    {
        uint8_t data;
        Address(reg);
        Read(data);
        return data;
    }
}