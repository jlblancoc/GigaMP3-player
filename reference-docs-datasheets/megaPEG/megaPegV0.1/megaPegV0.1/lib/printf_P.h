#ifndef _PRINTF_P_H_
#define _PRINTF_P_H_

#include <progmem.h>

extern void _printf_P (char const *fmt0, ...);

#define printf(format, args...)   _printf_P(PSTR(format) , ## args)

#endif
