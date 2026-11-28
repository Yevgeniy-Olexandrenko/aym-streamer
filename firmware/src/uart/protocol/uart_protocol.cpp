#include "uart_protocol.h"
#include "../driver/uart_driver.h"
#include <avr/io.h>
#include <avr/pgmspace.h>

namespace UART
{
	// Timer1 counts at F_CPU / 1024 (64 us at 16 MHz, it wraps after 4.2 s).
	// When no frame comes for 100 ms (5 frames at 50 Hz), the chips are reset.
	static constexpr uint16_t FRAME_TIMEOUT = uint16_t(uint32_t(F_CPU) / 1024 / 10);

	// the channels of each PSG::Stereo value, 3 chars each
	static const char s_stereo[] PROGMEM = "ABCACBBACBCACABCBA";

	void WriteText(const char* text_P)
	{
		while (char c = pgm_read_byte(text_P++)) Driver::Write(c);
	}

	void WriteHexU32(uint32_t value)
	{
		for (uint8_t i = 8; i-- > 0;)
		{
			uint8_t digit = uint8_t(value >> (4 * i)) & 0x0F;
			Driver::Write(digit < 10 ? '0' + digit : 'A' - 10 + digit);
		}
	}

	void WriteConfig(const psg_config_t& config)
	{
		const char* channels_P = s_stereo + 3 * uint8_t(config.stereo);
		for (uint8_t i = 0; i < 3; ++i) Driver::Write(pgm_read_byte(channels_P + i));

		// the clock is 7 digits, from 1000000 to 2000000 Hz
		char digits[7];
		uint32_t value = uint32_t(config.clock);
		for (uint8_t i = 7; i-- > 0; value /= 10) digits[i] = char('0' + value % 10);
		for (uint8_t i = 0; i < 7; ++i) Driver::Write(digits[i]);
	}

	void WriteChipName(PSG::ChipId chipid)
	{
		const char* name_P = PSTR("Unknown");
		switch (chipid)
		{
		case PSG::ChipId::AY8910:  name_P = PSTR("AY8910" ); break;
		case PSG::ChipId::AY8930:  name_P = PSTR("AY8930" ); break;
		case PSG::ChipId::YM2149:  name_P = PSTR("YM2149" ); break;
		case PSG::ChipId::VARIANT: name_P = PSTR("VARIANT"); break;
		case PSG::ChipId::NONE:    name_P = PSTR("NONE"   ); break;
		}
		WriteText(name_P);
	}

	void WriteLineEnd()
	{
		Driver::Write('\r');
		Driver::Write('\n');
	}

	void Parser::Start()
	{
		// Timer1 in normal mode counts freely, no interrupts: the main loop
		// reads TCNT1, no ISR uses the 16-bit registers of Timer1
		TCCR1A = 0;
		TCCR1B = (1 << CS12 | 1 << CS10); // clock F_CPU / 1024
		TIMSK1 = 0;

		Driver::Open();
		m_errors = Driver::GetErrors();
	}

	Parser::Event Parser::Next()
	{
		uint8_t data;
		while (Driver::Read(data))
		{
			// a lost byte breaks the pairs: skip to the next frame,
			// the player has no sync byte and goes on
			uint8_t errors = Driver::GetErrors();
			if (errors != m_errors)
			{
				m_errors = errors;
				if (!m_player) m_state = State::Marker;
			}

			m_waiting = false;
			m_muted = false;
			Event event = parse_byte(data);
			if (event != Event::None) return event;
		}

		if (m_player)
		{
			// the player waits for a byte: no byte for 100 ms
			// is a pause or the end of the file, mute once
			uint16_t time = TCNT1;
			if (!m_waiting)
			{
				m_waiting = true;
				m_wait_time = time;
			}
			else if (!m_muted && uint16_t(time - m_wait_time) >= FRAME_TIMEOUT)
			{
				m_muted = true;
				return Event::PlayMute;
			}
			return Event::None;
		}

		if (m_playing && uint16_t(TCNT1 - m_frame_time) >= FRAME_TIMEOUT)
		{
			m_state = State::Marker;
			m_playing = false;
			return Event::Timeout;
		}
		return Event::None;
	}

	Parser::Event Parser::parse_byte(uint8_t data)
	{
		if (m_state == State::Text) return parse_text_byte(data);
		if (m_state >= State::Header) return parse_play(data);

		if (m_state == State::Data)
		{
			m_data = data;
			m_state = State::Address;
			return Event::Register;
		}

		if (data == 0xFF)
		{
			m_state = State::Address;
			m_playing = true;
			m_frame_time = TCNT1;
			return Event::Frame;
		}

		if (is_printable(data))
		{
			// a printable char is never an address
			start_text(data);
		}
		else if (m_state == State::Address)
		{
			if (data == 0xFE) return Event::Chip1;
			if (data < 0x10)
			{
				m_addr = data;
				m_state = State::Data;
			}
			else m_state = State::Marker;
		}
		return Event::None;
	}

	void Parser::start_text(uint8_t data)
	{
		m_text[0] = data;
		m_length = 1;
		m_state = State::Text;
	}

