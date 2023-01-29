#include "src/psg-access.h"
#include "src/uart-stream.h"

SoundChip m_soundChip;

void setup()
{
    m_soundChip.Init();
    m_soundChip.SetClock(SoundChip::Clock::F1_75MHZ);
    m_soundChip.SetStereo(SoundChip::Stereo::ABC);

    UART::Stream::Start(m_soundChip);
    sei();
}

void loop()
{
    //
}
