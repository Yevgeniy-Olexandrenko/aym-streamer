#include "src/psg/AdvancedAccess.h"
#include "src/uart-stream.h"

psg::AdvancedAccess m_psg;

void inputHandler(const char* str)
{
    UARTStream.Print(F("echo: "));
    UARTStream.Println(str);
}

void setup()
{
    // power on and configure PSG
    m_psg.PowerOn();
    m_psg.SetClock(psg::F1_75MHZ);
    //m_psg.SetStereo(psg::Stereo::ACB);

    // start and configure UART stream
    UARTStream.Start(m_psg);
    UARTStream.SetInputHandler(inputHandler);

    // print firmware wellcome
    UARTStream.Println('-', 4 + 12 + 4);
    UARTStream.Print  (' ', 4); UARTStream.Println(F("AYM STREAMER"));
    UARTStream.Print  (' ', 4); UARTStream.Println(F("FW: v2.1"));
    UARTStream.Println('-', 4 + 12 + 4);

    // print PSG info
    UARTStream.Print(F("hash:\t"));
    UARTStream.Println(uint32_t(m_psg.GetType()), true);
    UARTStream.Print(F("chip:\t"));
    switch(m_psg.GetType())
    {
    case psg::Type::NotFound:   UARTStream.Println(F( "Not Found!"       )); break;
    case psg::Type::Compatible: UARTStream.Println(F( "AY/YM Compatible" )); break;
    case psg::Type::AY8910A:    UARTStream.Println(F( "AY-3-8910A"       )); break;
    case psg::Type::AY8912A:    UARTStream.Println(F( "AY-3-8912A"       )); break;
    case psg::Type::AY8913A:    UARTStream.Println(F( "AY-3-8913A"       )); break;
    case psg::Type::AY8930:     UARTStream.Println(F( "Microchip AY8930" )); break;
    case psg::Type::YM2149F:    UARTStream.Println(F( "Yamaha YM2149F"   )); break;
    case psg::Type::AVRAY_FW26: UARTStream.Println(F( "AVR-AY (FW:26)"   )); break;
    default:                    UARTStream.Println(F( "Bad or Unknown!"  )); break;
    }
    UARTStream.Print(F("clock:\t"));
    UARTStream.Println(m_psg.GetClock());
}

void loop()
{
    //
}
