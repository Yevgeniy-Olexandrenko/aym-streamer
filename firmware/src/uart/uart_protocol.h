#pragma once

#include <avr/pgmspace.h>
#include "../psg/model/psg_turbosound.h"

namespace UART
{
	struct psg_config_t { PSG::Stereo stereo; PSG::Clock clock; };

	// text output for the replies of the streamer
	void WriteText(const char* text_P);
	void WriteHexU32(uint32_t value);
	void WriteConfig(const psg_config_t& config);
	void WriteChipName(PSG::ChipId chipid);
	void WriteLineEnd();

	// The stream of the host: each frame is one packet, 0xFF (chip 0)
	// and for TurboSound 0xFE (chip 1), each followed by the 'addr, data'
	// pairs of the changed registers. Addresses are 0x00-0x0F, so 0xFF
	// and 0xFE are markers only where an address is expected.
	//
	// A printable char where an address is expected starts a text command,
	// '\r' or '\n' ends it. The command 'ABC1773400' sets the stereo and
	// the clock, the protocol replies with SendInfo. Other commands go to
	// the text handler of the streamer.
	class Parser
	{
	public:
		enum class Event : uint8_t
		{
			None,     // no more received bytes
			Frame,    // 0xFF: a new frame, chip 0
			Chip1,    // 0xFE: chip 1
			Register, // a register: GetAddr, GetData
			Config,   // the config command: GetConfig
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

		// the stereo and the clock of the config command
		const psg_config_t& GetConfig() const { return m_config; }

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
		psg_config_t m_config{ PSG::Stereo::ABC, PSG::Clock::F_DEF };
	};

	// The host stream to the TurboSound chips of Basic or of Advanced model.
	template<class PSGModel>
	class Protocol
	{
	public:
		using psg_t = PSG::TurboSound<PSGModel>;
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
					commit(psg);
					psg.Select(0);
					break;

				case Event::Chip1:
					psg.Select(1);
					break;

				case Event::Register:
					psg.SetRegister(m_parser.GetAddr(), m_parser.GetData());
					break;

				case Event::Config:
					SetConfig(psg, m_parser.GetConfig());
					SendInfo(psg);
					break;

				case Event::Text:
					if (m_handler) m_handler(m_parser.GetText());
					break;

				case Event::Timeout:
					// the host stopped: mute the chips. Advanced keeps the
					// state, the next frame of the host restores it
					psg.Mute();
					break;
				}
			}
		}

		// Apply a config as the config command does, without a reply.
		// The reset puts the chips into a known state before a new stream.
		static void SetConfig(psg_t& psg, const psg_config_t& config)
		{
			psg.Reset();
			psg.SetClock(config.clock);
			set_stereo(psg, config.stereo);
		}

		// The config the chips use, as the reply gives it: Advanced
		// clamps the clock to 1-2 MHz, Basic plays ABC on the real clock.
		static psg_config_t GetConfig(psg_t& psg)
		{
			return psg_config_t{ get_stereo(psg), psg.GetClock() };
		}

		// The reply to the config command, 4 lines: the streamer type, the
		// stereo and the clock it uses, the type of chip 0 and of chip 1.
		static void SendInfo(psg_t& psg)
		{
			WriteText(is_advanced() ? PSTR("Advanced") : PSTR("Basic"));
			WriteLineEnd();
			WriteConfig(GetConfig(psg));
			WriteLineEnd();

			uint8_t selected = psg.GetSelected();
			for (uint8_t chip = 0; chip < 2; ++chip)
			{
				psg.Select(chip);
				WriteChipName(psg.GetChipId());
				WriteLineEnd();
			}
			psg.Select(selected);
		}

	private:
		// the model differences, specialized below
		static bool is_advanced();
		static void set_stereo(psg_t& psg, PSG::Stereo stereo);
		static PSG::Stereo get_stereo(psg_t& psg);
		static void commit(psg_t& psg);

	private:
		Parser m_parser;
		TextHandler m_handler{ nullptr };
	};

	// Basic does not process the stream: it writes each register at once,
	// and the channels go to the outputs as wired, so the stereo is ABC.
	template<> inline bool Protocol<PSG::Basic>::is_advanced() { return false; }
	template<> inline void Protocol<PSG::Basic>::set_stereo(psg_t&, PSG::Stereo) {}
	template<> inline PSG::Stereo Protocol<PSG::Basic>::get_stereo(psg_t&) { return PSG::Stereo::ABC; }
	template<> inline void Protocol<PSG::Basic>::commit(psg_t&) {}

	// Advanced writes a frame on commit and remaps the channels.
	template<> inline bool Protocol<PSG::Advanced>::is_advanced() { return true; }
	template<> inline void Protocol<PSG::Advanced>::set_stereo(psg_t& psg, PSG::Stereo stereo) { psg.SetStereo(stereo); }
	template<> inline PSG::Stereo Protocol<PSG::Advanced>::get_stereo(psg_t& psg) { return psg.GetStereo(); }
	template<> inline void Protocol<PSG::Advanced>::commit(psg_t& psg) { psg.Update(); }
}
