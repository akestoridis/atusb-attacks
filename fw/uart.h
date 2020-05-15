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

#ifndef UART_H_
#define	UART_H_

#include <stdio.h>

void uart_init(void);
int uart_write_char(char c, FILE* stream);
void uart_new_line(void);

#endif /* UART_H_ */
