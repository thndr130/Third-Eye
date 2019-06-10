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

unsigned char motion = 0;
enum outputStates {O_init, O_receive} O_state;
	
int OutTick(int O_state){
	switch(O_state){
		case O_init:
			O_state = O_receive;
			break;
		case O_receive:
			O_state = O_receive;
			break;
		default:
			O_state = O_init;
			break;
	}
	switch(O_state){
		case O_init:
			motion = 0;
			break;
		case O_receive:
			if(USART_HasReceived(0)){
				motion = USART_Receive(0);
				//LCD_ClearScreen();
				_delay_ms(100);
			if (motion==1){
					LCD_DisplayString(1, "Motion Detected");
			}
			else {
				LCD_DisplayString(1, "No Motion");
			}
				_delay_ms(100);
			}
			break;
		default:
			break;
	}
	return O_state;
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	
	const unsigned short numTasks = 1;
	task tasks[numTasks];
	unsigned char i = 0;
	
	tasks[i].state = 0;
	tasks[i].period = 50;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &OutTick;
	
	LCD_init();
	LCD_ClearScreen();
	TimerSet(50);
	TimerOn();
	initUSART(0);
    
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
		    tasks[i].elapsedTime += 50;
			//_delay_ms(100);
	    }
	    while(!TimerFlag);
	    TimerFlag = 0;
    }
}

