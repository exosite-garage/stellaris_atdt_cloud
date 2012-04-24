//*****************************************************************************
//
// exosite_gprs.c - GPRS AT commands 
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
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. EXOSITE SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
//***************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/uart.h"
#include "exosite_gprs.h"
#include "exosite_utils.h"

#ifdef PART_LM3S6965
#include "inc/lm3s6965.h"    
#endif

#ifdef PART_LM3S1968
#include "inc/lm3s1968.h"    
#endif

#ifdef PART_RDK_IDM_L35
#include "inc/lm3s1958.h"    
#endif

// LOCALLY DEFINED VARIABLES
unsigned char gprsstate;
volatile unsigned char conn_status = 0;

struct http_packet{
  char status[20];        // Http status code  ex: 200 OK 
  char date[36];          // Date: Fri, 03 Feb 2012 06:17:17 GMT
  char body[255];        // 
};

static struct http_packet http_response;

// CONNECTION STATUS BIT DEFINES
#define CONN_SGACT 			0x01
#define CONN_SD 				0x02
#define CONN_CLOUD 			0x04
#define CONN_NOCARRIER 	0x08
#define CONN_OK 				0x10

// GPRS STATES
enum GPRS_STATES {
	GPRS_STATE_AT_CHECK,
	GPRS_STATE_AT_CREG,
	GPRS_STATE_AT_CGDCONT,
	GPRS_STATE_AT_CGQMIN,
	GPRS_STATE_AT_CGQREQ,
	GPRS_STATE_AT_SCFG,
	GPRS_STATE_AT_SGACT_GET,
	GPRS_STATE_AT_SGACT_SET,
	GPRS_STATE_AT_SD,
	GPRS_STATE_AT_SH  	
};

// OTHER LOCAL DEFINES
#define GPRS_RETRY_INTERVAL        10      //  AT command retry Interval  
#define GPRS_RETRY_MAX              3      //  AT command retry Count Max

/*==============================================================================
* GPRS_init
*
* Initializes GPRS communications.  Ran at startup.
*=============================================================================*/
void
GPRS_init()
{
  unsigned char error = 0;
  unsigned char retryCount = 0;
  gprsstate = GPRS_STATE_AT_CHECK;
  WriteUILine("AT: GPRS Initialization...", LCD_DEBUG_LINE);
  while(gprsstate < GPRS_STATE_AT_SD)
  {
    switch (gprsstate)
    {
      case GPRS_STATE_AT_CHECK:
        WriteUILine("AT: GPRS On Check...", LCD_DEBUG_LINE);
        error = GPRS_On_Check();
      break;
      case GPRS_STATE_AT_CREG:
        WriteUILine("AT: Check For Network.", LCD_DEBUG_LINE); //Only checks for home network, not roaming
        error = Get_AT_CREG();
      break;
      case GPRS_STATE_AT_CGDCONT:
        WriteUILine("AT: GPRS CGDCONT...", LCD_DEBUG_LINE);
        error = Send_AT_CGDCONT();
      break;
      case GPRS_STATE_AT_CGQMIN:
        WriteUILine("AT: GPRS CGQMIN...", LCD_DEBUG_LINE);             
        error = Send_AT_CGQMIN();
      break;
      case GPRS_STATE_AT_CGQREQ:
        WriteUILine("AT: GPRS CGQREQ...", LCD_DEBUG_LINE);               
        error = Send_AT_CGQREQ();
      break; 
      case GPRS_STATE_AT_SGACT_GET:
        WriteUILine("AT: GPRS SGACT GET...", LCD_DEBUG_LINE);  
        error = Get_AT_SGACT();
      break;
      case GPRS_STATE_AT_SGACT_SET:
        WriteUILine("AT: GPRS SGACT SET...", LCD_DEBUG_LINE); 
        error = Send_AT_SGACT();
      break;
    }

    if (error == AT_REPLY_OK)
    {
      WriteUILine("AT Response: OK", LCD_DEBUG_LINE);
      if (gprsstate == GPRS_STATE_AT_SGACT_GET)
        gprsstate = GPRS_STATE_AT_SD;
      else
        gprsstate++;
    }
    else if (error == AT_REPLY_SGACT_NOT_SET)
    {
      WriteUILine("AT Response: SGACT Not Set", LCD_DEBUG_LINE);
      gprsstate = GPRS_STATE_AT_SGACT_SET;
    }
    else    //error == AT_REPLY_ERROR  or AT_REPLY_TIMED_OUT, retry 
    {
      if (gprsstate == GPRS_STATE_AT_CREG)
      {
        WriteUILine("AT Response: No Network Yet", LCD_DEBUG_LINE);
      }
      else
      {
        WriteUILine("AT Response: ERROR", LCD_DEBUG_LINE);
      }
      if(retryCount < GPRS_RETRY_MAX)
      {
        retryCount++;
      }
      else 
      {
        retryCount = 0;
        gprsstate = GPRS_STATE_AT_CHECK; //start over
        WriteUILine("AT: Init Start Over", LCD_DEBUG_LINE);
      }
      Delay(GPRS_RETRY_INTERVAL);
    }
  }
  WriteUILine("GPRS Initialized", LCD_DEBUG_LINE);
}


