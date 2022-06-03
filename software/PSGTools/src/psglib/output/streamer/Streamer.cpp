#include <thread>
#include "Streamer.h"
#include "stream/Frame.h"

namespace
{
	const bool k_processForAY8930 = !true;
}

Streamer::Streamer(int comPortIndex)
	: m_portIndex(comPortIndex)
{
}

Streamer::~Streamer()
{
	Close();
}

std::string Streamer::name() const
{
	return ("Streamer -> " + chip.toString());
}

bool Streamer::Open()
{
	m_port.Open(m_portIndex);
	if (m_isOpened = m_port.SetBaudRate(SerialPort::BaudRate::_57600))
	{
		// wait before AYM Streame become ready
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	return m_isOpened;
}

bool Streamer::Init(const Stream& stream)
{
	chip.count(Chip::Count::OneChip);
	chip.model(Chip::Model::Compatible);
	chip.frequency(Chip::Frequency::F1773400);
	chip.channels(Chip::Channels::ABC);
	return true;
}

bool Streamer::OutFrame(const Frame& frame, bool force)
{
	if (m_isOpened)
	{
		Frame output = k_processForAY8930 ? ProcessForAY8930(frame) : frame;

		char buffer[16 * 2 + 1];
		int  bufPtr = 0;

		for (uint8_t reg = 0; reg < 16; ++reg)
		{
			if (force || output.IsChanged(reg))
			{
				// register number and value
				buffer[bufPtr++] = char(reg);
				buffer[bufPtr++] = char(output.Read(reg));
			}
		}

		// frame end marker
		buffer[bufPtr++] = char(0xFF);
		int bytesSent = m_port.SendBinary(buffer, bufPtr);
		if (bytesSent != bufPtr) m_isOpened = false;
	}
	return m_isOpened;
}

void Streamer::Close()
{
	m_port.Close();
}

Frame Streamer::ProcessForAY8930(const Frame& frame) const
{
	auto ProcessChannel = [](int chan, Frame& frame)
	{
		uint8_t mixer = frame.Read(Mixer) >> chan;
		uint8_t vol_e = frame.Read(A_Volume + chan);
		
		bool enableT = !(mixer & 0x01);
		bool enableN = !(mixer & 0x08);
		bool enableE =  (vol_e & 0x10);

		if (enableE && !(enableT || enableN))
		{
			// enable tone and set period to zero
			frame.Update(Mixer, (mixer & ~0x01) << chan);
			frame.UpdatePeriod(A_Period + 2 * chan, 0x0);
			return true;
		}
		return false;
	};

	Frame output = frame;
	static bool prevFrameModified = false;

	if (prevFrameModified)
	{
		prevFrameModified = false;
		for (uint8_t reg = 0; reg <= 7; ++reg) 
			output.changed(0, reg) = true;
	}

	prevFrameModified |= ProcessChannel(0, output);
	prevFrameModified |= ProcessChannel(1, output);
	prevFrameModified |= ProcessChannel(2, output);
	return output;
}
