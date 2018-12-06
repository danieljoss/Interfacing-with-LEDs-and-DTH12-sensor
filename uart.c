/*************************************************************************
*  Title:    Interrupt UART library with receive/transmit circular buffers
*  Author:   Peter Fleury <pfleury@gmx.ch>   http://tinyurl.com/peterfleury
*  File:     $Id: uart.c,v 1.15.2.4 2015/09/05 18:33:32 peter Exp $
*  Software: AVR-GCC 4.x
*  Hardware: any AVR with built-in UART,
*  License:  GNU General Public License
*
*  DESCRIPTION:
*   An interrupt is generated when the UART has finished transmitting or
*   receiving a byte. The interrupt handling routines use circular buffers
*   for buffering received and transmitted data.
*
*   The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE variables define
*   the buffer size in bytes. Note that these variables must be a
*   power of 2.
*
*  USAGE:
*   Refere to the header file uart.h for a description of the routines.
*   See also example test_uart.c.
*
*  NOTES:
*   Based on Atmel Application Note AVR306
*
*  LICENSE:
*   Copyright (C) 2015 Peter Fleury, GNU General Public License Version 3
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "uart.h"


/*
 *  constants and macros
 */

/* size of RX/TX buffers */
#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1)
#define UART_TX_BUFFER_MASK ( UART_TX_BUFFER_SIZE - 1)

#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
# error RX buffer size is not a power of 2
#endif
#if ( UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK )
# error TX buffer size is not a power of 2
#endif


#if defined(__AVR_AT90S2313__) || defined(__AVR_AT90S4414__) || defined(__AVR_AT90S8515__) || \
    defined(__AVR_AT90S4434__) || defined(__AVR_AT90S8535__) || \
    defined(__AVR_ATmega103__)
/* old AVR classic or ATmega103 with one UART */
# define UART0_RECEIVE_INTERRUPT  UART_RX_vect
# define UART0_TRANSMIT_INTERRUPT UART_UDRE_vect
# define UART0_STATUS             USR
# define UART0_CONTROL            UCR
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRR
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
#elif defined(__AVR_AT90S2333__) || defined(__AVR_AT90S4433__)
/* old AVR classic with one UART */
# define UART0_RECEIVE_INTERRUPT  UART_RX_vect
# define UART0_TRANSMIT_INTERRUPT UART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRR
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
#elif defined(__AVR_AT90PWM216__) || defined(__AVR_AT90PWM316__)
/* AT90PWN216/316 with one USART */
# define UART0_RECEIVE_INTERRUPT  USART_RX_vect
# define UART0_TRANSMIT_INTERRUPT USART_UDRE_vect
# define UART0_STATUS             UCSRA
# define UART0_CONTROL            UCSRB
# define UART0_CONTROLC           UCSRC
# define UART0_DATA               UDR
# define UART0_UDRIE              UDRIE
# define UART0_UBRRL              UBRRL
# define UART0_UBRRH              UBRRH
# define UART0_BIT_U2X            U2X
# define UART0_BIT_RXCIE          RXCIE
# define UART0_BIT_RXEN           RXEN
# define UART0_BIT_TXEN           TXEN
# define UART0_BIT_UCSZ0          UCSZ0
# define UART0_BIT_UCSZ1          UCSZ1
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega8A__) || \
    defined(__AVR