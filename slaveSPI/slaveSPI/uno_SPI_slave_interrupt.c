/*
 * uno_SPI_slave_interrupt.c
 *
 * Created: 4.7.2019 8:50:36
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

#define TRIGGER_PIN  PB0  // define trigger pin as PB0 (pin 8)
#define ECHO_PIN     PB1  // define echo pin as PB1 (pin 9)
#define LED_PIN      PD3  // define LED pin as PD3 (pin 3)

bool movementDetected = false;

static void USART_init(uint16_t ubrr) // unsigned int
{
    /* Set baud rate in the USART Baud Rate Registers (UBRR) */
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    
    /* Enable receiver and transmitter on RX0 and TX0 */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //NOTE: the ATmega328p has 1 UART: 0
    
    /* Set frame format: 8 bit data, 2 stop bit */
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00);
    
}

static void USART_Transmit(unsigned char data, FILE *stream)
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0)))
    {
        ;
    }
    
    /* Put the data into a buffer, then send/transmit the data */
    UDR0 = data;
}

static char USART_Receive(FILE *stream)
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

// variables for storing data used in SPI communication
unsigned char g_spi_send_data[20] = "slave to master\n";
unsigned char g_spi_receive_data[20];
volatile bool g_b_is_transfer_complete = 0;
volatile int8_t g_spi_index = 0;
volatile int8_t g_spi_receive_index = 0;

/* use interrupts to send receive message */
ISR (SPI_STC_vect)
{   
    unsigned char spi_interrupt_byte = SPDR;
    unsigned char transfer_end_check = '\n';
    SPDR = g_spi_send_data[g_spi_index];
    g_spi_index++;
    
    if(g_spi_receive_index < sizeof(g_spi_receive_data))
    {
        g_spi_receive_data[g_spi_receive_index++] = spi_interrupt_byte;
        
        /* check if the received byte == '\n' */
        if (transfer_end_check == spi_interrupt_byte)
        {
            g_b_is_transfer_complete = 1;
			printf("RECEIVED");
        }
        
    }
    
}

void init() {
	// set trigger pin as output and echo pin as input
	DDRB |= (1 << TRIGGER_PIN);
	DDRB &= ~(1 << ECHO_PIN);

	// set timer1 prescaler to 8 and enable overflow interrupt
	TCCR1B |= (1 << CS11);
	TIMSK1 |= (1 << TOIE1);
}

void init_uart(void);

int main(void)
{
	uint16_t time_us;
	uint32_t distance_cm = 0;
	uint32_t prev_distance_cm = 0;
	(void)time_us; // suppress unused variable warning
	char buffer[20];
	(void)buffer; // suppress unused variable warning
	
	init();
	init_uart();
	printf("Starting the scan...");
	
    /* set MISO as output, pin 12 (PB4)*/
    DDRB  |= (1 << PB4);
    /* set SPI enable (SPE) and interrupt enable (SPIE) */
    SPCR  |= (1 << 6) | (1 << 7);
    SPDR = 0;
    
    // enable global interrupts
    sei();
    
    // initialize the UART with 9600 BAUD
    USART_init(MYUBRR);
    
    // redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
    
    /* send message to master and receive message from master */
    while (1) 
    {
		//printf("%d\n", movementDetected);
		// send a 10us pulse to the trigger pin to start a measurement
		PORTB |= (1 << TRIGGER_PIN);
		_delay_us(10);
		PORTB &= ~(1 << TRIGGER_PIN);
		
		// wait for the echo pin to go high
		while (!(PINB & (1 << ECHO_PIN)));

		// start timer1
		TCNT1 = 0;

		// wait for the echo pin to go low
		while (PINB & (1 << ECHO_PIN));

		// stop timer1 and calculate the time it took for the sound to travel
		// from the sensor to the object and back
		time_us = TCNT1 * 0.5;

		// calculate the distance in centimeters
		distance_cm = time_us / 58;

		// initialize prev_distance_cm to the first measured distance value
		if (prev_distance_cm == 0) {
			prev_distance_cm = distance_cm;
		}

		// check if the difference between the current distance and the previous
		// distance is greater than a threshold (10 cm in this case)

		if (abs(distance_cm - prev_distance_cm) > 10) {
			//SEND MESSAGE TO MASTER
			//movementDetected = true;
			printf("MOVEMENT");
			//printf("%d\n", movementDetected);
		}

		// update the previous distance with the current distance
		prev_distance_cm = distance_cm;
		
		
		
        if(1 == g_b_is_transfer_complete)
        {
            printf(g_spi_receive_data);
            g_b_is_transfer_complete = 0;
            g_spi_index = 0;
            g_spi_receive_index = 0;

        }
        
    }
    
    return 0;
}

// timer1 overflow interrupt service routine
ISR(TIMER1_OVF_vect) {
	// reset timer1
	TCNT1 = 0;
}