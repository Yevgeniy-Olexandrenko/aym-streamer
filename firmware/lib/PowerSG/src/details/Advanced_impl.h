#include <string.h>
#include <avr/pgmspace.h>
#include "drivers/DriverHelper.h"

namespace PowerSG
{
    template<class Reg, typename Off> 
    constexpr uint32_t to_mask(const Reg& reg, const Off& off)
    {
        return (UINT32_C(1) << (uint8_t(reg) + uint8_t(off)));
    }

    template<class Reg> 
    constexpr uint32_t to_mask(const Reg& reg)
    {
        return to_mask(reg, 0);
    }

    const uint8_t e_fine[] PROGMEM = 
    {
        uint8_t(Reg::EA_Fine), uint8_t(Reg::EB_Fine), uint8_t(Reg::EC_Fine)
    };

    const uint8_t e_coarse[] PROGMEM = 
    {
        uint8_t(Reg::EA_Coarse), uint8_t(Reg::EB_Coarse), uint8_t(Reg::EC_Coarse)
    };

    const uint8_t e_shape[] PROGMEM = 
    {
        uint8_t(Reg::EA_Shape), uint8_t(Reg::EB_Shape), uint8_t(Reg::EC_Shape)
    };

    template <typename driver_t>
    void Advanced<driver_t>::Reset()
    {
        Simple<driver_t>::Reset();
        memset(&m_input, 0, sizeof(State));
    }

    template <typename driver_t>
    void Advanced<driver_t>::SetDefaultClock()
    {
        SetClock(F1_77MHZ);
    }

    template <typename driver_t>
    void Advanced<driver_t>::SetClock(clk_t clk)
    {
        // limit the clock frequency range
        if (clk < F1_00MHZ) clk = F1_00MHZ;
        if (clk > F2_00MHZ) clk = F2_00MHZ;

        // set real/virtual clock frequency
        Simple<driver_t>::SetClock(clk);
        m_clock = clk;
    }

    template <typename driver_t>
    clk_t Advanced<driver_t>::GetClock() const
    {
        return m_clock;
    }

    template <typename driver_t>
    void Advanced<driver_t>::SetStereo(Stereo stereo)
    {
        m_sstereo = stereo;
        m_dstereo = stereo;
    }

    template <typename driver_t>
    Stereo Advanced<driver_t>::GetStereo() const
    {
        return m_dstereo;
    }

    template <typename driver_t>
    void Advanced<driver_t>::SetRegister(raddr_t addr, rdata_t data)
    {
        // set register data indirectly via bank switching
        // register address must be in range 0x00-0x0F
        if (addr < 0x10)
        {
            // redirect access to registers of bank B if
            // exp mode is active and bank B is selected
            if (m_input.status.exp_mode == 0xB0)
            {
                addr += BankB_Fst;
            }
            set_register(m_input, Reg(addr), data);
        }
    }

    template <typename driver_t>
    void Advanced<driver_t>::GetRegister(raddr_t addr, rdata_t &data)
    {
        // get register data indirectly via bank switching
        // register address must be in range 0x00-0x0F
        if (addr < 0x10)
        {
            // redirect access to registers of bank B if
            // exp mode is active and bank B is selected
            if (m_input.status.exp_mode == 0xB0)
            {
                addr += BankB_Fst;
            }
            get_register(m_input, Reg(addr), data);
        }
    }

    template <typename driver_t>
    void Advanced<driver_t>::SetRegister(Reg reg, rdata_t data)
    {
        // set register data directly
        set_register(m_input, reg, data);
    }

    template <typename driver_t>
    void Advanced<driver_t>::GetRegister(Reg reg, rdata_t &data)
    {
        // get register data directly
        get_register(m_input, reg, data);
    }

    template <typename driver_t>
    void Advanced<driver_t>::Update()
    {
        if (m_input.status.changed)
        {
            // copy input state to output and reset changes
            memcpy(&m_output, &m_input, sizeof(State));
            m_input.status.changed = 0;

            // process outout state and write to PSG
            process_clock_conversion();
            process_channels_remapping();
            process_compatible_mode_fix();
            
            check_output_changes();
            write_output_to_chip();
        }
    }

