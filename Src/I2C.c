//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//      Copyright (C) 2015 Peter Walsh, Milford, NH 03055
//      All Rights Reserved under the MIT license as outlined below.
//
//  FILE
//      I2C.c
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
//      I2CInit(KHz,UseInternalPullups);        // Called once at startup
//                                              // 100  => 100 KHz speed
//                                              // TRUE => Use internal pullups
//
//      uint8_t Buffer;                         // Buffer to write/read
//
//      PutI2C (SlaveAddr,nBytes,Buffer,NoStop);// Initiate block write
//      PutI2CW(SlaveAddr,nBytes,Buffer);       // Initiate write, wait for completion
//
//      GetI2C (SlaveAddr,nBytes,Buffer);       // Initiate block read
//      GetI2CW(SlaveAddr,nBytes,Buffer);       // Initiate read, wait for completion
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
//  VERSION:    2013.01.06
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
#include <avr/interrupt.h>

#include "PortMacros.h"
#include "I2C.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
// This is a convoluted protocol. Define "debug" below to enable an array of
//   information to be set while operations are in progress. The main program
//   can then examine the results or print them out &c
//
#ifdef DEBUG_I2C

uint8_t  I2C_Debug[I2C_DEBUG_SIZE];
uint8_t *I2C_DebugPtr;

#define ADD_DEBUG(_x_)                                                                  \
    { if( I2C_DebugPtr < I2C_Debug+I2C_DEBUG_SIZE ) *I2C_DebugPtr++ = (_x_); }          \

#define INIT_DEBUG                                                                      \
    { memset(I2C_Debug,0xFF,sizeof(I2C_Debug)); I2C_DebugPtr = I2C_Debug; }             \

#else

#define ADD_DEBUG(_x_)
#define INIT_DEBUG

#endif
//
//////////////////////////////////////////////////////////////////////////////////////////

static struct {
    uint8_t     SlaveAddr;      // Slave address
    uint8_t     nBytes;         // Number of bytes left to process
    uint8_t    *Buffer;         // Buffer for request
    bool        NoStop;         // Don't send stop after request
    I2C_STATUS  Status;         // Status of request
    } I2C NOINIT;

//
// TWSR values, assumes prescaler bits have been zeroed.
//
#define TW_START					0x08
#define TW_REP_START				0x10
#define TW_ARB_LOST				    0x38
// Master Transmitter
#define TW_MT_SLA_ACK				0x18
#define TW_MT_SLA_NACK				0x20
#define TW_MT_DATA_ACK				0x28
#define TW_MT_DATA_NACK				0x30
// Master Receiver
#define TW_MR_SLA_ACK				0x40
#define TW_MR_SLA_NACK				0x48
#define TW_MR_DATA_ACK				0x50
#define TW_MR_DATA_NACK				0x58
// Slave Transmitter
#define TW_ST_SLA_ACK				0xA8
#define TW_ST_ARB_LOST_SLA_ACK		0xB0
#define TW_ST_DATA_ACK				0xB8
#define TW_ST_DATA_NACK				0xC0
#define TW_ST_LAST_DATA				0xC8
// Slave Receiver
#define TW_SR_SLA_ACK				0x60
#define TW_SR_ARB_LOST_SLA_ACK		0x68
#define TW_SR_GCALL_ACK				0x70
#define TW_SR_ARB_LOST_GCALL_ACK	0x78
#define TW_SR_DATA_ACK				0x80
#define TW_SR_DATA_NACK				0x88
#define TW_SR_GCALL_DATA_ACK		0x90
#define TW_SR_GCALL_DATA_NACK		0x98
#define TW_SR_STOP					0xA0
// Misc
#define TW_NO_INFO					0xF8
#define TW_BUS_ERROR				0x00

//
// Slave address bit indicating READ or WRITE
//
#define SLAVE_READ                  0x01

//
// Some useful macros
//
#define START_I2C   _SET_MASK(TWCR,_PIN_MASK(TWINT) | _PIN_MASK(TWSTA));
#define STOP_I2C    _SET_MASK(TWCR,_PIN_MASK(TWINT) | _PIN_MASK(TWSTO));
#define STEP_I2C    _SET_BIT(TWCR,TWINT);

