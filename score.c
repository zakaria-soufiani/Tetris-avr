/*
 * score.c
 *
 * Written by Peter Sutton
 */

#include "score.h"

// Variable to keep track of the score. We declare it as static
// to ensure that it is only visible within this module - other
// modules should call the functions below to modify/access the
// variable.
static uint32_t score;
static uint32_t row_number;

void init_score(void) {
	score = 0;
}

void add_to_score(uint16_t value) {
	score += value;
}

uint32_t get_score(void) {
	return score;
}

void init_row(void){
	row_number = 0;
}

void add_to_row(uint16_t value){
	row_number += value;
}

uint32_t get_row(void){
	return row_number;
}
