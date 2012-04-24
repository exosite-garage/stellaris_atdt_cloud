//*****************************************************************************
//
// exosite_utils.c - Helper functions for Exosite Cloud Communications
//
// Copyright (c) 2012 Exosite LLC.  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:

//  * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of Exosite LLC nor the names of its contributors may
//    be used to endorse or promote products derived from this software 
//    without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/systick.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "exosite_gprs.h"
#include "exosite_utils.h"

#ifdef PART_LM3S6965
#include "inc/lm3s6965.h"    
#include "drivers/rit128x96x4.h"
#endif

#ifdef PART_LM3S1968
#include "inc/lm3s1968.h"
#include "drivers/rit128x96x4.h"
#endif

#ifdef PART_RDK_IDM_L35
#include "inc/lm3s1958.h"
#include "grlib/grlib.h"
#include "utils/ustdlib.h"
#include "drivers/kitronix320x240x16_ssd2119.h"
#endif


// Local definitions
volatile unsigned long g_tickCount = 0;
volatile unsigned long m_second_count = 0;

// Ring buffer controls
volatile unsigned char ringHead = 0;
volatile unsigned char ringTail = 0;
volatile unsigned char ringItems = 0;
volatile unsigned char ringPacket[NUM_RING_PACKETS];



#ifdef PART_LM3S6965
#define UARTBASE UART0_BASE
#endif

#ifdef PART_LM3S1968
#define UARTBASE UART2_BASE
#endif

#ifdef PART_RDK_IDM_L35
#define UARTBASE UART2_BASE
tContext sContext;
tRectangle sRect;
#endif

    
/*==============================================================================
* UARTIntHandler
*
* UART interrupt handler.
*=============================================================================*/
void
UARTIntHandler(void)
{
  unsigned long ulStatus;
  // Get the interrrupt status.
  ulStatus = UARTIntStatus(UARTBASE, true);
  
  // Clear the asserted interrupts.
  UARTIntClear(UARTBASE, ulStatus);

  while(UARTCharsAvail(UARTBASE))
  {
    ringPacket[ringHead] = (char)UARTCharGetNonBlocking(UARTBASE);
    IncrementRingHead(); 
  }     
}


/*==============================================================================
* UARTSend
*
* UART send routine - transmits a buffer of data to the UART.
*=============================================================================*/
void 
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount)
{
  // Loop while there are more characters to send.
  while(ulCount--)
  {
    // Write the next character to the UART.
    UARTCharPut(UARTBASE, *pucBuffer++);
  }  
}


/*==============================================================================
* ReadUARTBufferChar
*
* Read a single character from the UART buffer.  Returns 1 for success.
*=============================================================================*/
unsigned char 
UARTReadBufferChar(unsigned char* uartChar) 
{
  if (ringItems) {
    *uartChar = ringPacket[ringTail];
    IncrementRingTail();
    return 1;
  }

  return 0;
}


/*==============================================================================
* ReadUARTBufferLine
*
* Reads characters out of the UART buffer into the caller buffer until a CRLF 
* is found or the buffer size is hit.  Returns number of bytes read into buffer.
* Populates fullLine with a '1' if a full line was read.  
*=============================================================================*/
unsigned char 
UARTReadBufferLine(unsigned char * fullLine, unsigned int checkTime, char *buffer, unsigned char bufSize, char *termStr)
{
char * lineptr = buffer;
  char * maxptr = buffer + bufSize;
  unsigned long startTime = g_tickCount;

  while ((g_tickCount - startTime) < checkTime) {		
    while (UARTReadBufferChar((unsigned char *)lineptr)) {
      if ((lineptr + 1) > maxptr) { // too many bytes to read into our line!
        return lineptr - buffer;
      } else lineptr++;
      if (exstrnloc(buffer,termStr,lineptr - buffer)) { 
        *fullLine = 1;
        return lineptr - buffer;
      }
    }
  }

  //if we read a partial line, but timed out, we still return how many 
  //bytes we read just in case the caller wants to try to assemble the line
  return lineptr - buffer;
}