#ifdef CALL_I2CISR
extern  void I2CISR();
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// I2CInit - Initialize I2C
//
// This routine initializes the I2C based on the settings above. Called from
//   init.
//
// Inputs:      None.
//
// Outputs:     None.
//
void I2CInit(uint8_t KHz, uint8_t OurAddr, bool UseInternalPullups) {

    memset(&I2C,0,sizeof(I2C));

    _CLR_BIT(PRR,PRTWI);                    // Power up the I2C

    //
    // Enable internal pullups if requested
    //
    if( UseInternalPullups ) {
        _CLR_BIT(          MCUCR,     PUD); // Clear disable pullup flag
        _SET_BIT(_PORT(TWI_PORT), SCL_BIT); // Set to enable pullups
        _SET_BIT(_PORT(TWI_PORT), SDA_BIT); // Set to enable pullups
        }
    //
    // Enable pullups if requested (above), but don't *disable* them if
    //   not needed (that's the caller's responsibility).
    //
    else {
        _CLR_BIT(_PORT(TWI_PORT), SCL_BIT); // Set to disable pullups
        _CLR_BIT(_PORT(TWI_PORT), SDA_BIT); // Set to disable pullups
        }

    //
    // Set bitrate in KHz.
    //
    _CLR_BIT(TWSR,TWPS0);       // Prescaler => 1
    _CLR_BIT(TWSR,TWPS1);

    TWBR = (F_CPU/(1000*KHz) - 16)/2;

    //
    // Enable TWI (two-wire interface), enable interrupts
    //
    I2C.Status = I2C_COMPLETE;

    _SET_BIT(TWCR,TWEN);        // Enable TWI
    _SET_BIT(TWCR,TWIE);        // Enable Interrupts

    //
    // Enable ACK of received bytes
    //
    TWAR = OurAddr;
    _CLR_BIT(TWCR,TWEA);

    INIT_DEBUG;
    }


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//
// PutI2C - Initiate block write to I2C port
//
// Inputs:      Slave address
//              Number of bytes to write
//              Ptr to data to write
//              TRUE if should not send STOP after operation complete.
//
// Outputs:     None.
//
void PutI2C(uint8_t SlaveAddr, uint8_t nBytes,uint8_t *Buffer, bool NoStop) {

    //
    // Send start signal and wait for response.
    //
    I2C.SlaveAddr = SlaveAddr << 1;         // Low order bit clr ==> Write
    I2C.nBytes    = nBytes;
    I2C.Buffer    = Buffer;
    I2C.Status    = I2C_WORKING;
    I2C.NoStop    = NoStop;

    INIT_DEBUG;

    START_I2C;
    }


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//
// GetI2C - Initiate block read from I2C port
//
// Inputs:      Slave address
//              Number of bytes to read
//              Ptr to data to write
//
// Outputs:     None.
//
void GetI2C(uint8_t SlaveAddr, uint8_t nBytes,uint8_t *Buffer) {

    //
    // Send start signal and wait for response.
    //
    I2C.SlaveAddr = (SlaveAddr << 1) | 1;   // Low order bit set ==> Read
    I2C.nBytes    = nBytes;
    I2C.Buffer    = Buffer;
    I2C.Status    = I2C_WORKING;

    INIT_DEBUG;

    START_I2C;
    }


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//
// I2CBusy   - Return TRUE if I2C is busy sending output
//
// Inputs:      None
//
// Outputs:     TRUE  if I2C is busy sending output
//              FALSE if I2C is idle
//
bool I2CBusy(void) { return I2C.Status == I2C_WORKING; }

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//
// I2CStatus - Return status of last operation
//
// Inputs:      None
//
// Outputs:     Status (could be I2C_Working, or status of last op)
//
I2C_STATUS I2CStatus(void) { return I2C.Status; }

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//
// TWI_vect - TWI interrupt state machine
//
// Take the next step in whichever TWI operation is in progress.
//
// Inputs:      None. (ISR)
//
// Outputs:     None.
//
ISR(TWI_vect) {
    uint8_t Status = TWSR & (~(_PIN_MASK(TWPS0) | _PIN_MASK(TWPS1)));

    ADD_DEBUG(Status);
    ADD_DEBUG(TWCR);

    switch(Status) {

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_START - Start condition has been transmitted,
        //
        // Turn off start, send slave address
        //
        case TW_START:
        case TW_REP_START:
            TWDR = I2C.SlaveAddr;
            _CLR_BIT(TWCR,TWSTA);           // Indirectly clears TWINT as well :-)
            ADD_DEBUG(I2C.SlaveAddr);
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_MT_SLA_ACK  - Slave acknowledged address.
        // TW_MT_DATA_ACK - Slave received data
        //
        // If no [more] data to send, terminate the transfer. Otherwise, send the
        //   next data byte.
        //
        case TW_MT_SLA_ACK:
        case TW_MT_DATA_ACK:
            //
            // If no more bytes to send, send STOP to finish transfer.
            //
            if( I2C.nBytes == 0 ) {
                I2C.Status = I2C_COMPLETE;
                //
                // Don't send a STOP if the caller so chooses. This allows the
                //   caller to setup an address (via PutI2C) and immediately
                //   read data from a slave. Such as an EEPROM.
                //
                // Caller will make a 2nd I2C call, resulting in a restart.
                //
                if( !I2C.NoStop )
                    STOP_I2C;

                ADD_DEBUG(I2C.SlaveAddr);
#ifdef CALL_I2CISR
                I2CISR();
#endif
                return;
                }

            //
            // Otherwise, send [more] data to the slave
            //
            TWDR = *I2C.Buffer++;
            I2C.nBytes--;
            STEP_I2C;
            ADD_DEBUG(I2C.SlaveAddr);
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_MT_SLA_NACK - No slave acknowledged address (transmit)
        // TW_MR_SLA_NACK - No slave acknowledged address ( receive)
        //
        case TW_MT_SLA_NACK:
        case TW_MR_SLA_NACK:
            I2C.Status = I2C_NO_SLAVE_ACK;
            STOP_I2C;
            ADD_DEBUG(I2C.SlaveAddr);
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_MT_DATA_NACK - Slave didn't acknowledge data (transmit)
        //
        case TW_MT_DATA_NACK:
            I2C.Status = I2C_SLAVE_DATA_NACK;
            STOP_I2C;
            ADD_DEBUG(I2C.SlaveAddr);
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_ARB_LOST - Arbitration lost
        //
        // Enter slave mode by stepping the I2C. We don't need to send a STOP, because
        //   we've lost arbitration.
        //
        case TW_ARB_LOST:
            I2C.Status = I2C_ARB_LOST;
            STEP_I2C;
            ADD_DEBUG(I2C.SlaveAddr);
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_MR_SLA_ACK  - Slave acknowledged address
        //
        // Start the first read.
        //
        case TW_MR_SLA_ACK:
            //
            // Special case - if ZERO bytes are to be read, don't step the byte
            //   reading mechanism (below). Just stop the transfer and return.
            //
            if( I2C.nBytes == 0 ) {
                I2C.Status = I2C_COMPLETE;
                STOP_I2C;
                ADD_DEBUG(I2C.SlaveAddr);
                return;
                }

            //
            // Otherwise, setup to ACK all bytes except the last, which gets NACK.
            //
            if( I2C.nBytes == 1 ) { _CLR_BIT(TWCR,TWEA); }  // Last byte gets NACK
            else                  { _SET_BIT(TWCR,TWEA); }  // Enable ack of data
            STEP_I2C;
            ADD_DEBUG(I2C.SlaveAddr);
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_MR_DATA_ACK  - Slave sent data, we sent ACK
        // TW_MR_DATA_NACK - Slave sent data, we sent NACK (meaning - last data byte)
        //
        // If no [more] data to receive, terminate the transfer. Otherwise, start the
        //   next read.
        //
        case TW_MR_DATA_ACK:
        case TW_MR_DATA_NACK:
            //
            // Get the sent byte
            //
            *I2C.Buffer++ = TWDR;
            I2C.nBytes--;

            //
            // Send a NACK on the last data byte
            //
            if( I2C.nBytes == 1 )
                _CLR_BIT(TWCR,TWEA);


            //
            // If no more bytes to get, send STOP to finish transfer.
            //
            if( I2C.nBytes == 0 ) {
                I2C.Status = I2C_COMPLETE;
                STOP_I2C;

                ADD_DEBUG(I2C.SlaveAddr);

#ifdef CALL_I2CISR
                I2CISR();
#endif
                return;
                }

            //
            // Otherwise, request more data from the slave
            //
            STEP_I2C;
            return;

        //////////////////////////////////////////////////////////////////////////////////
        //
        // TW_BUS_ERROR - [TWI] Bus error. Stop and return error
        //
        case TW_BUS_ERROR:
            I2C.Status = I2C_BUS_ERROR;
            STOP_I2C;
            ADD_DEBUG(I2C.SlaveAddr);
            return;
        }

    }
