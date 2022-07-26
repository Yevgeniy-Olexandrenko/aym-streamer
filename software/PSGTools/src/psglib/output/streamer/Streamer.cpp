#include <thread>
#include "Streamer.h"
#include "stream/Frame.h"

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
	return ("Streamer -> " + m_chip.toString());
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
	m_chip.count(Chip::Count::OneChip);
	m_chip.model(Chip::Model::Compatible);
	m_chip.frequency(Chip::Frequency::F1773400);
	m_chip.channels(Chip::Channels::ABC);
	return true;
}

void Streamer::Close()
{
	m_port.Close();
}

void Streamer::WriteToChip(int chip, const std::vector<uint8_t>& data)
{
	auto dataSize = (int)data.size();
	auto dataBuff = (const char*)data.data();
	auto sentSize = m_port.SendBinary(dataBuff, dataSize);
	if (sentSize != dataSize) m_isOpened = false;
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
