#pragma once

#include "../psg/mode/psg_turbosound.h"

namespace UART
{
	// The stream of the host: each frame is one packet, 0xFF (chip 0)
	// and for TurboSound 0xFE (chip 1), each followed by the 'addr, data'
	// pairs of the changed registers. Addresses are 0x00-0x0F, so 0xFF
	// and 0xFE are markers only where an address is expected.
	//
	// A printable char where an address is expected starts a text command,
	// '\r' or '\n' ends it. The command 'ABC1773400' sets the stereo and
	// the clock, the reply is 4 lines: the streamer type, the stereo and
	// the clock it uses, the type of chip 0 and of chip 1. Other commands
	// go to the text handler of the streamer.
	class Parser
	{
	public:
		enum class Event : uint8_t
		{
			None,     // no more received bytes
			Frame,    // 0xFF: a new frame, chip 0
			Chip1,    // 0xFE: chip 1
			Register, // a register: GetAddr, GetData
			Config,   // the config command: GetStereo, GetClock
			Text,     // other text command: GetText
			Timeout   // no frame for 100 ms
		};

		// open the UART and start the frame timer
		void Start();

		// take the next event from the received bytes
		Event Next();

		uint8_t     GetAddr()   const { return m_addr; }
		uint8_t     GetData()   const { return m_data; }
		const char* GetText()   const { return m_text; }

		// the last config, also set by the streamer
		PSG::Stereo GetStereo() const { return m_stereo; }
		PSG::Clock  GetClock()  const { return m_clock; }
		void SetConfig(PSG::Stereo stereo, PSG::Clock clock);

		// send the reply to the config command
		static void SendInfo(bool advanced, PSG::Stereo stereo,
			PSG::Clock clock, PSG::ChipId chip0, PSG::ChipId chip1);

	private:
		Event parse_byte(uint8_t data);
		Event parse_text();
		bool  parse_config();

	private:
		enum class State : uint8_t
		{
			Marker,  // wait for 0xFF after a start, an error or a timeout
			Address, // wait for a marker, an address or a text
			Data,    // wait for the data of the address
			Text     // wait for the end of the text
		};

		static constexpr uint8_t TEXT_SIZE = 16;

		State       m_state{ State::Marker };
		uint8_t     m_addr{ 0 };
		uint8_t     m_data{ 0 };
		uint8_t     m_errors{ 0 };      // last error count of the driver
		bool        m_playing{ false }; // frames come, the chips sound
		uint16_t    m_frame_time{ 0 };  // Timer1 count at the last frame
		uint8_t     m_length{ 0 };      // 0xFF when the text is too long
		char        m_text[TEXT_SIZE + 1];
		PSG::Stereo m_stereo{ PSG::Stereo::ABC };
		PSG::Clock  m_clock{ PSG::Clock::F_DEF };
	};

	// text output for the replies of the streamer
	void WriteText(const char* text_P);
	void WriteHex(uint32_t value);
	void WriteConfig(PSG::Stereo stereo, PSG::Clock clock); // 'ABC1773400'
	void WriteLineEnd();
	const char* ChipName(PSG::ChipId chip); // in PROGMEM

	// The parts of the protocol that differ between the modes. Basic does
	// not process the stream: it takes only the clock of the config, and
	// the reply gives the real clock and stereo ABC.
	inline bool IsAdvanced(PSG::TurboSound<PSG::Basic>&) { return false; }
	inline bool IsAdvanced(PSG::TurboSound<PSG::Advanced>&) { return true; }

	inline void Commit(PSG::TurboSound<PSG::Basic>&) {}
	inline void Commit(PSG::TurboSound<PSG::Advanced>& psg) { psg.Commit(); }

	inline void Configure(PSG::TurboSound<PSG::Basic>& psg, PSG::Stereo, PSG::Clock clock)
	{
		psg.SetClock(clock);
	}

	inline void Configure(PSG::TurboSound<PSG::Advanced>& psg, PSG::Stereo stereo, PSG::Clock clock)
	{
		psg.SetClock(clock);
		psg.SetStereo(stereo);
	}

	// The host stream to the chips of Basic or of Advanced mode.
	template<class PSGMode>
	class Protocol
	{
	public:
		using psg_t = PSG::TurboSound<PSGMode>;
		using TextHandler = void (*)(const char* text);

		// the handler gets the text commands that are not the config
		void Start(TextHandler handler = nullptr)
		{
			m_handler = handler;
			m_parser.Start();
		}

		// handle all received bytes and the frame timeout
		void Process(psg_t& psg)
		{
			using Event = Parser::Event;
			for (;;)
			{
				switch (m_parser.Next())
				{
				case Event::None:
					return;

				case Event::Frame:
					// a new frame starts at the time of the host frame
					// timer: write the previous frame to the chips
					UART::Commit(psg);
					psg.Select(0);
					break;

				case Event::Chip1:
					psg.Select(1);
					break;

				case Event::Register:
					psg.SetRegister(m_parser.GetAddr(), m_parser.GetData());
					break;

				case Event::Config:
					UART::Configure(psg, m_parser.GetStereo(), m_parser.GetClock());
					SendInfo(psg);
					break;

				case Event::Text:
					if (m_handler) m_handler(m_parser.GetText());
					break;

				case Event::Timeout:
					// the host stopped: mute the chips, the last frame is
					// lost but the host sends all registers on a new start
					psg.Reset();
					break;
				}
			}
		}

		// apply a config as the config command does, without a reply
		void Configure(psg_t& psg, PSG::Stereo stereo, PSG::Clock clock)
		{
			m_parser.SetConfig(stereo, clock);
			UART::Configure(psg, stereo, clock);
		}

		// The config the streamer uses, as the reply gives it: Advanced
		// clamps the clock to 1-2 MHz, Basic plays ABC on the real clock.
		void GetConfig(psg_t& psg, PSG::Stereo& stereo, PSG::Clock& clock)
		{
			stereo = (IsAdvanced(psg) ? m_parser.GetStereo() : PSG::Stereo::ABC);
			clock = psg.GetClock();
		}

		// send the reply to the config command
		void SendInfo(psg_t& psg)
		{
			PSG::Stereo stereo; PSG::Clock clock;
			GetConfig(psg, stereo, clock);

			PSG::ChipId chip0, chip1;
			GetChips(psg, chip0, chip1);
			Parser::SendInfo(IsAdvanced(psg), stereo, clock, chip0, chip1);
		}

		// the types of both chips, the selected chip is kept
		static void GetChips(psg_t& psg, PSG::ChipId& chip0, PSG::ChipId& chip1)
		{
			uint8_t chip = psg.GetSelected();
			psg.Select(0); chip0 = psg.GetChipId();
			psg.Select(1); chip1 = psg.GetChipId();
			psg.Select(chip);
		}

	private:
		Parser m_parser;
		TextHandler m_handler{ nullptr };
	};
}
