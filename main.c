//*****************************************************************************
//
// main.c - Main application function
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

#include "utils/ustdlib.h"
#include "utils/exosite_gprs.h"
#include "utils/exosite_utils.h"
#include "utils/exosite_cloud.h"

// only define PUBLICTEMP if 'pubtemp' data source is available
//#define PUBLICTEMP 1

#define LOOPDELAY 60 //seconds

/*==============================================================================
* main
*
* This is our entry point and main application function.
*=============================================================================*/
int
main(void)
{
  unsigned char pingNum = 0;
  char pingNumStr[5];
  char messageStr[30];
  char valueStr[10];


  //Initialize cloud-communication cependent peripherals & turns on interrupts
  Exosite_Init();

  WriteUILine("Exosite GPRS Demo", LCD_TITLE_LINE);
#ifdef PART_LM3S6965
  WriteUILine("UART0,115200,8,N,1", LCD_UART_LINE);
#endif
#ifdef PART_LM3S1968
  WriteUILine("UART2,115200,8,N,1", LCD_UART_LINE);
#endif
#ifdef PART_RDK_IDM_L35
  WriteUILine("UART2,115200,8,N,1", LCD_UART_LINE);
#endif
  //Start cloud communications
  Exosite_Start();

  // Loop forever in our main app
  while(1)
  {
    WriteUILine("Pre POST", LCD_DEBUG_LINE);
    WriteUILine(LCD_BLANK_LINE, LCD_STATUS_LINE);
    exstrcpy(messageStr, "ping=");
    exitoa(pingNum, pingNumStr);
    exstrcat(messageStr,pingNumStr);
    if (AT_REPLY_OK == Exosite_Write(messageStr)) {
      exstrcat(messageStr," -> POST value");
      WriteUILine(messageStr, LCD_DEBUG_LINE);
    }
    else
    {
      WriteUILine("POST Error", LCD_DEBUG_LINE);
    }

    Delay(2);

    WriteUILine("Pre GET", LCD_DEBUG_LINE);
    WriteUILine(LCD_BLANK_LINE, LCD_STATUS_LINE);
    if (AT_REPLY_OK == Exosite_Read("interval", valueStr)) {
      exstrcpy(messageStr, "interval is: ");
      exstrcat(messageStr,valueStr);
      WriteUILine(messageStr, LCD_DEBUG_LINE);
    }
    else
    {
      WriteUILine("GET Error", LCD_DEBUG_LINE);
    }

#ifdef PUBLICTEMP
    //
    // get public temperature
    //
    Delay(2);

    WriteUILine("Pre GET Temp", LCD_DEBUG_LINE);
    WriteUILine(LCD_BLANK_LINE, LCD_STATUS_LINE);
    if (AT_REPLY_OK == Exosite_Read("pubtemp", valueStr)) {
      exstrcpy(messageStr, "Outside Temp is: ");
    exstrcat(messageStr,valueStr);
    exstrcat(messageStr," F");
    WriteUILine(messageStr, LCD_DEBUG_LINE);
    WriteUILine(messageStr, LCD_DATA_LINE);
    }
    else
    {
      WriteUILine("GET Error", LCD_DEBUG_LINE);
      WriteUILine("Outside Temp Not Available", LCD_DATA_LINE);
    }
#endif

    Delay(LOOPDELAY);

    if (100 == pingNum++)
      pingNum = 0;
  }
}


