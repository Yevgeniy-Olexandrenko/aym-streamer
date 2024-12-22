#include "Simple.h"
namespace PowerSG
{
    template <typename driver_t>
    void Simple<driver_t>::begin()
    {
        m_driver.chip_power_on();
        setDefaultClock();
        reset();
    }

    template <typename driver_t>
    void Simple<driver_t>::reset()
    {
        m_driver.chip_reset();
    }

    template <typename driver_t>
    void Simple<driver_t>::setDefaultClock()
    {
        // ZX-Spectrum PSG clock
        setClock(1773400);
    }

    template <typename driver_t>
    inline void Simple<driver_t>::setClock(Clock clock)
    {
        // limit the clock frequency range
        if (clock < 1000000) clock = 1000000;
        if (clock > 2000000) clock = 2000000;
        m_driver.chip_set_clock(clock);
    }

    template <typename driver_t>
    Clock Simple<driver_t>::getClock() const
    {
        Clock clock = 0;
        const_cast<Simple*>(this)->m_driver.chip_get_clock(clock);
        return clock;
    }

    template <typename driver_t>
    ChipId Simple<driver_t>::getChipId()
    {
        // detect type of PSG on first request
        if (getClock() != 0 && !m_chipid)
        {
            m_chipid = 7;
            test_wr_rd_regs(0x00);
            test_wr_rd_regs(0x10);
            test_wr_rd_latch(0x00);
            test_wr_rd_latch(0x10);
            test_wr_rd_exp_mode(0xA0);
            test_wr_rd_exp_mode(0xB0);
            reset();
        }
        return ChipId(m_chipid);
    }

    template <typename driver_t>
    void Simple<driver_t>::setRegister(raddr_t addr, rdata_t data)
    {
        // this behavior can be overridden
        m_driver.chip_address(addr);
        m_driver.chip_write(data);
    }

    template <typename driver_t>
    void Simple<driver_t>::getRegister(raddr_t addr, rdata_t &data)
    {
        // this behavior can be overridden
        m_driver.chip_address(addr);
        m_driver.chip_read(data);
    }

    template <typename driver_t>
    void Simple<driver_t>::update()
    {
        // for compatibility with descendants
    }

    template <typename driver_t>
    void Simple<driver_t>::update_chipid(rdata_t data)
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
            update_chipid(data);
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
            update_chipid(data);
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
            update_chipid(data);
        }
    }
}
