#pragma once

#include "../psg/mode/psg_turbosound.h"

namespace UART
{
	// The stream of psg-tools: each frame is one packet, 0xFF (chip 0)
	// and for TurboSound 0xFE (chip 1), each followed by the 'addr, data'
	// pairs of the changed registers. Addresses are 0x00-0x0F, so 0xFF
	// and 0xFE are markers only where an address is expected.
	class Protocol
	{
	public:
		using psg_t = PSG::TurboSound<PSG::Advanced>;

		// open the UART and start the frame timer
		void Start();

		// handle all received bytes and the frame timeout
		void Process(psg_t& psg);

	private:
		enum class State : uint8_t
		{
			Marker,  // wait for 0xFF after a start, an error or a timeout
			Address, // wait for a marker or an address
			Data     // wait for the data of the address
		};

		State    m_state{ State::Marker };
		uint8_t  m_addr{ 0 };
		uint8_t  m_errors{ 0 };      // last error count of the driver
		bool     m_playing{ false }; // frames come, the chips sound
		uint16_t m_frame_time{ 0 };  // Timer1 count at the last frame
	};
}
