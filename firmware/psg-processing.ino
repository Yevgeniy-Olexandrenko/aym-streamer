// -----------------------------------------------------------------------------
// PSG Data Processing
// -----------------------------------------------------------------------------

enum PSG_Stereo
{
    PSG_STEREO_ABC = 0,
    PSG_STEREO_ACB = 1,
    PSG_STEREO_CBA = 2,
};

const byte reg_mask[] PROGMEM = 
{
    0xFF, 0x0F, 0xFF, 0x0F,
    0xFF, 0x0F, 0x1F, 0xFF,
    0x1F, 0x1F, 0x1F, 0xFF,
    0xFF, 0x0F, 0xFF, 0xFF
};

PSG_Stereo psg_stereo = PSG_STEREO_ABC;

void PSG_ProcessAndSend(byte reg, byte data)
{
    if (psg_stereo == PSG_STEREO_ACB)
    {
        switch(reg)
        {
            case 0x02: reg = 0x04; break;
            case 0x03: reg = 0x05; break;
            case 0x04: reg = 0x02; break;
            case 0x05: reg = 0x03; break;
            case 0x07:
                byte a = (data & 0b00001001);
                byte b = (data & 0b00010010);
                byte c = (data & 0b00100100);
                data = a | (b << 1) | (c >> 1);
                break;
            case 0x09: reg = 0x0A; break;
            case 0x0A: reg = 0x09; break;
        }
    }

    if (psg_stereo == PSG_STEREO_CBA)
    {
        switch(reg)
        {
            case 0x00: reg = 0x04; break;
            case 0x01: reg = 0x05; break;
            case 0x04: reg = 0x02; break;
            case 0x05: reg = 0x03; break;
            case 0x07:
                byte a = (data & 0b00001001);
                byte b = (data & 0b00010010);
                byte c = (data & 0b00100100);
                data = (a << 2) | b | (c >> 2);
                break;
            case 0x08: reg = 0x0A; break;
            case 0x0A: reg = 0x08; break;
        }
    }

    data &= pgm_read_byte(&reg_mask[reg]);
    PSG_Send(reg, data);
}
