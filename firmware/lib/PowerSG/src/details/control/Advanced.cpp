#include <string.h>
#include <avr/pgmspace.h>

#include "Advanced.h"
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

    const raddr_t e_fine[] PROGMEM = 
    {
        raddr_t(Reg::EA_Fine),
        raddr_t(Reg::EB_Fine),
        raddr_t(Reg::EC_Fine)
    };

    const raddr_t e_coarse[] PROGMEM = 
    {
        raddr_t(Reg::EA_Coarse), 
        raddr_t(Reg::EB_Coarse), 
        raddr_t(Reg::EC_Coarse)
    };

    const raddr_t e_shape[] PROGMEM = 
    {
        raddr_t(Reg::EA_Shape), 
        raddr_t(Reg::EB_Shape), 
        raddr_t(Reg::EC_Shape)
    };

    Advanced::Advanced(Driver &driver)
        : Simple(driver)
        , m_clock(0)
        , m_sstereo(Stereo::ABC)
        , m_dstereo(Stereo::ABC)
    {}

    void Advanced::reset()
    {
        Simple::reset();
        memset(&m_input, 0, sizeof(state_t));
    }

    void Advanced::setClock(Clock clock)
    {
        // limit the clock frequency range
        if (clock < F1_00MHZ) clock = F1_00MHZ;
        if (clock > F2_00MHZ) clock = F2_00MHZ;

        // set real/virtual clock frequency
        Simple::setClock(clock);
        m_clock = clock;
    }

    Clock Advanced::getClock() const
    {
        return m_clock;
    }

    void Advanced::setStereo(Stereo stereo)
    {
        m_sstereo = stereo;
        m_dstereo = stereo;
    }

    Stereo Advanced::getStereo() const
    {
        return m_dstereo;
    }

    void Advanced::setRegister(raddr_t addr, rdata_t data)
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

    void Advanced::getRegister(raddr_t addr, rdata_t &data) const
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

    void Advanced::setRegister(Reg reg, rdata_t data)
    {
        // set register data directly
        set_register(m_input, reg, data);
    }

    void Advanced::getRegister(Reg reg, rdata_t &data) const
    {
        // get register data directly
        get_register(m_input, reg, data);
    }

    void Advanced::update()
    {
        if (m_input.status.changed)
        {
            // copy input state to output and reset changes
            memcpy(&m_output, &m_input, sizeof(state_t));
            m_input.status.changed = 0;

            // process outout state and write to PSG
            process_clock_conversion();
            process_channels_remapping();
            process_compatible_mode_fix();
            
            check_output_changes();
            write_output_to_chip();
        }
    }

    void Advanced::set_register(state_t &state, Reg reg, rdata_t data)
    {
        // preserve the state of exp mode and bank of regs
        // separately from the shape of channel A envelope
        if ((raddr_t(reg) & 0x0F) == Mode_Bank)
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

    void Advanced::get_register(const state_t &state, Reg reg, rdata_t &data) const
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
        if ((raddr_t(reg) & 0x0F) == Mode_Bank)
        {
            data |= state.status.exp_mode;
        }
    }

    void Advanced::process_clock_conversion()
    {
    #ifndef DISABLE_CLOCK_CONVERSION
        const Clock vclock = getClock();
        const Clock rclock = Simple::getClock();

        if (rclock != vclock)
        {
            rpair_t t_bound = (m_output.status.exp_mode ? 0xFFFF : 0x0FFF);
            rpair_t n_bound = (m_output.status.exp_mode ? 0x00FF : 0x001F);

            // safe period conversion based on clock ratio
            const auto convert_period = [&](rpair_t& period, rpair_t bound)
            {
                uint32_t converted = ((period * (rclock >> 5) / (vclock >> 6) + 1) >> 1);
                period = rpair_t(converted > bound ? bound : converted);
            };

            // convert tone and envelope periods
            for (int i = 0; i < 3; ++i)
            {
                convert_period(m_output.channels[i].t_period.full, t_bound);
                convert_period(m_output.channels[i].e_period.full, 0xFFFF);
            }
            m_output.status.changed |= 0b01100001111111; // hack

            // convert noise period
            rpair_t period = m_output.commons.n_period;
            convert_period(period, n_bound);
            m_output.commons.n_period = rdata_t(period);
        }
    #endif
    }

    void Advanced::process_channels_remapping()
    {
    #ifndef DISABLE_CHANNELS_REMAPPING
        // restrict stereo modes available for exp mode
        m_dstereo = (m_output.status.exp_mode && m_sstereo != Stereo::ABC && m_sstereo != Stereo::ACB)
            ? Stereo::ABC
            : m_sstereo;

        if (m_dstereo != Stereo::ABC)
        {
            const auto swap_register = [&](raddr_t reg_l, raddr_t reg_r)
            {
                rdata_t data_l, data_r;
                get_register(m_output, Reg(reg_l), data_l);
                get_register(m_output, Reg(reg_r), data_r);
                set_register(m_output, Reg(reg_l), data_r);
                set_register(m_output, Reg(reg_r), data_l);
            };

            const auto swap_channels = [&](raddr_t l, raddr_t r)
            {
                // swap tone period and tone volume/envelope enable
                swap_register(raddr_t(Reg::A_Fine) + 2 * l, raddr_t(Reg::A_Fine) + 2 * r);
                swap_register(raddr_t(Reg::A_Coarse) + 2 * l, raddr_t(Reg::A_Coarse) + 2 * r);
                swap_register(raddr_t(Reg::A_Volume) + l, raddr_t(Reg::A_Volume) + r);

                if (m_output.status.exp_mode)
                {
                    // swap tone duty cycle and envelope period
                    swap_register(raddr_t(Reg::A_Duty) + l, raddr_t(Reg::A_Duty) + r);
                    swap_register(pgm_read_byte(e_fine + l), pgm_read_byte(e_fine + r));
                    swap_register(pgm_read_byte(e_coarse + l), pgm_read_byte(e_coarse + r));

                    // swap envelope shape
                    raddr_t shape_l = pgm_read_byte(e_shape + l);
                    raddr_t shape_r = pgm_read_byte(e_shape + r);
                    bool schanged_l = (m_output.status.changed & to_mask(shape_l));
                    bool schanged_r = (m_output.status.changed & to_mask(shape_r));
                    swap_register(shape_l, shape_r);
                    if (schanged_l) m_output.status.changed |= to_mask(shape_r);
                    else m_output.status.changed &= ~to_mask(shape_r);
                    if (schanged_r) m_output.status.changed |= to_mask(shape_l);
                    else m_output.status.changed &= ~to_mask(shape_l);
                }

                // swap bits in mixer register
                rdata_t msk_l = (0b00001001 << l);
                rdata_t msk_r = (0b00001001 << r);
                rdata_t mix_l = ((m_output.commons.mixer & msk_l) >> l);
                rdata_t mix_r = ((m_output.commons.mixer & msk_r) >> r);
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

    void Advanced::process_compatible_mode_fix()
    {
    #ifndef DISABLE_COMPATIBLE_MODE_FIX
        if (getChipId() == ChipId::AY8930)
        {
            for (int i = 0; i < 3; ++i)
            {
                rdata_t volume = m_output.channels[i].t_volume;
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

    void Advanced::check_output_changes()
    {
        // apply changes in registers made by processing
        rdata_t i_data, o_data;
        for (raddr_t addr = BankA_Fst; addr <= BankB_Lst; ++addr)
        {
            get_register(m_input,  Reg(addr), i_data);
            get_register(m_output, Reg(addr), o_data);

            if (i_data != o_data)
                m_output.status.changed |= to_mask(addr);
        }
    }

    void Advanced::write_output_to_chip()
    {
        // write changes in output state to the PSG
        rdata_t data; bool switch_banks = false;
        if (this->getChipId() == ChipId::AY8930 && m_output.status.exp_mode)
        {
            // check for changes in registers of bank B
            for (raddr_t addr = BankB_Fst; addr <= BankB_Lst; ++addr)
            {
                if (m_output.status.changed & to_mask(addr))
                {
                    // we have changes, so first
                    // of all we switch to bank B
                    if (!switch_banks)
                    {
                        switch_banks = true;
                        get_register(m_output, Reg::Mode_Bank, data);
                        data &= 0x0F; data |= 0xB0;
                        Simple::setRegister(Mode_Bank, data);
                    }

                    // send register data to chip (within bank B)
                    get_register(m_output, Reg(addr), data);
                    Simple::setRegister(addr & 0x0F, data);
                }
            }

            if (switch_banks)
            {
                // we wrote something to bank B,
                // so we switch back to bank A
                get_register(m_output, Reg::Mode_Bank, data);
                data &= 0x0F; data |= 0xA0;
                Simple::setRegister(Mode_Bank, data);
            }
        }

        // check for changes in registers of bank A
        for (raddr_t addr = BankA_Fst; addr <= BankA_Lst; ++addr)
        {
            if (m_output.status.changed & to_mask(addr))
            {
                // skip the 'mode/bank' register if 
                // we've done a bank switch before
                if (switch_banks && addr == Mode_Bank) continue;

                // send register data to chip (within bank A)
                get_register(m_output, Reg(addr), data);
                Simple::setRegister(addr & 0x0F, data);
            }
        }
    }
}
