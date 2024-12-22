#include <Arduino.h>
#include <PowerSG.h>

PowerSG::Simple<PowerSG::DefPAccess> m_psgSimple;
PowerSG::Advanced<PowerSG::DefPAccess> m_psgAdvanced;

void setup()
{
    m_psgSimple.PowerOn();
    m_psgSimple.SetClock(1750000);
    m_psgSimple.Update();

    m_psgAdvanced.PowerOn();
    m_psgAdvanced.SetClock(PowerSG::F1_75MHZ);
    m_psgAdvanced.Update();

    // TODO
}

void loop()
{
    // TODO
}
