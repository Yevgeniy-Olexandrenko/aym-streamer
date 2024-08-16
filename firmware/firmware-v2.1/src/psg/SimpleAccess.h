#pragma once

#include <stdint.h>

// helpers
#define set_bits(reg, bits) reg |=  (bits)
#define res_bits(reg, bits) reg &= ~(bits)

#define set_bit(reg, bit) reg |=  (1 << (bit))
#define res_bit(reg, bit) reg &= ~(1 << (bit))
#define isb_set(reg, bit) bool((reg) & (1 << (bit)))

namespace psg
{
    enum class Type : uint32_t
    {
        NotFound   = 0x67E019C7, // chip not found
        Compatible = 0xF56B7047, // Variants: WF19054, JFC95101, KC89C72 etc
        AY8910A    = 0x2CFF954F, // GI AY-3-8910A, Microchip AY38910A/P
        AY8912A    = 0x11111111, // TODO: not implemented
        AY8913A    = 0x22222222, // TODO: not implemented
        AY8930     = 0x4EFE6E06, // Microchip AY8930
        YM2149F    = 0x1D750557, // Yamaha YM2149F
        AVRAY_FW26 = 0x33333333  // TODO: Emulator (https://www.avray.ru)
    };

    using Clock = uint32_t; 
    
    class SimpleAccess
    {
    public:
        // power on and reset operations with PSG
        void PowerOn();
        virtual void Reset();
        
        // set/get operations with PSG configuration
        Type GetType();
        virtual Clock GetClock() const;
        virtual void  SetClock(Clock clock);
        virtual void  SetDefaultClock();

        // set/get operations with PSG registers
        virtual void SetRegister(uint8_t reg, uint8_t data);
        virtual void GetRegister(uint8_t reg, uint8_t& data) const;

    protected:
        // basic read/write operations with PSG
        void Address(uint8_t reg);
        void Write(uint8_t data);
        void Read(uint8_t& data);

    private:
        // tests to detect the type of PSG
        void update_hash(uint8_t data);
        void test_wr_rd_regs(uint8_t offset);
        void test_wr_rd_latch(uint8_t offset);
        void test_wr_rd_exp_mode(uint8_t mode_bank);

    private: // 5 bytes
        uint32_t m_typeHash { 0 };
        uint8_t  m_clockDiv { 0 };
    };
}