    template <typename driver_t>
    void Advanced<driver_t>::set_register(State &state, Reg reg, rdata_t data)
    {
        // preserve the state of exp mode and bank of regs
        // separately from the shape of channel A envelope
        if ((uint8_t(reg) & 0x0F) == Mode_Bank)
        {
            state.status.exp_mode = (data & 0xF0);
            data &= 0x0F; reg = Reg::Mode_Bank;
        }

        // map register to corresponding field of state
        switch(reg)
        {
            // bank A
            case Reg::A_Fine:    state.channels[0].t_period.fine = data; break;
            case Reg::A_Coarse:  state.channels[0].t_period.coarse = data; break;
            case Reg::B_Fine:    state.channels[1].t_period.fine = data; break;
            case Reg::B_Coarse:  state.channels[1].t_period.coarse = data; break;
            case Reg::C_Fine:    state.channels[2].t_period.fine = data; break;
            case Reg::C_Coarse:  state.channels[2].t_period.coarse = data; break;
            case Reg::N_Period:  state.commons.n_period = data; break;
            case Reg::Mixer:     state.commons.mixer = data; break;
            case Reg::A_Volume:  state.channels[0].t_volume = data; break;
            case Reg::B_Volume:  state.channels[1].t_volume = data; break;
            case Reg::C_Volume:  state.channels[2].t_volume = data; break;
            case Reg::EA_Fine:   state.channels[0].e_period.fine = data; break;
            case Reg::EA_Coarse: state.channels[0].e_period.coarse = data; break;
            case Reg::EA_Shape:  state.channels[0].e_shape = data; break;

            // bank B
            case Reg::EB_Fine:   state.channels[1].e_period.fine = data; break;
            case Reg::EB_Coarse: state.channels[1].e_period.coarse = data; break;
            case Reg::EC_Fine:   state.channels[2].e_period.fine = data; break;
            case Reg::EC_Coarse: state.channels[2].e_period.coarse = data; break;
            case Reg::EB_Shape:  state.channels[1].e_shape = data; break;
            case Reg::EC_Shape:  state.channels[2].e_shape = data; break;
            case Reg::A_Duty:    state.channels[0].t_duty = data; break;
            case Reg::B_Duty:    state.channels[1].t_duty = data; break;
            case Reg::C_Duty:    state.channels[2].t_duty = data; break;
            case Reg::N_AndMask: state.commons.n_and_mask = data; break;
            case Reg::N_OrMask:  state.commons.n_or_mask = data; break;
        }

        // mark register as changed
        state.status.changed |= to_mask(reg);
    }

    template <typename driver_t>
    void Advanced<driver_t>::get_register(const State &state, Reg reg, rdata_t &data) const
    {
        // map register to corresponding field of state
        switch(reg)
        {
            // bank A
            case Reg::A_Fine:    data = state.channels[0].t_period.fine; break;
            case Reg::A_Coarse:  data = state.channels[0].t_period.coarse; break;
            case Reg::B_Fine:    data = state.channels[1].t_period.fine; break;
            case Reg::B_Coarse:  data = state.channels[1].t_period.coarse; break;
            case Reg::C_Fine:    data = state.channels[2].t_period.fine; break;
            case Reg::C_Coarse:  data = state.channels[2].t_period.coarse; break;
            case Reg::N_Period:  data = state.commons.n_period; break;
            case Reg::Mixer:     data = state.commons.mixer; break;
            case Reg::A_Volume:  data = state.channels[0].t_volume; break;
            case Reg::B_Volume:  data = state.channels[1].t_volume; break;
            case Reg::C_Volume:  data = state.channels[2].t_volume; break;
            case Reg::EA_Fine:   data = state.channels[0].e_period.fine; break;
            case Reg::EA_Coarse: data = state.channels[0].e_period.coarse; break;
            case Reg::EA_Shape:  data = state.channels[0].e_shape; break;

            // bank B
            case Reg::EB_Fine:   data = state.channels[1].e_period.fine; break;
            case Reg::EB_Coarse: data = state.channels[1].e_period.coarse; break;
            case Reg::EC_Fine:   data = state.channels[2].e_period.fine; break;
            case Reg::EC_Coarse: data = state.channels[2].e_period.coarse; break;
            case Reg::EB_Shape:  data = state.channels[1].e_shape; break;
            case Reg::EC_Shape:  data = state.channels[2].e_shape; break;
            case Reg::A_Duty:    data = state.channels[0].t_duty; break;
            case Reg::B_Duty:    data = state.channels[1].t_duty; break;
            case Reg::C_Duty:    data = state.channels[2].t_duty; break;
            case Reg::N_AndMask: data = state.commons.n_and_mask; break;
            case Reg::N_OrMask:  data = state.commons.n_or_mask; break;
        }

        // combine the current state of exp mode and bank
        // of regs with the shape of channel A envelope
        if ((uint8_t(reg) & 0x0F) == Mode_Bank)
        {
            data |= state.status.exp_mode;
        }
    }

