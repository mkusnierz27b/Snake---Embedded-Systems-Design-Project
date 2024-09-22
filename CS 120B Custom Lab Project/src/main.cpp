/*         Name & E-mail: Matthew Kusnierz mkusn002@ucr.edu


 *         Discussion Section: 23


 *         Assignment: Custom Lab Project Demo 1


 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.


 *


 *         Demo Link: https://youtu.be/o8S1-OOmCOU


 */
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include "LCD.h"

#define NUM_TASKS 4 //TODO: Change to the number of tasks being used

//Global variables for communication between tasks
bool powerState;
bool loseState;
unsigned char score;


//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
const unsigned long TASK1_PERIOD = 17;
const unsigned long TASK2_PERIOD = 17;
const unsigned long TASK3_PERIOD = 17;
const unsigned long TASK4_PERIOD = 500;


const unsigned long GCD_PERIOD = 1;//TODO:Set the GCD Period

task tasks[NUM_TASKS]; // declared task array with 5 tasks


void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

//TODO: Create your tick functions for each task

enum task1_states {task1_init, task1_off, task1_waitOn, task1_on, task1_waitOff} task1_state; //On and off
enum task2_states {task2_init, task2_off, task2_waitOn, task2_on, task2_waitOff} task2_state; //Lose On and off
enum task3_states {task3_init, task3_off, task3_start,task3_PauseWait, task3_pause, task3_lose} task3_state; //LCD display
enum task4_states {task4_wait, task4_note1, task4_note2, task4_note3, task4_note4, task4_note5, task4_note6} task4_state; //Passive buzzer



//IR remote, turns game on and off
int task1_tick_function(int state) {
    switch(state) {
        case task1_init:
            
            powerState = 0;
            loseState = 0;
            state = task1_off;
            break;

        case task1_off:
            if(GetBit(PINC, 4)) {
                state = task1_waitOn;
                powerState = 1;
                loseState = 0;
                PORTB = SetBit(PORTB, 0, 1);
            }

            break;

        case task1_waitOn:
            if(!GetBit(PINC, 4)) state = task1_on;
            break;

        case task1_on:
            if(GetBit(PINC, 4)) {
                state = task1_waitOff;
                powerState = 0;
                loseState = 0;
                PORTB = SetBit(PORTB, 0, 0);
            }

            break;

        case task1_waitOff:
            if(!GetBit(PINC, 4)) state = task1_off;
            break;

        default:
            state = task1_init;
            break;
    }

    return state;
}

int task2_tick_function(int state) {
    if(!powerState) state = task2_init;
    switch(state) {
        case task2_init:
            loseState = 0;
            state = task2_off;
            break;

        case task2_off:
            if(GetBit(PINC, 5)) {
                state = task2_waitOn;
                loseState = 1;
            }

            break;

        case task2_waitOn:
            if(!GetBit(PINC, 5)) state = task2_on;
            break;

        case task2_on:
            if(GetBit(PINC, 5)) {
                state = task2_waitOff;
                loseState = 0;
            }

            break;

        case task2_waitOff:
            if(!GetBit(PINC, 5)) state = task2_off;
            break;

        default:
            state = task2_init;
            break;
    }

    return state;
}

