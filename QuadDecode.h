#ifndef QUADDECODE_H
#define QUADDECODE_H

#include <stdint.h>
#include "mk20dx128.h"
#include "core_pins.h"

/*
* Code simplified (and functionality reduced) from tlb's excellent code announced and discussed here
*    https://forum.pjrc.com/threads/26803-Hardware-Quadrature-Code-for-Teensy-3-x
* Changes:
*   - removed >64k irq code
*   - untemplated into a single monolith
*
* HARDWARE DETAILS
*          Teensy ARM
*    ENCXA	 3	PTA12	 28	Input	    FTM1    7
*    ENCXB	 4	PTA13	 29	Input	    FTM1    7
*    ENCYA	32	PTB18	 41	Input	    FTM2    6
*    ENCYB	25	PTB19	 42	Input	    FTM2    6
*
* KINETIS REGISTERS
*   PORTx_PCRn
*      Bit 10-8 MUX - function Multiplexor (0 = Disabled - Analog) See P223 of Manual
*      Bit    4 PFE - Passive Filter Enable
*      Bit    1 PE  - Pull Enable
*      Bit    0 PS  - Pull Select 0 - Down, 1 - Up
*      0x00000712 = Alt7-QD_FTM1,FilterEnable,Pulldown
*
*   FTM1_CNT Counter value
*      Bit 15-0 is counter value
*      Writing any value updates counter with CNTIN
*   FTM1_MOD Modulo (Max value)
*      Bit 15-0 is counter value - set to 0xFFFF
*      Write to CNT first
*   FTM1_MODE Features Mode Selection
*      Bit0 FTMEN FTM Enable - [WPDIS must be 1 to write]
*      Bit2 WPDIS Write Protect to Disable
*        Set WPDIS, then set FTMEN - must be set to access FTM regs
*   FTM1_FMS Fault Mode Status
*      Bit 6 WPEN Write Protect Enable
*        Write 1 to clear WPDIS
*   FTM1_FILTER
*      Filter out pulses shorter than CHxFVALx 4 system clocks
*      Bit 7-4 CH1FVAL for PHB
*      Bit 3-0 CH0FVAL for PHA
*   FTM1_QDCTRL Quadrature Decoder Control and Status
*      Bit 7 PHAFLTREN Phase A Filter Enable
*      Bit 6 PHBFLTREN Phase B Filter Enable
*      Bit 5 PHAPOL Phase A Polarity
*      Bit 4 PHBPOL Phase B Polarity
*      Bit 3 QUADMODE Quadrature Decoder Mode
*        0 for Quadrature
*      Bit 2 QUADIR Counting Direction
*      Bit 1 TOFDIR Timer Overflow Dir
*        0 was set on bottom of counting
*        1 was set on top of counting
*      Bit 0 QUADEN Quadrature Mode Enable
*	       1 is quadrature mode enabled [WPDIS to write]
*   FTM1_SC FTM1 Status and Control
*     Bit 7 TOF Timer Overflow Flag
*     Bit 6 TOIE Timer Overflow Interrupt Enable
*     Bit 5 CPWMS Center Aligned PWM Select
*       Resets to 0 (Write when WPDIS is 1)
*       Rest of bits are 0 (Write when WPDIS is 1)
*   FTM1_C0SC  FTM1 Channel 0 Status and Control
*     Set WPDIS before writing control bits
*     Bit 7 CHF Channel Flag
*       Channel event occured
*       Read and write 0 to clear
*     Bit 6 CHIE Channel Interrupt Enable
*       Set for compare interrupt
*     Bit 5 MSB Channel Mode Select B
*       Set to 0
*     Bit 4 MSA Channel Mode Select A
*       Set to 1
*     Bit 3:2  ELS Edge or Level Select
*       Set to 0:0
*   FTM1_COMBINE
*     Set WPDIS to 1 before writing
*     DECAPEN (Dual Edge Capture Enable)
*     COMBINE (Combine Channels)
*     Resets to all zero
*   FTM1_C0V Channel 0 Value
*     Channel Compare Value
*     Set to 0x80 - halfway thru count
*   FTM1_STATUS
*     Duplicate of CHnF bit for all channels
*     Bit 0 CH0F
*/

class QuadDecode_t {
  public:
    QuadDecode_t() {

      // Pin Assignments

      // FTM1 Pins
      // K20 pin 28,29
      // Bit 8-10 is Alt Assignment
      PORTA_PCR12 = 0x00000712;   //Alt7-QD_FTM1,FilterEnable,Pulldown
      PORTA_PCR13 = 0x00000712;   //Alt7-QD_FTM1,FilterEnable,Pulldown

      // FTM2 Pins
      // K20 pin 41,42
      // Bit 8-10 is Alt Assignment
      PORTB_PCR18 = 0x00000612;   //Alt6-QD_FTM2,FilterEnable,Pulldown
      PORTB_PCR19 = 0x00000612;   //Alt6-QD_FTM2,FilterEnable,Pulldown

      //Set FTMEN to be able to write registers
      FTM1_MODE=0x04;	    // Write protect disable - reset value
      FTM1_MODE=0x05;	    // Set FTM Enable

      FTM2_MODE=0x04;	    // Write protect disable - reset value
      FTM2_MODE=0x05;	    // Set FTM Enable

      // Set registers written in pins_teensy.c back to default
      FTM1_CNT = 0;
      FTM1_MOD = 0;
      FTM1_C0SC =0;
      FTM1_C1SC =0;
      FTM1_SC = 0;

      FTM2_CNT = 0;
      FTM2_MOD = 0;
      FTM2_C0SC =0;
      FTM2_C1SC =0;
      FTM2_SC = 0;

      // Set registers to count quadrature
      FTM1_FILTER=0x22;	// 2x4 clock filters on both channels
      FTM1_CNTIN=0;
      FTM1_MOD=0xFFFF;	// Maximum value of counter
      FTM1_CNT=0;		    // Updates counter with CNTIN

      FTM2_FILTER=0x22;	// 2x4 clock filters on both channels
      FTM2_CNTIN=0;
      FTM2_MOD=0xFFFF;	// Maximum value of counter
      FTM2_CNT=0;	    	// Updates counter with CNTIN

      // Set Registers for output compare mode - for IRQ?
      //FTM1_COMBINE=0;	    // Reset value, make sure
      //FTM1_C0SC=0x10;	      // Bit 4 Channel Mode
      //FTM1_C0V= COMP_LOW;	    // Initial Compare Interrupt Value

      FTM1_QDCTRL=0b11000001;	    // Quadrature control
      FTM2_QDCTRL=0b11000001;	    // Quadrature control
      //        Filter enabled, QUADEN set

      // Write Protect Enable
      FTM1_FMS=0x40;		// Write Protect, WPDIS=1
      FTM2_FMS=0x40;		// Write Protect, WPDIS=1
    }

    void setCounter1( int16_t c ) {
      FTM1_CNT = c;
    }

    void setCounter2( int16_t c ) {
      FTM2_CNT = c;
    }

    int16_t getCounter1( ) {
      int16_t c = FTM1_CNT;

      return c;
    }

    int16_t getCounter2( ) {
      int16_t c = FTM2_CNT;

      return c;
    }

};

QuadDecode_t QuadDecode;

#endif
