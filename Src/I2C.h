//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//      Copyright (C) 2015 Peter Walsh, Milford, NH 03055
//      All Rights Reserved under the MIT license as outlined below.
//
//  FILE
//      I2C.h
//
//  SYNOPSIS
//
//      //////////////////////////////////////
//      //
//      // In I2C.h
//      //
//      ...Choose Polled or interrupt mode (Default: Interrupt)
//
//      //////////////////////////////////////
//      //
//      // In main.c
//      //
//      I2CInit(KHz,OurAddr,UseInternalPullups);// Called once at startup
//                                              // 100  => 100 KHz speed
//                                              // Our slave address
//                                              // TRUE => Use internal pullups
//
//      uint8_t Bytes;                          // Buffer to write/read
//
//      PutI2C (SlaveAddr,nBytes,Bytes,NoStop); // Initiate block write
//      PutI2CW(SlaveAddr,nBytes,Bytes);        // Initiate write, wait for completion
//
//      GetI2C (SlaveAddr,nBytes,Bytes);        // Initiate block read
//      GetI2CW(SlaveAddr,nBytes,Bytes);        // Initiate read, wait for completion
//
//      if( I2CBusy() ) ...                     // TRUE if hardware in use
//
//      void I2CISR(void) {...}                 // Process result of command
//
//      Status = I2CStatus();                   // Return status of last command
//
//  DESCRIPTION
//
//      A simple I2C driver module for interrupt driven communications
//        on an AVR processor.
//
//  VERSION:    2014.11.06
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

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// Polled mode/ISR mode depends on the next definition.
//
// Defined (ie - uncommented) means call I2CISR. Undefined (commented out)
//   means don't.
// 
//#define CALL_I2CISR

//
// This is a convoluted protocol. Define "debug" below to enable an array of
//   information to be set while operations are in progress. The main program
//   can then examine the results or print them out &c
//
// See I2C.c for details.
//
//#define DEBUG_I2C
#define I2C_DEBUG_SIZE  30  // Max # of bytes to be recorded

//
// End of user configurable options
//
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

typedef enum {
    I2C_COMPLETE,           // Operation completed, no errors
    I2C_WORKING,            // Working on request - try again later
    I2C_NO_SLAVE_ACK,       // No slave acknowledged address
    I2C_SLAVE_DATA_NACK,    // Slave NACK'd a data transfer
    I2C_REP_START,          // Repeated start sent - internal error
    I2C_ARB_LOST,           // Arbitration lost during transfer
    I2C_BUS_ERROR,          // I2C bus error during transmission
    I2C_LAST_ERROR = I2C_BUS_ERROR,
    } I2C_STATUS;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// Sigh. The standard Atmega 168 file doesn't define these.
//
#ifndef TWI_PORT

#define TWI_PORT    C                   // Port containing SCL, SDA, &c

#define SCL_BIT     5                   // Bits in TWI port
#define SDA_BIT     4

#endif

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// I2CInit - Initialize I2C
//
// This routine initializes the I2C based on the passed speed. Called from
//   init.
//
// Inputs:      Desired communications speed, in KHz
//              Our slave address
//              TRUE = Use internal bus pullups
//
// Outputs:     None.
//
void I2CInit(uint8_t KHz, uint8_t OurAddr, bool UseInternalPullups);

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// I2CStatus - Return status of last command
//
// Inputs:      None.
//
// Outputs:     Status of last command
//
I2C_STATUS I2CStatus(void);


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// PutI2C - Initiate block write to I2C port
//
// Inputs:      Slave address
//              Number of bytes to write
//              Ptr to data to write
//              TRUE if should not send STOP after operation completes.
//
// Outputs:     None.
//
void PutI2C(uint8_t SlaveAddr, uint8_t nBytes,uint8_t *Bytes, bool NoStop);

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// PutI2CW - Initiate block write to I2C port, wait for completion
//
// Like PutI2C, but will block until complete.
//
// Inputs:      Slave address
//              Number of bytes to write
//              Ptr to data to write
//
// Outputs:     None.
//
#define PutI2CW(_s_,_n_,_b_,_p_)                                                \
    { PutI2C(_s_,_n_,_b_,_p_);                                                  \
      while( I2CBusy() );                                                       \
      }                                                                         \

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// I2CBusy - Return TRUE if I2C is busy sending or receiving
//
// Inputs:      None.
//
// Outputs:     TRUE  if I2C is busy sending or receiving
//              FALSE if I2C is idle
//
bool I2CBusy(void);

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// GetI2C - Initiate block read from I2C port
//
// Inputs:      Slave address
//              Number of bytes to read
//              Ptr to data to receive buffer
//
// Outputs:     None.
//
void GetI2C(uint8_t SlaveAddr,uint8_t nBytes,uint8_t *Bytes);


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// GetI2CW - Initiate block write to I2C port, wait for completion
//
// Like GetI2C, but will block until complete.
//
// Inputs:      Slave address
//              Number of bytes to write
//              Ptr to data to write
//
// Outputs:     None.
//
#define GetI2CW(_s_,_n_,_b_)                                                    \
    { GetI2C(_s_,_n_,_b_);                                                      \
      while( I2CBusy() );                                                       \
      }                                                                         \


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//
// I2CISR - User's I2C update routine
//
// Inputs:      None.
//
// Outputs:     None.
//
// NOTE: Only defined if CALL_I2CISR is #defined, see above.
//
#ifdef CALL_I2CISR
void I2CISR(void);
#endif

#endif // I2C_H - entire file
