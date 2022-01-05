void print_hex(byte data)
{
    Serial.print(data > 0x0F ? "0x" : "0x0");
    Serial.print(data, HEX);
    Serial.print(' ');
}

byte get_pattern(byte reg)
{
    return (reg < 0x10 ? 0xFF : ((reg & 0x01) ? 0xAA : 0x55));
}

void setup()
{
    PSG_Init();
    Serial.begin(9600);
    delay(1000);

    byte dump[64];

    // test 1
    for (byte reg = 0; reg < (16 + 16); ++reg)
    {
        PSG_Send(reg, get_pattern(reg));
    }
    for (byte reg = 0; reg < (16 + 16); ++reg)
    {
        dump[0x00 + reg] = PSG_Receive(reg);
    }

    // test 2
    for (byte reg = 0; reg < (16 + 16); ++reg)
    {
        byte data;
        PSG_Address(reg);
        PSG_Write(get_pattern(reg));
        PSG_Read(data);
        dump[0x20 + reg] = data;
    }

    // compute hash
    uint32_t hash = 7;
    for (byte i = 0; i < 64; ++i)
    {
        hash = 31 * hash + uint32_t(dump[i]);
    }

    Serial.println("\nTesting result dump:");
    for (byte row = 0; row < 4; ++row)
    {
        for (byte col = 0; col < 16; ++col)
        {
            byte data = dump[row * 16 + col];
            print_hex(data);
        }
        Serial.println();
    }

    Serial.println("\nTesting result hash:");
    print_hex(hash >> 24);
    print_hex(hash >> 16);
    print_hex(hash >>  8);
    print_hex(hash >>  0);
    Serial.println();

    Serial.println("\nPSG chip type:");
    switch (hash)
    {
    case 0xB517C407:
        Serial.println("Not Found!");
        break;

    case 0x82C3D6B7:
        Serial.println("AY-3-8910");
        break;
        
    case 0x32935207:
        Serial.println("YM2149F");
        break;

    case 0x91C6E007:
        Serial.println("KC89C72");
        break;

    case 0xA20B7F4F:
        Serial.println("AVR-AY (FW:26)");
        break;
    
    default:
        Serial.println("Bad or Unknown!");
        break;
    }
    Serial.println();
}

void loop()
{
}
