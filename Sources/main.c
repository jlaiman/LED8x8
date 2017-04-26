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
#include <stdlib.h>

// All funtions after main should be initialized here

// Note: inchar and outchar can be used for debugging purposes
char inchar(void);
void outchar(char x);

// LED graphic functions
void initializeGraphics(void);
void loadPattern(void);
void copyPattern(int color, char pat[]);

// SPI functions
void shiftLedArray(void);
void shiftout(char x);

//  Constant declarations
#define COLORS          3 // number of color channels
#define RED             0 // red channel is index 0
#define GREEN           1 // green channel is index 1
#define BLUE            2 // blue channel is index 2
#define ROWS            8 // number of LED rows
#define BASSTHRESH      .75 // threshold for bass hit/kick
#define NUMPATTERNS     5 // number of patterns for LED graphic
#define RANDINT(max)    ((int)(rand() * max)) // returns random int
#define NEXTINT(n, mod) ((n + 1) % mod) // returns next int
#define PREVINT(n, mod) ((n + mod - 1) % mod) // returns previous int
			 		  		
//  Variable declarations
int i;
int j;
int color; // color index for loading patterns
char ledarray[COLORS][ROWS]; // buffer of led values
char lowPass; // low-pass ATD value
char micOut; // mic out ATD value

//  Graphic struct and variables
typedef struct PatternSeq {
  int levels; // number of levels in pattern sequence
  char bass[ROWS]; // bass pattern
  char ** sequence; // audio pattern sequence
} Pattern;

Pattern patterns[NUMPATTERNS]; // array of all Patterns
int patIndex; // index for current working pattern
	 	   		
// Initializations
void initializations(void) {

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
  SPICR1_CPHA = 0;  // sample at even edges
  SPICR1_MSTR = 1;  // set as master
  SPICR1_SPE  = 1;  // enable spi system
  
// Initialize the TIM
  TSCR1_TEN = 1;    // enable tim system
  TIOS_IOS7 = 1;    // set ch7 for output compare
  TSCR2 |= 0x04;    // set pre-scale to 16 (tim freq = 24M/16 = 1.5M)
                    // tim period = 1/1.5M = 6.67us 
  TSCR2_TCRE = 1;   // reset TCNT on successful oc7
  TC7 = 1500;       // period = 1500 * 6.67us = 1ms
  TIE_C7I = 0;      // disable TC7 interupts initially
  
/* 
 Initialize the ATD to sample 2 D.C. input voltages (range: 0 to 5V)
 on Channel 0 and 1. The ATD should be operated in normal flag clear
 mode using nominal sample time/clock prescaler values, 8-bit, unsigned, non-FIFO mode.
*/	 	   			 		  			 		  		
  ATDCTL2 = 0x80; // power up ATD
  ATDCTL3 = 0x10; // set conversion sequence length to TWO
  ATDCTL4 = 0x85; // select 8-bit resolution and sample time
  
/* Initialize RTI for 2.048 ms interrupt rate */
  CRGINT_RTIE = 1; // enable RTI interrupt
  RTICTL = 0x50; // RTI rate of 2.048 ms
  
/* Initialize graphic patterns */
  initializeGraphics();

}
	 		  			 		  		
 
// Main (non-terminating loop)
 
