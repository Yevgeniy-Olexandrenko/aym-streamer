#include <avr/io.h>
#include <util/delay.h>

#include "SimpleAccess.h"
#include "WiringConfig.h"

// helpers
#define wait_for_delay(ns) _delay_us(0.001f * (ns))
#define BUS_MASK (1 << BUS_BDIR | 1 << BUS_BC1)

namespace psg
{
    enum // timing delays (nano seconds)
    {
        tAS = 400, // Address Setup Time
        tAH = 100, // Address Hold Time
        tDW = 500, // Write Data Pulse Width
        tDH = 100, // Write Data Hold Time
        tDA = 400, // Read Data Access Time
        tTS = 100, // Tristate Delay Time
        tRW = 500, // Reset Pulse Width
        tRB = 100  // Reset to Bus Control Delay Time
    };

    // -------------------------------------------------------------------------

    inline void set_data_bus(uint8_t data)
    {
        // set ports to output
        set_bits(LSB_DDR, LSB_MASK);
        set_bits(MSB_DDR, MSB_MASK);

        // set data bits to output ports
        LSB_PORT = (LSB_PORT & ~LSB_MASK) | (data & LSB_MASK);
        MSB_PORT = (MSB_PORT & ~MSB_MASK) | (data & MSB_MASK);
    }

    inline void get_data_bus(uint8_t& data)
    {
        // get bata bits from input ports
        data = (LSB_PIN & LSB_MASK) | (MSB_PIN & MSB_MASK);
    }

    inline void release_data_bus()
    {
        // setup ports to input
        res_bits(LSB_DDR, LSB_MASK);
        res_bits(MSB_DDR, MSB_MASK);

        // enable pull-up resistors
        set_bits(LSB_PORT, LSB_MASK);
        set_bits(MSB_PORT, MSB_MASK);
    }

    inline void set_ctrl_bus_addr()  { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BDIR | 1 << BUS_BC1; }
    inline void set_ctrl_bus_write() { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BDIR; }
    inline void set_ctrl_bus_read()  { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BC1;  }
    inline void set_ctrl_bus_inact() { BUS_PORT = (BUS_PORT & ~BUS_MASK); }

    // -------------------------------------------------------------------------

    void SimpleAccess::PowerOn()
    {
        // setup hardware pins
        set_bit(BUS_DDR, BUS_BDIR); // output
        set_bit(BUS_DDR, BUS_BC1 ); // output
        set_bit(SIG_DDR, SIG_RES ); // output
        set_bit(SIG_DDR, SIG_CLK ); // output
        set_ctrl_bus_inact();
        release_data_bus();

        // setup default clock and reset
        SetDefaultClock();
        Reset();
    }

    void SimpleAccess::Reset()
    {
        // 'Reset PSG' sequence
        res_bit(SIG_PORT, SIG_RES);
        wait_for_delay(tRW);
        set_bit(SIG_PORT, SIG_RES);
        wait_for_delay(tRB);
    }

    Type SimpleAccess::GetType()
    {
        // detect type of PSG on first request
        if (m_clockDiv && !m_typeHash)
        {
            m_typeHash = 7;
            test_wr_rd_regs(0x00);
            test_wr_rd_regs(0x10);
            test_wr_rd_latch(0x00);
            test_wr_rd_latch(0x10);
            test_wr_rd_exp_mode(0xA0);
            test_wr_rd_exp_mode(0xB0);
            Reset();
        }
        return Type(m_typeHash);
    }

    Clock SimpleAccess::GetClock() const
    {
        // real PSG clock based on Timer2 divider
        return (F_CPU / m_clockDiv);
    }

    void SimpleAccess::SetClock(Clock clock)
    {
        // configure Timer2 as a clock source for PSG
        TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
        TCCR2B = (1 << WGM22 ) | (1 << CS20 );

        // configure Timer2 divider
        m_clockDiv = uint8_t(F_CPU / clock);
        OCR2A = (m_clockDiv - 1);
        OCR2B = (m_clockDiv / 2);
    }

    void SimpleAccess::SetDefaultClock()
    {
        // configure Timer2 divider to 0x09
        SetClock(1777777);
    }

    void SimpleAccess::SetRegister(uint8_t reg, uint8_t data)
    {
        // this behavior can be overridden
        Address(reg); Write(data);
    }

    void SimpleAccess::GetRegister(uint8_t reg, uint8_t& data) const
    {
        // this behavior can be overridden
        Address(reg); Read(data);
    }

    // -------------------------------------------------------------------------

    void SimpleAccess::Address(uint8_t reg)
    {
        // 'Latch Address' sequence
        set_ctrl_bus_addr();
        set_data_bus(reg);
        wait_for_delay(tAS);
        set_ctrl_bus_inact();
        wait_for_delay(tAH);
        release_data_bus();
    }

    void SimpleAccess::Write(uint8_t data)
    {
        // 'Write to PSG' sequence
        set_data_bus(data);
        set_ctrl_bus_write();
        wait_for_delay(tDW);
        set_ctrl_bus_inact();
        wait_for_delay(tDH);
        release_data_bus();
    }

    void SimpleAccess::Read(uint8_t& data)
    {
        // 'Read from PSG' sequence
        set_ctrl_bus_read();
        wait_for_delay(tDA);
        get_data_bus(data);
        set_ctrl_bus_inact();
        wait_for_delay(tTS);
    }

    // -------------------------------------------------------------------------

    void SimpleAccess::update_hash(uint8_t data)
    {
        m_typeHash = (31 * m_typeHash + uint32_t(data)); 
    }

    void SimpleAccess::test_wr_rd_regs(uint8_t offset)
    {
        for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
        {
            data = 0xFF;
            if (reg == 0x07) data = 0x3F;
            if (reg == 0x0D) data = 0x0F;
            Address(reg); Write(data);
        }
        for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
        {
            Address(reg); Read(data);
            update_hash(data);
        }
    }

    void SimpleAccess::test_wr_rd_latch(uint8_t offset)
    {
        for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
        {
            data = 0xFF;
            if (reg == 0x07) data = 0x3F;
            if (reg == 0x0D) data = 0x0F;
            Address(reg); Write(data); Read(data);
            update_hash(data);
        }
    }

    void SimpleAccess::test_wr_rd_exp_mode(uint8_t mode_bank)
    {
        Address(0x0D); Write(mode_bank | 0x0F);
        for (uint8_t reg = 0; reg < 16 - 2; ++reg)
        {
            if (reg == 0x0D) continue;
            Address(reg); Write(0xFF);
        }
        for (uint8_t data, reg = 0; reg < 16 - 2; ++reg)
        {
            Address(reg); Read(data);
            update_hash(data);
        }
    }
}
