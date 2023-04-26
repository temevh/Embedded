#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define TRIG_PIN PB0
#define ECHO_PIN PB1
#define SS_PIN PB2

volatile uint16_t distance = 0;
volatile uint8_t flag = 0;

void init_uart(void);

void initSPI_slave()
{
    DDRB = (1 << PB4); // MISO pin as output
    SPCR = (1 << SPE); // Enable SPI
}

void init()
{
    // set trigger pin as output and echo pin as input
    DDRB |= (1 << TRIG_PIN);
    DDRB &= ~(1 << ECHO_PIN);

    // set timer1 prescaler to 8 and enable overflow interrupt
    TCCR1B |= (1 << CS11);
    TIMSK1 |= (1 << TOIE1);

    // set SS pin as input with pull-up resistor
    DDRB &= ~(1 << SS_PIN);
    PORTB |= (1 << SS_PIN);
}

void sendSPI(uint8_t data)
{
    // select slave
    PORTB &= ~(1 << SS_PIN);

    // send data
    SPDR = data;

    // wait for transmission to complete
    while (!(SPSR & (1 << SPIF)));

    // deselect slave
    PORTB |= (1 << SS_PIN);
}

ISR(TIMER1_OVF_vect)
{
    flag = 1;
}

ISR(TIMER1_CAPT_vect)
{
    static uint16_t start = 0;
    uint16_t stop = ICR1;

    if (flag == 1) // Rising edge detected
    {
        start = ICR1;
        flag = 0;
    }
    else // Falling edge detected
    {
        uint16_t elapsed = stop - start;
        distance = (elapsed * 0.0343) / 2; // Convert to centimeters
    }
}

int main()
{
	uint16_t time_us;
	uint32_t distance_cm = 0;
	uint32_t prev_distance_cm = 0;
	(void)time_us; // suppress unused variable warning
	char buffer[20];
	(void)buffer; // suppress unused variable warning
	
	init();
	initSPI_slave();
	//initHC_SR04();

	sei(); // Enable global interrupts
	init_uart();
	printf("Slave scanning...");

	while (1)
	{
		PORTB |= (1 << TRIG_PIN);
		_delay_us(10);
		PORTB &= ~(1 << TRIG_PIN);

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
		//printf("Distance: %lu \n", distance_cm);

		// initialize prev_distance_cm to the first measured distance value
		if (prev_distance_cm == 0) {
			prev_distance_cm = distance_cm;
		}
		
		if (abs(distance_cm - prev_distance_cm) > 170)
		{
			printf("MOVEMENT DETECTED");	
			printf("Distance: %lu \n", distance_cm);
			sendSPI(1); // Send signal to master
			distance = 0; // Reset distance
			_delay_ms(500); // Wait before detecting again
		}
		
		prev_distance_cm = distance_cm;
	}
}
