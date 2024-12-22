#pragma once

#include "drivers/Driver.h"

namespace PowerSG
{
    class m328_driver : public Driver
    {
    public:
        void chip_power_on() override;
        void chip_set_clock(uint32_t clock) override;
        void chip_get_clock(uint32_t &clock) override;

        void chip_reset() override;
        void chip_address(uint8_t addr) override;
        void chip_write(uint8_t data) override;
        void chip_read(uint8_t &data) override;
    
    private:
        uint8_t m_clockDiv { 0 };
    };
}

