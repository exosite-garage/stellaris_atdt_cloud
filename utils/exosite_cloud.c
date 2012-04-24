//*****************************************************************************
//
// exosite_cloud.c - Prototypes for the Exosite Cloud Read/Write 
//                 
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
#include "driverlib/interrupt.h"
#include "exosite_cloud.h"
#include "exosite_gprs.h"
#include "exosite_strings.h"
#include "exosite_utils.h"

// Internal Globals
char usercik[41];


/*==============================================================================
* Exosite_Init
*
* Initializes peripherals required for communications
*=============================================================================*/
char 
Exosite_Init(void)
{
	//Initialize board-specific peripherals
	InitializeBoard();

  // Setup the UI for eye candy and debug output
	InitializeUI();
  
  // Enable processor interrupts.
  IntMasterEnable();
  
  return 1; 
}


/*==============================================================================
* Exosite_Start
*
* This starts our communications with our Internet-capable transport
*=============================================================================*/
char 
Exosite_Start(void)
{
  WriteUILine("Starting Exosite Comm.", LCD_DEBUG_LINE);
  // Get CIK
  Exosite_Use_CIK(DEVICE_CIK);
  
  //GPRS init
  GPRS_init();
  
  //FUTURE: Try to provision - if provision denied, used cached CIK
  
  return 1; 
}


/*==============================================================================
* Exosite_Write
*
* Sends a HTTP POST request to the Exosite server. Sends the value to the alias
* resource - both alias and value are contained in the string data buffer in the
* format "alias=value"
*=============================================================================*/
char
Exosite_Write(char *data)
{
  unsigned char result = AT_REPLY_NONE;
  char tempstr[75];
  char lenstr[10];
  volatile int length = 0;

  WriteUILine(LCD_BLANK_LINE, LCD_HTTPRESP_LINE);
  result = GPRS_On_Check();
  GPRS_Socket_Close();  
  
  if (AT_REPLY_OK == GPRS_Socket_Open())
  {  
    UARTSend((unsigned char *)POST_HEADER,exstrlen(POST_HEADER));
    UARTSend((unsigned char *)M2_HOST,exstrlen(M2_HOST));

    exstrcpy(tempstr,(char *)CIK_HEADER);
    exstrcat(tempstr, usercik);
    exstrcat(tempstr, "\r\n");
    UARTSend((unsigned char *)tempstr, exstrlen(tempstr));
    
    UARTSend((unsigned char *)POST_CONTENT_TYPE,exstrlen(POST_CONTENT_TYPE));

    exstrcpy(tempstr,(char *)CONTENT_LENGTH);
    exitoa(exstrlen(data), lenstr);
    exstrcat(tempstr, lenstr);
    exstrcat(tempstr, "\r\n\r\n");
    exstrcat(tempstr, data); 
    exstrcat(tempstr, "\r\n\r\n");
    length = exstrlen(tempstr);
    UARTSend((unsigned char *)tempstr, exstrlen(tempstr)); 

    result = ReadHTTPResponse(0);

    if (result == AT_REPLY_OK)
      WriteUILine("POST OK", LCD_STATUS_LINE);
    else if (result == AT_REPLY_ERROR)
      WriteUILine("POST Error", LCD_STATUS_LINE);
    else
      WriteUILine("P No Response", LCD_STATUS_LINE);

    readHTTPStatus(tempstr, 20);
    WriteUILine(tempstr, LCD_HTTPRESP_LINE);
    Delay(2);
    GPRS_Exit_Data_Mode();
    GPRS_Socket_Close();
  }

  return result;
}


/*==============================================================================
* Exosite_Read
*
* Sends a HTTP GET request to the Exosite server.  Gets the value of the alias
* resource and returns the value into the "value" buffer provided by the caller.
*=============================================================================*/
char
Exosite_Read(char *alias, char *value)
{
  unsigned char result = AT_REPLY_NONE;
  char tempstr[75];
  
  WriteUILine(LCD_BLANK_LINE, LCD_HTTPRESP_LINE);   
  result = GPRS_On_Check();
  
  GPRS_Socket_Close(); 
  if (AT_REPLY_OK == GPRS_Socket_Open())
  {
    exstrcpy(tempstr,(char *)GET_HEADER);
    exstrcat(tempstr, alias);
    exstrcat(tempstr, (char *)HTTP11);  	
    UARTSend((unsigned char *)tempstr, exstrlen(tempstr)); 

    UARTSend((unsigned char *)M2_HOST,exstrlen(M2_HOST));

    exstrcpy(tempstr,(char *)CIK_HEADER);
    exstrcat(tempstr, usercik);
    exstrcat(tempstr, "\r\n");
    UARTSend((unsigned char *)tempstr, exstrlen(tempstr)); 

    UARTSend((unsigned char *)GET_CONTENT_TYPE,exstrlen(GET_CONTENT_TYPE));
    
    result = ReadHTTPResponse(1);

    if (result == AT_REPLY_OK) {
      char *lineptr = tempstr;
      readHTTPBody(tempstr, sizeof(tempstr));
      lineptr = exstrnloc(tempstr,"=",sizeof(tempstr));
      lineptr++; //go past the =
      exstrcpy(value, lineptr);
      WriteUILine("GET OK", LCD_STATUS_LINE);
    } else if (result == AT_REPLY_ERROR)
      WriteUILine("GET Error", LCD_STATUS_LINE);
    else
      WriteUILine("G No Response", LCD_STATUS_LINE);

    readHTTPStatus(tempstr, 20);	
    WriteUILine(tempstr, LCD_HTTPRESP_LINE);
    GPRS_Exit_Data_Mode();
    GPRS_Socket_Close();
  }

  return result;
}


/*==============================================================================
* Exosite_Set_CIK
*
* Currently does nothing.  In the future, this can be called during runtime to
* update the board's cloud identifier/auth string in non-volatile.
*=============================================================================*/
void
Exosite_Set_CIK(char *cik)
{
  // Future: write CIK to flash
}


/*==============================================================================
* Exosite_Use_CIK
*
* Called during runtime to change the board's cloud identifier/auth string.  
* NOTE that this function does not update non-volatile.  Anything set here will
* be gone upon power cycle or reset.
*=============================================================================*/
void
Exosite_Use_CIK(char *cik)
{
  exstrcpy(usercik,cik);
}
