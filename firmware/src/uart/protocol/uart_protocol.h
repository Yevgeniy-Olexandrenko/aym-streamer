#pragma once

#include <avr/pgmspace.h>
#include "../../psg/model/psg_turbosound.h"

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
	//
	// The text 'PSG' with 0x1A starts the player: a PSG file of psg-tools
	// (a header of 16 or 4 bytes, then 0xFF a frame, 0xFE N N * 4 frames,
	// 0xFD the end, 'R V' a register: 0x00-0x0F chip 0, 0x80-0x8F chip 1).
	// The sender is paced by XON/XOFF. 0xFD, or no byte for 100 ms while
	// the player waits for one (a pause, the end), mutes the chips. The
	// player stays on until the MCU reset. A printable char is never an
	// address of a PSG file: in the player it starts a text too, so the
	// next file and the commands work. The config command comes from a
	// host: it ends the player, the stream of the host goes on.
	class Parser
	{
	public:
		enum class Event : uint8_t
		{
			None,     // no more received bytes
			Frame,    // 0xFF: a new frame, chip 0
			Chip1,    // 0xFE: chip 1
			Register, // a register: GetAddr, GetData, player: GetChip
			Config,   // the config command: GetConfig
			Text,     // other text command: GetText
			Timeout,  // no frame for 100 ms
			Play,     // the player starts a file: GetFrameRate
			PlayFrame,// the player frame ends, the next is GetFrames later
			PlayMute  // the player has no data: a pause or the end
		};

		// open the UART and start the frame timer
		void Start();

		// take the next event from the received bytes
		Event Next();

		uint8_t     GetAddr()   const { return m_addr; }
		uint8_t     GetData()   const { return m_data; }
		const char* GetText()   const { return m_text; }
		uint8_t     GetChip()   const { return m_chip; }
		uint8_t     GetFrameRate() const { return m_rate; }
		uint16_t    GetFrames() const { return m_frames; }

		// the stereo and the clock of the config command
		const psg_config_t& GetConfig() const { return m_config; }

	private:
		Event parse_byte(uint8_t data);
		void  start_text(uint8_t data);
		Event parse_text_byte(uint8_t data);
		Event parse_text();
		bool  parse_config();
		Event parse_play(uint8_t data);
		Event start_play();
		void  leave_play();

		static bool is_printable(uint8_t data) { return (data >= 0x20 && data < 0x7F); }

	private:
		enum class State : uint8_t
		{
			Marker,  // wait for 0xFF after a start, an error or a timeout
			Address, // wait for a marker, an address or a text
			Data,    // wait for the data of the address
			Text,    // wait for the end of the text

			// the player, from Header on
			Header,  // the header of the PSG file
			PlayAddr,// wait for a marker or a register
			PlayData,// wait for the data of the register
			PlaySkip // wait for N of 0xFE N
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

		// the player
		bool        m_player{ false };  // the player is on
		uint8_t     m_chip{ 0 };        // chip of the register
		uint8_t     m_rate{ 50 };       // frames per second
		uint16_t    m_frames{ 1 };      // frames to the next frame
		bool        m_waiting{ false }; // no byte at the last Next
		bool        m_muted{ false };   // PlayMute is sent for this wait
		uint16_t    m_wait_time{ 0 };   // Timer1 count when the wait began
	};

	// The frame clock of the player on Timer1 (F_CPU / 1024, 64 us). The
	// period has a fraction: 50 Hz is 312.5 counts, the ticks keep it.
	class FrameTimer
	{
	public:
		// the first tick comes 'delay' frames later
		void Start(uint8_t rate, uint8_t delay);

		// the time of the tick has come
		bool IsDue() const;

		// the next tick, one frame after the last tick
		void Next();

		// the next tick, one frame after now (after a late frame)
		void Restart();

	private:
		uint16_t m_tick{ 0 };   // Timer1 count of the next tick
		uint16_t m_period{ 0 }; // counts of one frame
		uint8_t  m_rest{ 0 };   // fraction of the period, in 1/rate
		uint8_t  m_sum{ 0 };    // sum of the fractions, in 1/rate
		uint8_t  m_rate{ 50 };
	};

	// The host stream and the PSG player to the TurboSound chips of Basic
	// or of Advanced model.
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

		// handle the received bytes and the frame timeout
		void Process(psg_t& psg)
		{
			if (m_play) play(psg);
			else stream(psg);
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
		// the stream of the host
		void stream(psg_t& psg)
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

				case Event::Play:
					start_play(psg);
					return;

				default:
					break;
				}
			}
		}

		// The player: the registers of a frame go to the model at once,
		// the frame is committed on its tick. When its bytes are late,
		// the tick waits for them. The next frame is parsed after the
		// commit and after the empty frames of 0xFE N.
		void play(psg_t& psg)
		{
			using Event = Parser::Event;
			while (!m_ready && !m_hold)
			{
				Event event = m_parser.Next();
				if (event == Event::None) break;
				switch (event)
				{
				case Event::Register:
					psg.Select(m_parser.GetChip());
					psg.SetRegister(m_parser.GetAddr(), m_parser.GetData());
					break;

				case Event::PlayFrame:
					m_ready = true;
					m_next = m_parser.GetFrames();
					break;

				case Event::PlayMute:
					// a pause or the end: Advanced keeps the
					// state, the next frame of the file restores it
					psg.Mute();
					break;

				case Event::Play:
					// the next file
					start_play(psg);
					break;

				case Event::Text:
					if (m_handler) m_handler(m_parser.GetText());
					break;

				case Event::Config:
					// a host: the parser is back to its stream
					m_play = false;
					SetConfig(psg, m_parser.GetConfig());
					SendInfo(psg);
					return;

				default:
					break;
				}
			}

			// a late frame plays at once, the ticks start again from it
			// (Timer1 wraps after 4.2 s, a long pause must not wait for it)
			if (!m_late && !m_timer.IsDue()) return;
			if (m_hold)
			{
				m_hold--;
				m_timer.Next();
			}
			else if (m_ready)
			{
				commit(psg);
				m_ready = false;
				m_hold = m_next - 1;
				if (m_late) m_timer.Restart();
				else m_timer.Next();
				m_late = false;
			}
			else m_late = true;
		}

		void start_play(psg_t& psg)
		{
			// a known state for the file, the config stays;
			// 3 frames of time let the sender fill the buffer
			psg.Reset();
			m_play = true;
			m_ready = false;
			m_late = false;
			m_hold = 0;
			m_timer.Start(m_parser.GetFrameRate(), 3);
		}

		// the model differences, specialized below
		static bool is_advanced();
		static void set_stereo(psg_t& psg, PSG::Stereo stereo);
		static PSG::Stereo get_stereo(psg_t& psg);
		static void commit(psg_t& psg);

	private:
		Parser m_parser;
		TextHandler m_handler{ nullptr };

		// the player
		FrameTimer m_timer;
		bool     m_play{ false };  // the player is on
		bool     m_ready{ false }; // all registers of the frame are parsed
		bool     m_late{ false };  // the tick waits for the frame
		uint16_t m_hold{ 0 };      // empty frames before the next frame
		uint16_t m_next{ 1 };      // frames from this frame to the next
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
