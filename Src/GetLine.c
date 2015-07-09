//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//      Copyright (C) 2015 Peter Walsh, Milford, NH 03055
//      All Rights Reserved under the MIT license as outlined below.
//
//  FILE
//      GetLine.c
//
//  DESCRIPTION
//
//      Input (serial) line processing
//
//      Read an input line until EOL, with BS and CR-LF processing
//
//  VERSION:    2014.11.05
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

#include <string.h>
#include <ctype.h>

#include "GetLine.h"
#include "Serial.h"
#include "VT100.h"

//
// Define this next def and serial input will be echoed back to the user
//   (Debugging thingy.)
//
//#define ECHO_INPUT

//
// Maximum size of an input entry.  In other words, the maximum
//   number of characters that can be entered on a single line
//   for input over the serial port.
//
#define MAX_CMD_LENGTH    100             

//
// Backspace and ESC character
//
#define BACKSPACE   '\b'
#define ESC         '\033'

#define INPUT_COL   1
#define INPUT_ROW   21

#ifndef PROMPT
#define PROMPT      "Cmd> "
#endif

//
// IsDelimiter() macro
//
static char LineBuffer[MAX_CMD_LENGTH+1];
static int  nChars;

/////////////////////////////////////////////////////////////////////////////////
//
// StrEQ - Compare two strings without regard to case
//
// Inputs:      Char string to check
//              Char string to check against
//
// Outputs:     TRUE  if strings are equal
//              FALSE otherwise
//
bool StrEQ(const char *String1, const char *String2) {

    while( *String1 != 0 ) {
        char    c1 = *String1++;
        char    c2 = *String2++;

        if( c1 == ' ' )
            return(true);

        if( isalpha(c1) ) c1 = toupper(c1);
        if( isalpha(c2) ) c2 = toupper(c2);

        if( c1 != c2 ) 
            return(false);
        }

    //
    // Note that if strlen(String2) < strlen(String1), the NUL at the
    //   end of String2 will cause a preemptive return here before we
    //   access off the end of the string.
    //
    if( *String2 != 0 ) 
        return(false);

    return(true);
    }

/////////////////////////////////////////////////////////////////////////////////
//
// InitLineBuffer - Initialize line buffer for next input
//
// Inputs:      None.
//
// Outputs:     None.
//
static void InitLineBuffer() {

    memset(LineBuffer,0,sizeof(LineBuffer));
    nChars = 0;
    }

/////////////////////////////////////////////////////////////////////////////////
//
// GetLineInit - System initialization for line processing
//
// Inputs:      None.
//
// Outputs:     None.
//
void GetLineInit() {

    InitLineBuffer();
    Prompt();
    }

/////////////////////////////////////////////////////////////////////////////////
//
// Prompt - Print out a prompt
//
// Inputs:      None.
//
// Outputs:     None.
//
void Prompt(void) {

#ifndef LINE_BASED
    CursorPos(INPUT_COL,INPUT_ROW);
#endif
    ClearEOL;
    PrintString(PROMPT);
    }

/////////////////////////////////////////////////////////////////////////////////
//
// PlotCursor - Put cursor in correct position for input
//
// Inputs:      None.
//
// Outputs:     None.
//
void PlotCursor(void) {

#ifndef LINE_BASED
    PrintStringP(PSTR("\033["));
    PrintD(INPUT_ROW,0);
    PrintStringP(PSTR(";"));
    PrintD(INPUT_COL+strlen(LineBuffer)+strlen(PROMPT),0);
    PrintStringP(PSTR("H"));
#endif
    }

/////////////////////////////////////////////////////////////////////////////////
//
// ProcessSerialInput - Parse serial line input chars
//
// Collect chars until a terminator is seen, then pass the input 
//   buffer to SystemCommand().
//
// Inputs:      Serial input char to process
//
// Outputs:     None.
//
void ProcessSerialInput(char InChar) {

    //
    // A short buffer for echoed characters. Since some characters echo as
    //   more than one output char (BS -> BS-SP-BS), we need space for
    //   a couple of chars and the terminating NUL.
    //
    char    TempOutLine[6];

    //
    // Always ignore NUL characters. This can be noise on the serial
    //   line, or (more probably) the caller not bothering to check
    //   the return value from the input routine.
    //
    if( InChar == 0 )
        return;

    //
    // Always accept BS character as "Erase previous character"
    //
    if( InChar == BACKSPACE ) {
        if( nChars != 0 ) {        // If not at start of line
            PlotCursor();
            LineBuffer[--nChars] = 0;
            TempOutLine[0] = BACKSPACE;
            TempOutLine[1] = ' ';
            TempOutLine[2] = BACKSPACE;
            TempOutLine[3] = 0;
            PrintString(TempOutLine);
            }
        return;
        }

    //
    // Setup to echo the characters.
    //
    if( InChar == '\r' ) {              // Switch \r to \r\n
        TempOutLine[0] = '\r';
        TempOutLine[1] = '\n';
        TempOutLine[2] = 0;
        }
    else if( InChar != '\n' ) {         // And print nothing for \n
        TempOutLine[0] = InChar;
        TempOutLine[1] = 0;
        }
    else TempOutLine[0] = 0;

    PlotCursor();
    PrintString(TempOutLine);

    //
    // In the lingo of the system, a '\r' indicates EOL
    //
    if( InChar == '\r' || InChar == ESC ) {

        //
        // ESC causes us to clear the line buffer and process ESC
        //   as a command.
        //
        if( InChar == ESC ) 
            strcpy(LineBuffer,ESC_CMD);

        SerialCommand(LineBuffer);
        InitLineBuffer();
        Prompt();
        return;
        }
    else if (InChar == '\n')               // Do not add \n characters to
        return;                            // LineBuffer.


    //
    // Not a TERMINATOR character, must be part of a COMMAND.  Add it to the
    //   line buffer string if there is room.
    //
    if( nChars < MAX_CMD_LENGTH )
        LineBuffer[nChars++] = InChar;
    }
