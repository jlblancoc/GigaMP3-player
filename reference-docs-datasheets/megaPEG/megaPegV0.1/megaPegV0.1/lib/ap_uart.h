#ifndef _AP_UART_H_
#define _AP_UART_H_

#include <progmem.h>

//#define  FCLK 11059200ul
//#define UART_BAUD    115200ul
#define FCLK 10000000ul
#define UART_BAUD    9600ul

#define UART_CONST (FCLK/(16ul*UART_BAUD)-1)
/*
#define BAUD_ERR   (UART_BAUD-(FCLK/(16ul*(UART_CONST+1))))

#if (BAUD_ERR != 0 || UART_CONST>255)
	#error "UART baud rate is inaccurate!"
#endif
*/
#define PTRTYPE_RAM   0
#define PTRTYPE_FLASH 1

void uart_init(void);
void uart_sendchar(char c);
void uart_sendstr(char *pc, unsigned char pt);

#define PRGPRINT(s) uart_sendstr(PSTR(s), PTRTYPE_FLASH);

#endif
