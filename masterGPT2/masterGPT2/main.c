#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>


void initSPI_master()
{
	DDRB = (1 << PB2) | (1 << PB1); // MOSI and SCK pins as output
	DDRB &= ~(1 << PB0); // MISO pin as input
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); // Enable SPI as master with f_osc/16
}
void init_uart(void);

uint8_t receiveSPI()
{
	SPDR = 0x00;
	while (!(SPSR & (1 << SPIF)));
	return SPDR;
}
int main()
{

	init_uart();
	initSPI_master();

	//stdout = fdevopen((void*)0, stdout);
	printf("Starting scan...");
	while (1)
	{
		
		
		uint8_t data = receiveSPI();

		if (data == 1)
		{
			printf("MOVEMENT\n");
			_delay_ms(500); // Wait before detecting again
		}
		
	}
	
}
