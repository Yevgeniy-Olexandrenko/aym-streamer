// -----------------------------------------------------------------------------
// PSG Access via PIO
// -----------------------------------------------------------------------------

// data bus bits D0-D3 (Arduino pins A0-A3)
#define LSB_DDR  DDRC
#define LSB_PORT PORTC
#define LSB_PIN  PINC
#define LSB_MASK 0x0F

// data bus bits D4-D7 (Arduino pins D4-D7)
#define MSB_DDR  DDRD
#define MSB_PORT PORTD
#define MSB_PIN  PIND
#define MSB_MASK 0xF0

// control bus BC1 and BDIR signals
#define BUS_PORT PORTB
#define BUS_DDR  DDRB
#define BC1_PIN  PB0    // Arduino pin D8
#define BDIR_PIN PB1    // Arduino pin D9

// RESET signal
#define RES_PORT PORTD
#define RES_DDR  DDRD
#define RES_PIN  PD2    // Arduino pin D2

// CLOCK signal
#define CLK_DDR  DDRD
#define CLK_PIN  PD3    // Arduino pin D3
