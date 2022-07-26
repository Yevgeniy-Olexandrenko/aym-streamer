#include "Output.h"
#include "stream/Frame.h"

Output::Output()
    : m_isOpened(false)
{
}

std::string Output::toString() const
{
    return (GetOutputDeviceName() + " -> " + m_chip.toString());
}

bool Output::Write(const Frame& frame)
{
#if AY8930_PERFORM_ENV_FIX
    const Frame& processedFrame = m_chip.model() == Chip::Model::AY8930
        ? AY8930_FixEnvelope(frame)
        : frame;
#else
    const Frame& processedFrame = frame;
#endif

    WriteToChip(0, processedFrame);
    if (m_chip.count() == Chip::Count::TwoChips)
    {
        WriteToChip(1, processedFrame);
    }

    return m_isOpened;
}

void Output::WriteToChip(int chip, const Frame& frame)
{
    if (m_isOpened)
    {
        std::vector<uint8_t> data;

        if (frame.IsExpMode(chip))
        {
            bool switchBanks = false;
            for (Register reg = BankB_Fst; reg < BankB_Lst; ++reg)
            {
                if (frame.IsChanged(chip, reg))
                {
                    if (!switchBanks)
                    {
                        data.push_back(Mode_Bank);
                        data.push_back(frame.Read(chip, Mode_Bank) | 0x10);
                        switchBanks = true;
                    }
                    data.push_back(reg);
                    data.push_back(frame.Read(chip, reg));
                }
            }
            if (switchBanks)
            {
                data.push_back(Mode_Bank);
                data.push_back(frame.Read(chip, Mode_Bank));
            }
        }

        for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
        {
            if (frame.IsChanged(chip, reg))
            {
                data.push_back(reg);
                data.push_back(frame.Read(chip, reg));
            }
        }

        data.push_back(0xFF);
        WriteToChip(chip, data);
    }
}

const Frame& Output::AY8930_FixEnvelope(const Frame& frame) const
{
    static Frame s_frame;
//  s_frame = Frame::CreateComposition(s_frame, frame);
    s_frame = frame;

    for (int chan = 0; chan < 3; ++chan) 
        AY8930_FixEnvelopeInChannel(0, s_frame, chan);

    if (m_chip.count() == Chip::Count::TwoChips)
    {
        for (int chan = 0; chan < 3; ++chan)
            AY8930_FixEnvelopeInChannel(1, s_frame, chan);
    }
    return s_frame;
}

void Output::AY8930_FixEnvelopeInChannel(int chip, Frame& frame, int chan) const
{
    uint8_t mixer = frame.Read(chip, Mixer) >> chan;
    uint8_t vol_e = frame.Read(chip, A_Volume + chan);

    bool enableT = !(mixer & 0x01);
    bool enableN = !(mixer & 0x08);
    bool enableE =  (vol_e & 0x10);

    if (enableE && !(enableT || enableN))
    {
        // enable tone and set period to zero
        frame.Update(chip, Mixer, (mixer & ~0x01) << chan);
        frame.UpdatePeriod(chip, A_Period + 2 * chan, 0x0);
    }
}
