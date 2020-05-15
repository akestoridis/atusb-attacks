/*
 * fw/uart.h - Functions needed for debugging over uart
 *
 * Code adapted from http://www.roboternetz.de/wissen/index.php/UART_mit_avr-gcc
 * and http://www.mikrocontroller.net/articles/AVR-GCC-Tutorial
 *
 * Published under the Creative Commons Share-Alike licence
 * https://creativecommons.org/licenses/by-sa/2.0/de/
 *
 * S. Salewski 2007
 *
 * Adapted by
 * Josef Filzmaier 2017
 */

#include <avr/io.h>
#include "uart.h"

#define USART_BAUD 38400UL
#define F_CPU 8000000UL

#define Wait_USART_Ready() while (!(UCSR1A & (1<<UDRE1)))
#define UART_UBRR (F_CPU/(16L*USART_BAUD)-1)

// initialize USART, 8N1 mode
void
uart_init(void)
{
/* TODO: Find a working configuration for uart for the atmega32u2 */
#if CHIP == at90usb1287
	CLKPR = (1 << CLKPCE);
	CLKPR = 0; // clock prescaler == 0, so we have 16 MHz mpu frequency
	UBRR1 = UART_UBRR;
	UCSR1C = (1 << UCSZ10) | (1 << UCSZ11);
	UCSR1B = (1 << TXEN1);
	do
	{
		UDR1;
	}
	while (UCSR1A & (1 << RXC1));
#endif

}

int uart_write_char(char c, FILE* stream)
{
	if (c == '\n'){
		uart_new_line();
	}
	else {
		Wait_USART_Ready();
		UDR1 = c;
	}
	return 0;
}

void
uart_new_line(void)
{
	Wait_USART_Ready();
	UDR1 = '\r';
	Wait_USART_Ready();
	UDR1 = '\n';
}
