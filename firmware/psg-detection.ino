// -----------------------------------------------------------------------------
// PSG Type Detection
// -----------------------------------------------------------------------------

void print_init()
{
    cli();
    UBRR0H = BAUD_RATE_CALC(BAUD_RATE_OUTPUT) >> 8;
    UBRR0L = BAUD_RATE_CALC(BAUD_RATE_OUTPUT);
    UCSR0A = 0x00;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    sei();
}

void print_chr(byte ch)
{
    while ((UCSR0A & (1 << UDRE0)) == 0) {}
    UDR0 = ch;
}

void print_str(const __FlashStringHelper* str)
{
    const byte* p = (const byte *)str;
    while (byte c = pgm_read_byte(p++))
    {
        print_chr(c);
    }
}

void print_nib(byte nib)
{
    nib &= 0x0F;
    print_chr((nib >= 0x0A ? 'A' - 0x0A : '0') + nib);
}

void print_hex(byte data)
{
    print_nib(data >> 4);
    print_nib(data);
}

void print_u08(uint8_t data)
{
    print_str(F("0x"));
    print_hex(data);
    print_chr(' ');
}

void print_u32(uint32_t data)
{
    print_str(F("0x"));
    print_hex(data >> 24);
    print_hex(data >> 16);
    print_hex(data >> 8);
    print_hex(data);
    print_chr(' ');
}

// -----------------------------------------------------------------------------

enum PSG_Type
{
    PSG_TYPE_NOT_FOUND   = 0x00,
    PSG_TYPE_AY_3_8910   = 0x01,
    PSG_TYPE_YM2149F     = 0x02,
    PSG_TYPE_KC89C72     = 0x03,
    PSG_TYPE_AVR_AY_26   = 0x04,
    PSG_TYPE_BAD_UNKNOWN = 0xFF
};

PSG_Type psg_type = PSG_TYPE_NOT_FOUND;

byte get_pattern(byte reg)
{
    return (reg < 0x10 ? 0xFF : ((reg & 0x01) ? 0xAA : 0x55));
}

void PSG_Detect()
{
    print_init();
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

    // print results
    print_str(F("\nTesting result dump:\n"));
    for (byte row = 0; row < 4; ++row)
    {
        for (byte col = 0; col < 16; ++col)
        {
            byte data = dump[row * 16 + col];
            print_u08(data);
        }
        print_chr('\n');
    }

    print_str(F("\nTesting result hash:\n"));
    print_u32(hash);
    print_chr('\n');

    print_str(F("\nPSG chip type:\n"));
    switch (hash)
    {
    case 0xB517C407:
        psg_type = PSG_TYPE_NOT_FOUND;
        print_str(F("Not Found!\n"));
        break;

    case 0x82C3D6B7:
        psg_type = PSG_TYPE_AY_3_8910;
        print_str(F("AY-3-8910\n"));
        break;
        
    case 0x32935207:
        psg_type = PSG_TYPE_YM2149F;
        print_str(F("YM2149F\n"));
        break;

    case 0x91C6E007:
        psg_type = PSG_TYPE_KC89C72;
        print_str(F("KC89C72\n"));
        break;

    case 0xA20B7F4F:
        psg_type = PSG_TYPE_AVR_AY_26;
        print_str(F("AVR-AY (FW:26)\n"));
        break;
    
    default:
        psg_type = PSG_TYPE_BAD_UNKNOWN;
        print_str(F("Bad or Unknown!\n"));
        break;
    }
    print_chr('\n');
    print_chr('\n');
}
