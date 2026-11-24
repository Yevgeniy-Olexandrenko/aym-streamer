#include "src/psg/mode/psg_turbosound.h"

PSG::TurboSound<PSG::Advanced> m_psg;

void setup()
{
	m_psg.PowerOn();
	m_psg.SetClock(PSG::Clock::F_1750000);
	m_psg.SetStereo(PSG::Stereo::ACB);

	// TODO

	m_psg.Commit();

	// TODO

}

void loop()
{
	// TODO
}
