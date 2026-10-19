/*
 * File:   uart.c
 * Author: sidha
 *
 * Created on 15 October, 2026, 11:40 AM
 */


#include <xc.h>
#include "uart.h"

void init_uart(void)
{
	/* Serial initialization */
	RX_PIN = 1;
	TX_PIN = 0;

	TX9 = 0;
	/* UART Tarsmition enable bit */
	TXEN = 1;

	SYNC = 0;
	/* Send the Break character bit */
	SENDB = 0;

	BRGH = 1;

	SPEN = 1;
	/* 9bit RX enable or disable bit */
	RX9 = 0;
	/* Continous reception enable or disable */ 
	CREN = 1;

	BRG16 = 0;
	
	SPBRG = 129;


	/* TX interrupt flag bit */
	TXIF = 0;

	/* RX interrupt enable bit */
	RCIF = 0;
}

void putch(unsigned char byte) 
{
	/* Output one byte */
	/* Set when register is empty */
	while(!TXIF)
	{
		continue;
	}
	TXIF = 0;
	TXREG = byte;
} 

int puts(const char *s)
{
	while(*s)		
	{
		putch(*s++);	
	}
	return 0;
}

unsigned char getch(void)
{

	while(!RCIF)
	{
		continue;
	}
	RCIF = 0;
	return RCREG;
}

unsigned char getche(void)
{
	unsigned char c;

	putch(c = getch());

	return (c);
}
