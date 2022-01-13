
#include "firmware_config.h"
#include "psg-access.h"
                
#define BAUD_RATE_CALC(baud) ((16000000 / 16 / baud) - 1)

void setup()
{
    delay(1000);

    PSG_Init();
    PSG_Detect();
    PSG_Reset();

    SerialToPSG_Init();
}

void loop()
{
    while(true)
    {
        SerialToPSG_Update();
    }
}
