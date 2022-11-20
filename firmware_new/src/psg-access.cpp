// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#include "psg-access.h"
#include <avr/io.h>
#include <util/delay.h>

#ifdef  PSG_UART_DEBUG
#define UART_DEBUG
#endif
#include "uart-debug.h"

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

enum Hash
{
    NotFound     = 0x67E019C7,
    Compatible   = 0xF56B7047,
    AY8910A      = 0x2CFF954F,
    AY8912A      = 0x00000000,
    AY8930       = 0x4EFE6E06,
    YM2149F      = 0x1D750557,
    AVRAY_FW26   = 0x11111111,
};

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

// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------

void PSG::Address(uint8_t reg)
{
    set_data_bus(reg);
    set_control_bus_addr();
    control_bus_delay(tAS);
    set_control_bus_inact();
    control_bus_delay(tAH);
    release_data_bus();
}

void PSG::Write(uint8_t data)
{
    set_data_bus(data);
    set_control_bus_write();
    control_bus_delay(tDW);
    set_control_bus_inact();
    control_bus_delay(tDH);
    release_data_bus();
}

void PSG::Read(uint8_t& data)
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

PSG::PSG()
    : m_hash(Hash::NotFound)
{
}

void PSG::Init()
{
    dbg_open(9600);

    // setup reset
    set_bit(RES_DDR,  RES_PIN);
    Reset();

    // setup control and data bus
    set_bit(BUS_DDR, BDIR_PIN);
    set_bit(BUS_DDR, BC1_PIN);
    set_control_bus_inact();
    release_data_bus();
    detect();

    // setup clock and reset
    set_bit(CLK_DDR, CLK_PIN);
    SetClock(F1_77MHZ);
    Reset();

    // print clock configuration
    dbg_print_str(F("PSG chip clock:\n"));
    dbg_print_str(F("virt: "));
    dbg_print_num(m_vclock);
    dbg_print_str(F("\nreal: "));
    dbg_print_num(m_rclock);
    dbg_print_ln();

    dbg_print_ln();
    dbg_close();
}

void PSG::Reset()
{
    res_bit(RES_PORT, RES_PIN);
    control_bus_delay(tRW);
    set_bit(RES_PORT, RES_PIN);
    control_bus_delay(tRB);
}

void PSG::SetClock(uint32_t clock)
{
    if (clock >= F1_00MHZ && clock <= F2_00MHZ)
    {
        // compute real and virtual clock
        uint8_t divider = (F_CPU / clock);
        m_rclock = (F_CPU / divider);
        m_vclock = clock;

        // configure Timer2
        TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
        TCCR2B = (1 << WGM22 ) | (1 << CS20 );
        OCR2A  = (divider - 1);
        OCR2B  = (divider / 2);
    }
}

PSG::Type PSG::GetType() const
{
    switch (m_hash)
    {
    case Hash::NotFound:   return Type::NotFound;
    case Hash::Compatible: return Type::Compatible;
    case Hash::AY8910A:    return Type::AY8910A;
    case Hash::AY8912A:    return Type::AY8912A;
    case Hash::AY8930:     return Type::AY8930;
    case Hash::YM2149F:    return Type::YM2149F;
    case Hash::AVRAY_FW26: return Type::AVRAY_FW26;
    }
    return Type::BadOrUnknown;
}

void PSG::SetRegister(uint8_t reg, uint8_t data)
{
    Address(reg);
    Write(data);
}

void PSG::GetRegister(uint8_t reg, uint8_t& data) const
{
    Address(reg);
    Read(data);
}

// -----------------------------------------------------------------------------
// Privates
// -----------------------------------------------------------------------------

void PSG::detect()
{
    // perform detection by a series of tests
    dbg_print_str(F("Testing result dump:\n"));
    reset_hash();
    do_test_wr_rd_regs(0x00);
    do_test_wr_rd_regs(0x10);
    do_test_wr_rd_latch(0x00);
    do_test_wr_rd_latch(0x10);
    do_test_wr_rd_extmode(0xA0);
    do_test_wr_rd_extmode(0xB0);

    // print result hash
    dbg_print_str(F("\nTesting result hash:\n"));
    dbg_print_dwrd(m_hash);
    dbg_print_ln();

    // print a type of the chip
    dbg_print_str(F("\nPSG chip type:\n"));
    switch (GetType())
    {
    case Type::NotFound:   dbg_print_str(F("Not Found!\n")); break;
    case Type::Compatible: dbg_print_str(F("AY/YM Compatible\n")); break;
    case Type::AY8910A:    dbg_print_str(F("AY-3-8910A\n")); break;
    case Type::AY8912A:    dbg_print_str(F("AY-3-8912A\n")); break;
    case Type::AY8930:     dbg_print_str(F("AY8930\n")); break;
    case Type::YM2149F:    dbg_print_str(F("YM2149F\n")); break;
    case Type::AVRAY_FW26: dbg_print_str(F("AVR-AY (FW:26)\n")); break;
    default: dbg_print_str(F("Bad or Unknown!\n")); break;
    }
    dbg_print_ln();
}

void PSG::reset_hash() { m_hash = 7; }
void PSG::update_hash(uint8_t data) { m_hash = 31 * m_hash + uint32_t(data); }

void PSG::do_test_wr_rd_regs(uint8_t offset)
{
    for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
    {
        data = (reg == Mixer ? 0x3F : (reg == Mode_Bank ? 0x0F : 0xFF));
        Address(reg);
        Write(data);
    }
    for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
    {
        Address(reg);
        Read(data);
        update_hash(data);
        dbg_print_byte(data);
        dbg_print_sp();
    }
    dbg_print_ln();
}

void PSG::do_test_wr_rd_latch(uint8_t offset)
{
    for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
    {
        data = (reg == Mixer ? 0x3F : (reg == Mode_Bank ? 0x0F : 0xFF));
        Address(reg);
        Write(data);
        Read(data);
        update_hash(data);
        dbg_print_byte(data);
        dbg_print_sp();
    }
    dbg_print_ln();
}

void PSG::do_test_wr_rd_extmode(uint8_t mode_bank)
{
    Address(Mode_Bank);
    Write(mode_bank | 0x0F);
    for (uint8_t reg = 0; reg < 16 - 2; ++reg)
    {
        if (reg == Mode_Bank) continue;
        Address(reg);
        Write(0xFF);
    }
    for (uint8_t data, reg = 0; reg < 16 - 2; ++reg)
    {
        Address(reg);
        Read(data);
        update_hash(data);
        dbg_print_byte(data);
        dbg_print_sp();
    }
    dbg_print_ln();
}
