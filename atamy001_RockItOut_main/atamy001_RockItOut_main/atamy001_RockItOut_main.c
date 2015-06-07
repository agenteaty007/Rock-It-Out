/*	atamy001_RockItOut_main.c - 06/05/15
 *	Name & E-mail:  Alberto Tam Yong (atamy001@ucr.edu)
 *	CS Login: atamy001
 *	Lab Section: 21
 *	Assignment: Custom Project
 *	Exercise Description: The Rock It Out game is a karaoke style musical game.
 *	
 *	I acknowledge all content contained herein, excluding template or example 
 *	code, is my own original work.
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "lib\bit.h"
#include "lib\timer.h"
#include "lib\io.c"
#include "lib\usart.h"

/*
	Pins connected as follows
	PORTA
		LCD
			EN A2
			RST A3
		BUTTONS
			BUTTON_1 A4
			BUTTON_2 A5
			BUTTON_3 A6
			BUTTON_4 A7
			
	PORTB
		Shift Register 1
			SER B0
			RCLOCK B1
			SRCLOCK B2
		Shift Register 2
			SER B3
			RCLOCK B4
			SRCLOCK B5
			
	PORTC
		LCD
			DATA0 to DATA7, C0 to C7
			
	PORTD
		USART
			RX D1
*/
//--------Find GCD function -------------------------------
unsigned long int findGCD (unsigned long int a,
unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ---------------------------

//--------Task scheduler data structure--------------------
// Struct for Tasks represent a running process in our
// simple real-time operating system.
/*Tasks should have members that include: state, period, a
measurement of elapsed time, and a function pointer.*/
typedef struct _task {
	//Task's current state, period, and the time elapsed
	// since the last tick
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	//Task tick function
	int (*TickFct)(int);
} task;
//--------End Task scheduler data structure----------------

//Serial
unsigned int usart_reading_total = 0;
unsigned char usart_reading1 = 0;
unsigned char usart_reading2 = 0;

void serial_input()
{
	if(USART_HasReceived())
	{
		usart_reading1 = USART_Receive();
		usart_reading2 = USART_Receive();
		
		usart_reading_total = usart_reading1;
		
		if(usart_reading2)
		{
			usart_reading_total += usart_reading2*256;
		}
	}
}
//End of Serial

//LED Matrix
unsigned char LED_matrix_pos[8][8];

//Fill 2D array with zeroes
void init_LED_matrix(unsigned char matrix[8][8])
{
	unsigned char i = 0;
	unsigned char j = 0;
	for(i = 0; i < 8 ; i++)
	{
		for(j = 0; j < 8 ; j++)
		{
			matrix[i][j] = 0;
		}
	}
}

unsigned char data = 0;
unsigned char data2 = 0;

//Second shift register
/*
	Pins connected as follows on PORTB
	SER B3
	RCLOCK B4
	SRCLOCK B5
*/
void transmit_data2(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		//PORTB = 0x08;
		PORTB = 0x00;
		// set SER = next bit of data to be sent.
		PORTB |= (((~data >> i)<<3) & (0x01<<3));
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= (0x01<<5);
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= (0x01<<4);
	// clears all lines in preparation of a new transmission
	//PORTB = 0x00;
}

//First shift register
/*
	Pins connected as follows on PORTB
	SER B0
	RCLOCK B1
	SRCLOCK B2
*/
void transmit_data(unsigned char data,unsigned char data2) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		//PORTB = 0x08;
		transmit_data2(data2);
		PORTB = 0x00;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x04;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x02;
}

//Function refreshes LED matrix row by row
void LED_matrix2(unsigned char matrix[8][8])
{
	int j;
	
	if(data > 7)
	{
		data = 0;
	}
	
	
	data2 = 0;
	for(j = 0; j<8; j++)
	{
		data2 |= (matrix[data][j] << j);
	}
	transmit_data(0x01<<data,data2);
	data++;
}

unsigned char data_char = 0x00;

//Function refreshed LED matrix LED by LED
void LED_matrix3(unsigned char matrix[8][8])
{
	if(data > 7)
	{
		data = 0;
	}
	if(data2 > 7)
	{
		data2 = 0;
		data++;
	}
	data_char = 0x00;
	data_char |= (matrix[data][data2] << data2);
	transmit_data(0x01<<data,data_char);
	data2++;
}
//End of LED Matrix and shift register control functions


