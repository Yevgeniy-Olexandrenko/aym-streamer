#include "DecodeVTX.h"
#include "lh5_decode.h"

namespace
{
	const uint16_t AYSignature = 0x7961; // "ay"
	const uint16_t YMSignature = 0x6D79; // "ym"
}

bool DecodeVTX::Open(Stream& stream)
{
	bool isDetected = false;
	std::ifstream fileStream;
	fileStream.open(stream.file, std::fstream::binary);

	if (fileStream)
	{
		fileStream.seekg(0, fileStream.end);
		uint32_t fileSize = (uint32_t)fileStream.tellg();

		if (fileSize > 20)
		{
			Header hdr; // data size here
			fileStream.seekg(0, fileStream.beg);
			fileStream.read((char*)(&hdr), sizeof(hdr));

			Chip::Model chipType(Chip::Model::Unknown);
			if (hdr.signature == AYSignature) chipType = Chip::Model::AY8910;
			if (hdr.signature == YMSignature) chipType = Chip::Model::YM2149;

			if (chipType != Chip::Model::Unknown)
			{
				stream.chip.model(chipType);

				if (hdr.stereo == Stereo::MONO) stream.chip.channels(Chip::Channels::MONO);
				if (hdr.stereo == Stereo::ABC ) stream.chip.channels(Chip::Channels::ABC);
				if (hdr.stereo == Stereo::ACB ) stream.chip.channels(Chip::Channels::ACB);

				stream.chip.freqValue(hdr.chipFreq);
				stream.play.frameRate(hdr.frameFreq);

				auto GetTextProperty = [](std::ifstream& stream)
				{
					std::string str;
					if (stream) std::getline(stream, str, char(0x00));
					return str;
				};

				// read all properties to move forward on stream
				std::string title = GetTextProperty(fileStream);
				std::string author = GetTextProperty(fileStream);
				std::string program = GetTextProperty(fileStream); // store in extras
				std::string tracker = GetTextProperty(fileStream); // store in extras
				std::string comment = GetTextProperty(fileStream);

				stream.info.title(title);
				stream.info.artist(author);
				stream.info.comment(comment);
				stream.info.type("VTX stream");

				// unpack frames data
				uint32_t packedSize = fileSize - (uint32_t)fileStream.tellg();
				uint8_t* packedData = new uint8_t[packedSize];

				if (fileStream.read((char*)packedData, packedSize))
				{
					uint32_t depackedSize = hdr.dataSize;
					m_data = new uint8_t[depackedSize];

					if (lh5_decode(packedData, m_data, depackedSize, packedSize)) 
						isDetected = true;
					else 
						delete[] m_data;
				}
				delete[] packedData;

				// prepare information on frames
				m_frames = (hdr.dataSize / 14);
				m_loop   = hdr.loop;
				m_frame  = 0;
			}
		}
		fileStream.close();
	}
	return isDetected;
}

bool DecodeVTX::Decode(Frame& frame)
{
	uint8_t* dataPtr = m_data + m_frame;
	m_frame++;

	for (uint8_t reg = 0; reg < 14; reg++)
	{
		frame.Update(reg, *dataPtr);
		dataPtr += m_frames;
	}

	return (m_frame < m_frames);
}

void DecodeVTX::Close(Stream& stream)
{
	stream.loop.frameId(m_loop);
	delete[] m_data;
}
