//*****************************************************************************
//
// gprs.h - Prototypes for the GPRS AT command
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

#ifndef __GPRS_H__
#define __GPRS_H__

// FILL IN YOUR INFO HERE FOR CIK (portals.exosite.com/manage/devices)
#define DEVICE_CIK "f62c91f19f13562838be1cf77bea47fca203a832" // NOTE - NOT USED, HAR

// FILL IN YOUR INFO HERE FOR CELLULAR PROVIDERS APN
#define SIM_APN "cellular.apn.com" //"emome" // NOTE - NOT USED, HAR

// AT RESPONSES
enum AT_RESPONSES {
  AT_REPLY_NONE,
  AT_REPLY_OK,
  AT_REPLY_SGACT_NOT_SET,
  AT_REPLY_ERROR,
  AT_REPLY_TIMED_OUT
};

// FUNCTION PROTOS
void GPRS_init();
unsigned char GPRS_Check_NoCarrier(void);
unsigned char GPRS_On_Check(void);
unsigned char Get_AT_CREG(void);
unsigned char Send_AT_CGDCONT(void);
unsigned char Send_AT_CGQREQ(void);
unsigned char Send_AT_CGQMIN(void);
unsigned char Send_AT_SCFG(void);
unsigned char Send_AT_SGACT(void);
unsigned char Get_AT_SGACT(void);
unsigned char GPRS_Socket_Open(void);
unsigned char GPRS_Socket_Close(void);
void GPRS_Exit_Data_Mode(void);

unsigned char atResponseCheck(unsigned int delayTime, char *needle, char *termStr);
unsigned char ReadHTTPResponse(unsigned char isGet);

void readHTTPStatus(char *status, char bufsize);
void readHTTPDate(char *date, char bufsize);
void readHTTPBody(char *body, char bufsize);

#endif

