#include <Arduino.h>
#include <PowerSG.h>

PowerSG::Simple<PowerSG::DefPAccess> m_psgSimple;
//PowerSG::Advanced m_psgAdvanced;
//PowerSG::Simulated m_psgSimulated;

void setup()
{
    m_psgSimple.PowerOn();
    m_psgSimple.SetClock(1750000);

//    m_psgAdvanced.PowerOn();
//    m_psgAdvanced.SetClock(PowerSG::F1_75MHZ);

    // TODO
}

void loop()
{
    // TODO
}
