#include "src/psg-access.h"
#include "src/uart-stream.h"

SinglePSG m_psg;

void inputHandler(const char* str)
{
    UARTStream.Print(F("echo: "));
    UARTStream.Println(str);
}

void setup()
{
    m_psg.Init();
    //m_psg.SetClock(SinglePSG::Clock::F1_75MHZ);
    //m_psg.SetStereo(SinglePSG::Stereo::ABC);
    UARTStream.Start(m_psg);
    UARTStream.SetInputHandler(inputHandler);

    // print firmware wellcome
    UARTStream.Println('-', 16);
    UARTStream.Println(F("  AYM STREAMER"));
    UARTStream.Println(F("  FW: v1.0"));
    UARTStream.Println('-', 16);

    // print PSG info
    UARTStream.Print(F("PSG type: "));
    switch(m_psg.GetType())
    {
    case SinglePSG::Type::NotFound:   UARTStream.Println(F("Not Found!")); break;
    case SinglePSG::Type::Compatible: UARTStream.Println(F("AY/YM Compatible")); break;
    case SinglePSG::Type::AY8910A:    UARTStream.Println(F("AY-3-8910A")); break;
    case SinglePSG::Type::AY8912A:    UARTStream.Println(F("AY-3-8912A")); break;
    case SinglePSG::Type::AY8913A:    UARTStream.Println(F("AY-3-8913A")); break;
    case SinglePSG::Type::AY8930:     UARTStream.Println(F("Microchip AY8930")); break;
    case SinglePSG::Type::YM2149F:    UARTStream.Println(F("Yamaha YM2149F")); break;
    case SinglePSG::Type::AVRAY_FW26: UARTStream.Println(F("Emulator AVR-AY (FW:26)")); break;
    default:                          UARTStream.Println(F("Bad or Unknown!")); break;
    }
}

void loop()
{
    //
}
