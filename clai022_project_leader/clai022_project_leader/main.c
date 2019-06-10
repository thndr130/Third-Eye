/*
 *	Partner 1 Name & E-mail: Catherine Lai clai022@ucr.edu
 *	Lab Section: 28
 *	Assignment: Final Project
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */ 

#include <avr/io.h>
#include "io.c"
#include "timer.h"
#include "scheduler.h"
#include "usart.h"
#include <util/delay.h>

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //Adjust ASPS2-0 for high resolution
}

//room temperature: 25.7C 78.2F = 1+2+4+8+16+128 = 159
//100F = 1+2+4+8+16+32+128 = 188
//50F = 1+2+4+128= 135

// 0.954 hz is lowest frequency possible with this function,
// based on settings in PWM_on()
// Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT3 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

unsigned char input = 0;
unsigned short motion = 0;
unsigned short temp = 159; 
unsigned short low_end = 135; //50F
unsigned short high_end = 188; //100F
unsigned short light = 0x00;
unsigned short sound = 0;
enum sendStates {S_init, S_send} S_state;
enum testTemps {T_init, T_test} T_state;
enum testSounds {O_init, O_test} O_state;

int sendState(int S_state){
	switch(S_state){
		case S_init:
			S_state = S_send;
			break;
		case S_send:
			S_state = S_send;
			break;
		default:
			S_state = S_init;
			break;
	}
	switch(S_state){
		case S_init:
			motion = 0;
			break;
		case S_send:
			motion = PINB & 0x01;
			if (USART_IsSendReady(0)){
				USART_Send(motion, 0);
			}
			break;
		default:
			break;
	}
	return S_state;
}

int testTemp(int T_state){
	switch(T_state){
		case T_init:
			T_state = T_test;
			break;
		case T_test:
			T_state = T_test;
			break;
		default:
			T_state = T_init;
			break;
	}
	switch(T_state){
		case T_init:
			temp = 159;
			set_PWM(0);
			break;
		case T_test:
			temp = ADC;
			if (temp >= high_end || temp < low_end){
				//turn on speaker
				set_PWM(261.63);
			}
			else {
				//turn off speaker
				set_PWM(0);
			}
			break;
		default:
			break;
	}
	return T_state;
}

int testSound(int O_state){
	switch(O_state){
		case O_init:
			O_state = O_test;
			break;
		case O_test:
			O_state = O_test;
			break;
		default:
			O_state = O_init;
			break;
	}
	switch(O_state){
		case O_init:
			sound = 0;
			break;
		case O_test:
			if ((PINC & 0x01) == 1){
				PORTC = 0xF0;
				_delay_ms(1000);
			}
			else {
				PORTC = 0x00;	
			}
			break;
		default:
			break;
	}
	return O_state;
}

int main(void)
{
	DDRA = 0x00; PORTA = 0x00;
    DDRB = 0xF0; PORTB = 0x0F;
	DDRC = 0xF0; PORTC = 0x0F;
	
	const unsigned short numTasks = 3;
	task tasks[numTasks];
	unsigned char i = 0;
	
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &sendState;
	
	i++;
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &testTemp;
	
	i++;
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &testSound;
	
	TimerSet(100);
	TimerOn();
	initUSART(0);
	PWM_on();
	ADC_init();
		
    while (1) 
    {// Scheduler code
	    for ( i = 0; i < numTasks; i++ ) {
		    // Task is ready to tick
		    if ( tasks[i].elapsedTime >= tasks[i].period ) {
			    // Setting next state for task
			    tasks[i].state = tasks[i].TickFct(tasks[i].state);
			    // Reset the elapsed time for next tick.
			    tasks[i].elapsedTime = 0;
		    }
		    tasks[i].elapsedTime += 100;
	    }
	    while(!TimerFlag);
	    TimerFlag = 0;
    }
}

