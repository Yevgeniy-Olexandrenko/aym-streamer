#include "drivers/DriverEnable.h"
#if defined(USE_M328_PDRIVER)

#include "m328_driver.h"
#include "m328_wiring.h"

#include <util/delay.h>
#include "drivers/DriverHelper.h"

#define wait_for_delay(ns) _delay_us(0.001f * (ns))
#define BUS_MASK (1 << BUS_BDIR | 1 << BUS_BC1)

namespace PowerSG
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

    void m328_driver::chip_power_on()
    {
        // setup hardware pins
        set_bit(BUS_DDR, BUS_BDIR); // output
        set_bit(BUS_DDR, BUS_BC1 ); // output
        set_bit(SIG_DDR, SIG_RES ); // output
        set_bit(SIG_DDR, SIG_CLK ); // output
        set_ctrl_bus_inact();
        release_data_bus();
    }

    void m328_driver::chip_set_clock(uint32_t clock)
    {
        // choosing a divider for the closest clock frequency
        m_clockDiv = uint8_t(F_CPU / clock);
        uint32_t minClock = (F_CPU / (m_clockDiv + 1));
        uint32_t maxClock = (F_CPU / (m_clockDiv + 0));
        if (clock - minClock < maxClock - clock) m_clockDiv++;

        // configure Timer2 as a clock source for PSG
        TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
        TCCR2B = (1 << WGM22 ) | (1 << CS20 );
        OCR2A  = (m_clockDiv - 1);
        OCR2B  = (m_clockDiv / 2);
    }

    void m328_driver::chip_get_clock(uint32_t &clock)
    {
        // real PSG clock based on Timer2 divider
        if (m_clockDiv != 0)
        {
            clock = (F_CPU / m_clockDiv);
        }
    }

    void m328_driver::chip_reset()
    {
        // 'Reset PSG' sequence
        res_bit(SIG_PORT, SIG_RES);
        wait_for_delay(tRW);
        set_bit(SIG_PORT, SIG_RES);
        wait_for_delay(tRB);
    }

    void m328_driver::chip_address(uint8_t addr)
    {
        // 'Latch Address' sequence
        set_ctrl_bus_addr();
        set_data_bus(addr);
        wait_for_delay(tAS);
        set_ctrl_bus_inact();
        wait_for_delay(tAH);
        release_data_bus();
    }

    void m328_driver::chip_write(uint8_t data)
    {
        // 'Write to PSG' sequence
        set_data_bus(data);
        set_ctrl_bus_write();
        wait_for_delay(tDW);
        set_ctrl_bus_inact();
        wait_for_delay(tDH);
        release_data_bus();
    }

    void m328_driver::chip_read(uint8_t &data)
    {
        // 'Read from PSG' sequence
        set_ctrl_bus_read();
        wait_for_delay(tDA);
        get_data_bus(data);
        set_ctrl_bus_inact();
        wait_for_delay(tTS);
    }
}
#endif