/*=============================================================================
*                        AT COMMANDS
*=============================================================================*/


/*==============================================================================
* GPRS_On_Check
*
* Check if GPRS interface is active...
*=============================================================================*/
unsigned char 
GPRS_On_Check(void)
{
  UARTSend((unsigned char *)"at\r\n", 4);

  return atResponseCheck(2, "OK", "\r\n");
}

/*==============================================================================
* Get_AT_CREG
*
* Request status on context activation request.  Telit specific.
*=============================================================================*/
unsigned char 
Get_AT_CREG(void)
{
  UARTSend((unsigned char *)"AT+CREG?\r\n", 11);

  return atResponseCheck(5, "+CREG: 0,1", "\r\n\r\nOK\r\n");  
}

/*==============================================================================
* Send_AT_CGDCONT
*
* Define connection context (APN, etc...)
*=============================================================================*/
unsigned char 
Send_AT_CGDCONT(void)
{
//UARTSend((unsigned char *)"at+CGDCONT=1,\"IP\",\"f3prepaid\",\"0.0.0.0\",0,0\r\n",45);
  UARTSend((unsigned char *)"at+CGDCONT=1,\"IP\",\"",19);
  UARTSend((unsigned char *)SIM_APN,exstrlen(SIM_APN));
  UARTSend((unsigned char *)"\",\"0.0.0.0\",0,0\r\n",17);

   return atResponseCheck(5, "OK", "\r\n");
}


/*==============================================================================
* Send_AT_CGQMIN
*
* Define minimum Quality of Service profile
*=============================================================================*/
unsigned char 
Send_AT_CGQMIN(void)
{
  UARTSend((unsigned char *)"at+cgqmin=1,0,0,0,0,0\r\n", 23);

  return atResponseCheck(5, "OK", "\r\n");
}


/*==============================================================================
* Send_AT_CGQREQ
*
* Define requsted/ideal Quality of Service profile
*=============================================================================*/
unsigned char 
Send_AT_CGQREQ(void)
{
  UARTSend((unsigned char *)"at+cgqreq=1,0,0,3,0,0\r\n", 23);

  return atResponseCheck(5, "OK", "\r\n");    
}


/*==============================================================================
* Send_AT_SCFG
*
* Configure domain name server
*=============================================================================*/
unsigned char 
Send_AT_SCFG(void)
{
  UARTSend((unsigned char *)"at#scfg=1,1,512,90,100,50\r\n", 27);

  return atResponseCheck(5, "OK", "\r\n");
}


/*==============================================================================
* Send_AT_SGACT
*
* Activate context set in Send_AT_CGDCONT.  Telit specific.
*=============================================================================*/
unsigned char 
Send_AT_SGACT(void)
{
  UARTSend((unsigned char *)"AT#SGACT=1,1\r\n", 14);

  return atResponseCheck(15, "SGACT: 1,1", "\r\n\r\nOK\r\n");
}


/*==============================================================================
* Get_AT_SGACT
*
* Request status on context activation request.  Telit specific.
*=============================================================================*/
unsigned char 
Get_AT_SGACT(void)
{
  UARTSend((unsigned char *)"AT#SGACT?\r\n", 11);

  return atResponseCheck(5, "SGACT: 1,1", "\r\n\r\nOK\r\n");
}


