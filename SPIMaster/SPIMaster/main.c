#define F_CPU 16000000UL
#define FOSC 16000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define LCD_RS 2
#define LCD_EN 3
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7

int f_buzz = 850;
int prescaler = 1;
unsigned char spi_send_data[20] = "";
unsigned char spi_receive_data[20];

//Functions to use the LCD
void LCD_Command(unsigned char cmnd)
{
	PORTD = (PORTD & 0x0F) | (cmnd & 0xF0); // Sending upper nibble
	PORTD &= ~(1<<LCD_RS); // RS = 0 for command
	PORTD |= (1<<LCD_EN); // Enable pulse
	_delay_us(1);
	PORTD &= ~(1<<LCD_EN);
	_delay_us(200);

	PORTD = (PORTD & 0x0F) | (cmnd << 4); // Sending lower nibble
	PORTD |= (1<<LCD_EN);
	_delay_us(1);
	PORTD &= ~(1<<LCD_EN);
	_delay_ms(2);
}

void LCD_Data(unsigned char data)
{
	PORTD = (PORTD & 0x0F) | (data & 0xF0); // Sending upper nibble
	PORTD |= (1<<LCD_RS); // RS = 1 for data
	PORTD |= (1<<LCD_EN);
	_delay_us(1);
	PORTD &= ~(1<<LCD_EN);
	_delay_us(200);

	PORTD = (PORTD & 0x0F) | (data << 4); // Sending lower nibble
	PORTD |= (1<<LCD_EN);
	_delay_us(1);
	PORTD &= ~(1<<LCD_EN);
	_delay_ms(2);
}

void LCD_Init()
{
	_delay_ms(20); // LCD Power ON delay always >15ms

	// Initialization sequence
	LCD_Command(0x02); // Initialize LCD in 4-bit mode
	LCD_Command(0x28); // 2 Lines, 5x7 matrix
	LCD_Command(0x0C); // Display on, cursor off
	LCD_Command(0x06); // Increment cursor (shift cursor to right)
	LCD_Command(0x01); // Clear display screen
	_delay_ms(2);
}

void LCD_Print(char *str)
{
	while(*str != '\0')
	{
		LCD_Data(*str);
		str++;
	}
}

void LCD_Print_Row(char *str, uint8_t row)
{
	// Set cursor position based on the row parameter
	if (row == 0) {
		LCD_Command(0x80); // Set cursor to first row
		} else {
		LCD_Command(0xC0); // Set cursor to second row
	}

	// Print the string
	while(*str != '\0')
	{
		LCD_Data(*str);
		str++;
	}
}

void LCD_Clear()
{
	LCD_Command(0x01); // Clear display screen
	_delay_ms(2);
}

static void USART_init(uint16_t ubrr)
{
    /* Set baud rate in the USART Baud Rate Registers (UBRR) */
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    
    /* Enable receiver and transmitter on RX0 and TX0 */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); 
    
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
    
    /* Puts the data into a buffer, then sends/transmits the data */
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

//Debugging function
void init_uart(void);

int TOP_value (int f_buzz, int scale)
{
	long f_cp = F_CPU;
	int TOP=f_cp/(2*f_buzz*scale);
	return TOP;
}

//Interrupt for the timer
ISR(TIMER1_COMPA_vect)
{
	TCNT1 = 0;
}

void alarm(int f_buzz, int prescaler) {
	/* Set pin 9 as output pin */
	DDRB |= (1 << PB1);
	/* Enable interrupt */
	sei();
	
	/* Reset timer 1 to prevent undesired actions */
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	
	/* Set compare output mode (Compare Output A Mode) */
	TCCR1A |= (1 << COM1A0);
	
	/* Set wave generation mode to mode 9 (Wave Generation Mode)*/
	TCCR1A |= (1 << WGM10);
	TCCR1B |= (1 << WGM13);
	
	/* Enable interrupts based on output comparison (Output Compare A Match Interrupt Enable) */
	TIMSK1 |= (1 << OCIE1A);
	/* Set frequency */
	OCR1A = TOP_value(f_buzz, prescaler);
	
	while (1) {
		TCCR1B |= (1 << CS11);
		int i;
		for (i=1;i<10;i=i+1) {
			_delay_ms(20);
		OCR1A = TOP_value(f_buzz*i, prescaler);}
		for (i=10;i>0;i=i-1) {
			_delay_ms(20);
			OCR1A = TOP_value(f_buzz*i, prescaler);
		}
	}
}

void time_is_up(){
	LCD_Clear();
	LCD_Clear("ALERT!");
	_delay_ms(2000);
	//##################################################
	//call alarm, sound the buzzer
	//##################################################
	
	//alarm(f_buzz, prescaler);
};

//Function that is triggered when the slave-board sends a 222 code (movement detected)
bool start_timer(){
	LCD_Init();

	int count = 30; // start timer at 30 seconds
	LCD_Print_Row("GIVE PASS", 0);
	while(count >= 0) {
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "Time: %02d sec", count); // format timer value
		LCD_Print_Row(buffer, 1);
		_delay_ms(1000); // wait for 1 second
		count--; // decrement timer
		
		
		 PORTB &= ~(1 << PB2); // SS LOW
		 
		 for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
		 {
			 SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
			 while(!(SPSR & (1 << SPIF))){;}
			 spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
		 }
		 
		 PORTB |= (1 << PB2); // SS HIGH
		
		if (spi_receive_data[0] == '3') {
			return true;  //If user enters the correct pass, return true
		} else if (spi_receive_data[0] == '4') {
			LCD_Clear();
			LCD_Print("Incorrect pass!");
			_delay_ms(3000);
			return false; //If user enters the incorrect pass, return false
		}
		
	}

	LCD_Clear(); // clear the display
	return false; //If user doesnt enter the correct password on time, return false

}

// Setup the stream functions for UART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int main(void)
{
	// Set PORTD pins 2-7 as output
	DDRD |= (1<<LCD_RS) | (1<<LCD_EN) | (1<<LCD_D4) | (1<<LCD_D5) | (1<<LCD_D6) | (1<<LCD_D7);
	
	init_uart();
	LCD_Init();
	LCD_Clear();
	LCD_Print("SCANNING...");

    /* set SS, MOSI and SCK as output, pins 53 (PB0), 51 (PB2) and 52 (PB1) */
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
	
    /* send message to slave and receive message from slave */
    while (1) 
    {
        
        PORTB &= ~(1 << PB2); // SS LOW
           
        for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
        {
            SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
				 while(!(SPSR & (1 << SPIF))){;}
            spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register
        }
            
        PORTB |= (1 << PB2); // SS HIGH
		printf(spi_receive_data);
		
		bool passwordCorrect;
		
		//If the board receives a code 222 (movement detected) from the slave
		if (spi_receive_data[0] == '2') {
			printf(spi_receive_data);
			LCD_Clear();
			passwordCorrect = start_timer();
			LCD_Clear();
			if (passwordCorrect == false){
				LCD_Print_Row("ALERT!", 0);
				LCD_Print_Row("POLICE!", 1);
				alarm(f_buzz, prescaler);
			} else{
				LCD_Clear();
				LCD_Print_Row("Correct pass", 0);
			}
		} else if (spi_receive_data[0] == '1'){
			LCD_Clear();
			LCD_Print_Row("RESET", 0);
		}
	
	}
    return 0;
}