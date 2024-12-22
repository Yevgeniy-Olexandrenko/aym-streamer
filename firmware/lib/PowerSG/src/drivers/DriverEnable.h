#pragma once

#if defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
#define USE_M328_DRIVER
#endif

#if defined(__AVR_ATmega168PB__) || defined(__AVR_ATmega168PA__) || defined(__AVR_ATmega168P__) || defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168__)
#define USE_M328_DRIVER
#endif

#if defined(__AVR_ATmega88PB__) || defined(__AVR_ATmega88PA__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega88A__) || defined(__AVR_ATmega88__)
#define USE_M328_DRIVER
#endif

// TODO
// #if defined(__AVR_ATmega8A__) || defined(__AVR_ATmega8__)
// #define USE_M328_DRIVER
// #endif