/*==============================================================================
* GPRS_Socket_Open
*
* Open a connection with a remote host.  Telit specific.
*=============================================================================*/
unsigned char 
GPRS_Socket_Open(void)
{
  static unsigned char socketFailures = 0;
  //UARTSend((unsigned char *)"AT#SKTD=0,80,\"173.255.209.28\"\r\n",32);
  //HAR: review hard coded IP addresses -> perhapse swap for replaceable
  //ip via "ip" API call
  UARTSend((unsigned char *)"AT#SD=1,0,80,\"173.255.209.28\",0,0,0\r\n",37);

  unsigned char result = atResponseCheck(15, "CONNECT", "\r\n");

  if (result == AT_REPLY_OK) {
    socketFailures = 0;
    WriteUILine("Socket Open", LCD_STATUS_LINE);
  } else {
    if (result == AT_REPLY_ERROR) 
      WriteUILine("Socket Error", LCD_STATUS_LINE);
    else
      WriteUILine("S No response", LCD_STATUS_LINE);

    if (socketFailures++ > 5) {
      GPRS_init();
      socketFailures = 0;
    }
  }
  return result;
}

/*==============================================================================
* GPRS_Socket_Close
*
* Close a connection with a remote host.  Telit specific.
*=============================================================================*/ 
void 
GPRS_Exit_Data_Mode(void)
{
  UARTSend((unsigned char *)"+++",3);
  if (AT_REPLY_OK != atResponseCheck(10, "OK", "\r\n"))
  Delay(1);
  
  Delay(2);
}

/*==============================================================================
* GPRS_Socket_Close
*
* Close a connection with a remote host.  Telit specific.
*=============================================================================*/ 
unsigned char 
GPRS_Socket_Close(void)
{

  UARTSend((unsigned char *)"AT#SH=1\r\n",9);
  return atResponseCheck(5, "OK", "\r\n");
}

/*==============================================================================
* GPRS_Check_NoCarrier
*
* Check for connection to time out.
*=============================================================================*/ 
unsigned char GPRS_Check_NoCarrier(void)
{
  return atResponseCheck(10, "NO CARRIER", "\r\n");
}


/*=============================================================================
*                     RECEIVE DATA PARSING ROUTINES
*=============================================================================*/


/*==============================================================================
* atResponseCheck
*
* Checks for the expected response in our ring buffer.  This is a destructive
* read from the ring buffer - once read, the data is gone.
*=============================================================================*/ 
unsigned char 
atResponseCheck(unsigned int checkTime, char *needle, char *termStr)
{
  char linebuffer[100];
  char * lineptr = linebuffer;
  unsigned char fullLine = 0;
  unsigned char result = AT_REPLY_TIMED_OUT;
  unsigned char line_length = 0;
  unsigned long startTime = g_tickCount;

  WriteUILine(LCD_BLANK_LINE, LCD_AT_LINE);
  // a '0' in checkTime means read a very very long time
  if (!checkTime) checkTime = 0xFFFFFFFF;

  // keep checking until we either get the response we want or until 
  // checkTime is elapsed
  while ((AT_REPLY_OK != result) && ((g_tickCount - startTime) < checkTime)) {
    line_length = UARTReadBufferLine(&fullLine, checkTime, lineptr, sizeof(linebuffer) - (lineptr - linebuffer), termStr);

    //check if we read a full line...
    if (fullLine) {
      line_length += lineptr - linebuffer;
      if (exstrnloc(linebuffer,needle,line_length)) {
        result = AT_REPLY_OK;
      } else {  //if we didn't find the response, but have a line...
        if (exstrnloc(linebuffer,"ERROR",line_length)) { 	// check for errors
          result = AT_REPLY_ERROR;
        } else {
          if (exstrnloc(linebuffer,"OK",line_length)) { // check for ?? some edge case???
            result = AT_REPLY_SGACT_NOT_SET;
          }
        } 
      }
      //since we did not get AT_REPLY_OK yet, we continue to read the next line
      lineptr = linebuffer;
      line_length = 0;
    } else 
      if ((lineptr + line_length - linebuffer) <= sizeof(linebuffer)) lineptr += line_length; // move lineptr in case we need to read again to get a full line
        else break; //we can't read any more
  }

  linebuffer[line_length] = 0;
  WriteUILine(linebuffer, LCD_AT_LINE);

  return result;
}

