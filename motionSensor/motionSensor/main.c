#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define TRIGGER_PIN  PB0  // define trigger pin as PB0 (pin 8)
#define ECHO_PIN     PB1  // define echo pin as PB1 (pin 9)
#define LED_PIN      PD3  // define LED pin as PD3 (pin 3)

void init() {
	// set trigger pin as output and echo pin as input
	DDRB |= (1 << TRIGGER_PIN);
	DDRB &= ~(1 << ECHO_PIN);

	// set LED pin as output
	DDRD |= (1 << LED_PIN);

	// set timer1 prescaler to 8 and enable overflow interrupt
	TCCR1B |= (1 << CS11);
	TIMSK1 |= (1 << TOIE1);
}

void init_uart(void);

int main(void) {
	uint16_t time_us;
	uint32_t distance_cm = 0;
	uint32_t prev_distance_cm = 0;
	(void)time_us; // suppress unused variable warning
	char buffer[20];
	(void)buffer; // suppress unused variable warning

	init();
	init_uart();
	printf("Starting the scan...");

	while (1) {
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
		if (prev_distance_cm == 100) {
			prev_distance_cm = distance_cm;
		}

		// check if the difference between the current distance and the previous
		// distance is greater than a threshold (10 cm in this case)
		if (abs(distance_cm - prev_distance_cm) > 10) {
			// turn on the LED
			PORTD |= (1 << LED_PIN);
			printf("MOVEMENT");
			} else {
			// turn off the LED
			PORTD &= ~(1 << LED_PIN);
		}

		// update the previous distance with the current distance
		prev_distance_cm = distance_cm;
	}
}

// timer1 overflow interrupt service routine
ISR(TIMER1_OVF_vect) {
	// reset timer1
	TCNT1 = 0;
}
