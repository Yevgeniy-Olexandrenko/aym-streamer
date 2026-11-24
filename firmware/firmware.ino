#include "src/psg/mode/psg_turbosound.h"
#include "src/uart/uart_protocol.h"

PSG::TurboSound<PSG::Advanced> m_psg;
UART::Protocol m_protocol;

void setup()
{
	m_psg.PowerOn();
	m_protocol.Start();
}

void loop()
{
	m_protocol.Process(m_psg);
}