/*==============================================================================
* ReadHTTPResponse
*
* Reads the HTTP response.  This should only be called immediately after 
* sending an HTTP POST or GET packet to the server.
* 
* POST Response (typical)
*   HTTP/1.1 204 No Content\r\n
*   Date: Wed, 15 Feb 2012 10:29:37 GMT\r\n
*   Server: misultin/0.8.1-exosite\r\n
*   Connection: Keep-Alive\r\n
*   Content-Length: 0\r\n
*   \r\n
* 
* GET Response (typical)
*   HTTP/1.1 200 OK\r\n
*   Date: Wed, 15 Feb 2012 10:29:38 GMT\r\n
*   Server: misultin/0.8.1-exosite\r\n
*   Connection: Keep-Alive\r\n
*   Content-Length: 6\r\n
*   \r\n
*   temp=2
*=============================================================================*/ 
unsigned char ReadHTTPResponse(unsigned char isGet)
{
  char linebuffer[100];
  char * lineptr;
  int bodyChars = 0;
  unsigned char line_length = 0;
  unsigned char fullLine = 0;

  //HTTP header
  line_length = UARTReadBufferLine(&fullLine, 20, linebuffer, sizeof(linebuffer), "\r\n");
  if (!fullLine || !(exstrnloc(linebuffer,"HTTP/1.1 20",line_length))) //we truncate the check so it works for both GET and POST
    return AT_REPLY_ERROR;
  exstrncpy(http_response.status, linebuffer, line_length);
  http_response.status[line_length] = 0; 

  //Date header
  line_length = UARTReadBufferLine(&fullLine, 3, linebuffer, sizeof(linebuffer), "\r\n");
  if (!fullLine || !(exstrnloc(linebuffer,"Date:",line_length)))
    return AT_REPLY_ERROR;
  exstrncpy(http_response.date, linebuffer, line_length);
  http_response.date[line_length] = 0;

  //Server header	
  line_length = UARTReadBufferLine(&fullLine, 3, linebuffer, sizeof(linebuffer), "\r\n");
  if (!fullLine || !(exstrnloc(linebuffer,"Server:",line_length)))
    return AT_REPLY_ERROR;

  //Connection header
  line_length = UARTReadBufferLine(&fullLine, 3, linebuffer, sizeof(linebuffer), "\r\n");
  if (!fullLine || !(exstrnloc(linebuffer,"Connection:",line_length)))
    return AT_REPLY_ERROR;

  //Content-Length
  line_length = UARTReadBufferLine(&fullLine, 3, linebuffer, sizeof(linebuffer), "\r\n");
  if (!fullLine || !(exstrnloc(linebuffer,"Content-Length:",line_length))) {
    return AT_REPLY_ERROR;
  } else { //read content length
    lineptr = exstrnloc(linebuffer,":",line_length);
    lineptr++; //go past the :
    lineptr++; //go past the space
    bodyChars = exatoi(lineptr);
  }

  //Blank Line
  line_length = UARTReadBufferLine(&fullLine, 3, linebuffer, sizeof(linebuffer), "\r\n");
  if (!fullLine) // \r\n only
    return AT_REPLY_ERROR;

//If a GET request, read the body	  	  	        
  if (isGet) {
    lineptr = http_response.body;
    while (bodyChars) {
      if (UARTReadBufferChar((unsigned char *)lineptr)) {
        lineptr++;
        bodyChars--;
      }
    }
    *lineptr = 0; //null terminate
  }
  return AT_REPLY_OK;	
}


/*==============================================================================
* readHTTPStatus
*
* Returns latest HTTP Response Status line
*=============================================================================*/ 
void readHTTPStatus(char *status, char bufsize)
{
  char copylen = bufsize - 1;

  if (copylen > exstrlen(http_response.status))
    copylen = exstrlen(http_response.status);

  exstrncpy(status,http_response.status,copylen);
  status += copylen;
  *status = 0; // null terminate
}


/*==============================================================================
* readHTTPDate
*
* Returns latest HTTP Response Status line
*=============================================================================*/ 
void readHTTPDate(char *date, char bufsize)
{
  char copylen = bufsize - 1;

  if (copylen > exstrlen(http_response.date))
    copylen = exstrlen(http_response.date);

  exstrncpy(date,http_response.date,copylen);
  date += copylen;
  *date = 0; // null terminate  
}


/*==============================================================================
* readHTTPBody
*
* Returns latest HTTP Body
*=============================================================================*/ 
void readHTTPBody(char *body, char bufsize)
{
  char copylen = bufsize - 1;

  if (copylen > exstrlen(http_response.body))
    copylen = exstrlen(http_response.body);

  exstrncpy(body,http_response.body,copylen);
  body += copylen;
  *body = 0; // null terminate    
}

