#include "src/psg/mode/psg_advanced.h"

PSG::Advanced m_psg[2];
int chip = 0;

void setup()
{
	m_psg[chip].PowerOn();
	m_psg[chip].SetClock(PSG::Clock::F_1750000);
	m_psg[chip].SetStereo(PSG::Stereo::ACB);

	// TODO

	m_psg[chip].Commit();
	
	// TODO

}

void loop()
{
	// TODO
}
