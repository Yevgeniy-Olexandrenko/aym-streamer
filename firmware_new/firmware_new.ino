#include "src/psg-access.h"
#include "src/uart-stream.h"

SinglePSG m_psg;

void setup()
{
    m_psg.Init();
    m_psg.SetClock(SinglePSG::Clock::F1_75MHZ);
    m_psg.SetStereo(SinglePSG::Stereo::ABC);
    UARTStream.Start(m_psg);
}

void loop()
{
    //
}