/*==============================================================================
* IncrementRingHead
*
* Move head forward a notch.  Called when adding data.
*=============================================================================*/
void 
IncrementRingHead() 
{
  if (++ringItems > NUM_RING_PACKETS) {
    //we just overran tail
    WriteUILine("BUFFER OVERFLOW!", LCD_DEBUG_LINE);
    ringHead = 0;
    ringTail = 0;
    ringItems = 0;  
  } else if (NUM_RING_PACKETS == ++ringHead) 
    ringHead = 0; //wrap ringHead
}


/*==============================================================================
* IncrementRingTail
*
* Move tail forward a notch.  Called when using/removing data.
*=============================================================================*/
void 
IncrementRingTail() 
{
  if (ringItems) { // only allow tail to move if there is available data
    ringItems--;
    if (NUM_RING_PACKETS == ++ringTail) 
      ringTail = 0;  //wrap ringTail
  }
}


/*==============================================================================
* SysTickIntHandler
*
* This is the handler for this SysTick interrupt.  If using FATFs, it requires
* a timer tick every 10mS for internal timing purposes.
*=============================================================================*/
void
SysTickIntHandler(void)
{
  g_tickCount ++;
}


/*==============================================================================
* Delay
*
* Second-based delay timer. Depending upon the current SysTick value, the delay 
* will be between N-1 and N seconds (i.e. N-1 full seconds are guaranteed, 
* along with the remainder of the current second).
*=============================================================================*/
void
Delay(unsigned long ulSeconds)
{
  // Loop while there are more seconds to wait.
  while(ulSeconds--)
  {
    // Wait until the SysTick value is less than 1000.
    while(SysTickValueGet() > 1000);

    // Wait until the SysTick value is greater than 1000.
    while(SysTickValueGet() < 1000);
  }
}


/*==============================================================================
* InitializeUI
*
* Setup debug and user-output UI.  Hardware dependent.
*=============================================================================*/
void 
InitializeUI(void)
{
#ifdef PART_LM3S6965    
  // Initialize the OLED display and write status.
  RIT128x96x4Init(1000000);
#endif
#ifdef PART_LM3S1968
  // Initialize the OLED display and write status.
  RIT128x96x4Init(1000000);
#endif
#ifdef PART_RDK_IDM_L35
  //tContext sContext;
  //tRectangle sRect;
  
  //
  // Initialize the display driver.
  //
  Kitronix320x240x16_SSD2119Init();

  //
  // Turn on the backlight.
  //
  Kitronix320x240x16_SSD2119BacklightOn(255);

  //
  // Initialize the graphics context.
  //
  GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);

  //
  // Fill the top 24 rows of the screen with blue to create the banner.
  //
  sRect.sXMin = 0;
  sRect.sYMin = 0;
  sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
  sRect.sYMax = 23;
  GrContextForegroundSet(&sContext, ClrDarkTurquoise);
  GrRectFill(&sContext, &sRect);

  //
  // Put a white box around the banner.
  //
  GrContextForegroundSet(&sContext, ClrBlack);
  //GrRectDraw(&sContext, &sRect);

  //
  // Put the application name in the middle of the banner.
  //
  
  GrContextFontSet(&sContext, g_pFontCm20);
  GrStringDrawCentered(&sContext, "Exosite Simple Demo Application", -1,
                       GrContextDpyWidthGet(&sContext) / 2, 11, 0);

  //
  // Say hello using the Computer Modern 40 point font.
  //
  /*
  GrContextForegroundSet(&sContext, ClrWhite);
  GrContextFontSet(&sContext, g_pFontCm20);
  GrStringDrawCentered(&sContext, "Hello Exosite!", -1,
                       GrContextDpyWidthGet(&sContext) / 2,
                       ((GrContextDpyHeightGet(&sContext) - 24) / 2) + 24,
                       0);
*/
  //
  // Flush any cached drawing operations.
  //
  GrFlush(&sContext);
#endif
}