//State Machines

//Strings for menu and other messages on LCD display
const unsigned char init_intro[] = "Rock It Out!";
const unsigned char init_intro2[] = "By Alberto";
const unsigned char string_menu_songs[] = "Songs";
const unsigned char string_menu_songs_hb[] = "Happy Birthday";
const unsigned char string_game_over[] = "Game Over";
const unsigned char string_points[] = "Points: ";
const unsigned char string_menu_settings[] = "Settings";
const unsigned char string_menu_live_frequency[] = "Live frequencies";
const unsigned char string_menu_instructions[] = "Instructions";
const unsigned char string_menu_credit[] = "Credits";

//Happy Birthday song variables
//Lyrics
const unsigned char hb_lyrics1[] = "Happy Birthday to you";
const unsigned char hb_lyrics2[] = "Happy Birthday dear Alberto";

const unsigned int musical_scale[] = {117,133,148,160,181,200,222,235};

//Musical notes (frequencies according to measured readings)
const unsigned int game_hb[] = {117,117,133,117,160,148,
								117,117,133,117,181,160,
								117,117,235,200,160,160,148,133,
								235,235,222,181,200,181};

//Duration per note
const unsigned int game_hb_timing[] = {1,1,2,2,2,2,
										1,1,2,2,2,2,
										1,1,2,2,1,1,1,1,
										1,1,2,2,2,2};

//Size of arrays
const unsigned int game_hb_size = 26;

//End of Happy birthday song variables

//Variables for overall game control
unsigned char tmpA = 0x00;
unsigned char startGame = 0x00;
unsigned char gameOver = 0x00;

unsigned int gameStep = 0;
unsigned int stepCount = 0;
unsigned int points = 0;
unsigned int frequency_input = 0;
unsigned int margin_error = 10;

enum SM1_States {SM1_init,SM1_init2,SM1_mainMenu,
				SM1_menu_songs_wait,SM1_menu_songs,SM1_menu_songs_wait2,
				SM1_game,SM1_gameOver_pre,SM1_gameOver,SM1_gameOver_wait,SM1_restart,
				SM1_menu_settings,SM1_menu_settings_wait,SM1_menu_settings_wait2,
				} SM1_state;

