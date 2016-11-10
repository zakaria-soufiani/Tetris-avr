/*
 * project.c
 *
 * Main file for the Tetris Project.
 *
 * Author: Peter Sutton. Modified by Mohamed Zakaria Soufiani
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>		// For random()

#include "ledmatrix.h"
#include "scrolling_char_display.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "score.h"
#include "timer0.h"
#include "game.h"
#include "joystick.h"
#include "sevenseg.h"

#define F_CPU 8000000L
#include <util/delay.h>

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void splash_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);
void handle_new_lap(void);

// ASCII code for Escape character
#define ESCAPE_CHAR 27

static uint8_t game_loaded ;

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete
	splash_screen();
	
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	// Set up our main timer to give us an interrupt every millisecond
	init_timer0();
	
	// Turn on global interrupts
	sei();
}

void splash_screen(void) {
	// Reset display attributes and clear terminal screen then output a message
	set_display_attribute(TERM_RESET);
	clear_terminal();
	
	hide_cursor();	// We don't need to see the cursor when we're just doing output
	move_cursor(3,3);
	printf_P(PSTR("Tetris"));
	
	move_cursor(3,5);
	set_display_attribute(FG_GREEN);	// Make the text green
	printf_P(PSTR("CSSE2010/7201 Tetris Project by Mohamed Zakaria Soufiani"));	
	set_display_attribute(FG_WHITE);	// Return to default colour (White)
	
	// Output the scrolling message to the LED matrix
	// and wait for a push button to be pushed.
	ledmatrix_clear();
	
	// Red message the first time through
	PixelColour colour = COLOUR_RED;
	while(1) {
		set_scrolling_display_text("ID 43119703", colour);
		// Scroll the message until it has scrolled off the 
		// display or a button is pushed. We pause for 130ms between each scroll.
		while(scroll_display()) {
			_delay_ms(130);
			if(button_pushed() != -1) {
				// A button has been pushed
				return;
			}
		}
		// Message has scrolled off the display. Change colour
		// to a random colour and scroll again.
		switch(random()%4) {
			case 0: colour = COLOUR_LIGHT_ORANGE; break;
			case 1: colour = COLOUR_RED; break;
			case 2: colour = COLOUR_YELLOW; break;
			case 3: colour = COLOUR_GREEN; break;
		}
	}
}

void new_game(void) {
	// Initialise the game and display
	init_game();
	
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the score
	init_score();
	init_seven_seg();
	
	// Delete any pending button pushes or serial input
	empty_button_queue();
	clear_serial_input_buffer(); 
	}

void play_game(void) {
	uint32_t last_drop_time, current_time;
	int8_t button;
	char serial_input, escape_sequence_char;
	uint8_t characters_into_escape_sequence = 0;
	uint16_t speed_factor;
	
	// Record the last time a block was dropped as the current time -
	// this ensures we don't drop a block immediately.
	current_time= get_clock_ticks();
	last_drop_time = current_time;
	
	
	// We play the game forever. If the game is over, we will break out of
	// this loop. The loop checks for events (button pushes, serial input etc.)
	// and on a regular basis will drop the falling block down by one row.
	while(1) {
		//speed up the game 
		current_time = get_clock_ticks();
		speed_factor = 600;
		if(get_score()>=100){
			speed_factor = 500;
		}
		if(get_score()>=500){
			speed_factor = 450;
		}
		if(get_score()>=800){
			speed_factor = 350;
		}
		if(get_score()>=1000){
			speed_factor = 300;
		}
		if(get_score()>=2000){
			speed_factor = 200;
		}
		
		
		// Check for input - which could be a button push or serial input.
		// Serial input may be part of an escape sequence, e.g. ESC [ D
		// is a left cursor key press. We will be processing each character
		// independently and can't do anything until we get the third character.
		// At most one of the following three variables will be set to a value 
		// other than -1 if input is available.
		// (We don't initalise button to -1 since button_pushed() will return -1
		// if no button pushes are waiting to be returned.)
		// Button pushes take priority over serial input. If there are both then
		// we'll retrieve the serial input the next time through this loop
		serial_input = -1;
		escape_sequence_char = -1;
		button = button_pushed();
		game_loaded = 1;
		
		if(button == -1) {
			// No push button was pushed, see if there is any serial input
			if(serial_input_available()) {
				// Serial data was available - read the data from standard input
				serial_input = fgetc(stdin);
				// Check if the character is part of an escape sequence
				if(characters_into_escape_sequence == 0 && serial_input == ESCAPE_CHAR) {
					// We've hit the first character in an escape sequence (escape)
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
				} else if(characters_into_escape_sequence == 1 && serial_input == '[') {
					// We've hit the second character in an escape sequence
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
				} else if(characters_into_escape_sequence == 2) {
					// Third (and last) character in the escape sequence
					escape_sequence_char = serial_input;
					serial_input = -1;  // Don't further process this character - we
										// deal with it as part of the escape sequence
					characters_into_escape_sequence = 0;
				} else {
					// Character was not part of an escape sequence (or we received
					// an invalid second character in the sequence). We'll process 
					// the data in the serial_input variable.
					characters_into_escape_sequence = 0;
				}
			}
		}
		
		// Process the input. 
		if(button==3 || escape_sequence_char=='D') {
			// Attempt to move left
			(void)attempt_move(MOVE_LEFT);
		} else if(button==0 || escape_sequence_char=='C') {
			// Attempt to move right
			(void)attempt_move(MOVE_RIGHT);
		} else if (button==2 || escape_sequence_char == 'A') {
			// Attempt to rotate
			(void)attempt_rotation();
		} else if (escape_sequence_char == 'B') {
		// Attempt to drop block
		if(!attempt_drop_block_one_row()) {
			// Drop failed - fix block to board and add new block
			if(!fix_block_to_board_and_add_new_block()) {
				break;	// GAME OVER
			}
			add_to_score(1);
			hide_cursor();
			move_cursor(50,3);
			printf_P(PSTR("SCORE: %6d"),get_score());
			move_cursor(50,5);
			printf_P(PSTR("Completed Rows: %6d"),get_row());
		}
		last_drop_time = get_clock_ticks();
	}
	else if(button == 1 || serial_input == ' '){
		// Attempt to drop block
		// press space or button to drop from height
		while(attempt_drop_block_one_row()==1){
			if(!attempt_drop_block_one_row()){
				// Drop failed - fix block to board and add new block
			}
		}
		if(!fix_block_to_board_and_add_new_block()) {
			break;	// GAME OVER
		}
		add_to_score(1);
		hide_cursor();
		move_cursor(50,3);
		printf_P(PSTR("SCORE: %6d"),get_score());
		move_cursor(50,5);
		printf_P(PSTR("Completed Rows: %6d"),get_row());
		last_drop_time = get_clock_ticks();
	}
		 
		 if(serial_input == 'n' || serial_input == 'N'){
				init_game();
				clear_terminal();
				init_score();
				init_seven_seg();
				empty_button_queue();
				clear_serial_input_buffer();
				hide_cursor();
				move_cursor(50,3);
				printf_P(PSTR("SCORE: %6d"),get_score());
				move_cursor(50,5);
				printf_P(PSTR("Completed Rows: %6d"),get_row());
			 
		 }
		
		// else - invalid input or we're part way through an escape sequence -
		// do nothing
		
		// Check for timer related events here
		if(get_clock_ticks() >= last_drop_time + speed_factor) {
			// 600ms (0.6 second) has passed since the last time we dropped
			// a block, so drop it now.
			if(!attempt_drop_block_one_row()) {
				// Drop failed - fix block to board and add new block
				if(!fix_block_to_board_and_add_new_block()) {
					break;	// GAME OVER
				}
				add_to_score(1);
				hide_cursor();
				move_cursor(50,3);
				printf_P(PSTR("SCORE: %6d"),get_score());
				move_cursor(50,5);
				printf_P(PSTR("Completed Rows: %6d"),get_row());
			}
			last_drop_time = get_clock_ticks();
		}
	}
	// If we get here the game is over. 
}
		

void handle_game_over() {
	
	hide_cursor();
	move_cursor(50,3);
	printf_P(PSTR("SCORE: %6d"),get_score());
	move_cursor(50,5);
	printf_P(PSTR("Completed Rows: %6d"),get_row());
	// Print a message to the terminal. 
	move_cursor(10,14);
	printf_P(PSTR("GAME OVER"));
	move_cursor(10,15);
	printf_P(PSTR("Press a button to start again"));
	while(button_pushed() == -1) {
		; // wait until a button has been pushed
	}
	
}