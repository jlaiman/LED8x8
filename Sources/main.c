// ***********************************************************************
//  ECE 362 Final Project
//
// 8x8 LED display                    
// ***********************************************************************
//	 	   			 		  			 		  		
// Completed by: Ben Ng, Joe Aronson, John Laiman, Max Brown
//               I'm not telling you my class number
//               Never completed
// ***********************************************************************

#include <hidef.h>           /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

// All funtions after main should be initialized here

// Note: inchar and outchar can be used for debugging purposes
char inchar(void);
void outchar(char x);

// SPI functions
void shiftout(char x);
			 		  		
//  Variable declarations
int i;
int j;
char ledarray[3][8];// buffer of led values

// Led array masks
//int red0 = 0;    // red   from 0 - 7
//int blu0 = 8;    // blue  from 8 - 15
//int gre0 = 16;   // green from 16 - 23
	 	   		
// Initializations
void  initializations(void) {

// Set the PLL speed (bus clock = 24 MHz)
  CLKSEL = CLKSEL & 0x80; // disengage PLL from system
  PLLCTL = PLLCTL | 0x40; // turn on PLL
  SYNR = 0x02;            // set PLL multiplier
  REFDV = 0;              // set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; // engage PLL
  
// Disable watchdog timer (COPCTL register)
  COPCTL = 0x40;    // COP off - RTI and COP stopped in BDM-mode

// Initialize asynchronous serial port (SCI) for 9600 baud, no interrupts
  SCIBDH =  0x00;   // set baud rate to 9600
  SCIBDL =  0x9C;   // 24,000,000 / 16 / 156 = 9600 (approx)  
  SCICR1 =  0x00;   // $9C = 156
  SCICR2 =  0x0C;   // initialize SCI for program-driven operation

// Initialize the SPI
  DDRM |= 0x30;     // initialize portm 4 and 5 for output
  SPIBR = 0x01;     // initialize baud rate for 6Mbps 
                    // (we may need to change this depending 
                    // on shift register speed)
  SPICR1_CPHA = 0;  // sample at odd edges
  SPICR1_MSTR = 1;  // set as master
  SPICR1_SPE  = 1;  // enable spi system
  
// Initialize the TIM
  TSCR1_TEN = 1;    // enable tim system
  TIOS_IOS7 = 1;    // set ch7 for output compare
  TSCR2 |= 0x04;    // set pre-scale to 16 (tim freq = 24M/16 = 1.5M)
                    // tim period = 1/1.5M = 6.67us 
  TSCR2_TCRE = 1;   // reset TCNT on successful oc7
  TC7 = 1500;       // period = 1500 * 6.67us = 1ms
  TIE_C7I = 1;      // enable TC7 interupts

}
	 		  			 		  		
 
// Main (non-terminating loop)
 
void main(void) {
	initializations(); 		  			 		  		
	EnableInterrupts;


  for(;;) {

	 	   			 		  			 		  		

  } /* loop forever */
  
}  /* make sure that you never leave main */



// ***********************************************************************                       
// RTI interrupt service routine: rti_isr
//
//  Initialized for 5-10 ms (approx.) interrupt rate - note: you need to
//    add code above to do this
//
//  Samples state of pushbuttons (PAD7 = left, PAD6 = right)
//
//  If change in state from "high" to "low" detected, set pushbutton flag
//     leftpb (for PAD7 H -> L), rghtpb (for PAD6 H -> L)
//     Recall that pushbuttons are momentary contact closures to ground
//
//  Also, keeps track of when one-tenth of a second's worth of RTI interrupts
//     accumulate, and sets the "tenth second" flag         	   			 		  			 		  		
 
interrupt 7 void RTI_ISR( void)
{
 // set CRGFLG bit to clear RTI device flag
  	CRGFLG = CRGFLG | 0x80; 
	

}

/*
***********************************************************************                       
  TIM interrupt service routine

  Initialized for 1.0 ms interrupt rate
  
  Shifts out LED data every 1.0 ms 		  			 		  		
;***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
  // clear TIM CH 7 interrupt flag 
 	TFLG1 = TFLG1 | 0x80;
 	
 	for(i = 0; i < 3; i++) {
 	  for(j = 0; j < 8; j++) {
 	    shiftout(ledarray[i][j]);
 	  }
 	}
 	
}

/*
***********************************************************************
  shiftout: Transmits the character x to external shift 
            register using the SPI.  It should shift MSB first.  
             
            MISO = PM[4]
            SCK  = PM[5]
***********************************************************************
*/
 
void shiftout(char x)

{
  // read the SPTEF bit, continue if bit is 1
  // write data to SPI data register
  // wait for 30 cycles for SPI data to shift out 
  if(SPISR_SPTEF) {
    SPIDR = x;
    asm {
      psha
      pshc
      ldaa  #7
    loop:  
      dbne  a,loop
      pulc
      pula
    }
  }
}


// ***********************************************************************
// Character I/O Library Routines for 9S12C32 (for debugging only)
// ***********************************************************************
// Name:         inchar
// Description:  inputs ASCII character from SCI serial port and returns it
// ***********************************************************************
char  inchar(void) {
  /* receives character from the terminal channel */
        while (!(SCISR1 & 0x20)); /* wait for RDR input */
    return SCIDRL;
 
}

// ***********************************************************************
// Name:         outchar
// Description:  outputs ASCII character passed in outchar()
//                  to the SCI serial port
// ***********************************************************************/
void outchar(char ch) {
  /* transmits a character to the terminal channel */
    while (!(SCISR1 & 0x80));  /* wait for TDR empty */
    SCIDRL = ch;
}