void SM1Menu()
{
	tmpA = (PINA & 0xF0);
	switch(SM1_state)
	{
		case SM1_init:
			if((tmpA & 0x80))
				SM1_state = SM1_init;
			else
				SM1_state = SM1_init2;
			break;
		case SM1_init2:
			SM1_state = SM1_mainMenu;
			break;
		case SM1_mainMenu:
			if(!(tmpA & 0x80))
				SM1_state = SM1_menu_songs_wait;
			else if(!(tmpA & 0x20))
				SM1_state = SM1_menu_settings_wait;
			else
				SM1_state = SM1_mainMenu;
			break;
		case SM1_menu_settings_wait:
			if(!(tmpA & 0x20))
				SM1_state = SM1_menu_settings_wait;
			else
				SM1_state = SM1_menu_settings;
			break;
		case SM1_menu_settings:
			if(!(tmpA & 0x10))
				SM1_state = SM1_menu_settings_wait2;
			else
				SM1_state = SM1_menu_settings;
			break;
		case SM1_menu_settings_wait2:
			if(!(tmpA & 0x10))
				SM1_state = SM1_menu_settings_wait2;
			else
				SM1_state = SM1_init2;
			break;
		case SM1_menu_songs_wait:
			if(!(tmpA & 0x80))
				SM1_state = SM1_menu_songs_wait;
			else
				SM1_state = SM1_menu_songs;
			break;
		case SM1_menu_songs:
			if(!(tmpA & 0x80))
				SM1_state = SM1_menu_songs_wait2;
			else
				SM1_state = SM1_menu_songs;
			break;
		case SM1_menu_songs_wait2:
			if(!(tmpA & 0x80))
				SM1_state = SM1_menu_songs_wait2;
			else
				SM1_state = SM1_game;
			break;
		case SM1_game:
			if(gameOver)
				SM1_state = SM1_gameOver_pre;
		
			else if(!(tmpA & 0x80))
			{
				gameOver = 1;
				SM1_state = SM1_gameOver_pre;
			}
		
			else
				SM1_state = SM1_game;
			break;
		case SM1_gameOver_pre:
			SM1_state = SM1_gameOver;
		case SM1_gameOver:
			if(!(tmpA & 0x80))
				SM1_state = SM1_gameOver_wait;
			else
				SM1_state = SM1_gameOver;
			break;
		case SM1_gameOver_wait:
			if(!(tmpA & 0x80))
				SM1_state = SM1_gameOver_wait;
			else
				SM1_state = SM1_restart;
			break;
		case SM1_restart:
			SM1_state = SM1_mainMenu;
		default:
			break;
	}
	switch(SM1_state)
	{
		case SM1_init:
			LCD_Cursor(1);
			LCD_DisplayString(1,init_intro);
			LCD_DisplayString(17,init_intro2);
			LCD_Cursor(33); //hide cursor
			break;
		case SM1_init2:
			LCD_ClearScreen();
			break;
		case SM1_mainMenu:
			LCD_Cursor(1);
			LCD_DisplayString(1,string_menu_songs);
			LCD_Cursor(33); //hide cursor
			break;
		case SM1_menu_settings_wait:
			LCD_DisplayString(1,string_menu_settings);
			break;
		case SM1_menu_settings_wait2:
			LCD_ClearScreen();
			LCD_DisplayString(1,string_menu_songs);
			break;
		case SM1_menu_songs_wait:
			LCD_ClearScreen();
			break;
		case SM1_menu_songs:
			LCD_Cursor(1);
			LCD_DisplayString(1,string_menu_songs_hb);
		
			LCD_Cursor(33); //hide cursor
			break;
		case SM1_menu_songs_wait2:
			LCD_ClearScreen();
			break;
		case SM1_game:
			startGame = 1;
		
			//Debugging
			//Show information about the game during gameplay
			LCD_ClearScreen();
			LCD_Cursor(1);
			//LCD_WriteData(gameStep + '0');
			LCD_WriteData(((gameStep%1000)/100) + '0');
			LCD_WriteData(((gameStep%100)/10)+'0');
			LCD_WriteData((gameStep%10) + '0');
		
			LCD_Cursor(5);
			LCD_WriteData(((frequency_input%1000)/100) + '0');
			LCD_WriteData(((frequency_input%100)/10)+'0');
			LCD_WriteData((frequency_input%10) + '0');
		
			LCD_Cursor(10);
			LCD_WriteData(((game_hb[gameStep]%1000)/100) + '0');
			LCD_WriteData(((game_hb[gameStep]%100)/10)+'0');
			LCD_WriteData((game_hb[gameStep]%10) + '0');
		
			LCD_Cursor(17);
			LCD_WriteData(((points%1000)/100) + '0');
			LCD_WriteData(((points%100)/10)+'0');
			LCD_WriteData((points%10) + '0');
		
			LCD_Cursor(33); //hide cursor
		
			/*
			//Lyrics
			if(gameStep == 0|| gameStep == 12|| gameStep == 20)
			if(gameStep < 12 || gameStep >= 20)
			{
				LCD_DisplayString(1,hb_lyrics1);
				LCD_Cursor(33);
			}
			else if(gameStep >=12 && gameStep < 20)
			{
				LCD_DisplayString(1,hb_lyrics2);
				LCD_Cursor(33);
			}
			*/
		
			break;
		case SM1_gameOver_pre:
			LCD_ClearScreen();
			break;
		case SM1_gameOver:
			LCD_Cursor(1);
			LCD_DisplayString(1,string_game_over);
			LCD_DisplayString(17,string_points);
		
			LCD_Cursor(25);
			LCD_WriteData(((points%1000)/100) + '0');
			LCD_WriteData(((points%100)/10)+'0');
			LCD_WriteData((points%10) + '0');
		
			LCD_Cursor(33); //hide cursor
			break;
		case SM1_gameOver_wait:
			LCD_ClearScreen();
			break;
		case SM1_restart:
			startGame = 0;
			gameOver = 0;
			gameStep = 0;
			stepCount = 0;
			points = 0;
			break;
		default:
			break;
	}
}

enum SM2_States {SM2_init,SM2_waitSerial,SM2_increasePoints,SM2_gameOver} SM2_state;

