#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

int main(void)
{
	// set row pins as output and column pins as input
	DDRC = 0xF0;
	// enable pull-up resistors on column pins
	PORTC = 0x0F;
	
	while(1)
	{
		// scan the keypad
		int row, col;
		for(row = 0; row < 4; row++)
		{
			// set one row pin low at a time
			PORTC = ~(1 << (row + 2));
			// check column pins for low signal
			for(col = 0; col < 4; col++)
			{
				if(!(PINC & (1 << (col + 4))))
				{
					// button pressed
					printf("Button pressed: %c\n", '1' + row * 4 + col);
					// wait for button release
					while(!(PINC & (1 << (col + 4))));
				}
			}
		}
		// delay to debounce buttons
		_delay_ms(10);
	}
	return 0;
}
