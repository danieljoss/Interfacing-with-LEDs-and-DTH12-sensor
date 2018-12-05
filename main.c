/**
 *  @file    main.c
 *  @author  Jose Torres, Daniel Gomez , Brno University of Technology, Czechia
 *  @version V1.2
 *  @date    2,12, 2018
 *  @brief   Turning on lEDs depending on the temperature capted by DTH12.
 */

/* Includes ------------------------------------------------------------------*/
#include "settings.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>         /* itoa() function */
#include "twi.h"
#include "uart.h"

/* Constants and macros ------------------------------------------------------*/
/**
 *  @brief Define UART buad rate.
 */
#define UART_BAUD_RATE 9600
#define DHT12 0x5c


struct values{
    uint8_t humidity_integer;
    uint8_t humidity_decimal;
    uint8_t temperature_integer;
    uint8_t temperature_decimal;
};
struct values Meteo_values;
/* Function prototypes -------------------------------------------------------*/
/**
 *  @brief Initialize UART, TWI, and Timer/Counter1.
 */
void setup(void);

/**
 *  @brief TWI Finite State Machine transmits all slave addresses.
 */
void fsm_twi_scanner(void);

/* Global variables ----------------------------------------------------------*/
typedef enum {
    IDLE_STATE = 1,
    HUMIDITY_STATE,
    TEMPERATURE_STATE,
    UART_STATE,
  
} state_t;
/* FSM for scanning TWI bus */
state_t twi_state = IDLE_STATE;

char uart_string[5];     // used in itoa convertion function
char uart_string2[10];
char uart_string3[10];
char uart_string4[10];


/* Functions -----------------------------------------------------------------*/
int main(void)
{
    /* Initializations */
    setup();

    /* Enables interrupts by setting the global interrupt mask */
    sei();

    /* Forever loop */
    while (1) {

        // display constantly the data of humidy and temperature
        // Convert int data type to string data type
        
        uart_puts("\r\n---Humidity values---:\r\n");
        itoa (Meteo_values.humidity_integer, uart_string, 10);

        uart_puts(uart_string);

        uart_puts(".");

        itoa (Meteo_values.humidity_decimal, uart_string2, 10);
        uart_puts(uart_string2);
        uart_puts("\r\n---Temperature values---:\r\n");

        itoa (Meteo_values.temperature_integer, uart_string3, 10);
        uart_puts(uart_string3);
        uart_puts(".");

        itoa (Meteo_values.temperature_decimal, uart_string4, 10);
        uart_puts(uart_string4);
            }
        return 0;
        }


/*******************************************************************************
 * Function: setup()
 * Purpose:  Initialize UART, TWI, and Timer/Counter1.
 * Input:    Non
 * Returns:  None
 ******************************************************************************/
void setup(void)
{
    DDRB |=_BV(PB5); // set PIN 13 as output
    DDRB |= _BV(PB4); // set PIN 12 as output
    DDRB |= _BV(PB3); // set PIN 11 as output
    
    /* Initialize UART: asynchronous, 8-bit data, no parity, 1-bit stop */
    uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));

    /* Initialize TWI */
    twi_init();

    /* Timer/Counter1: update FSM state */
    /* Clock prescaler 64 => overflows every 262 ms */
    TCCR1B |= _BV(CS11) | _BV(CS10);
    /* Overflow interrupt enable */
    TIMSK1 |= _BV(TOIE1);
}

/*******************************************************************************
 * Function: Timer/Counter1 overflow interrupt
 * Purpose:  Update state of TWI Finite State Machine.
 ******************************************************************************/
ISR(TIMER1_OVF_vect)
{
    fsm_twi_scanner();

}

/*******************************************************************************
 * Function: fsm_twi_scanner()
 * Purpose:  TWI Finite State Machine transmits all slave addresses.
 * Input:    None
 * Returns:  None
 ******************************************************************************/
void fsm_twi_scanner(void)
{
    /* Static variable inside a function keeps its value between callings */
    //static uint8_t slave_address = 0;
    uint8_t twi_status;


    switch (twi_state) {
    case IDLE_STATE:

        twi_state = HUMIDITY_STATE;

        break;
    case HUMIDITY_STATE:

        twi_status = twi_start((DHT12<<1) + TWI_WRITE);

        if (twi_status==0){

            twi_write (0x00);
            twi_stop ();

            twi_start((DHT12<<1) + TWI_READ);
            Meteo_values.humidity_integer = twi_read_ack();
            Meteo_values.humidity_decimal = twi_read_nack();
            twi_stop ();
            twi_state = TEMPERATURE_STATE;


        }
        else {
            uart_puts("Not connected H");

            twi_state = IDLE_STATE;
        }
        break;

    case TEMPERATURE_STATE:

        twi_status = twi_start((DHT12<<1) + TWI_WRITE);
        if (twi_status==0){
            twi_write (0x02);

            twi_stop ();
            twi_start((DHT12<<1) + TWI_READ);
            Meteo_values.temperature_integer = twi_read_ack();
            Meteo_values.temperature_decimal = twi_read_nack();
            twi_stop ();
            twi_state = UART_STATE;


        }
        else {
            uart_puts("Not connected T");
            twi_state = IDLE_STATE;
        }
        break;

    case UART_STATE:

      // Dependending on the temperature, 1 LED will
      // turn on, the others will be off

        if(Meteo_values.temperature_integer<=28){
            PORTB |= _BV(PB5); // turn on
            PORTB &= ~_BV(PB3); // turn off
            PORTB &= ~_BV(PB4);
        }
        if(Meteo_values.temperature_integer>28 && Meteo_values.temperature_integer<40){
            PORTB |= _BV(PB4);
            PORTB &= ~_BV(PB5);
            PORTB &= ~_BV(PB3);
        }
        if(Meteo_values.temperature_integer>=40){
            PORTB |= _BV(PB3);
            PORTB &= ~_BV(PB4);
            PORTB &= ~_BV(PB5);
        }

        twi_state = IDLE_STATE;
        break;

    default:
        twi_state = IDLE_STATE;
    } /* End of switch (twi_state) */

}

/* END OF FILE ****************************************************************/