void main(void) {
  DisableInterrupts;
	initializations(); 		  			 		  		
	EnableInterrupts;
	
  TIE_C7I = 1;      // enable TC7 interupts


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
 
interrupt 7 void RTI_ISR( void) {
 // set CRGFLG bit to clear RTI device flag
  	CRGFLG = CRGFLG | 0x80; 
	

}

/*
***********************************************************************                       
  TIM interrupt service routine

  Initialized for 1.0 ms interrupt rate
  
  Shifts out LED data every 1.0 ms 		  			 		  		
***********************************************************************
*/

interrupt 15 void TIM_ISR(void) {
  // clear TIM CH 7 interrupt flag 
 	TFLG1 = TFLG1 | 0x80;
  
 	ATDCTL5 = 0x10; // start ATD conversion
 	while (ATDSTAT0_SCF == 0) {} // wait for conversion complete
 	lowPass = ATDDR0H; // retrieve/load low-pass ATD value
 	micOut = ATDDR1H; // retrieve/load mic out ATD value
 	
 	loadPattern();
 	
 	shiftLedArray();
 	
}

void initializeGraphics() {
  patIndex = 0;
  
  // patterns[0] is concentric squares and outer border bass
  patterns[0].levels = 4;
  patterns[0].sequence = (char **)malloc(sizeof(char *) * patterns[0].levels);
  for (i = 0; i < patterns[0].levels; i++) {
    patterns[0].sequence[i] = (char *)malloc(sizeof(char) * ROWS);
  }
  for (i = 0; i < ROWS; i++) {
    if (i == 0 || i == 7) {
      patterns[0].bass[i] = 0xFF;
      patterns[0].sequence[3][i] = 0xFF;
    } else {
      patterns[0].bass[i] = 0x81;
      patterns[0].sequence[3][i] = 0x81;
    }
    if (i == 3 || i == 4) {
      patterns[0].sequence[0][i] = 0x18;
    } else {
      patterns[0].sequence[0][i] = 0x00;
    }
    if (i == 2 || i == 5) {
      patterns[0].sequence[1][i] = 0x3C;
    } else if (i == 3 || i == 4) {
      patterns[0].sequence[1][i] = 0x24;
    } else {
      patterns[0].sequence[1][i] = 0x00;
    }
    if (i == 1 || i == 6) {
      patterns[0].sequence[2][i] = 0x7E;
    } else if (i > 1 && i < 6) {
      patterns[0].sequence[2][i] = 0x42;
    } else {
      patterns[0].sequence[2][i] = 0x00;
    }
  }
  
  // patterns[1] is 4 4x4 squares and outer border bass
  patterns[1].levels = 4;
  patterns[1].sequence = (char **)malloc(sizeof(char *) * patterns[1].levels);
  for (i = 0; i < patterns[1].levels; i++) {
    patterns[1].sequence[i] = (char *)malloc(sizeof(char) * ROWS);
  }
  for (i = 0; i < ROWS; i++) {
    if (i == 0 || i == 7) {
      patterns[1].bass[i] = 0xFF;
    } else {
      patterns[1].bass[i] = 0x81;
    }
    if (i < 4) {
      patterns[1].sequence[0][i] = 0xF0; 
      patterns[1].sequence[1][i] = 0x0F; 
      patterns[1].sequence[2][i] = 0x00;
      patterns[1].sequence[3][i] = 0x00;
    } else {
      patterns[1].sequence[0][i] = 0x00;
      patterns[1].sequence[1][i] = 0x00;
      patterns[1].sequence[2][i] = 0xF0; 
      patterns[1].sequence[3][i] = 0x0F;
    }
  }
  
  // patterns[2] is 9 inner 2x2 squares and outer border bass
  patterns[2].levels = 3;
  patterns[2].sequence = (char **)malloc(sizeof(char *) * patterns[2].levels);
  for (i = 0; i < patterns[2].levels; i++) {
    patterns[2].sequence[i] = (char *)malloc(sizeof(char) * ROWS);
  }
  for (i = 0; i < ROWS; i++) {
    if (i == 0 || i == 7) {
      patterns[2].bass[i] = 0xFF;
    } else {
      patterns[2].bass[i] = 0x81;
    }
    if (i == 1 || i == 2 || i == 5 || i == 6) { 
      patterns[2].sequence[0][i] = 0x00;
      patterns[2].sequence[1][i] = 0x18;
      patterns[2].sequence[2][i] = 0x66;
    } else if (i == 3 || i == 4) {
      patterns[2].sequence[0][i] = 0x18;
      patterns[2].sequence[1][i] = 0x66;
      patterns[2].sequence[2][i] = 0x00;
    } else {
      patterns[2].sequence[0][i] = 0x00;
      patterns[2].sequence[1][i] = 0x00;
      patterns[2].sequence[2][i] = 0x00;
    }
  }
  
  // patterns[3] is horizontal bars with vertical bass
  patterns[3].levels = 4;
  patterns[3].sequence = (char **)malloc(sizeof(char *) * patterns[3].levels);
  for (i = 0; i < patterns[3].levels; i++) {
    patterns[3].sequence[i] = (char *)malloc(sizeof(char) * ROWS);
  }
  for (i = 0; i < ROWS; i++) {
    patterns[3].bass[i] = 0x81;
    if (i == 3 || i == 4) {
      patterns[3].sequence[0][i] = 0xFF;
    } else {
      patterns[3].sequence[0][i] = 0x00;
    }
    if (i == 2 || i == 5) {
      patterns[3].sequence[1][i] = 0xFF;
    } else {
      patterns[3].sequence[1][i] = 0x00;
    }
    if (i == 1 || i == 6) {
      patterns[3].sequence[2][i] = 0xFF;
    } else {
      patterns[3].sequence[2][i] = 0x00;
    }
    if (i == 0 || i == 7) {
      patterns[3].sequence[3][i] = 0xFF;
    } else {
      patterns[3].sequence[3][i] = 0x00;
    }
  }
  
  // patterns[4] is vertical bars with horizontal bass
  patterns[4].levels = 4;
  patterns[4].sequence = (char **)malloc(sizeof(char *) * patterns[4].levels);
  for (i = 0; i < patterns[4].levels; i++) {
    patterns[4].sequence[i] = (char *)malloc(sizeof(char) * ROWS);
  }
  for (i = 0; i < ROWS; i++) {
    if (i == 0 || i == 7) {
      patterns[4].bass[i] = 0xFF;
    } else {
      patterns[4].bass[i] = 0x00;
    }
    patterns[4].sequence[0][i] = 0x18;
    patterns[4].sequence[1][i] = 0x24;
    patterns[4].sequence[2][i] = 0x42;
    patterns[4].sequence[3][i] = 0x81;
  }
}

/*
***********************************************************************                       
  loadPattern
  
  Loads ledarray with a pattern
***********************************************************************
*/

void loadPattern() {
  if (lowPass >= BASSTHRESH) {
    // fills bass pattern white
    copyPattern(RED, patterns[patIndex].bass);
    copyPattern(GREEN, patterns[patIndex].bass);
    copyPattern(BLUE, patterns[patIndex].bass);
  }
  
  color = RANDINT(COLORS); // randomize starting color
  for (i = 0; i < (int)(micOut * patterns[patIndex].levels); i++) {
    copyPattern(color, patterns[patIndex].sequence[i]);
    color = NEXTINT(color, COLORS);
  }
}

/*
***********************************************************************                       
  copyPattern(int color)
  
  Copies the current pattern into specified color channel
***********************************************************************
*/

void copyPattern(int color, char pat[]) {
  for (i = 0; i < ROWS; i++) {
    ledarray[color][i] = pat[i];
  }
}

/*
***********************************************************************                       
  shiftLedArray
  
  Shifts out current value of ledarray to LED's on board 		  			 		  		
***********************************************************************
*/

void shiftLedArray(void) {
  for (i = 2; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
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
void shiftout(char x) {
  // read the SPTEF bit, continue if bit is 1
  // write data to SPI data register
  // wait for 30 cycles for SPI data to shift out 
  while (SPISR_SPTEF == 0) {
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

