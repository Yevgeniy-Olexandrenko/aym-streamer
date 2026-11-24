#include "uart_protocol.h"
#include "uart_driver.h"
#include <avr/io.h>

namespace UART
{
	// Timer1 counts at F_CPU / 1024 (64 us at 16 MHz, it wraps after 4.2 s).
	// When no frame comes for 100 ms (5 frames at 50 Hz), the chips are reset.
	static constexpr uint16_t FRAME_TIMEOUT = uint16_t(uint32_t(F_CPU) / 1024 / 10);

	void Protocol::Start()
	{
		// Timer1 in normal mode counts freely, no interrupts: the main loop
		// reads TCNT1, no ISR uses the 16-bit registers of Timer1
		TCCR1A = 0;
		TCCR1B = (1 << CS12 | 1 << CS10); // clock F_CPU / 1024
		TIMSK1 = 0;

		Driver::Open();
		m_errors = Driver::GetErrors();
	}

	void Protocol::Process(psg_t& psg)
	{
		uint8_t data;
		while (Driver::Read(data))
		{
			// a lost byte breaks the pairs: skip to the next frame
			uint8_t errors = Driver::GetErrors();
			if (errors != m_errors)
			{
				m_errors = errors;
				m_state = State::Marker;
			}

			if (m_state == State::Data)
			{
				psg.SetRegister(m_addr, data);
				m_state = State::Address;
			}
			else if (data == 0xFF)
			{
				// a new frame starts at the time of the host frame
				// timer: write the previous frame to the chips
				psg.Commit();
				psg.Select(0);
				m_state = State::Address;
				m_playing = true;
				m_frame_time = TCNT1;
			}
			else if (m_state == State::Address)
			{
				if (data == 0xFE)
				{
					psg.Select(1);
				}
				else if (data < 0x10)
				{
					m_addr = data;
					m_state = State::Data;
				}
				else m_state = State::Marker;
			}
		}

		// the host stopped: mute the chips, the last frame is
		// lost but the host sends all registers on a new start
		if (m_playing && uint16_t(TCNT1 - m_frame_time) >= FRAME_TIMEOUT)
		{
			psg.Reset();
			m_state = State::Marker;
			m_playing = false;
		}
	}
}
