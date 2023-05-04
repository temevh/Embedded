#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"
#include <stdlib.h>

uint16_t movement_spotted = 1;
uint16_t times_up;
uint16_t password_correct = 0;
uint16_t password_wrong = 0;
volatile uint8_t seconds = 30;
char seconds_to_string[24];
	
void time_is_up(){
	lcd_clrscr();
	lcd_puts("Times up!");
	lcd_gotoxy(0,1);
	lcd_puts("Police coming!");
	_delay_ms(2000);
	
	//##################################################
	//call alarm, sound the buzzer
	//##################################################
	
	};

int main(void){
	
	
	lcd_init(LCD_DISP_ON);
	
	int16_t test_number = 1023;
	char test_char_array[16];
	itoa(test_number, test_char_array, 10);
	
	lcd_clrscr();
	
	while(1){
		if(movement_spotted == 1){
			
			lcd_puts("Password: ");
			_delay_ms(2000);
			
			//################################################
			//start timer here
			//################################################
			
			
			//the timer should just work by putting seconds++; in there
			/*example
		
			ISR(TIMER1_COMPA_vect) // Interrupt Service Routine for Timer1 compare match
			{
				timer_counter++; // Increment the timer counter
				if (timer_counter == 1000) // If 1 second has elapsed
				{
					seconds++; // Increment the seconds counter
					timer_counter = 0; // Reset the timer counter
				
					########################
					//calls function raise time lcd
					//puts the seconds counted in the timer to the lcd
					void raise_time_lcd(){
						lcd_gotoxy(0,1);
						lcd_puts(itoa(seconds, seconds_to_string, 10));
					}
				
					###########################
				
				}
			}
		
			as the volatile uint8_t seconds = 0; is a public variable you can just call it when ever
			*/

		
		
			//##################################################
			//get keypad keys for visible password from the mega
			//##################################################
		
			//##################################################
			//example how to put the password after the password text
			//prolly need to make a trigger for pressing the different keys
			// * and the index goes back one only if it is greater than 10
			// 1-9, a-d and the index goes up
			//lcd_gotoxy(index, 0);
			//##################################################
			lcd_gotoxy(10,0);
			lcd_puts("1");
			_delay_ms(2000);
		
			lcd_gotoxy(11,0);
			lcd_puts("2");
			_delay_ms(2000);
		
		
			//The clock's run out, time's up, over, blaow!
			if(seconds >= 30){
				time_is_up();
				break;
			}
		
			//password is correct
			if(password_correct == 1){
				lcd_clrscr();
				lcd_puts("Password correct!");
				break;
			}
		
			//password is wrong
			if(password_wrong == 1){
				lcd_clrscr();
				lcd_puts("Wrong password!");
				break;
			}
		}
	}
}