	Parser::Event Parser::parse_text_byte(uint8_t data)
	{
		// the stream or the player goes on after the text
		const State next = (m_player ? State::PlayAddr : State::Marker);
		if (data == '\r' || data == '\n')
		{
			m_state = next;
			Event event = parse_text();

			// the config in the player comes from a host:
			// back to the stream of the host
			if (event == Event::Config && m_player) leave_play();
			return event;
		}
		if (is_printable(data))
		{
			// a too long text is not a command
			if (m_length < TEXT_SIZE) m_text[m_length++] = data;
			else m_length = 0xFF;
			return Event::None;
		}
		if (data == 0x1A && m_length == 3 &&
			m_text[0] == 'P' && m_text[1] == 'S' && m_text[2] == 'G')
		{
			// the signature of a PSG file, the header goes on
			m_state = State::Header;
			m_length = 4;
			return Event::None;
		}

		// another byte ends the text, parse it again
		m_state = next;
		return parse_byte(data);
	}

	Parser::Event Parser::parse_text()
	{
		// 0xFF: the text was too long
		if (m_length > TEXT_SIZE) return Event::None;
		if (parse_config()) return Event::Config;

		m_text[m_length] = '\0';
		return Event::Text;
	}

	bool Parser::parse_config()
	{
		// 'ABC1773400': the order of the channels, the clock in Hz
		if (m_length != 10) return false;

		uint8_t a = m_text[0] - 'A';
		uint8_t b = m_text[1] - 'A';
		uint8_t c = m_text[2] - 'A';
		if (a > 2 || b > 2 || c > 2 || a == b || a == c || b == c) return false;

		uint32_t clock = 0;
		for (uint8_t i = 3; i < 10; ++i)
		{
			uint8_t digit = m_text[i] - '0';
			if (digit > 9) return false;
			clock = clock * 10 + digit;
		}

		// ABC, ACB, BAC, BCA, CAB, CBA: two values for each first channel
		m_config.stereo = PSG::Stereo(2 * a + (b > c));
		m_config.clock = PSG::Clock(clock);
		return true;
	}

	Parser::Event Parser::parse_play(uint8_t data)
	{
		switch (m_state)
		{
		case State::Header:
			// byte 4 is the version, byte 5 the frame rate (version 10
			// and later). A version 0xFF is a header of 4 bytes, then
			// this 0xFF opens frame 1: the player starts at it.
			if (m_length == 4)
			{
				if (data == 0xFF)
				{
					m_rate = 50;
					return start_play();
				}
				m_data = data;
			}
			else if (m_length == 5)
			{
				m_rate = (m_data >= 10 && data ? data : 50);
			}
			if (++m_length == 16) return start_play();
			break;

		case State::PlayAddr:
			// a printable char is never an address of a PSG file:
			// a command, or the signature of the next file
			if (is_printable(data)) start_text(data);
			else if (data == 0xFF)
			{
				m_frames = 1;
				return Event::PlayFrame;
			}
			else if (data == 0xFE) m_state = State::PlaySkip;
			else if (data == 0xFD) return Event::PlayMute;
			else
			{
				m_addr = data;
				m_state = State::PlayData;
			}
			break;

		case State::PlayData:
			// 0x00-0x0F chip 0, 0x80-0x8F chip 1, the other
			// registers are of other devices (MSX): skip them
			m_state = State::PlayAddr;
			if (m_addr & 0x70) break;
			m_chip = (m_addr >> 7);
			m_addr &= 0x0F;
			m_data = data;
			return Event::Register;

		default: // State::PlaySkip
			// 0xFE 0 opens no frame, the frame goes on
			m_state = State::PlayAddr;
			if (data)
			{
				m_frames = uint16_t(data) * 4;
				return Event::PlayFrame;
			}
			break;
		}
		return Event::None;
	}

	Parser::Event Parser::start_play()
	{
		// the frame timeout of the stream does not work in the player
		m_state = State::PlayAddr;
		m_player = true;
		m_playing = false;
		m_waiting = false;
		m_muted = false;
		Driver::SetFlowControl(true);
		return Event::Play;
	}

	void Parser::leave_play()
	{
		m_state = State::Marker;
		m_player = false;
		Driver::SetFlowControl(false);
	}

	// Timer1 counts in one second
	static constexpr uint16_t TIMER_RATE = uint16_t(uint32_t(F_CPU) / 1024);

	void FrameTimer::Start(uint8_t rate, uint8_t delay)
	{
		m_rate = rate;
		m_period = TIMER_RATE / rate;
		m_rest = uint8_t(TIMER_RATE % rate);
		m_sum = 0;
		m_tick = TCNT1;
		while (delay--) Next();
	}

	bool FrameTimer::IsDue() const
	{
		return int16_t(TCNT1 - m_tick) >= 0;
	}

	void FrameTimer::Next()
	{
		// the fractions sum up to one more count now and then
		m_tick += m_period;
		if (m_sum >= m_rate - m_rest)
		{
			m_sum -= (m_rate - m_rest);
			m_tick++;
		}
		else m_sum += m_rest;
	}

	void FrameTimer::Restart()
	{
		m_tick = TCNT1;
		Next();
	}
}
