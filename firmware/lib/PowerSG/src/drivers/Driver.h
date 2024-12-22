#pragma once

#include <stdint.h>

namespace PowerSG
{
    using clk_t = uint32_t;
    using raddr_t = uint8_t;
    using rdata_t = uint8_t;

    class Driver
    {
    public:
        virtual void chip_power_on()  = 0;
        virtual void chip_set_clock(clk_t clk) = 0;
        virtual void chip_get_clock(clk_t &clk) = 0;

        virtual void chip_reset() = 0;
        virtual void chip_address(raddr_t addr) = 0;
        virtual void chip_write(rdata_t data) = 0;
        virtual void chip_read(rdata_t &data) = 0;
    };
}
