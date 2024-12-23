#pragma once

#include "Simple.h"

//#define DISABLE_CLOCK_CONVERSION
//#define DISABLE_CHANNELS_REMAPPING
//#define DISABLE_COMPATIBLE_MODE_FIX

namespace PowerSG
{
    enum
    {
        F1_00MHZ = 1000000, // Amstrad CPC
        F1_50MHZ = 1500000, // Vetrex console
        F1_75MHZ = 1750000, // Pentagon 128K
        F1_77MHZ = 1773400, // ZX-Spectrum (default)
        F2_00MHZ = 2000000  // Atari ST
    };

    enum class Stereo : uint8_t
    { 
        ABC, ACB, BAC, BCA, CAB, CBA 
    };

    enum class Reg : uint8_t
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
        Mode_Bank = raddr_t(Reg::EA_Shape), // numeric value of the AY8930 mode/bank register
        BankA_Fst = raddr_t(Reg::A_Fine),   // numeric value of the first register in bank A
        BankA_Lst = raddr_t(Reg::EA_Shape), // numeric value of the last register in bank A
        BankB_Fst = raddr_t(Reg::EB_Fine),  // numeric value of the first register in bank B
        BankB_Lst = raddr_t(Reg::N_OrMask)  // numeric value of the last register in bank B
    };

    using rpair_t = uint16_t;

    class Advanced : public Simple
    {
        union period_t
        {
            rpair_t full;
            struct { rdata_t fine, coarse; };
        };

        struct channel_t
        {
            period_t t_period;
            rdata_t  t_volume;
            rdata_t  t_duty;
            period_t e_period;
            rdata_t  e_shape;
        };

        struct commons_t
        {
            rdata_t n_period;
            rdata_t n_and_mask;
            rdata_t n_or_mask;
            rdata_t mixer;
        };

        struct status_t
        {
            uint32_t changed;
            uint8_t  exp_mode;
        };

        struct state_t
        {
            channel_t channels[3];
            commons_t commons;
            status_t  status;
        };

    public:
        Advanced(Driver& driver);
        void reset() override;
 
        void setClock(Clock clock) override;
        Clock getClock() const override;

        void setStereo(Stereo stereo);
        Stereo getStereo() const;

        void setRegister(raddr_t addr, rdata_t data) override;
        void getRegister(raddr_t addr, rdata_t &data) const override;
        void setRegister(Reg reg, rdata_t data);
        void getRegister(Reg reg, rdata_t &data) const;
        void update();

    private:
        void set_register(state_t& state, Reg reg, rdata_t data);
        void get_register(const state_t& state, Reg reg, rdata_t &data) const;

        void process_clock_conversion();
        void process_channels_remapping();
        void process_compatible_mode_fix();

        void check_output_changes();
        void write_output_to_chip();

    private:
        uint32_t m_clock;
        Stereo   m_sstereo;
        Stereo   m_dstereo;
        state_t  m_input;
        state_t  m_output;
    };
}
