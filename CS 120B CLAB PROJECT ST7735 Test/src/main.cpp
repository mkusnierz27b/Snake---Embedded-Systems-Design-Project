/*         Name & E-mail: Matthew Kusnierz mkusn002@ucr.edu


 *         Discussion Section: 23


 *         Assignment: Custom Lab Project Final Submission


 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.


 *


 *         Demo Link: https://youtu.be/YVADGBSmJvo


 */
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
//#include "serialATmega.h"
#include "irAVR.h"
#include "game.h"
#include <math.h>

#define NUM_TASKS 3 //TODO: Change to the number of tasks being used

//Global variables for communication between tasks
bool powerState;
bool loseState;
bool reset;
decode_results results;
Game game;
bool signal;
// unsigned char counter;



//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;


//TODO: Define Periods for each task
const unsigned long TASK1_PERIOD = 500;
const unsigned long TASK2_PERIOD = 500;
const unsigned long TASK3_PERIOD = 300; //quarter = 100 bpm


const unsigned long GCD_PERIOD = findGCD(500,300);//TODO:Set the GCD Period

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

enum task1_states {task1_init, task1_off, task1_on, task1_lose} task1_state; //game
enum task2_states {task2_init, task2_off, task2_waitOff, task2_on, task2_waitOn, task2_lose} task2_state; //on and off
enum task3_states {task3_init, task3_wait, task3_lose} task3_state; //buzzer

//controls the screen and runs the game
int task1_tick_function(int state) {
    if(!powerState) state = task1_off;
    switch(state) {
        case task1_init:
            state = task1_off;
            break;

        case task1_off:
            if(powerState) {
                state = task1_on;
                game.reset();
            }
            break;

        case task1_on:
            if(reset) state = task1_off;
            if(loseState) {
                state = task1_lose;
                SPI_SEND_COMMAND(CASET);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(127);

                SPI_SEND_COMMAND(RASET);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(0x00);
                SPI_SEND_DATA(127);

                SPI_SEND_COMMAND(RAMWR);
                for(unsigned int i = 0; i < 128*128; i++) {
                    SPI_SEND_DATA(0x00);
                    SPI_SEND_DATA(0x1F);
                }

                //printing backwards because LCD is upside down
                game.draw_string(64,64,"!ESOL UOY");
                game.draw_char(64,32,(char)(game.getScore()%10+0x30));
                game.draw_char(70,32,(char)(game.getScore()%100/10+0x30));
                game.draw_char(76,32,(char)(game.getScore()%1000/100+0x30));
                game.draw_string(82,32," :EROCS");    
            }
            break;

        case task1_lose:
            if(!loseState) {
                state = task1_on;
                game.reset();
            }
            break;

        default:
            state = task1_init;
            break;
    }

    switch(state) {
        case task1_init:
            
            break;

        case task1_off:
            SPI_SEND_COMMAND(CASET);
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(127);

            SPI_SEND_COMMAND(RASET);
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(0x00);
            SPI_SEND_DATA(127);

            SPI_SEND_COMMAND(RAMWR);
            for(unsigned int i = 0; i < 128*128; i++) {
                SPI_SEND_DATA(0xFF);
                SPI_SEND_DATA(0xFF);
            }
            loseState = false;
            break;

        case task1_on:
            game.move();
            game.eat();
            game.draw();
            loseState = game.die();
            break;

        case task1_lose:
            
            break;

        default:
            break;
    }

    return state;
}

