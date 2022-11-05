// -----------------------------------------------------------------------------
// PSG Type Detection
// -----------------------------------------------------------------------------

#include "psg-detect.h"
#include "psg-access.h"
#include "uart.h"

static psg_type type = PSG_TYPE_NOT_FOUND;

uint8_t get_pattern(uint8_t reg)
{
//  return (reg < 0x10 ? 0xFF : ((reg & 0x01) ? 0xAA : 0x55));
    return (reg < 0x10 ? (reg == 0x07 ? 0x3F : 0xFF) : ((reg & 0x01) ? 0xAA : 0x55));
}

void PSG_Detect()
{
    UART_Open(9600);
    uint8_t dump[64];

    // test 1
    for (uint8_t reg = 0; reg < (16 + 16); ++reg)
    {
        PSG_Send(reg, get_pattern(reg));
    }
    for (uint8_t reg = 0; reg < (16 + 16); ++reg)
    {
        dump[0x00 + reg] = PSG_Receive(reg);
    }

    // test 2
    for (uint8_t reg = 0; reg < (16 + 16); ++reg)
    {
        uint8_t data;
        PSG_Address(reg);
        PSG_Write(get_pattern(reg));
        PSG_Read(data);
        dump[0x20 + reg] = data;
    }

    // compute hash
    uint32_t hash = 7;
    for (uint8_t i = 0; i < 64; ++i)
    {
        hash = 31 * hash + uint32_t(dump[i]);
    }

    // print results
    UART_PutString(F("\nTesting result dump:\n"));
    for (uint8_t row = 0; row < 4; ++row)
    {
        for (uint8_t col = 0; col < 16; ++col)
        {
            UART_PutByteHex(dump[row * 16 + col]);
            UART_PutSP();
        }
        UART_PutLN();
    }

    UART_PutString(F("\nTesting result hash:\n"));
    UART_PutDWordHex(hash);
    UART_PutLN();

    UART_PutString(F("\nPSG chip type:\n"));
    switch (hash)
    {
    case 0xB517C407:
        type = PSG_TYPE_NOT_FOUND;
        UART_PutString(F("Not Found!\n"));
        break;

    case 0xF8033537:
        type = PSG_TYPE_AY_3_8910;
        UART_PutString(F("AY-3-8910\n"));
        break;
        
    case 0xA7D2B087:
        type = PSG_TYPE_YM2149F;
        UART_PutString(F("YM2149F\n"));
        break;

    case 0x07063E87:
        type = PSG_TYPE_KC89C72;
        UART_PutString(F("KC89C72\n"));
        break;

    case 0x7DD1BE8F:
        type = PSG_TYPE_AVR_AY_26;
        UART_PutString(F("AVR-AY (FW:26)\n"));
        break;
    
    default:
        type = PSG_TYPE_BAD_UNKNOWN;
        UART_PutString(F("Bad or Unknown!\n"));
        break;
    }
    
    UART_PutLN();
    UART_PutLN();
    UART_Close();
}

psg_type PSG_Type()
{
    return type;
}
