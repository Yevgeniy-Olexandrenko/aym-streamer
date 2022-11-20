// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include "psg-config.h"

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

    PSG();

// -----------------------------------------------------------------------------
// Low Level Interface
// -----------------------------------------------------------------------------
public:
    void Init();
    void SetClock(uint32_t clock);

    void Reset();
    void Address(uint8_t data);
    void Write(uint8_t data);
    void Read(uint8_t& data);

// -----------------------------------------------------------------------------
// High Level Interface
// -----------------------------------------------------------------------------
public:
    Type GetType() const;
    bool IsReady() const;
    void SetRegister(uint8_t reg, uint8_t data);
    void GetRegister(uint8_t reg, uint8_t& data) const;
    void Update();

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
    void process_clock_conversion();
    void process_ay8930_envelope_fix();
    void write_state_to_chip();

private:
#ifdef PSG_PROCESSING
    union Period
    {
        uint16_t full;
        struct { uint8_t fine, coarse; };
    };

    struct Channel
    {
        Period  t_period;
        uint8_t t_volume;
        uint8_t t_duty;
        Period  e_period;
        uint8_t e_shape;
    };

    struct Commons
    {
        uint8_t n_period;
        uint8_t n_and_mask;
        uint8_t n_or_mask;
        uint8_t mixer;
    };

    struct Status
    {
        uint32_t changed;
        uint8_t exp_mode;
    };

    struct State
    {
        Channel channels[3];
        Commons commons;
        Status  status;
    };

    State m_states[2];
    uint8_t m_sindex;
#endif

    uint32_t m_hash;
    uint32_t m_vclock;
    uint32_t m_rclock;
};
