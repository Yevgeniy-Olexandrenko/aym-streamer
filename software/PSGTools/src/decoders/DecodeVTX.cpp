#include <string>
#include <fstream>
#include "DecodeVTX.h"

namespace
{
	const uint16_t VTXv2_SIG_AY = 0x7961; // "ay"
	const uint16_t VTXv2_SIG_YM = 0x6D79; // "ym"
}

////////////////////////////////////////////////////////////////////////////////

bool DecodeVTX::Open(Module& module)
{
	std::ifstream fileStream;
	fileStream.open(module.GetFilePath(), std::fstream::binary);

	if (fileStream)
	{
		fileStream.seekg(0, fileStream.end);
		uint32_t fileSize = (uint32_t)fileStream.tellg();

		if (fileSize > 20)
		{
			VTXHeader hdr; // data size here
			fileStream.seekg(0, fileStream.beg);
			fileStream.read((char*)(&hdr), sizeof(hdr));

			ChipType chipType(ChipType::Unknown);
			if (hdr.signature == VTXv2_SIG_AY) chipType = ChipType::AY;
			if (hdr.signature == VTXv2_SIG_YM) chipType = ChipType::YM;

			if (chipType != ChipType::Unknown)
			{
				module.SetChipType(chipType);

				if (hdr.stereo == VTXStereo::MONO) module.SetChipStereo(ChipStereo::MONO);
				if (hdr.stereo == VTXStereo::ABC ) module.SetChipStereo(ChipStereo::ABC );
				if (hdr.stereo == VTXStereo::ACB ) module.SetChipStereo(ChipStereo::ACB );

				module.SetChipFreqValue(hdr.chipFreq);
				module.SetFrameRate(hdr.frameFreq);
				m_loopFrameId = hdr.loop;

				auto GetTextProperty = [](std::ifstream& stream)
				{
					std::string str;
					if (stream) std::getline(stream, str, char(0x00));
					return str;
				};

				std::string title   = GetTextProperty(fileStream);
				std::string author  = GetTextProperty(fileStream);
				std::string program = GetTextProperty(fileStream);
				std::string tracker = GetTextProperty(fileStream);
				std::string comment = GetTextProperty(fileStream);

				module.SetTitle(title);
				module.SetArtist(author);
				module.SetType("VTX stream");

				uint32_t packedSize = fileSize - (uint32_t)fileStream.tellg();
				uint8_t* packedData = new uint8_t[packedSize];
				if (fileStream.read((char*)packedData, packedSize))
				{
					uint32_t depackedSize = hdr.dataSize;
				}
			}
		}
		fileStream.close();
	}

	return false;
}

bool DecodeVTX::Decode(Frame& frame)
{
	return false;
}

void DecodeVTX::Close(Module& module)
{
}
