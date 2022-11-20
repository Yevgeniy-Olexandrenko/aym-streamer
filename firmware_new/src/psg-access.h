// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#pragma once
#define PSG_UART_DEBUG
#include <stdint.h>

class PSG
{
// -----------------------------------------------------------------------------
// PSG Definitions
// -----------------------------------------------------------------------------
public:
    enum
    {
        // bank A
        A_Fine    = 0x00,
        A_Coarse  = 0x01,
        B_Fine    = 0x02,
        B_Coarse  = 0x03,
        C_Fine    = 0x04,
        C_Coarse  = 0x05,
        N_Period  = 0x06,
        Mixer     = 0x07,
        A_Volume  = 0x08,
        B_Volume  = 0x09,
        C_Volume  = 0x0A,
        EA_Fine   = 0x0B,
        EA_Coarse = 0x0C,
        EA_Shape  = 0x0D,

        // bank B
        EB_Fine   = 0x10,
        EB_Coarse = 0x11,
        EC_Fine   = 0x12,
        EC_Coarse = 0x13,
        EB_Shape  = 0x14,
        EC_Shape  = 0x15,
        A_Duty    = 0x16,
        B_Duty    = 0x17,
        C_Duty    = 0x18,
        N_AndMask = 0x19,
        N_OrMask  = 0x1A,

        // aliases
        E_Fine    = EA_Fine,
        E_Coarse  = EA_Coarse,
        E_Shape   = EA_Shape,
        Mode_Bank = EA_Shape,

        A_Period  = A_Fine,
        B_Period  = B_Fine,
        C_Period  = C_Fine,
        E_Period  = E_Fine,
        EA_Period = EA_Fine,
        EB_Period = EB_Fine,
        EC_Period = EC_Fine,

        BankA_Fst = 0x00,
        BankA_Lst = 0x0D,
        BankB_Fst = 0x10,
        BankB_Lst = 0x1D,
    };

    enum Type
    {
        NotFound     = 0x00,
        Compatible   = 0x01,
        AY8910A      = 0x03,
        AY8912A      = 0x05,
        AY8930       = 0x06,
        YM2149F      = 0x07,
        AVRAY_FW26   = 0x08,
        BadOrUnknown = 0xFF
    };

    enum
    {
        F1_00MHZ = 1000000,
        F1_50MHZ = 1500000,
        F1_75MHZ = 1750000,
        F1_77MHZ = 1773400,
        F2_00MHZ = 2000000,
    };

// -----------------------------------------------------------------------------
// Low Level Access
// -----------------------------------------------------------------------------
public:
    void Address(uint8_t data);
    void Write(uint8_t data);
    void Read(uint8_t& data);

// -----------------------------------------------------------------------------
// High Level Access
// -----------------------------------------------------------------------------
public:
    PSG();
    void Init();
    void Reset();
    void SetClock(uint32_t clock);
    Type GetType() const;
    void SetRegister(uint8_t reg, uint8_t data);
    void GetRegister(uint8_t reg, uint8_t& data) const;

// -----------------------------------------------------------------------------
// Privates
// -----------------------------------------------------------------------------
private:
    void detect();
    void reset_hash();
    void update_hash(uint8_t data);
    void do_test_wr_rd_regs(uint8_t offset);
    void do_test_wr_rd_latch(uint8_t offset);
    void do_test_wr_rd_extmode(uint8_t mode_bank);

private:
    uint32_t m_hash;
    uint32_t m_vclock;
    uint32_t m_rclock;
};