    template <typename driver_t>
    void Advanced<driver_t>::process_clock_conversion()
    {
    #ifndef DISABLE_CLOCK_CONVERSION
        const clk_t vclock = GetClock();
        const clk_t rclock = Simple<driver_t>::GetClock();

        if (rclock != vclock)
        {
            uint16_t t_bound = (m_output.status.exp_mode ? 0xFFFF : 0x0FFF);
            uint16_t n_bound = (m_output.status.exp_mode ? 0x00FF : 0x001F);

            // safe period conversion based on clock ratio
            const auto convert_period = [&](uint16_t& period, uint16_t bound)
            {
                uint32_t converted = ((period * (rclock >> 5) / (vclock >> 6) + 1) >> 1);
                period = uint16_t(converted > bound ? bound : converted);
            };

            // convert tone and envelope periods
            for (int i = 0; i < 3; ++i)
            {
                convert_period(m_output.channels[i].t_period.full, t_bound);
                convert_period(m_output.channels[i].e_period.full, 0xFFFF);
            }
            m_output.status.changed |= 0b01100001111111; // hack

            // convert noise period
            uint16_t period = m_output.commons.n_period;
            convert_period(period, n_bound);
            m_output.commons.n_period = uint8_t(period);
        }
    #endif
    }

    template <typename driver_t>
    void Advanced<driver_t>::process_channels_remapping()
    {
    #ifndef DISABLE_CHANNELS_REMAPPING
        // restrict stereo modes available for exp mode
        m_dstereo = (m_output.status.exp_mode && m_sstereo != Stereo::ABC && m_sstereo != Stereo::ACB)
            ? Stereo::ABC
            : m_sstereo;

        if (m_dstereo != Stereo::ABC)
        {
            const auto swap_register = [&](uint8_t reg_l, uint8_t reg_r)
            {
                uint8_t data_l, data_r;
                get_register(m_output, Reg(reg_l), data_l);
                get_register(m_output, Reg(reg_r), data_r);
                set_register(m_output, Reg(reg_l), data_r);
                set_register(m_output, Reg(reg_r), data_l);
            };

            const auto swap_channels = [&](uint8_t l, uint8_t r)
            {
                // swap tone period and tone volume/envelope enable
                swap_register(uint8_t(Reg::A_Fine) + 2 * l, uint8_t(Reg::A_Fine) + 2 * r);
                swap_register(uint8_t(Reg::A_Coarse) + 2 * l, uint8_t(Reg::A_Coarse) + 2 * r);
                swap_register(uint8_t(Reg::A_Volume) + l, uint8_t(Reg::A_Volume) + r);

                if (m_output.status.exp_mode)
                {
                    // swap tone duty cycle and envelope period
                    swap_register(uint8_t(Reg::A_Duty) + l, uint8_t(Reg::A_Duty) + r);
                    swap_register(pgm_read_byte(e_fine + l), pgm_read_byte(e_fine + r));
                    swap_register(pgm_read_byte(e_coarse + l), pgm_read_byte(e_coarse + r));

                    // swap envelope shape
                    uint8_t shape_l = pgm_read_byte(e_shape + l);
                    uint8_t shape_r = pgm_read_byte(e_shape + r);
                    bool schanged_l = (m_output.status.changed & to_mask(shape_l));
                    bool schanged_r = (m_output.status.changed & to_mask(shape_r));
                    swap_register(shape_l, shape_r);
                    if (schanged_l) m_output.status.changed |= to_mask(shape_r);
                    else m_output.status.changed &= ~to_mask(shape_r);
                    if (schanged_r) m_output.status.changed |= to_mask(shape_l);
                    else m_output.status.changed &= ~to_mask(shape_l);
                }

                // swap bits in mixer register
                uint8_t msk_l = (0b00001001 << l);
                uint8_t msk_r = (0b00001001 << r);
                uint8_t mix_l = ((m_output.commons.mixer & msk_l) >> l);
                uint8_t mix_r = ((m_output.commons.mixer & msk_r) >> r);
                res_bits(m_output.commons.mixer, msk_l | msk_r);
                set_bits(m_output.commons.mixer, mix_l << r);
                set_bits(m_output.commons.mixer, mix_r << l);
            };

            // swap channels in pairs
            switch(m_dstereo)
            {
            case Stereo::ACB:
                swap_channels(1, 2); // B <-> C
                break;

            case Stereo::BAC:
                swap_channels(0, 1); // A <-> B
                break;

            case Stereo::BCA:
                swap_channels(0, 1); // A <-> B
                swap_channels(1, 2); // B <-> C
                break;

            case Stereo::CAB:
                swap_channels(1, 2); // B <-> C
                swap_channels(0, 1); // A <-> B
                break;

            case Stereo::CBA:
                swap_channels(0, 2); // A <-> C
                break;

            default: break;
            }
        }
    #endif
    }

