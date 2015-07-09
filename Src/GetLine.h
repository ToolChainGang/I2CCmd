//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//      Copyright (C) 2015 Peter Walsh, Milford, NH 03055
//      All Rights Reserved under the MIT license as outlined below.
//
//  FILE
//      GetLine.h
//
//  SYNOPSIS
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

#ifndef GETLINE_H
#define GETLINE_H

#include <stdbool.h>

#define ESC_CMD     "\033"

extern void SerialCommand(char *);

//
// Define this next to avoid VT100 screen positioning and use regular line mode
//
#define LINE_BASED

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
bool StrEQ(const char *String1, const char *String2);

/////////////////////////////////////////////////////////////////////////////////
//
// InitGetLine - System initialization for command processing
//
// Inputs:      None.
//
// Outputs:     None.
//
void GetLineInit(void);

/////////////////////////////////////////////////////////////////////////////////
//
// Prompt - Print out a prompt
//
// Inputs:      None.
//
// Outputs:     None.
//
void Prompt(void);

/////////////////////////////////////////////////////////////////////////////////
//
// PlotCursor - Put cursor in correct position for input
//
// Inputs:      None.
//
// Outputs:     None.
//
void PlotCursor(void);

/////////////////////////////////////////////////////////////////////////////////
//
// ProcessSerialInput - Parse serial command input chars
//
// Collect chars until a terminator is seen, then pass the input 
//   buffer to SystemCommand().
//
// Inputs:      Serial input char to process
//
// Outputs:     None.
//
void ProcessSerialInput(char InChar);

#endif  // PARSE_H - Entire file 
