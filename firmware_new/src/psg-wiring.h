// +-------------+-----------+-----------+-----------+---------+-----------+----------------------------+
// | PSG Signal  | AY-3-8910 | AY-3-8912 | AY-3-8913 | Arduino | ATMega328 | Description                |
// +-------------+-----------+-----------+-----------+---------+-----------+----------------------------+
// | Vcc (+5V)   | 40        | 3         | 13        | +5V     | VCC       | Power supply +5V           |
// | Vss (GND)   | 1         | 6         | 1         | GND     | GND       | Ground reference           |
// | DA0         | 37        | 28        | 11        | A0      | PC0       | Data/Address Bit 0         |
// | DA1         | 36        | 27        | 10        | A1      | PC1       | Data/Address Bit 1         |
// | DA2         | 35        | 26        | 9         | A2      | PC2       | Data/Address Bit 2         |
// | DA3         | 34        | 25        | 8         | A3      | PC3       | Data/Address Bit 3         |
// | DA4         | 33        | 24        | 7         | D4      | PD4       | Data/Address Bit 4         |
// | DA5         | 32        | 23        | 6         | D5      | PD5       | Data/Address Bit 5         |
// | DA6         | 31        | 22        | 5         | D6      | PD6       | Data/Address Bit 6         |
// | DA7         | 30        | 21        | 4         | D7      | PD7       | Data/Address Bit 7         |
// | BC1         | 29        | 20        | 3         | D8      | PB0       | Bus Control 1              |
// | BC2         | 28        | 19        | --        | +5V     | VCC       | Bus Control 2              |
// | BDIR        | 27        | 18        | 2         | D9      | PB1       | Bus Direction              |
// | A8          | 25        | 17        | 23        | +5V     | VCC       | Address Bit 8              |
// | #A9         | 24        | --        | 22        | GND     | GND       | Address Bit 9 (Low active) |
// | #CHIPSELECT | --        | --        | 24        | GND     | GND       | Chip select (Low active)   |
// | #RESET      | 23        | 16        | 21        | D2      | PD2       | Reset (Low active)         |
// | CLOCK       | 22        | 15        | 20        | D3      | PD3       | Master clock (1-2 MHz)     |
// | ANALOG A    | 4         | 5         | 17        | --      | --        | Analog output channel A    |
// | ANALOG B    | 3         | 4         | 15        | --      | --        | Analog output channel B    |
// | ANALOG C    | 38        | 1         | 18        | --      | --        | Analog output channel C    |
// +-------------+-----------+-----------+-----------+---------+-----------+----------------------------+

// data bus bits DA0-DA3 (Arduino pins A0-A3)
#define LSB_DDR  DDRC
#define LSB_PORT PORTC
#define LSB_PIN  PINC
#define LSB_MASK 0b00001111

// data bus bits DA4-DA7 (Arduino pins D4-D7)
#define MSB_DDR  DDRD
#define MSB_PORT PORTD
#define MSB_PIN  PIND
#define MSB_MASK 0b11110000

// control bus BC1 and BDIR signals
#define BUS_PORT PORTB
#define BUS_DDR  DDRB
#define BC1_PIN  PB0    // Arduino pin D8
#define BDIR_PIN PB1    // Arduino pin D9

// #RESET signal
#define RES_PORT PORTD
#define RES_DDR  DDRD
#define RES_PIN  PD2    // Arduino pin D2

// CLOCK signal
#define CLK_DDR  DDRD
#define CLK_PIN  PD3    // Arduino pin D3
