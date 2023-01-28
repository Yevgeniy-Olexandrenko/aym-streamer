// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "psg-access.h"
#include "psg-wiring.h"

// processors
//#define PROCESS_CLOCK_CONVERSION
//#define PROCESS_CHANNELS_REMAPPING
#define PROCESS_COMPAT_MODE_FIX

// helpers
#define set_bits(reg, bits) reg |=  (bits)
#define res_bits(reg, bits) reg &= ~(bits)
#define set_bit(reg, bit) reg |=  (1 << (bit))
#define res_bit(reg, bit) reg &= ~(1 << (bit))
#define isb_set(reg, bit) bool((reg) & (1 << (bit)))

// -----------------------------------------------------------------------------
// Hardware Level Interface
// -----------------------------------------------------------------------------

namespace PSG
{
    enum // timing delays (nano seconds)
    {
        tAS = 400, // Address Setup Time
        tAH = 100, // Address Hold Time
        tDW = 500, // Write Data Pulse Width
        tDH = 100, // Write Data Hold Time
        tDA = 400, // Read Data Access Time
        tTS = 100, // Tristate Delay Time
        tRW = 500, // Reset Pulse Width
        tRB = 100  // Reset to Bus Control Delay Time
    };

    #define wait_for_delay(ns) _delay_us(0.001f * (ns))
    #define BUS_MASK (1 << BUS_BDIR | 1 << BUS_BC1)

// -----------------------------------------------------------------------------

    inline void set_data_bus(uint8_t data)
    {
        // set ports to output
        set_bits(LSB_DDR, LSB_MASK);
        set_bits(MSB_DDR, MSB_MASK);

        // set data bits to output ports
        LSB_PORT = (LSB_PORT & ~LSB_MASK) | (data & LSB_MASK);
        MSB_PORT = (MSB_PORT & ~MSB_MASK) | (data & MSB_MASK);
    }

    inline void get_data_bus(uint8_t& data)
    {
        // get bata bits from input ports
        data = (LSB_PIN & LSB_MASK) | (MSB_PIN & MSB_MASK);
    }

    inline void release_data_bus()
    {
        // setup ports to input
        res_bits(LSB_DDR, LSB_MASK);
        res_bits(MSB_DDR, MSB_MASK);

        // enable pull-up resistors
        set_bits(LSB_PORT, LSB_MASK);
        set_bits(MSB_PORT, MSB_MASK);
    }

    inline void set_ctrl_bus_addr()  { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BDIR | 1 << BUS_BC1; }
    inline void set_ctrl_bus_write() { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BDIR; }
    inline void set_ctrl_bus_read()  { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BC1;  }
    inline void set_ctrl_bus_inact() { BUS_PORT = (BUS_PORT & ~BUS_MASK); }

    // -----------------------------------------------------------------------------

    void Init()
    {
        static bool hw_init = false;
        if (!hw_init)
        {
            // init hardware only once
            hw_init = true;

            // setup hardware pins
            set_bit(BUS_DDR, BUS_BDIR); // output
            set_bit(BUS_DDR, BUS_BC1 ); // output
            set_bit(SIG_DDR, SIG_RES ); // output
            set_bit(SIG_DDR, SIG_CLK ); // output
            set_ctrl_bus_inact();
            release_data_bus();

            // setup default clock and reset
            SetClock(1777777); // devider is 0x09
            Reset();
        }
    }

    void Reset()
    {
        res_bit(SIG_PORT, SIG_RES);
        wait_for_delay(tRW);
        set_bit(SIG_PORT, SIG_RES);
        wait_for_delay(tRB);
    }

    void SetClock(uint32_t clock)
    {
        // configure Timer2 as a clock source
        uint8_t divider = (F_CPU / clock);
        TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
        TCCR2B = (1 << WGM22 ) | (1 << CS20 );
        OCR2A  = (divider - 1);
        OCR2B  = (divider / 2);
    }

    // 'Latch Address' sequence
    void Address(uint8_t reg)
    {
        set_ctrl_bus_addr();
        set_data_bus(reg);
        wait_for_delay(tAS);
        set_ctrl_bus_inact();
        wait_for_delay(tAH);
        release_data_bus();
    }

