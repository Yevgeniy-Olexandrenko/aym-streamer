#include "Simple.h"

namespace PowerSG
{
    Simple::Simple(Driver &driver)
        : m_driver(driver)
    {}

    void Simple::begin()
    {
        m_driver.chip_power_on();
        setDefaultClock();
        reset();
    }

    void Simple::reset()
    {
        m_driver.chip_reset();
    }

    void Simple::setDefaultClock()
    {
        // ZX-Spectrum PSG clock
        setClock(1773400);
    }

    void Simple::setClock(Clock clock)
    {
        // limit the clock frequency range
        if (clock < 1000000) clock = 1000000;
        if (clock > 2000000) clock = 2000000;
        m_driver.chip_set_clock(clock);
    }

    Clock Simple::getClock() const
    {
        Clock clock = 0;
        auto& cless = const_cast<Simple&>(*this);
        cless.m_driver.chip_get_clock(clock);
        return clock;
    }

    ChipId Simple::getChipId() const
    {
        // detect type of PSG on first request
        if (getClock() != 0 && !m_chipid)
        {
            auto& cless = const_cast<Simple&>(*this);
            cless.m_chipid = 7;
            cless.test_wr_rd_regs(0x00);
            cless.test_wr_rd_regs(0x10);
            cless.test_wr_rd_latch(0x00);
            cless.test_wr_rd_latch(0x10);
            cless.test_wr_rd_exp_mode(0xA0);
            cless.test_wr_rd_exp_mode(0xB0);
            cless.reset();
        }
        return ChipId(m_chipid);
    }

    void Simple::mute()
    {
        setRegister(0x08, 0x00);
        setRegister(0x09, 0x00);
        setRegister(0x0A, 0x00);
    }

    void Simple::setRegister(raddr_t addr, rdata_t data)
    {
        // this behavior can be overridden
        m_driver.chip_address(addr);
        m_driver.chip_write(data);
    }

    void Simple::getRegister(raddr_t addr, rdata_t &data) const
    {
        // this behavior can be overridden
        m_driver.chip_address(addr);
        m_driver.chip_read(data);
    }

    void Simple::update_chipid(rdata_t data)
    {
        m_chipid = (31 * m_chipid + uint32_t(data));
    }

    void Simple::test_wr_rd_regs(raddr_t offset)
    {
        rdata_t data;
        for (raddr_t addr = offset; addr < (offset + 14); ++addr)
        {
            data = 0xFF;
            if (addr == 0x07) data = 0x3F;
            if (addr == 0x0D) data = 0x0F;
            m_driver.chip_address(addr);
            m_driver.chip_write(data);
        }
        for (raddr_t addr = offset; addr < (offset + 14); ++addr)
        {
            m_driver.chip_address(addr);
            m_driver.chip_read(data);
            update_chipid(data);
        }
    }

    void Simple::test_wr_rd_latch(raddr_t offset)
    {
        rdata_t data;
        for (raddr_t addr = offset; addr < (offset + 14); ++addr)
        {
            data = 0xFF;
            if (addr == 0x07) data = 0x3F;
            if (addr == 0x0D) data = 0x0F;
            m_driver.chip_address(addr);
            m_driver.chip_write(data);
            m_driver.chip_read(data);
            update_chipid(data);
        }
    }

    void Simple::test_wr_rd_exp_mode(rdata_t mode_bank)
    {
        m_driver.chip_address(0x0D);
        m_driver.chip_write(mode_bank | 0x0F);
        for (raddr_t addr = 0; addr < 14; ++addr)
        {
            if (addr == 0x0D) continue;
            m_driver.chip_address(addr);
            m_driver.chip_write(0xFF);
        }
        rdata_t data;
        for (raddr_t addr = 0; addr < 14; ++addr)
        {
            m_driver.chip_address(addr);
            m_driver.chip_read(data);
            update_chipid(data);
        }
    }
}
