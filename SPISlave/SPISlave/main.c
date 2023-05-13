/*
 * uno_SPI_slave_basics.c
 *
 * Created: 4.7.2019 8:50:10
 * Author : Glutex
 */ 

/* ATmega328p (UNO) is slave */

#define F_CPU 16000000UL
#define FOSC 16000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)

#define PIR_SENSOR PD7    // Define the PIR sensor pin as PD7

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include "keypad.h"
#include <stdbool.h>
#include <string.h>

//keypad and password variables
uint8_t Keypress;
char str[2];
char password[5];
char submit[2] = "#";
char backspace[2] = "*";
    
char spi_send_data[5] = "000";
char spi_receive_data[20];

char correct_message[20] = "CCorrect pass";
char inCorrect_message[20] = "IIncorrect pass";
char givePass_message[20] = "EEnter pass";

char inCorrect_code[5] = "444";
char correct_code[5] = "333";
char givePass_code[5] = "222";

bool movementDetected = false;
bool passwordCorrect = false;


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


void init_uart(void);

void change_password(char changed_password[], size_t size){
	// Write data to eeprom
	for (uint16_t address_index = 0; address_index < size; address_index++){
		while(EECR & (1 << 1)){/* wait for the previous write operation to end */}
		
		EEAR = address_index;
		EEDR = changed_password[address_index];
		EECR |= (1 << 2); // master programming enable
		EECR |= (1 << 1); // EEPROM programming enable
	}
}

// Setup the stream functions for UART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int main(void)
{
	init_uart();
	KEYPAD_Init();
	//sei();
	
    /* set MISO as output, pin 12 (PB4)*/
    DDRB  = (1 << PB3);
    /* set SPI enable */
    SPCR  = (1 << 6);
	
	// Set PD7 as input
	DDRD &= ~(1 << PIR_SENSOR);
	
    // initialize the UART with 9600 BAUD
    USART_init(MYUBRR);
        
    // redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
	
	/* UNO has 1 kB (1023 bytes) of EEPROM memory and MEGA 2560 has 4 kB (4096 B) */
	uint16_t memory_address_max = 32;
	char correct_password[32]; // data read from eeprom
	
	//changing the password can be done here
	char changed_password[] = "6651";
	change_password(changed_password, sizeof(changed_password));
	
	//read data from eeprom
	for (uint16_t address_index = 0; address_index < memory_address_max; address_index++){
		while(EECR & (1 << 1)){/*wait for previous write operation to end*/}
		EEAR = address_index;
		EECR |=0x01; //enable EEPROM read
		correct_password[address_index] = EEDR;
	}

    /* send message to master and receive message from master */
    while (1) 
    {
		int i = 0;
		
		if (PIND & (1 << PIR_SENSOR)) {
			movementDetected = true;
			printf("DETECTED MOVEMENT, GIVE PASSWORD\n");
			//strcpy(spi_send_data, givePass_message);
			strcpy(spi_send_data, givePass_code);  // Set the spi_send_data to 222
			printf(spi_send_data);
			
		}
		
        for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
        {
            SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
            while(!(SPSR & (1 << SPIF))){;}
            spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
        }
		
        if(movementDetected == true){
			
			for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
			{
				SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
				while(!(SPSR & (1 << SPIF))){;}
				spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
			}
			//printf(spi_receive_data);
			while(passwordCorrect == false){
				printf("FLAG1");
				Keypress = KEYPAD_GetKey();
				//turns uint_8 to char
				sprintf(str, "%c", Keypress);

				if(str[0] == submit[0]){
					break;
				}
				if(str[0] == backspace[0] && i > 0){
					i--;
					password[i] = '\0';
				}
				else if(i < 4 && str[0] != backspace[0] && str[0] != submit[0]){
					password[i] = str[0];
					i++;
				}
				/*
				for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
				{
					SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
					while(!(SPSR & (1 << SPIF))){;}
					spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
				}*/
				//strcpy(spi_send_data, password);
				printf("\nPaswd: %s\n", password);

			}

			password[4] = '\0';
			printf("\nPassword entered: %s\n", password);
			//Wrong pass -> 444
			//Correct pass -> 333
			//Give pass -> 222

			printf("FLAG 2");
			if (strcmp(correct_password, password) != 0) {
				printf("Incorrect password.\n");
				strcpy(spi_send_data, inCorrect_code);
				printf(spi_send_data);
				
				for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
				{
					SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
					while(!(SPSR & (1 << SPIF))){;}
					spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
				}
				passwordCorrect = true;
			}else {
				printf("Correct password.\n");
				strcpy(spi_send_data, correct_code);
				printf("FLAG 3");
				printf("FLAG 4");
				printf(spi_send_data);
				printf("FLAG 5");
				for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
				{
					SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
					while(!(SPSR & (1 << SPIF))){;}
					spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
				}
				printf("FLAG 6");
				passwordCorrect = true;
				printf("FLAG 7");
			}
		}
		
		//
    }

    return 0;
}

