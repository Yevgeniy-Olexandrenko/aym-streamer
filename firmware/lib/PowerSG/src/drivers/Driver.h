#pragma once

#include <stdint.h>

namespace PowerSG
{
    class Driver
    {
    public:
        virtual void chip_power_on()  = 0;
        virtual void chip_set_clock(uint32_t clock) = 0;
        virtual void chip_get_clock(uint32_t &clock) = 0;

        virtual void chip_reset() = 0;
        virtual void chip_address(uint8_t addr) = 0;
        virtual void chip_write(uint8_t data) = 0;
        virtual void chip_read(uint8_t &data) = 0;
    };
}
