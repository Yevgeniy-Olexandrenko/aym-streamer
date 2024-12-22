#pragma once

#include "drivers/Driver.h"

namespace PowerSG
{
    enum class chipid_t : uint32_t
    {
        NotFound   = 0x67E019C7, // chip not found
        Compatible = 0xF56B7047, // Variants: WF19054, JFC95101, KC89C72 etc
        AY8910     = 0x2CFF954F, // GI AY-3-8910A, Microchip AY38910A/P
        AY8930     = 0x4EFE6E06, // Microchip AY8930
        YM2149F    = 0x1D750557, // Yamaha YM2149F
    };
    
    template <typename driver_t> class Simple
    {
    public:
        // power on and reset operations with PSG
        virtual void PowerOn();
        virtual void Reset();
        
        // set/get operations with PSG configuration
        virtual void SetDefaultClock();
        virtual void SetClock(clk_t clk);
        virtual clk_t GetClock() const;
        virtual chipid_t GetChipId();

        // set/get operations with PSG registers
        virtual void SetRegister(raddr_t addr, rdata_t data);
        virtual void GetRegister(raddr_t addr, rdata_t &data);
        virtual void Update();

    private:
        // tests to detect the type of PSG
        void update_hash(rdata_t data);
        void test_wr_rd_regs(raddr_t offset);
        void test_wr_rd_latch(raddr_t offset);
        void test_wr_rd_exp_mode(rdata_t mode_bank);

    private: // 5 bytes
        driver_t m_driver;
        uint32_t m_chipid { 0 };
    };
}