//LCD display
int task3_tick_function(int state) {
    if(!powerState) state = task3_init;

    switch(state) {
        case task3_init:
            state = task3_off;
            score = 0;
            break;

        case task3_off:
            serial_println("off");
            if(powerState) {
                state = task3_start;
                lcd_write_str("SCORE: ");
            }
            break;

        case task3_start:
            serial_println("start");
            if(loseState) {
                state = task3_lose;
                lcd_clear();
                lcd_write_str("YOU LOSE");
                lcd_goto_xy(1,0);
                lcd_write_str("FINAL SCORE: ");
                lcd_write_character((score % 100) + 0x30);
                lcd_write_character((score % 10) + 0x30);
            }
            else if(!GetBit(PINC, 2)) {
                state = task3_PauseWait;
                lcd_clear();
                lcd_write_str("PAUSED");
            }
            break;

        case task3_PauseWait:
            if(GetBit(PINC, 2)) state = task3_pause;
            break;

        case task3_pause:
            serial_println("pause");
            if(loseState) {
                state = task3_lose;
                lcd_clear();
                lcd_write_str("YOU LOSE");
                lcd_goto_xy(1,6);
                lcd_write_str("FINAL SCORE: ");
                lcd_write_character((score % 100) + 0x30);
                lcd_write_character((score % 10) + 0x30);
            }
            else if(ADC_read(0) > 600 || ADC_read(0) < 400 || ADC_read(1) > 600 || ADC_read(1) < 400) {
                state = task3_start;
                lcd_clear();
                lcd_write_str("SCORE: ");
            }    
            break;

        case task3_lose:
            serial_println("lose");
            break;

        default:
            state = task3_init;
            break;
    }

    switch(state) {
        case task3_init:
            break;

        case task3_off:
            lcd_clear();
            break;

        case task3_start:
            lcd_goto_xy(0,6);
            lcd_write_character((score % 100) + 0x30);
            lcd_write_character((score % 10) + 0x30);
            break;

        case task3_PauseWait:
            break;

        case task3_pause:
            break;

        case task3_lose:
            break;

        default:
            break;
    }

    return state;
}

int task4_tick_function(int state) {
    switch(state) {
        case task4_wait:
            if(loseState) {
                state = task4_note1;
                OCR1A = 128;
            }
            else OCR1A = 255;
            break;

        case task4_note1:

            TCCR1B = (TCCR1B & 0xF8) | 0x05;//set prescaler to 1024
            state = task4_note2;
            break;

        case task4_note2:

            TCCR1B = (TCCR1B & 0xF8) | 0x04;//set prescaler to 1024
            state = task4_note3;
            break;

        case task4_note3:

            TCCR1B = (TCCR1B & 0xF8) | 0x03;//set prescaler to 1024
            state = task4_note4;
            break;

        case task4_note4:

            TCCR1B = (TCCR1B & 0xF8) | 0x02;//set prescaler to 1024
            state = task4_note5;
            break;


        case task4_note5:

            TCCR1B = (TCCR1B & 0xF8) | 0x03;//set prescaler to 1024
            state = task4_note6;
            break;

        case task4_note6:

            TCCR1B = (TCCR1B & 0xF8) | 0x02;//set prescaler to 1024
            state = task4_wait;
            break;

        default:
            state = task4_wait;
            break;
    }

    return state;
}

int main(void) {
    //TODO: initialize all your inputs and ouputs
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0x00; PORTC = 0xFF;
    DDRD = 0xFF; PORTD = 0x00;

    ADC_init();   // initializes ADC
    lcd_init();
    serial_init(9600);

    //TODO: Initialize the buzzer timer/pwm(timer0)
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11); //CS11 sets the prescaler to be 8
    ICR1 = 256;
    OCR1A = 255;

    //TODO: Initialize tasks here
    // e.g. 
    tasks[0].period = TASK1_PERIOD;
    tasks[0].state = task1_init;
    tasks[0].elapsedTime = 17;
    tasks[0].TickFct = &task1_tick_function;

    tasks[1].period = TASK2_PERIOD;
    tasks[1].state = task2_init;
    tasks[1].elapsedTime = 17;
    tasks[1].TickFct = &task2_tick_function;

    tasks[2].period = TASK3_PERIOD;
    tasks[2].state = task3_init;
    tasks[2].elapsedTime = 17;
    tasks[2].TickFct = &task3_tick_function;

    tasks[3].period = TASK4_PERIOD;
    tasks[3].state = task4_wait;
    tasks[3].elapsedTime = 500;
    tasks[3].TickFct = &task4_tick_function;


    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}