/*==============================================================================
* WriteUILine
*
* Write a string to the UI.  Hardware dependent.
*=============================================================================*/
void 
WriteUILine(char * displaystring, unsigned char linenumber)
{
#ifdef PART_LM3S6965    
  RIT128x96x4StringDraw(LCD_BLANK_LINE,  12, linenumber, 15);
  RIT128x96x4StringDraw(displaystring,  12, linenumber, 15);
#endif
#ifdef PART_LM3S1968
  RIT128x96x4StringDraw(LCD_BLANK_LINE,  12, linenumber, 15);
  RIT128x96x4StringDraw(displaystring,  12, linenumber, 15);
#endif
#ifdef PART_RDK_IDM_L35

  if(linenumber == LCD_TITLE_LINE)
  {
    sRect.sXMin = 0;
    sRect.sYMin = 0;
    sRect.sXMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.sYMax = 23;
    GrContextForegroundSet(&sContext, ClrDarkTurquoise);
    GrRectFill(&sContext, &sRect);

    GrContextForegroundSet(&sContext, ClrBlack);
    GrContextFontSet(&sContext, g_pFontCm20);
    GrStringDrawCentered(&sContext, displaystring, -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         11,
                         false);	
  }
  else
  {

    GrContextForegroundSet(&sContext, ClrWhite);
    GrContextFontSet(&sContext, g_pFontCm12);
    GrStringDrawCentered(&sContext, LCD_BLANK_LINE, -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         linenumber + 24,
                         true);    
    GrStringDrawCentered(&sContext, displaystring, -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         linenumber+24,
                         true);	
  }

  //
  // Flush any cached drawing operations.
  //
  GrFlush(&sContext);
#endif

}


/*==============================================================================
* InitializeBoard
*
* Board and peripheral initialization.  Hardware dependent.
*=============================================================================*/
void
InitializeBoard(void)
{
#ifdef PART_LM3S6965    
  // Set the clocking to run directly from the crystal.
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);

  // Set up and enable the SysTick timer.  It will be used as a reference
  // for delay loops in the interrupt handlers.  The SysTick timer period
  // will be set up for one second.
  SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;
  
  g_tickCount = 0;
  SysTickPeriodSet(SysCtlClockGet());
  SysTickEnable();
  SysTickIntRegister(&SysTickIntHandler);
  SysTickIntEnable();
  
  // LED0 config
  GPIO_PORTF_DIR_R = 0x01;
  GPIO_PORTF_DEN_R = 0x01;    

  // Enable the peripherals used by this example.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Set GPIO A0 and A1 as UART pins.  --UART0
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  
  // Configure the UART for 115,200, 8-N-1 operation.
  UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                       UART_CONFIG_PAR_NONE));
  
  // Enable the UART interrupt.
  IntEnable(INT_UART0);
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);  	
#endif

#ifdef PART_LM3S1968
  // Set the clocking to run directly from the crystal.
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);

  // Set up and enable the SysTick timer.  It will be used as a reference
  // for delay loops in the interrupt handlers.  The SysTick timer period
  // will be set up for one second.
  SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;
  
  g_tickCount = 0;
  SysTickPeriodSet(SysCtlClockGet());
  SysTickEnable();
  SysTickIntRegister(&SysTickIntHandler);
  SysTickIntEnable();
  
  // LED0 config
  //GPIO_PORTF_DIR_R = 0x01;
  //GPIO_PORTF_DEN_R = 0x01;    

  // Enable the peripherals used by this example.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Set GPIO G0 and G1 as UART pins.  --UART2
  GPIOPinTypeUART(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  
  // Configure the UART for 115,200, 8-N-1 operation.
  UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 115200,
                      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                       UART_CONFIG_PAR_NONE));
  
  // Enable the UART interrupt.
  IntEnable(INT_UART2);
  UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT); 
#endif

