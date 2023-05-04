/* uno is master */

#define F_CPU 16000000UL
#define FOSC 16000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>

static void USART_init(uint16_t ubrr){
    /* Set baud rate in the USART Baud Rate Registers (UBRR) */
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    
    /* Enable receiver and transmitter on RX0 and TX0 */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); 
    
    /* Set frame format: 8 bit data, 2 stop bit */
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00);
}

static void USART_Transmit(unsigned char data, FILE *stream){
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0))){;}
    
    /* Puts the data into a buffer, then sends/transmits the data */
    UDR0 = data;
}

static char USART_Receive(FILE *stream){
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0))){;}
    
    /* Get the received data from the buffer */
    return UDR0;
}

// Setup the stream functions for UART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int main(void){
    /* set SS, MOSI and SCK as output, pins 16 (PB2), 17 (PB3) and 19 (PB5) */
    DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB5); // SS as output
    /* set SPI enable and master/slave select, making MEGA the master */
    SPCR |= (1 << 6) | (1 << 4);
    /* set SPI clock rate to 1 MHz */
    SPCR |= (1 << 0);

    // initialize the UART with 9600 BAUD
    USART_init(MYUBRR);
        
    // redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
    
    unsigned char spi_send_data[20] = "IDLE\n";
    unsigned char spi_receive_data[20];
	
	
	
	lcd_init(LCD_DISP_ON);
	
	int16_t test_number = 1023;
	char test_char_array[16];
	itoa(test_number, test_char_array, 10);
	
	lcd_clrscr();
	
	lcd_puts("Hello World!");
	
	lcd_gotoxy(0,1);
	lcd_puts(test_char_array);
	
	
	
	
	
	
    
    /* send message to slave and receive message from slave */
    while (1) {
        /* send byte to slave and receive a byte from slave */
        PORTB &= ~(1 << PB2); // SS LOW
            
        for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++){
                
            SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
            _delay_us(5);
            while(!(SPSR & (1 << SPIF))){/* wait until the transmission is complete */;}
            _delay_us(5);
            spi_receive_data[spi_data_index - 1] = SPDR; // receive byte from the SPI data register     
        }
            
        PORTB |= (1 << PB2); // SS HIGH
        printf(spi_receive_data);
        _delay_ms(500);
    } 
    return 0;
}