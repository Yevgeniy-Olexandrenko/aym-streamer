#pragma once

#ifdef UART_DEBUG
#include "uart.h"
#define dbg_open(n) { UART_Open(n); }
#define dbg_print_byte(d) { UART_PutByteHex(d); }
#define dbg_print_dwrd(d) { UART_PutDWordHex(d); }
#define dbg_print_num(n)  { UART_PutNumber(n); }
#define dbg_print_str(s)  { UART_PutString(s); }
#define dbg_print_sp() { UART_PutSP(); }
#define dbg_print_ln() { UART_PutLN(); }
#define dbg_close() { UART_Close(); }
#else
#define dbg_open(n)
#define dbg_print_byte(d)
#define dbg_print_dwrd(d)
#define dbg_print_num(n)
#define dbg_print_str(s)
#define dbg_print_sp()
#define dbg_print_ln()
#define dbg_close()
#endif
