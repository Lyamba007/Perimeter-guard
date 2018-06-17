///////////////////////////////////////
//									 //
// Perimeter.c						 //
// Author : Hoha Maksym				 //	
// Copyright and all rights reserved //
//									 //
///////////////////////////////////////


/*	
 * Set all necessary libraries:
	- input/output work for ATtiny2313
	- library for interrupt handlers (INT0, INT1)
 * Set standart MK frequency for clock divider (8 MHz)
*/ 
#include <avr/io.h> 
#define F_CPU 8000000UL
#include <avr/interrupt.h>

// Define special instruction for port reset
//
#define PortReset(PORT) PORT &= 0b00000000

// Define all transitions in FSM
//
#define START 1
#define FIRST_ON 2
#define SECOND_ON 3
#define THIRD_ON 4
#define FOURTH_ON 5
#define RING_ON 6
#define RING_OFF 7

// Define timescale which means 
// necessary register (TCCR1B) bits for frequency prescaler 
// (clk/prescaler_value (T1 or T2))
//
#define T1 0b00001100; // clk/256
#define T2 0b00001011; // clk/64

// State and next state start value
//
unsigned char state = START;
unsigned char nextState;

// For avoiding compiler warnings 
// need to declarate function
// 
void StateTransition(void);


// Function for initializing Timer1
//
void TimerInit(void)
{
	TIMSK   = 0b01000000; // Allow Timer1 interrupt compared with OCR1A (H and L - High bits and Low bits) 
	
	// Set value to compare for interrupt (31250)
	// 
	OCR1AH  = 0b01111010; 
	OCR1AL  = 0b00010010;
	
    TCCR1B  = T1; // Set prescaler
}


// Timer1 overflow interrupt handler 
//             
ISR (TIMER1_COMPA_vect)
{
	sei();				// Allow all interrupts 
	StateTransition();  // Start FSM
}


// Perimeter crossing handler
// 
ISR(INT0_vect)
{	
	// Need to check if it's evening 
	//  
	if(PIND&(1<<PIND4)) 
	{
		GIMSK  = 0b10000000; // Off signalization handler 
		state  = RING_ON;	 // Hyperstate entry
		TCCR1B = T2;		 // Set new prescaler
	}
	
	// System must be off afternoon 
	//
	else
	{
		state = START;
	}
	
	sei();
}


// Signalization off handler
//
ISR(INT1_vect)
{
	GIMSK  = 0b01000000; // Allow to use Perimeter crossing handler again
	state  = FIRST_ON;	 // Reload system 
	TCCR1B = T1;		 // Set default prescaler
	
	sei();
}


// Main function initializes all registers, inputs and outputs
// 
int main(void)
{
	GIMSK = 0b11000000; // Allow to use Perimeter crossing and Signalization off handlers
	MCUCR = 0b00001111; // Set special instructions for handlers working only if input has logic '1'
	TimerInit();		// Call Timer1 initialization
	
	// Control logic level in all pins
	// 0 - zero logic level; 1 - logic level is one
	//
	PortReset(PORTB); 
	PORTD = 0b0011100;
	
	// Set input and output roles
	// 
	DDRD = 0b1100011;  // D4-D3-D2 is input
	DDRB = 0b00011111; // B5-B0 is output

	sei();
	
	// ATTENTION! Not delete this cycle!  
	// It starts all system working process
	//
    while (1) 
    {
	}
}

// FSM working process function
//
void StateTransition(void)
{
	switch(state)
	{
		case START:
		
			PortReset(PORTB); // Reset all LED's

			// Check if it's evening
			//				
			if(PIND&(1<<PIND4))
			{
				nextState = FIRST_ON; 
			}
		
			else
			{
				nextState = START;
			}
	
		break;
		
		case FIRST_ON:
		
			// Need to check one more time
			// if the system has stable signal from sensor
			//
			if(PIND&(1<<PIND4))
			{
				PORTB = 0b00000001; // Set appropriate checking LED 
				nextState = SECOND_ON;
			}
		
			else
			{
				nextState = START;
			}
			
		break;
		
		case SECOND_ON:
			PORTB = 0b00000010;
			nextState = THIRD_ON;
		break;
		
		case THIRD_ON:
			PORTB = 0b00000100;
			nextState = FOURTH_ON;
		break;
		
		case FOURTH_ON:
			PORTB = 0b00001000;
			nextState = FIRST_ON;
		break;
		
		// Hyperstate 
		
			case RING_ON:
				PORTB = 0b00011111; //Enable all LED's and singalization
				nextState = RING_OFF;
			break;
		
			case RING_OFF:
				nextState = RING_ON;
			break;
		
		// end hyperstate
		
		// For avoiding all collapse
		// need to set default state
		//
		default:
			PortReset(PORTB);
			nextState = START;
		break;
	}
	
	state = nextState;
}
