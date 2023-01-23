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
    enum class Reg
    {
        // bank A registers
        A_Fine    = 0x00, // Tone A period (fine tune)
        A_Coarse  = 0x01, // Tone A period (coarse tune)
        B_Fine    = 0x02, // Tone B period (fine tune)
        B_Coarse  = 0x03, // Tone B period (coarse tune)
        C_Fine    = 0x04, // Tone C period (fine tune)
        C_Coarse  = 0x05, // Tone C period (coarse tune)
        N_Period  = 0x06, // Noise period
        Mixer     = 0x07, // Mixer control + I/O enable
        A_Volume  = 0x08, // Amplitude A control
        B_Volume  = 0x09, // Amplitude B control
        C_Volume  = 0x0A, // Amplitude C control
        EA_Fine   = 0x0B, // Envelope/AY8930 Envelope A period (fine tune)
        EA_Coarse = 0x0C, // Envelope/AY8930 Envelope A period (coarse tune)
        EA_Shape  = 0x0D, // Envelope/AY8930 Envelope A shape + AY8930 mode/bank

        // bank B registers
        EB_Fine   = 0x10, // AY8930 Envelope B period (fine tune)
        EB_Coarse = 0x11, // AY8930 Envelope B period (coarse tune)
        EC_Fine   = 0x12, // AY8930 Envelope C period (fine tune)
        EC_Coarse = 0x13, // AY8930 Envelope C period (coarse tune)
        EB_Shape  = 0x14, // AY8930 Envelope B shape
        EC_Shape  = 0x15, // AY8930 Envelope C shape
        A_Duty    = 0x16, // AY8930 Duty A cycle
        B_Duty    = 0x17, // AY8930 Duty B cycle
        C_Duty    = 0x18, // AY8930 Duty C cycle
        N_AndMask = 0x19, // AY8930 Noise "AND" mask
        N_OrMask  = 0x1A, // AY8930 Noise "OR" mask

        // aliases
        E_Fine    = EA_Fine,   // comp mode alias for Envelope period (fine tune) 
        E_Coarse  = EA_Coarse, // comp mode alias for Envelope period (coarse tune) 
        E_Shape   = EA_Shape,  // comp mode alias for Envelope shape
        Mode_Bank = EA_Shape,  // alias for AY8930 mode/bank register
    };

    enum
    {
        Mode_Bank = uint8_t(Reg::EA_Shape), // numeric value of the AY8930 mode/bank register
        BankA_Fst = uint8_t(Reg::A_Fine),   // numeric value of the first register in bank A
        BankA_Lst = uint8_t(Reg::EA_Shape), // numeric value of the last register in bank A
        BankB_Fst = uint8_t(Reg::EB_Fine),  // numeric value of the first register in bank B
        BankB_Lst = uint8_t(Reg::N_OrMask)  // numeric value of the last register in bank B
    };

    enum class Type
    {
        NotFound     = 0x00, // chip not found
        Compatible   = 0x01, // Variants: WF19054, JFC95101, KC89C72 etc
        AY8910A      = 0x02, // GI AY-3-8910A, Microchip AY38910A/P
        AY8912A      = 0x03, // TODO: not implemented
        AY8913A      = 0x04, // TODO: not implemented
        AY8930       = 0x05, // Microchip AY8930
        YM2149F      = 0x06, // Yamaha YM2149F
        AVRAY_FW26   = 0x07, // Emulator (https://www.avray.ru)
        BadOrUnknown = 0xFF  // broken chip or unknown model
    };

    enum
    {
        F1_00MHZ = 1000000, // Amstrad CPC
        F1_50MHZ = 1500000, // Vetrex console
        F1_75MHZ = 1750000, // Pentagon 128K
        F1_77MHZ = 1773400, // ZX-Spectrum (default)
        F2_00MHZ = 2000000  // Atari ST
    };

    enum class Stereo { ABC, ACB, BAC, BCA, CAB, CBA };

    PSG();

// -----------------------------------------------------------------------------
// Low Level Interface
// -----------------------------------------------------------------------------
public:
    void Init();
    void Reset();
    void Address(uint8_t data);
    void Write(uint8_t data);
    void Read(uint8_t& data);

    // all PSGs share the same clock rate
    static void SetClock(uint32_t clock);
    static uint32_t GetClock();

// -----------------------------------------------------------------------------
// High Level Interface
// -----------------------------------------------------------------------------
public:
    Type GetType() const;
    bool IsReady() const;

    void   SetStereo(Stereo stereo);
    Stereo GetStereo() const;

    void SetRegister(uint8_t reg, uint8_t  data);
    void GetRegister(uint8_t reg, uint8_t& data) const;
    void SetRegister(Reg reg, uint8_t  data);
    void GetRegister(Reg reg, uint8_t& data) const;
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
    void do_test_wr_rd_exp_mode(uint8_t mode_bank);
    void process_clock_conversion();
    void process_channels_remapping();
    void process_compat_mode_fix();
    void reset_input_state();
    void write_output_state();

    uint32_t m_hash;

    // all PSGs share the same clock rate
    static uint32_t s_rclock;
    static uint32_t s_vclock;

    // by default source/destination mode is ABC
    Stereo m_sstereo = Stereo::ABC;
    Stereo m_dstereo = Stereo::ABC;

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
        uint8_t  exp_mode;
    };

    struct State
    {
        Channel channels[3];
        Commons commons;
        Status  status;
    };

    State   m_states[2];
    uint8_t m_current;
};