#ifdef PART_RDK_IDM_L35
  // Set the clocking to run directly from the crystal.
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);

  // Set up and enable the SysTick timer.  It will be used as a reference
  // for delay loops in the interrupt handlers.  The SysTick timer period
  // will be set up for one second.
  SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;
  
  g_tickCount = 0;
  SysTickPeriodSet(SysCtlClockGet());
  SysTickEnable();
  SysTickIntRegister(&SysTickIntHandler);
  SysTickIntEnable();
  
  // LED0 config
  //GPIO_PORTF_DIR_R = 0x01;
  //GPIO_PORTF_DEN_R = 0x01;    

  // Enable the peripherals used by this example.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Set GPIO G0 and G1 as UART pins.  --UART2
  GPIOPinTypeUART(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  
  // Configure the UART for 115,200, 8-N-1 operation.
  UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 115200,
                      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                       UART_CONFIG_PAR_NONE));
  
  // Enable the UART interrupt.
  IntEnable(INT_UART2);
  UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT); 
#endif

}


/*==============================================================================
* exstrncpy
*
* Copies one string into another for up to numbytes.  NOTE: does not null
* teriminate!!
*=============================================================================*/ 
void exstrncpy(char * dest, char * src, unsigned char numbytes)
{
  while (numbytes--) {
    *dest++ = *src++;
  }
}


/*==============================================================================
* exstrcat
*
* Copies one string into another and null terminates.
*=============================================================================*/ 
void exstrcpy(char * dest, char * src)
{
  while (*src != 0) {
    *dest++ = *src++;
  }
  *dest = 0; // null terminate		
}


/*==============================================================================
* exstrcat
*
* Combines two null-terminated strings into one null terminated string. NOTE: 
* first string MUST be large enough to add the second one in.
*=============================================================================*/ 
void exstrcat(char * dest, char * src)
{
  while(*dest != 0) dest++;

  while (*src != 0)
  {
    *dest++ = *src++;
  }
  *dest = 0; // null terminate
}


/*==============================================================================
* exstrlen
*
* Returns length of string.  NOTE: only counts up to 255 bytes long!! 
*=============================================================================*/ 
unsigned char exstrlen(const char * string)
{
  unsigned char retval = 0;

  while(*string++ != 0) {
    retval++;
    if (retval == 0) return 0; // if we wrap, just return null length 
  }

  return retval;
}


/*==============================================================================
* exstrnloc
*
* Returns position of needle location in haystack.  NOTE: we use limit because 
* haystack may not be null terminated... 
*=============================================================================*/ 
char * exstrnloc(const char * haystack, const char * needle, unsigned char limit)
{
  unsigned char retval = 0;
  unsigned char liveone = 0;
  char * needletest = (char *)needle;

  for (retval = 0; retval < limit; retval++) {
    if (*needletest == *haystack++) liveone = 1;
      else {
        needletest = (char *)needle;
        liveone = 0;
      }

    if (liveone) {
      needletest++;
      if (0 == *needletest) break;
    }
  }
  if (retval == limit) return 0;

  haystack -= exstrlen(needle);

  return (char *)(haystack);
}


/*==============================================================================
* exatoi
*
* Convert a string buffer to a value
*=============================================================================*/ 
int exatoi(char *string) {
  int res = 0;
  while ((*string >= '0') && (*string <= '9'))
    res = res * 10 + *string++ - '0';
    
  return res;
}


/*==============================================================================
* exitoa
*
* Convert an integer to a string
*=============================================================================*/   
void exitoa(int num, char *string)
{
  if (0 > num) {
    ++num;
    *string++ = '-';
    *exsput_ip1(-num, string) = '\0';
  } else {
    *exsput_i(num, string) = '\0';
  }
}


char *exsput_i(int num, char *string)
{
  if (num / 10 != 0) {
    string = exsput_i(num / 10, string);
  }
  *string++ = (char)('0' + num % 10); 
  return string;
}


char *exsput_ip1(int num, char *string)
{
  int digit;

  digit = (num % 10 + 1) % 10;
  if (num / 10 != 0) {
    string = (digit == 0 ? exsput_ip1 : exsput_i)(num / 10, string);
    *string++ = (char)('0' + digit);
  } else {
    if (digit == 0) {
      *string++ = '1';
    }
    *string++ = (char)('0' + digit); 
  }
  return string;
}

/*==============================================================================
* __error__
*
* The error routine that is called if the driver library encounters an error.
*=============================================================================*/
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif



