#include <Arduino.h>
#include <PowerSG.h>

PowerSG::Simple<PowerSG::DefPAccess> m_psg1;
PowerSG::Advanced<PowerSG::DefPAccess> m_psg2;

void setup()
{
    m_psg1.begin();
    m_psg1.setClock(1750000);
    m_psg1.update();

    m_psg2.begin();
    m_psg2.setClock(PowerSG::F1_75MHZ);
    m_psg2.setStereo(PowerSG::Stereo::ABC);
    m_psg2.update();

    PowerSG::ChipId chipId1 = m_psg1.getChipId();
    PowerSG::ChipId chipId2 = m_psg2.getChipId();

    // TODO
}

void loop()
{
    // TODO
}
