#include <Arduino.h>
#include <PowerSG.h>

PowerSG::PDriver  m_driver;
PowerSG::Simple   m_psg1(m_driver);
PowerSG::Advanced m_psg2(m_driver);

void setup()
{
    m_psg1.begin();
    m_psg1.setClock(1750000);

    m_psg2.begin();
    m_psg2.setClock(PowerSG::F1_75MHZ);
    m_psg2.setStereo(PowerSG::Stereo::ABC);
    m_psg2.update();

    PowerSG::ChipId chipId1 = m_psg1.getChipId();
    PowerSG::ChipId chipId2 = m_psg2.getChipId();

    constexpr auto sz1 = sizeof(PowerSG::Simple);
    constexpr auto sz2 = sizeof(PowerSG::Advanced);

    // TODO
}

void loop()
{
    // TODO
}