//Controls the actual gameplay
void SM2Game()
{
	switch(SM2_state)
	{
		case SM2_init:
			if(startGame == 1)
				SM2_state = SM2_waitSerial;
			else
				SM2_state = SM2_init;
			break;
		case SM2_waitSerial:
			if(gameStep >= game_hb_size || gameOver == 1)
				SM2_state = SM2_gameOver;
			else
				SM2_state = SM2_waitSerial;
			break;
		case SM2_gameOver:
			if(startGame == 0)
				SM2_state = SM2_init;
			else
				SM2_state = SM2_gameOver;
			break;
		default:
			break;
	}
	switch(SM2_state)
	{
		case SM2_waitSerial:
			//Take the current reading received at USART
			frequency_input = usart_reading_total;
			
			//Compare reading with game
			//Added threshold to account for a little bit of error from audio processing
			if(frequency_input > (game_hb[gameStep] - margin_error) && frequency_input < (game_hb[gameStep] + margin_error))
				points++; //increase points if frequencies match
			
			//Take up to 10 samples of readings per unit duration of a note on the game
			if((game_hb_timing[gameStep]*10) < stepCount)
			{
				gameStep++;
				stepCount = 0;
			}
			else
			{
				stepCount++;
			}
			break;
		case SM2_gameOver:
			gameOver = 1;
			startGame = 0;
			break;
		default:
			break;
	}
}

//Controls LED Matrix
void SM4LEDMatrix()
{
	//Only activate LED matrix during gameplay
	if(startGame == 1 || gameOver == 1)
	{
		//Show what note to play on the bottom of the LED matrix
		int i;
		for(i = 0; i < 8; i++)
		{
			if(game_hb[gameStep] == musical_scale[i])
				LED_matrix_pos[1][7-i] = 1;
			else
				LED_matrix_pos[1][7-i] = 0;
		}
		
		//Show points on the top of the LED matrix
		//Bargraph method
		if(points > 0)
		{
			LED_matrix_pos[7][8] = 1;
			for(i = 0; i < (points/10); i++)
			{
				LED_matrix_pos[7][7-i] = 1;
			}
		}
		
		LED_matrix2(LED_matrix_pos);
	}
}

//End of State Machines


int main(void)
{
	DDRA = 0x0F; PORTA = 0xF0;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x00; PORTD = 0xFF;
	
	//Initiate LCD
	LCD_init();
	LCD_ClearScreen();
	
	//Initiate USART
	initUSART();
	
	//Initiate LED Matrix
	init_LED_matrix(LED_matrix_pos);
	LED_matrix2(LED_matrix_pos);
	
	SM1_state = SM1_init;
	SM2_state = SM2_init;
	
	// Period for the tasks
	unsigned long int SMTick1_calc = 200;
	unsigned long int SMTick2_calc = 50;
	unsigned long int SMTick3_calc = 1;
	unsigned long int SMTick4_calc = 1;
	
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
	tmpGCD = findGCD(tmpGCD, SMTick3_calc);
	tmpGCD = findGCD(tmpGCD, SMTick4_calc);
	
	//Greatest common divisor for all tasks
	// or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;
	
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;
	unsigned long int SMTick4_period = SMTick4_calc/GCD;

	//Declare an array of tasks
	static task task1,task2,task3,task4;
	task *tasks[] = { &task1,&task2, &task3, &task4};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	// Task 1
	task1.state = -1;
	task1.period = SMTick1_period;
	task1.elapsedTime = SMTick1_period;
	task1.TickFct = &SM1Menu;
	
	// Task 2
	task2.state = -1;
	task2.period = SMTick2_period;
	task2.elapsedTime = SMTick2_period;
	task2.TickFct = &SM2Game;
	
	// Task 3
	task3.state = -1;
	task3.period = SMTick3_period;
	task3.elapsedTime = SMTick3_period;
	task3.TickFct = &serial_input;
	
	// Task 4
	task4.state = -1;
	task4.period = SMTick4_period;
	task4.elapsedTime = SMTick4_period;
	task4.TickFct = &SM4LEDMatrix;
	
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	// Scheduler for-loop iterator
	unsigned short i;
	while(1) {
		
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime ==
			tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	// Error: Program should not exit!
	return 0;
}