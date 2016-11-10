
#include "avr/io.h"

/* Seven segment display segment values for 0 to 9 */
uint8_t seven_seg_data[10] = {63,6,91,79,102,109,125,7,127,111};
uint8_t seg;

void init_seven_seg(void){
	DDRA = 0xFF;
	DDRC = 0;
	
	PORTA = seven_seg_data[0];
	
}

void update_seven_seg(uint32_t digit) {
	DDRA = 0xFF;
	DDRC = 0;
	
	PORTA = seven_seg_data[digit];

}

