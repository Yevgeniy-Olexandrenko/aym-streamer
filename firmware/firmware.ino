

#define BAUD_RATE_INFO          9600
#define BAUD_RATE_STREAM        57000

#define F_CPU                   16000000
#define BAUD_RATE_CALC(baud)    ((F_CPU / 16 / baud) - 1)

void setup()
{
    delay(1000);

    PSG_Init();
    PSG_Detect();

    SerialToPSG_Init();
}

void loop()
{
    while(true)
    {
        SerialToPSG_Update();
    }
}
