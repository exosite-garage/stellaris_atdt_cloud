//*****************************************************************************
//
// utils.h - Protoypes for exosite_utils.c                
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

#ifndef __EXOSITE_UTILS_H__
#define __EXOSITE_UTILS_H__

// defines
#define LCD_TITLE_LINE  		0
#define LCD_UART_LINE				12
#define LCD_ERROR_LINE			24
#define LCD_DEBUG_LINE		  36
#define LCD_STATUS_LINE			48
#define LCD_HTTPRESP_LINE		60
#define LCD_AT_LINE		  		84

#ifdef PART_LM3S6965
#define LCD_BLANK_LINE "                    "
#endif

#ifdef PART_LM3S1968
#define LCD_BLANK_LINE "                    "
#endif

#ifdef PART_RDK_IDM_L35
#define LCD_BLANK_LINE "                                   "
#define LCD_DATA_LINE		  		120
#endif




#define NUM_RING_PACKETS 100  //be careful here with RAM usage

// extern declarations defined in exosite_utils.c
extern volatile unsigned long g_tickCount;
extern volatile unsigned char ringPacket[NUM_RING_PACKETS];
extern volatile unsigned char ringHead;
extern volatile unsigned char ringTail;
extern volatile unsigned char ringItems;

// function protos
void SysTickIntHandler(void);
void Delay(unsigned long ulSeconds);
void InitializeBoard(void);
void InitializeUI(void);
void WriteUILine(char * string, unsigned char linenumber);

void exstrncpy(char * dest, char * src, unsigned char numbytes);
void exstrcpy(char * dest, char * src);
void exstrcat(char * dest, char * src);
char * exstrnloc(const char * haystack,const char * needle,unsigned char limit);
void exitoa(int, char *);
int exatoi(char *string);
char *exsput_i(int, char *);
char *exsput_ip1(int, char *);
unsigned char exstrlen(const char * string);

void UARTSend(const unsigned char *pucBuffer, unsigned long ulCount);
unsigned char UARTReadBufferChar(unsigned char *);
unsigned char UARTReadBufferLine(unsigned char *fullLine, unsigned int checkTime, char *buffer, unsigned char bufSize, char *termStr);
void IncrementRingHead(void);
void IncrementRingTail(void);
 
#endif