//controls the IR receiver and turns the screen off and on
int task2_tick_function(int state) {
    signal = IRdecode(&results);
    switch(state) {
        case task2_init:
            state = task2_off;
            break;

        case task2_off:
            if(signal)
                if(results.value == 0xFFA25D) state = task2_waitOff;
            
            break;

        case task2_waitOff:
            if(signal) {
                if(results.value != 0xFFFFFFFF) state = task2_on;
            }   
            else state = task2_on;

            break;

        case task2_on:
            if(signal) {
                if(results.value == 0xFFA25D) state = task2_waitOn;
                else if(results.value == 0xFF6897) {
                    state = task2_waitOff;
                    reset = true;
                }
            }
            else if(loseState) state = task2_lose;
                
            break;

        case task2_waitOn:
            if(signal) {
                if(results.value != 0xFFFFFFFF) state = task2_off;
            }
                
            else state = task2_off;
            break;

        case task2_lose:
            if(signal) {
                if(results.value == 0xFFA25D) state = task2_waitOn;
                else if(results.value == 0xFF6897) {
                    state = task2_waitOff;
                    loseState = false;
                }
            }
            break;

        default:
            state = task2_init;
            break;
    }

    switch(state) {
        case task2_init:
            reset = false;
            break;

        case task2_off:
            powerState = false;
            break;

        case task2_waitOff:
            break;

        case task2_on:
            reset = false;
            powerState = true;
            break;

        case task2_waitOn:
            break;

        case task2_lose:
            break;

        default:
            break;

    }
    IRresume();
    return state;
}

//controls the passive buzzer, plays music when the player loses
int task3_tick_function(int state) {
    static unsigned char counter;
    //Sequence of notes divided into eigth notes, number stored corresponds to a key on a piano
    unsigned char notes[74] = 
    {
        57,56,              //measure 1
        55,55,55,50,53,51,  //measure 2
        47,47,47,43,48,50,  //measure 3
        51,50,51,53,54,55,  //etc.
        56,56,56,56,55,55,
        60,60,60,58,54,55,
        58,58,56,56,56,56,
        58,58,58,56,52,53,
        56,56,55,55,55,55,
        56,56,56,55,50,51,
        55,55,53,50,53,50,
        44,43,47,48,50,51,
        50,51,55,53,56,55
    };

    switch(state) {
        case task3_init:
            state = task3_wait;
            break;

        case task3_wait:
            if(loseState) state = task3_lose;
            break;

        case task3_lose:
            if(!loseState) state = task3_wait;
            break;

        default:
            state = task3_init;
            break;
    }

    switch(state) {
        case task3_init:
            break;

        case task3_wait:
            OCR1A = 0;
            counter = 0;
            break;

        case task3_lose:
            double frequency = 440.0 * pow(2.0,(((double)notes[counter]-49.0)/12.0)); //Find frequency of note;
            ICR1 = F_CPU/(frequency * 8) - 1; //set TOP value
            OCR1A = ICR1/2; //set duty cycle to 50% (square wave)
            counter++;
            if(counter > 73) counter = 2; //repeat
    }
    return state;
}


int main(void) {
    //initialize all your inputs and ouputs
    DDRB = 0xFF; PORTB = 0x00;
    DDRC = 0x00; PORTC = 0xFF;
    DDRD = 0x7F; PORTD = 0x80;

    ADC_init();   // initializes ADC
    //serial_init(9600);

    //Initialize the IR receiver
    IRinit(&PORTD,&PIND,7);

    //Initialize the buzzer timer/pwm(timer1)
    TCCR1A |= (1 << WGM11) | (1 << COM1A1); //COM1A1 sets it to channel A
    TCCR1B |= (1 << WGM12) | (1 << WGM13) | (0x02); //sets the prescaler to be 64
    // ICR1 = 299999;
    // OCR1A = 9;

    //Setup Game
    SPI_INIT();

    SPI_SEND_COMMAND(CASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(127);

    SPI_SEND_COMMAND(RASET);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(0x00);
    SPI_SEND_DATA(127);

    SPI_SEND_COMMAND(RAMWR);
    for(unsigned int i = 0; i < 128*128; i++) {
        SPI_SEND_DATA(0xFF);
        SPI_SEND_DATA(0xFF);
    }


    //TODO: Initialize tasks here
    // e.g. 
    tasks[0].period = TASK1_PERIOD;
    tasks[0].state = task1_init;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct = &task1_tick_function;

    tasks[1].period = TASK2_PERIOD;
    tasks[1].state = task2_init;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct = &task2_tick_function;

    tasks[2].period = TASK3_PERIOD;
    tasks[2].state = task3_init;
    tasks[2].elapsedTime = 0;
    tasks[2].TickFct = &task3_tick_function;


    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}

    return 0;
}