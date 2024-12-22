#include "Simple.h"

namespace PowerSG
{
    template <typename driver_t>
    void Simple<driver_t>::PowerOn()
    {
        m_driver.chip_power_on();
        SetDefaultClock();
        Reset();
    }

    template <typename driver_t>
    void Simple<driver_t>::Reset()
    {
        m_driver.chip_reset();
    }

    template <typename driver_t>
    void Simple<driver_t>::SetDefaultClock()
    {
        // ZX-Spectrum PSG clock
        SetClock(1773400);
    }

    template <typename driver_t>
    void Simple<driver_t>::SetClock(clk_t clk)
    {
        // limit the clock frequency range
        if (clk < 1000000) clk = 1000000;
        if (clk > 2000000) clk = 2000000;
        m_driver.chip_set_clock(clk);
    }

    template <typename driver_t>
    clk_t Simple<driver_t>::GetClock() const
    {
        clk_t clk = 0;
        m_driver.chip_get_clock(clk);
        return clk;
    }

    template <typename driver_t>
    chipid_t Simple<driver_t>::GetChipId()
    {
        // detect type of PSG on first request
        if (GetClock() != 0 && !m_chipid)
        {
            m_chipid = 7;
            test_wr_rd_regs(0x00);
            test_wr_rd_regs(0x10);
            test_wr_rd_latch(0x00);
            test_wr_rd_latch(0x10);
            test_wr_rd_exp_mode(0xA0);
            test_wr_rd_exp_mode(0xB0);
            Reset();
        }
        return chipid_t(m_chipid);
    }

    template <typename driver_t>
    void Simple<driver_t>::SetRegister(raddr_t addr, rdata_t data)
    {
        // this behavior can be overridden
        m_driver.chip_address(addr);
        m_driver.chip_write(data);
    }

    template <typename driver_t>
    void Simple<driver_t>::GetRegister(raddr_t addr, rdata_t &data)
    {
        // this behavior can be overridden
        m_driver.chip_address(addr);
        m_driver.chip_read(data);
    }

    template <typename driver_t>
    void Simple<driver_t>::Update()
    {
        // for compatibility with descendants
    }

    template <typename driver_t>
    void Simple<driver_t>::update_hash(rdata_t data)
    {
        m_chipid = (31 * m_chipid + uint32_t(data));
    }

    template <typename driver_t>
    void Simple<driver_t>::test_wr_rd_regs(raddr_t offset)
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
            update_hash(data);
        }
    }

    template <typename driver_t>
    void Simple<driver_t>::test_wr_rd_latch(raddr_t offset)
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
            update_hash(data);
        }
    }

    template <typename driver_t>
    void Simple<driver_t>::test_wr_rd_exp_mode(rdata_t mode_bank)
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
            update_hash(data);
        }
    }
}
