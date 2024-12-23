#pragma once

#include "drivers/Driver.h"

namespace PowerSG
{
    enum class ChipId : uint32_t
    {
        NotFound   = 0x67E019C7, // chip not found
        Compatible = 0xF56B7047, // Variants: WF19054, JFC95101, KC89C72 etc
        AY8910     = 0x2CFF954F, // GI AY-3-8910A, Microchip AY38910A/P
        AY8930     = 0x4EFE6E06, // Microchip AY8930
        YM2149F    = 0x1D750557, // Yamaha YM2149F
    };

    using Clock = uint32_t;
    using raddr_t = uint8_t;
    using rdata_t = uint8_t;

    class Simple
    {
    public:
        Simple(Driver& driver);

        // power on and reset operations with PSG
        void begin();
        virtual void reset();
        
        // set/get operations with PSG configuration
        void setDefaultClock();
        virtual void  setClock(Clock clock);
        virtual Clock getClock() const;
        ChipId getChipId() const;

        // set/get operations with PSG registers
        void mute();
        virtual void setRegister(raddr_t addr, rdata_t data);
        virtual void getRegister(raddr_t addr, rdata_t &data) const;

    private:
        // tests to detect the type of PSG
        void update_chipid(rdata_t data);
        void test_wr_rd_regs(raddr_t offset);
        void test_wr_rd_latch(raddr_t offset);
        void test_wr_rd_exp_mode(rdata_t mode_bank);

    private:
        Driver&  m_driver;
        uint32_t m_chipid;
    };
}
