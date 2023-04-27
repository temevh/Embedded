/*
 * uno_SPI_slave_ISR_JS.c
 *
 * Created: 5.7.2019 10:01:31
 * Author : Glutex
 */ 

/* ATmega328p (UNO) is slave */

#define F_CPU 16000000UL
#define FOSC 16000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <stdbool.h>

volatile unsigned char g_joystick_data[6];
volatile uint8_t array_index = 0;

static void
USART_init(uint16_t ubrr) // unsigned int
{
    /* Set baud rate in the USART Baud Rate Registers (UBRR) */
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    
    /* Enable receiver and transmitter on RX0 and TX0 */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //NOTE: the ATmega328p has 1 UART: 0
    
    /* Set frame format: 8 bit data, 2 stop bit */
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00);
    
}

static void
USART_Transmit(unsigned char data, FILE *stream)
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0)))
    {
        ;
    }
    
    /* Put the data into a buffer, then send/transmit the data */
    UDR0 = data;
}

static char
USART_Receive(FILE *stream)
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0)))
    {
        ;
    }
    
    /* Get the received data from the buffer */
    return UDR0;
}

// Setup the stream functions for UART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

ISR
(SPI_STC_vect)
{
    /* add joystick array data to SPDR */
    SPDR = g_joystick_data[array_index++];
    
    if (array_index < sizeof(g_joystick_data))
    {
        ;
    }
    else
    {
        array_index = 0;
    }

}

void
joystick_data(volatile uint16_t *x_value_analog, volatile uint16_t *y_value_analog, volatile bool *b_switch_value)
{
    /* ADC conversion for X (A0) */
    ADMUX  &= ~(1 << 0); // select ADC0
    ADCSRA |=  (1 << 6); // start conversion
    _delay_us(5);
    while(ADCSRA & (1 << 6))
    {
        /* wait for conversion to finish */
        ;
    }
    _delay_us(5);
    *x_value_analog = ADC;
    _delay_us(5);
    /* ADC conversion for Y (A1) */
    ADMUX  |=  (1 << 0); // select ADC1
    ADCSRA |=  (1 << 6); // start conversion
    _delay_us(5);
    while(ADCSRA & (1 << 6))
    {
        /* wait for conversion to finish */
        ;
    }
    _delay_us(5);
    *y_value_analog = ADC;
    
    /* read Switch */
    *b_switch_value = (PIND & (1 << PD7));
    
    /* set data to arrays */
    g_joystick_data[0] = (*x_value_analog >> 8) & 0xFF;
    g_joystick_data[1] = *x_value_analog & 0xFF;
    g_joystick_data[2] = (*y_value_analog >> 8) & 0xFF;
    g_joystick_data[3] = *y_value_analog & 0xFF;
    g_joystick_data[4] = *b_switch_value;
}

int
main(void)
{
    /* set MISO as output, pin 12 (PB4)*/
    DDRB = (1 << PB4);
    /* set SPI enable (SPE) and interrupt enable (SPIE) */
    SPCR  |= (1 << 6) | (1 << 7);
    
    /* set up joystick ports */
    DDRC  &= ~(1 << PC0) & ~(1 << PC1); // A0 and A1 as X and Y inputs
    DDRD  &= ~(1 << PD7); // pin 7 as switch input
    PORTD |=  (1 << PD7); // enable pullup for switch
        
    /* set up ADC */
    ADMUX  |=  (1 << 6); // AVCC as ref
    ADCSRA |=  (1 << 7); // enable ADC
    ADCSRA |= (1 << 2) | (1 << 1) | (1 << 0); // set prescaler to 128
    ADCSRB &= ~(1 << 0) & ~(1 << 1) & ~(1 << 2); // freerunning mode
    
    // enable global interrupts 
    sei();
    
    // initialize the UART with 9600 BAUD
    USART_init(MYUBRR);
    
    // redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
    
    // Joystick value variables
    volatile uint16_t x_axis = 0;
    volatile uint16_t y_axis = 0;
    volatile bool b_switch = 1;
    
    /* send message to master and receive message from master */
    while (1)
    {
        joystick_data(&x_axis, &y_axis, &b_switch);    
       
        // convert results to char for USART printing
        char x_axis_char[16];
        char y_axis_char[16];
        char switch_char[16];
        
        itoa(x_axis,x_axis_char,10);
        itoa(y_axis,y_axis_char,10);
        itoa(b_switch,switch_char,10);
        
        printf("Joystick data: ");
        printf(x_axis_char);
        printf("\t");
        printf(y_axis_char);
        printf("\t");
        printf(switch_char);
        printf("\n");
        
    }
    
    return 0;
}
