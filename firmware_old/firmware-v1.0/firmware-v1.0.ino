
#include "firmware.h"
#include "src/psg-access.h"
#include "src/psg-detect.h"
#include "src/uart-stream.h"

void setup()
{
    delay(1000);

    PSG_Init();
    PSG_Detect();
    PSG_Reset();

    UARTStream_Open();
}

void loop()
{
    while(true)
    {
        UARTStream_Update();
    }
}
