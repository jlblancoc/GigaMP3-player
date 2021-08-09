#include <sig-avr.h>
#include "ap_uart.h"

volatile static unsigned char uart_tx_busy;
static unsigned char *uart_tx_buf;
static unsigned char uart_tx_prgptr;   // zero    => uart_tx_buf pointer is to RAM,
                                       // no-zero => uart_tx_buf pointer is to FLASH

SIGNAL(SIG_UART_TRANS) {
	unsigned char c;

	if (uart_tx_buf) {
		uart_tx_buf++;
		c = uart_tx_prgptr ? PRG_RDB(uart_tx_buf) : *uart_tx_buf;
		if (c!=0)
			outp(c, UDR);
		else {             // end of string
			uart_tx_buf=0;
			uart_tx_busy=0;
		}
	} else
		uart_tx_busy=0;
}

void uart_init(void) {
  uart_tx_prgptr=1; // default pointer type is RAM
	uart_tx_busy=0;
	uart_tx_buf=0;
  //**//outp(/*BV(RXCIE)|BV(RXEN)|*/BV(TXCIE)|BV(TXEN),UCR); // enable tx, disable rx
  outp(/*BV(RXCIE)|BV(RXEN)|BV(TXCIE)|*/BV(TXEN),UCR); // enable tx, disable rx
	outp( (unsigned char)UART_CONST, UBRR);              // set baud rate
    /* !!! remember to enable interrupts in main program! */
}

void uart_sendchar(char c) {
  //**//while (uart_tx_busy);
  //**//uart_tx_busy=1;
  while(bit_is_clear(USR, UDRE));
	outb(c, UDR);
}

void uart_sendstr(char *pc, unsigned char pt) {
	unsigned char c;

	while(uart_tx_busy);
	c = pt ? PRG_RDB(pc) : *pc;
	if (c!=0) {
		uart_tx_buf=pc;
		uart_tx_prgptr=pt;
		uart_tx_busy=1;

		outp(c, UDR);
	} else {				// empty string - release uart and do nothing
		uart_tx_buf=0;
	}
}