    template <typename driver_t>
    void Advanced<driver_t>::process_compatible_mode_fix()
    {
    #ifndef DISABLE_COMPATIBLE_MODE_FIX
        if (this->GetChipId() == chipid_t::AY8930)
        {
            for (int i = 0; i < 3; ++i)
            {
                uint8_t volume = m_output.channels[i].t_volume;
                if (m_output.status.exp_mode) volume >>= 1;

                // get tone, noise and envelope enable flags
                bool t_disable = isb_set(m_output.commons.mixer, 0 + i);
                bool n_disable = isb_set(m_output.commons.mixer, 3 + i);
                bool e_enable  = isb_set(volume, 4);

                // special case - pure envelope
                if (e_enable && t_disable && n_disable)
                {
                    // fix by enabling inaudible tone
                    m_output.channels[i].t_period.full = 0;
                    m_output.channels[i].t_duty = 0x08;
                    res_bit(m_output.commons.mixer, 0 + i);
                }
            }
        }
    #endif    
    }

    template <typename driver_t>
    void Advanced<driver_t>::check_output_changes()
    {
        // apply changes in registers made by processing
        uint8_t i_data, o_data;
        for (uint8_t reg = BankA_Fst; reg <= BankB_Lst; ++reg)
        {
            get_register(m_input,  Reg(reg), i_data);
            get_register(m_output, Reg(reg), o_data);

            if (i_data != o_data)
                m_output.status.changed |= to_mask(reg);
        }
    }

    template <typename driver_t>
    void Advanced<driver_t>::write_output_to_chip()
    {
        // write changes in output state to the PSG
        uint8_t data; bool switch_banks = false;
        if (this->GetChipId() == chipid_t::AY8930 && m_output.status.exp_mode)
        {
            // check for changes in registers of bank B
            for (uint8_t reg = BankB_Fst; reg <= BankB_Lst; ++reg)
            {
                if (m_output.status.changed & to_mask(reg))
                {
                    // we have changes, so first
                    // of all we switch to bank B
                    if (!switch_banks)
                    {
                        switch_banks = true;
                        get_register(m_output, Reg::Mode_Bank, data);
                        data &= 0x0F; data |= 0xB0;
                        Simple<driver_t>::SetRegister(Mode_Bank, data);
                    }

                    // send register data to chip (within bank B)
                    get_register(m_output, Reg(reg), data);
                    Simple<driver_t>::SetRegister(reg & 0x0F, data);
                }
            }

            if (switch_banks)
            {
                // we wrote something to bank B,
                // so we switch back to bank A
                get_register(m_output, Reg::Mode_Bank, data);
                data &= 0x0F; data |= 0xA0;
                Simple<driver_t>::SetRegister(Mode_Bank, data);
            }
        }

        // check for changes in registers of bank A
        for (uint8_t reg = BankA_Fst; reg <= BankA_Lst; ++reg)
        {
            if (m_output.status.changed & to_mask(reg))
            {
                // skip the 'mode/bank' register if 
                // we've done a bank switch before
                if (switch_banks && reg == Mode_Bank) continue;

                // send register data to chip (within bank A)
                get_register(m_output, Reg(reg), data);
                Simple<driver_t>::SetRegister(reg & 0x0F, data);
            }
        }
    }
}
