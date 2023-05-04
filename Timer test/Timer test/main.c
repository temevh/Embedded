#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

volatile uint8_t seconds = 0;

ISR(TIMER1_COMPA_vect) // Interrupt Service Routine for Timer1 compare match
{
	seconds++; // Increment the seconds counter
}

ISR(TIMER0_OVF_vect) // Interrupt Service Routine for Timer0 overflow
{
	TCNT0 = 131; // Set Timer0 counter value to create a 1 ms delay
}

int main(void)
{
	printf("hello");
	// Set timer mode to CTC
	TCCR1B |= (1 << WGM12);

	// Set prescaler to 256
	TCCR1B |= (1 << CS12);

	// Set compare match value to 15625
	OCR1A = 15625;

	// Enable compare match interrupt
	TIMSK1 |= (1 << OCIE1A);

	// Enable global interrupts
	sei();

	// Initialize the UART interface and other peripherals here
	// For example, to initialize the UART interface for 9600 baud, use:
	UBRR0 = F_CPU/16/9600-1;
	UCSR0B |= (1 << TXEN0);
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);

	// Initialize Timer0 for a 1 ms overflow interrupt
	TCCR0A = 0;
	TCCR0B = (1 << CS01) | (1 << CS00); // Set prescaler to 64
	TIMSK0 = (1 << TOIE0); // Enable overflow interrupt
	TCNT0 = 131; // Set Timer0 counter value to create a 1 ms delay

	// Initialize the seconds counter
	seconds = 0;

	while (1)
	{
		// Your application code here

		if (seconds >= 1) // If 1 second has elapsed
		{
			printf("Time: %d seconds\n", seconds); // Print the time
			seconds = 0; // Reset the seconds counter
		}
	}

	return 0;
}
