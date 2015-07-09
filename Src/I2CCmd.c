//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//      Copyright (C) 2015 Peter Walsh, Milford, NH 03055
//      All Rights Reserved under the MIT license as outlined below.
//
//  FILE
//      I2CCMD.c
//
//  DESCRIPTION
//
//      This is a simple AVR program to explore and test I2C devices
//
//  SYNOPSIS
//
//      Connect an I2C device to the TWI outputs as specified in the datasheet.
//
//      Compile, load, and run this module. The programn will accept commands
//        via the serial port.
//
//      See the HELP_SCREEN definition below for a list of available commands
//
//  VERSION:    2010.12.05
//
//////////////////////////////////////////////////////////////////////////////////////////
//
//  MIT LICENSE
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of
//    this software and associated documentation files (the "Software"), to deal in
//    the Software without restriction, including without limitation the rights to
//    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
//    of the Software, and to permit persons to whom the Software is furnished to do
//    so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//    all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//    INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//    PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <ctype.h>

#include "UART.h"
#include "Serial.h"
#include "I2C.h"
#include "GetLine.h"
#include "Parse.h"
#include "VT100.h"

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// Data declarations
//
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//
// Our slave address, if anyone on the bus attempts to address us as a slave.
//
#define OUR_I2C_ADDR    0x31

#define MAX_RWBYTES 0xF0

uint8_t SlaveAddr;
uint8_t Buffer[MAX_RWBYTES];
uint8_t nBytes;
uint8_t Reg;
uint8_t nSlaves;
I2C_STATUS Status;

uint8_t Value;
char    *Token;

uint8_t OurAddr = OUR_I2C_ADDR;

//
// Static layout of the help screen
//
#define HELP_SCREEN "\
R <slave> <nBytes>                Read  data bytes from slave\r\n\
W <slave> <Byte1> [<Byte2>] ...   Write data bytes to   slave\r\n\
S                                 Scan for slaves on bus\r\n\
D <slave> <reg> <nBytes>          Dump slave registers starting at <reg>\r\n\
G <slave> <reg> <nBytes>          Dump slave registers using repeated start\r\n\
\r\n\
H           Show this help panel\r\n\
?           Show this help panel\r\n\
\r\n\
All values hex, lead 0x may be omitted.\r\n\
Get  command uses repeated start.\r\n\
Dump command uses full write followed by read.\r\n\
"

#define BEEP    "\007"

char *StatusText[] = {
    "I2C_COMPLETE",
    "I2C_WORKING",
    "I2C_NO_SLAVE_ACK",
    "I2C_SLAVE_DATA_NACK",
    "I2C_REP_START",
    "I2C_MT_ARB_LOST",
    "I2C_BUS_ERROR" };

#define DS1307_ADDR 0x68

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// I2CCmd - Send commands to the I2C bus
//
// Inputs:      None. (Embedded program - no command line options)
//
// Outputs:     None. (Never returns)
//
int __attribute__((OS_main))  main(void) {

    //////////////////////////////////////////////////////////////////////////////////////
    //
    // Initialize the UART
    //
    UARTInit();
    I2CInit(100,OurAddr,true);

    sei();                              // Enable interrupts

    ClearScreen;
    //
    // End of init
    //
    //////////////////////////////////////////////////////////////////////////////////////

    PrintString("I2C CMD\r\n");
    PrintString("Type '?' for help");
    PrintCRLF();
    PrintCRLF();

    GetLineInit();

    //////////////////////////////////////////////////////////////////////////////////////
    //
    // All done with init,
    // 
    while(1) {
        //
        // Process user commands
        //
        ProcessSerialInput(GetUARTByte());
        } 
    }


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// DumpDebug - Print out debugging info set by the driver
//
// Inputs:      None (reads extern array set by driver)
//
// Outputs:     None.
//
#ifdef DEBUG_I2C

static void DumpDebug(void) {
    extern uint8_t  I2C_Debug[I2C_DEBUG_SIZE];
    extern uint8_t *I2C_DebugPtr;

    PrintString("Dbg: SS CC\r\n");

    for( uint8_t *TempPtr=I2C_Debug; TempPtr < I2C_DebugPtr; TempPtr += 3 ) {
        PrintD(TempPtr-I2C_Debug,3);
        PrintString(": ");
        PrintH(TempPtr[0]);
        PrintString(" ");
        PrintH(TempPtr[1]);
        PrintString(" ");
        PrintH(TempPtr[2]);
        PrintCRLF();
        }
    PrintCRLF();
    }

