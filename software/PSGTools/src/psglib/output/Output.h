#pragma once

#include <vector>
#include "processing/Processing.h"

#define AY8930_FORCE_TO_CHOOSE  0
#define AY8930_FORCE_TO_DISCARD 0

class Stream;
class Frame;

class Output : private Processing
{
public:
	Output();
	virtual ~Output();
	
	bool Open();
	bool Init(const Stream& stream);
	bool Write(const Frame& frame);
	void Close();

	const Frame& GetFrame() const;
	std::string toString() const;

protected:
	using Data = std::vector<std::pair<uint8_t, uint8_t>>;
	virtual const std::string GetDeviceName() const = 0;

	virtual bool OpenDevice() = 0;
	virtual bool InitDstChip(const Chip& srcChip, Chip& dstChip) = 0;
	virtual bool WriteToChip(int chip, const Data& data) = 0;
	virtual void CloseDevice() = 0;

private:
	void Reset() override;
	const Frame& operator()(const Frame& frame) override;

private:
	bool m_isOpened;
	Chip m_chip;

	ProcessingChain m_processingChain;
};
