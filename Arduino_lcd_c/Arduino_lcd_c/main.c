#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"
#include <stdlib.h>
#include <stdio.h>
#include "millis.h"

//millis source https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover/12588#12588
//https://www.programming-electronics-diy.xyz/2021/01/millis-and-micros-library-for-avr.html

uint16_t movement_spotted = 1;
uint16_t times_up;
uint16_t password_correct = 0;
uint16_t password_wrong = 0;
volatile uint8_t seconds = 20;
char seconds_to_string[24];
uint16_t interval = 1000;
uint16_t lastTime = 0;
uint8_t password_index = 10;
char keypad_button[2];
uint8_t password_put = 0;

	
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
	millis_init();
	millis_pause();
	
	int16_t test_number = 1023;
	char test_char_array[16];
	itoa(test_number, test_char_array, 10);
	lcd_gotoxy(0,0);
	lcd_clrscr();
	
	
	while(1){
		if(movement_spotted == 1){
			millis_resume();
			if(password_put == 0){
				lcd_puts("Password: ");
				password_put = 1;
			}
			
			//if a second has passed
			if(millis_get() - lastTime >= interval){
				lastTime = millis_get();
				seconds -= 1; // 1000ms interval (1s)
				
				if(seconds == 9){
					lcd_gotoxy(1,1);
					lcd_puts(" ");
				}
				lcd_gotoxy(0,1);
				lcd_puts(itoa(seconds, seconds_to_string, 10));
				
				//reset and pause the millis counter
				if(seconds <= 0){
					millis_reset();
					millis_pause();
					lastTime = 0;
					seconds = 30;
					
					//changes the lcd screen to "times up"
					time_is_up();
					password_put = 0;
					break;
				}
			}
		
			//##################################################
			//get keypad keys for visible password from the mega
			//assign pressed keypad key to keypad_button variable
			//##################################################
			
			//keypad_button = 
			
			if(keypad_button == "*" && password_index > 10){
				password_index--;
			}
			else if(keypad_button != "*" && password_index < 13){
				lcd_gotoxy(password_index,0);
				lcd_puts(keypad_button);
				password_index++;
			}
		
			//password is correct
			if(password_correct == 1){
				lcd_clrscr();
				lcd_puts("Password correct!");
				password_put = 0;
				break;
			}
		
			//password is wrong
			if(password_wrong == 1){
				lcd_clrscr();
				lcd_puts("Wrong password!");
				password_put = 0;
				break;
			}
		}
	}
}