    // 'Write to PSG' sequence
    void Write(uint8_t data)
    {
        set_data_bus(data);
        set_ctrl_bus_write();
        wait_for_delay(tDW);
        set_ctrl_bus_inact();
        wait_for_delay(tDH);
        release_data_bus();
    }

    // 'Read from PSG' sequence
    void Read(uint8_t& data)
    {
        set_ctrl_bus_read();
        wait_for_delay(tDA);
        get_data_bus(data);
        set_ctrl_bus_inact();
        wait_for_delay(tTS);
    }
}

// -----------------------------------------------------------------------------
// High Level Interface
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------

void SoundChip::Init()
{
    // init hardware and detect chip type
    PSG::Init();
    detect_type();

    // set default clock and reset
    SetClock(F1_77MHZ);
    Reset();
}

void SoundChip::Reset()
{
    PSG::Reset();
    memset(&m_input, 0, sizeof(State));
}

SoundChip::Type SoundChip::GetType() const
{
    return Type(m_typehash);
}

void SoundChip::SetClock(Clock clock)
{
    if (clock >= F1_00MHZ && clock <= F2_00MHZ)
    {
        uint8_t divider = (F_CPU / clock);
        m_rclock = (F_CPU / divider);
        m_vclock = clock;
        PSG::SetClock(m_rclock);
    }
}

SoundChip::Clock SoundChip::GetClock() const
{
    return m_vclock;
}

void SoundChip::SetStereo(Stereo stereo)
{
    m_sstereo = stereo;
    m_dstereo = stereo;
}

SoundChip::Stereo SoundChip::GetStereo() const
{
    return m_dstereo;
}

// set register data indirectly via bank switching
void SoundChip::SetRegister(uint8_t reg, uint8_t data)
{
    // register number must be in range 0x00-0x0F
    if (reg < 0x10)
    {
        // redirect access to registers of bank B if
        // exp mode is active and bank B is selected
        if (m_input.status.exp_mode == 0xB0)
        {
            reg += BankB_Fst;
        }
        set_register(m_input, Reg(reg), data);
    }
}

// get register data indirectly via bank switching
void SoundChip::GetRegister(uint8_t reg, uint8_t& data) const
{
    // register number must be in range 0x00-0x0F
    if (reg < 0x10)
    {
        // redirect access to registers of bank B if
        // exp mode is active and bank B is selected
        if (m_input.status.exp_mode == 0xB0)
        {
            reg += BankB_Fst;
        }
        get_register(m_input, Reg(reg), data);
    }
}

// set register data directly
void SoundChip::SetRegister(Reg reg, uint8_t  data)
{
    set_register(m_input, reg, data);
}

// get register data directly
void SoundChip::GetRegister(Reg reg, uint8_t& data) const
{
    get_register(m_input, reg, data);
}

// process state and write to PSG
void SoundChip::Update()
{
    if (m_input.status.changed)
    {
        m_output = m_input;
        process_clock_conversion();
        process_channels_remapping();
        process_compat_mode_fix();
        update_output_changes();
        write_output_state();
        m_input.status.changed = 0;
    }
}

// -----------------------------------------------------------------------------

void SoundChip::detect_type()
{
    m_typehash = 0x00000007;
    do_test_wr_rd_regs(0x00);
    do_test_wr_rd_regs(0x10);
    do_test_wr_rd_latch(0x00);
    do_test_wr_rd_latch(0x10);
    do_test_wr_rd_exp_mode(0xA0);
    do_test_wr_rd_exp_mode(0xB0);
}

void SoundChip::update_hash(uint8_t data)
{
    m_typehash = (31 * m_typehash + uint32_t(data)); 
}

void SoundChip::do_test_wr_rd_regs(uint8_t offset)
{
    for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
    {
        data = 0xFF;
        if (Reg(reg) == Reg::Mixer) data = 0x3F;
        if (Reg(reg) == Reg::Mode_Bank) data = 0x0F;
        PSG::Address(reg);
        PSG::Write(data);
    }
    for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
    {
        PSG::Address(reg);
        PSG::Read(data);
        update_hash(data);
    }
}

void SoundChip::do_test_wr_rd_latch(uint8_t offset)
{
    for (uint8_t data, reg = offset; reg < (offset + 16); ++reg)
    {
        data = 0xFF;
        if (Reg(reg) == Reg::Mixer) data = 0x3F;
        if (Reg(reg) == Reg::Mode_Bank) data = 0x0F;
        PSG::Address(reg); 
        PSG::Write(data);
        PSG::Read(data);
        update_hash(data);
    }
}

