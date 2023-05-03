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
#define UBRR_VAL (F_CPU/(16UL*BAUD)-1)

// Define the buzzer pin
#define BUZZER_PIN PB3

// Define the frequency and duty cycle values
#define FREQ_1 1000
#define DUTY_CYCLE_1 50
#define FREQ_2 2000
#define DUTY_CYCLE_2 75

// Define the prescaler value
#define PRESCALER 8

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "keypad.h"




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
unsigned char g_spi_send_data[20] = "";
unsigned char movementSpotted[20] = "MOVEMENT\n";
unsigned char noMovementSpotted[20] = "";
unsigned char g_spi_receive_data[20];
volatile bool g_b_is_transfer_complete = 0;
volatile int8_t g_spi_index = 0;
volatile int8_t g_spi_receive_index = 0;

//keypad and password variables
uint8_t Keypress;
char str[2];
char password[5];

// Define the OCR values for the two frequencies
uint16_t ocr1 = F_CPU / (2 * PRESCALER * FREQ_1) - 1;
uint16_t ocr2 = F_CPU / (2 * PRESCALER * FREQ_2) - 1;

bool movement = false;

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
        }
        
    }
    
}

// Initialize the buzzer
void init_buzzer() {
	// Set the buzzer pin as an output
	DDRB |= (1 << BUZZER_PIN);
	
	// Configure Timer/Counter1 for CTC mode
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS11);
	
	// Set the initial Compare Match value
	OCR1A = ocr1;
	
	// Enable Timer/Counter1 Compare Match interrupt
	TIMSK1 = (1 << OCIE1A);
}

// Interrupt service routine for Timer/Counter1 Compare Match
ISR(TIMER1_COMPA_vect) {
	static uint8_t state = 0;
	
	// Switch between the two frequencies
	if (state == 0) {
		OCR1A = ocr2;
		state = 1;
		} else {
		OCR1A = ocr1;
		state = 0;
	}
	
	// Adjust the duty cycle based on the current state
	if (state == 0) {
		// Set duty cycle 1
		PORTB |= (1 << BUZZER_PIN);
		_delay_us(DUTY_CYCLE_1);
		} else {
		// Set duty cycle 2
		PORTB |= (1 << BUZZER_PIN);
		_delay_us(DUTY_CYCLE_2);
	}
	
	// Turn off the buzzer
	PORTB &= ~(1 << BUZZER_PIN);
}


int main(void)
{
	KEYPAD_Init();
	init_buzzer();
	sei();
    /* set MISO as output, pin 12 (PB4)*/
    DDRB  |= (1 << PB3);
    /* set SPI enable (SPE) and interrupt enable (SPIE) */
    SPCR  |= (1 << 6) | (1 << 7);
    SPDR = 0;
	
	// set up serial communication
	UBRR0H = (uint8_t)(UBRR_VAL >> 8);
	UBRR0L = (uint8_t)UBRR_VAL;
	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	//Set up PIR sensor pin
	DDRD &= ~(1 << PD7);
    
    // enable global interrupts
    sei();
    
    // initialize the UART with 9600 BAUD
    USART_init(MYUBRR);
    
    // redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
    
    /* send message to master and receive message from master */
    while (1){
	    
		//for loop for getting a 4 long password
		for(int i = 0; i < 4;i++){
			Keypress = KEYPAD_GetKey();
			//turns uint_8 to char
			sprintf(str, "%c", Keypress);
			password[i] = str[0];
		}
		password[4] = '\0';
		printf("\nPassword entered: %s\n", password);
		
		if(password != "1234"){
			init_buzzer();
		}
		
		
	    if (PIND & (1 << PD7)) {
		    strcpy(g_spi_send_data, movementSpotted);
		    } else{
		    strcpy(g_spi_send_data, noMovementSpotted);
	    }
	    

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
