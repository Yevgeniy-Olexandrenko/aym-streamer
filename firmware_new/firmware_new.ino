#include "src/psg-access.h"
#include "src/psg-detect.h"

void setup()
{
    delay(1000);

    PSG_Init();
    PSG_Detect();
    PSG_Reset();

    // TODO
}

void loop()
{
    while(true)
    {
        // TODO
    }
}