void SoundChip::do_test_wr_rd_exp_mode(uint8_t mode_bank)
{
    PSG::Address(Mode_Bank);
    PSG::Write(mode_bank | 0x0F);
    for (uint8_t reg = 0; reg < 16 - 2; ++reg)
    {
        if (reg == Mode_Bank) continue;
        PSG::Address(reg);
        PSG::Write(0xFF);
    }
    for (uint8_t data, reg = 0; reg < 16 - 2; ++reg)
    {
        PSG::Address(reg);
        PSG::Read(data);
        update_hash(data);
    }
}

void SoundChip::set_register(State& state, Reg reg, uint8_t data)
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

void SoundChip::get_register(const State& state, Reg reg, uint8_t& data) const
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

static const uint8_t e_fine[] PROGMEM = {
    uint8_t(SoundChip::Reg::EA_Fine),
    uint8_t(SoundChip::Reg::EB_Fine),
    uint8_t(SoundChip::Reg::EC_Fine)};

static const uint8_t e_coarse[] PROGMEM = {
    uint8_t(SoundChip::Reg::EA_Coarse),
    uint8_t(SoundChip::Reg::EB_Coarse),
    uint8_t(SoundChip::Reg::EC_Coarse)};

static const uint8_t e_shape[] PROGMEM = {
    uint8_t(SoundChip::Reg::EA_Shape),
    uint8_t(SoundChip::Reg::EB_Shape),
    uint8_t(SoundChip::Reg::EC_Shape)};

void SoundChip::process_clock_conversion()
{
#if defined(PROCESS_CLOCK_CONVERSION)
    if (m_rclock != m_vclock)
    {
        uint16_t t_bound = (m_output.status.exp_mode ? 0xFFFF : 0x0FFF);
        uint16_t n_bound = (m_output.status.exp_mode ? 0x00FF : 0x001F);

        // safe period conversion based on clock ratio
        const auto convert_period = [&](uint16_t& period, uint16_t bound)
        {
            uint32_t converted = ((period * (m_rclock >> 8) / (m_vclock >> 9) + 1) >> 1);
            period = uint16_t(converted > bound ? bound : converted);
        };

        // convert tone and envelope periods
        for (int i = 0; i < 3; ++i)
        {
            convert_period(m_output.channels[i].t_period.full, t_bound);
            convert_period(m_output.channels[i].e_period.full, 0xFFFF);
        }

        // convert noise period
        uint16_t period = m_output.commons.n_period;
        convert_period(period, n_bound);
        m_output.commons.n_period = uint8_t(period);
    }
#endif
}

void SoundChip::process_channels_remapping()
{
#if defined(PROCESS_CHANNELS_REMAPPING)
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
        }
    }
#endif
}

void SoundChip::process_compat_mode_fix()
{
#if defined(PROCESS_COMPAT_MODE_FIX)
    if (GetType() == Type::AY8930)
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

// apply changes in registers made by processing
void SoundChip::update_output_changes()
{
    uint8_t i_data, o_data;
    for (uint8_t reg = BankA_Fst; reg <= BankB_Lst; ++reg)
    {
        get_register(m_input,  Reg(reg), i_data);
        get_register(m_output, Reg(reg), o_data);

        if (i_data != o_data)
            m_output.status.changed |= to_mask(reg);
    }
}

// write changes in output state to the PSG
void SoundChip::write_output_state()
{
    uint8_t data;
    bool switch_banks = false;

    if (GetType() == Type::AY8930 && m_output.status.exp_mode)
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
                    PSG::Address(Mode_Bank);
                    PSG::Write(data);
                }

                // send register data to chip (within bank B)
                get_register(m_output, Reg(reg), data);
                PSG::Address(reg & 0x0F);
                PSG::Write(data);
            }
        }

        if (switch_banks)
        {
            // we wrote something to bank B,
            // so we switch back to bank A
            get_register(m_output, Reg::Mode_Bank, data);
            data &= 0x0F; data |= 0xA0;
            PSG::Address(Mode_Bank);
            PSG::Write(data);
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
            PSG::Address(reg & 0x0F);
            PSG::Write(data);
        }
    }
}
