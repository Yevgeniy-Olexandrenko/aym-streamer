#include "uart_protocol.h"
#include "uart_driver.h"
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
		case PSG::ChipId::NotFound:   name_P = PSTR("NotFound");   break;
		case PSG::ChipId::Compatible: name_P = PSTR("Compatible"); break;
		case PSG::ChipId::AY8910:     name_P = PSTR("AY8910");     break;
		case PSG::ChipId::AY8930:     name_P = PSTR("AY8930");     break;
		case PSG::ChipId::YM2149F:    name_P = PSTR("YM2149");     break;
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
			// a lost byte breaks the pairs: skip to the next frame
			uint8_t errors = Driver::GetErrors();
			if (errors != m_errors)
			{
				m_errors = errors;
				m_state = State::Marker;
			}

			Event event = parse_byte(data);
			if (event != Event::None) return event;
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
		bool printable = (data >= 0x20 && data < 0x7F);

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

		if (m_state == State::Text)
		{
			if (data == '\r' || data == '\n')
			{
				m_state = State::Marker;
				return parse_text();
			}
			else if (printable)
			{
				// a too long text is not a command
				if (m_length < TEXT_SIZE) m_text[m_length++] = data;
				else m_length = 0xFF;
			}
			else m_state = State::Marker;
		}
		else if (printable)
		{
			// a printable char is never an address
			m_text[0] = data;
			m_length = 1;
			m_state = State::Text;
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
}
