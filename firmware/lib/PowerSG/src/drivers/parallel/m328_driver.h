#pragma once

#include "drivers/Driver.h"

namespace PowerSG
{
    class m328_driver : public Driver
    {
    public:
        void chip_power_on() override;
        void chip_set_clock(clk_t clk) override;
        void chip_get_clock(clk_t &clk) override;

        void chip_reset() override;
        void chip_address(raddr_t addr) override;
        void chip_write(rdata_t data) override;
        void chip_read(rdata_t &data) override;
    
    private:
        uint8_t m_clkDiv { 0 };
    };
}

