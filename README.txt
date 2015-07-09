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

In the serial monitor (hyperterm, putty, or whatever), enter H<CR> or ?<CR> to see the
help page list of available commands. These should be:


    R <slave> <nBytes>                Read  data bytes from slave
    W <slave> <Byte1> [<Byte2>] ...   Write data bytes to   slave
    S                                 Scan for slaves on bus
    D <slave> <reg> <nBytes>          Dump slave registers starting at <reg>
    G <slave> <reg> <nBytes>          Dump slave registers using repeated start
    
    H           Show this help panel
    ?           Show this help panel

    All values hex, lead 0x may be omitted.
    Get  command uses repeated start.
    Dump command uses full write followed by read.