#else

#define DumpDebug()

#endif


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// ParseValue - Parse next token as value
//
// Inputs:      None (uses next token on command line)
//
// Outputs:     TRUE  if valid number token seen
//              FALSE if some problem
//
static bool ParseValue(void) {
    Token = ParseToken();

    if( Token[0]          == '0' &&
        tolower(Token[1]) == 'x' )
        Token += 2;

    //
    // For the time being, just do hex chars
    //
    if( strlen(Token) == 0 ||
        strlen(Token)  > 2   )
        return false;

    if( !isxdigit(Token[0]) )
        return false;

    Value = toupper(Token[0]) > '9' ? toupper(Token[0]) - 'A' + 10 : Token[0] - '0';

    if( strlen(Token) == 1 )
        return true;

    if( !isxdigit(Token[1]) )
        return false;

    Value <<= 4;
    Value += toupper(Token[1]) > '9' ? toupper(Token[1]) - 'A' + 10 : Token[1] - '0';

    return true;
    }


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// ParseNBytes - Parse the # bytes token
//
// Inputs:      None (examines next token on line)
//
// Outputs:     TRUE  if value parsed correctly
//              FALSE if out or range or other error
//
static bool ParseNBytes(void) {

    if( !ParseValue() ) {
        PrintString("Unrecognized nBytes (");
        PrintString(Token);
        PrintString("), must 2 hex chars.\r\n");
        PrintString("Type '?' for help\r\n");
        PrintCRLF();
        return(false);
        }

    if( Value > MAX_RWBYTES ) {
        PrintString("nBytes too big (");
        PrintString(Token);
        PrintString("), must <= ");
        PrintH(MAX_RWBYTES);
        PrintString(".\r\n");
        PrintString("Type '?' for help\r\n");
        PrintCRLF();
        return(false);
        }

    if( Value == 0 ) {
        PrintString("nBytes cannot be zero! Causes hang!\r\n");
        PrintString("Type '?' for help\r\n");
        PrintCRLF();
        return(false);
        }

    nBytes = Value;
    return(true);
    }


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// PrintResults - Print out a text representation of the I2C status
//
// Inputs:      TRUE if should also dump buffer (from READ or DUMP cmd)
//
// Outputs:     None. Prints status
//
static void PrintResults(bool PrintBuffer) {
    Status = I2CStatus();

    if( Status <= I2C_LAST_ERROR ) PrintString(StatusText[Status-I2C_COMPLETE]);
    else                           PrintString("????");
    PrintString(" (");

    PrintH(Status);
    PrintString(")\r\n");

    if( PrintBuffer && Status == I2C_COMPLETE ) {
        PrintString("Data:\r\n");
        for( int i=0; i<nBytes; i++ ) {
            PrintString("  0x");
            PrintH(i);
            PrintString(": 0x");
            PrintH(Buffer[i]);
            PrintString("  0b");
            PrintB(Buffer[i]);
            PrintCRLF();
            }
        PrintCRLF();
        }
    }


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// SerialCommand - Manage command lines for this program
//
// Inputs:      Command line typed by user
//
// Outputs:     None.
//
void SerialCommand(char *Line) {
    char    *Command;

    ParseInit(Line);
    Command = ParseToken();

    //
    // R - Read bytes from slave
    //
    if( StrEQ(Command,"R") ) {
        if( !ParseValue() ) {
            PrintString("Unrecognized slave addr (");
            PrintString(Token);
            PrintString("), must 2 hex chars.\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }
        SlaveAddr = Value;

        if( !ParseNBytes() )
            return;

        memset(Buffer,0xFF,sizeof(Buffer));
        GetI2CW(SlaveAddr,nBytes,Buffer);
        PrintResults(true);
        DumpDebug();
        return;
        }


    //
    // W - Write bytes to slave
    //
    if( StrEQ(Command,"W") ) {
        if( !ParseValue() ) {
            PrintString("Unrecognized slave addr (");
            PrintString(Token);
            PrintString("), must 2 hex chars.\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }
        SlaveAddr = Value;

        for( nBytes = 0; nBytes < MAX_RWBYTES; nBytes++ ) {
            if( !ParseValue() )
                break;
            Buffer[nBytes] = Value;
            }

        if( ParseValue() ) {
            PrintString("Too much data (");
            PrintString(Token);
            PrintString("), must <= ");
            PrintH(MAX_RWBYTES);
            PrintString(".\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }

        PutI2CW(SlaveAddr,nBytes,Buffer,false);
        PrintResults(false);
        DumpDebug();
        return;
        }


    //
    // S - Scan for slaves by reading register (default: Reg 0)
    //
    if( StrEQ(Command,"S") ) {
        nSlaves = 0;
        PrintString("Addr: Result\r\n");
        for( SlaveAddr = 0; SlaveAddr <= 127; SlaveAddr++ ) {
            GetI2CW(SlaveAddr,1,Buffer);
            Status = I2CStatus();
            if( Status == I2C_NO_SLAVE_ACK )
                continue;
            PrintH(SlaveAddr);
            PrintString("  : ");
            PrintString(StatusText[Status-I2C_COMPLETE]);
            PrintCRLF();
            if( Status != I2C_NO_SLAVE_ACK )
                nSlaves++;
            }
        PrintD(nSlaves,0);
        PrintString(" responses\r\n");
        PrintCRLF();
        DumpDebug();
        return;
        }


    //
    // D - Dump specified registers from device
    //
    if( StrEQ(Command,"D") ) {
        if( !ParseValue() ) {
            PrintString("Unrecognized slave addr (");
            PrintString(Token);
            PrintString("), must 2 hex chars.\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }
        SlaveAddr = Value;

        if( !ParseValue() ) {
            PrintString("Unrecognized reg (");
            PrintString(Token);
            PrintString("), must 2 hex chars.\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }
        Reg = Value;

        if( !ParseNBytes() )
            return;

        memset(Buffer,0xFF,sizeof(Buffer));
        PutI2CW(SlaveAddr,1,&Reg,false);
        PrintString("Write: ");
        PrintResults(false);
        GetI2CW(SlaveAddr,nBytes,Buffer);
        PrintString("Read:  ");
        PrintResults(true);
        DumpDebug();
        return;
        }


    //
    // G - Get all registers using repeated start
    //
    if( StrEQ(Command,"D") ) {
        if( !ParseValue() ) {
            PrintString("Unrecognized slave addr (");
            PrintString(Token);
            PrintString("), must 2 hex chars.\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }
        SlaveAddr = Value;

        if( !ParseValue() ) {
            PrintString("Unrecognized reg (");
            PrintString(Token);
            PrintString("), must 2 hex chars.\r\n");
            PrintString("Type '?' for help\r\n");
            PrintCRLF();
            return;
            }
        Reg = Value;

        if( !ParseNBytes() )
            return;

        memset(Buffer,0xFF,sizeof(Buffer));
        PutI2CW(SlaveAddr,1,&Reg,true);
        PrintString("Write: ");
        PrintResults(false);
        GetI2CW(SlaveAddr,nBytes,Buffer);
        PrintString("Read:  ");
        PrintResults(true);
        DumpDebug();
        return;
        }


#ifdef DEBUG_I2C
    //
    // X - Do user-defined debug command
    //
    if( StrEQ(Command,"X") ) {
        DumpDebug();
        return;
        }
#endif


    /////////////////////////////////////////////////////////////////////////////////////
    //
    // Regular commands follow
    //

    //
    // HE - Help screen
    //
    if( StrEQ(Command,"H") ||
        StrEQ(Command,"?" ) ) {
        PrintCRLF();
        PrintString(HELP_SCREEN);
        PrintCRLF();
        return;
        }


    //
    // Not a recognized command. Let the user know he goofed.
    //
    PrintStringP(PSTR(BEEP));
    PrintStringP(PSTR("Unrecognized Command \""));
    PrintString (Command);
    PrintStringP(PSTR("\"\r\n"));
    PrintString("Type '?' for help\r\n");
    PrintCRLF();
    }
