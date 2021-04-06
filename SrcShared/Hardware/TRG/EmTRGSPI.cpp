/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmTRGSPI.h"


// --------------------------------------------------------------------------
// This file is used to emulate the SPI controller ... the SPI performs
// double duty for the TRGpro ... normally, it controls the touch screen ...
// however, if a latch bit is set on Port D, it will use the SPI to
// control the keypad and bus-state
// --------------------------------------------------------------------------

#define SPIKeyRow0              0x01
#define SPIKeyRow1              0x02
#define SPIKeyRow2              0x04
#define SPIBusSwapOff           0x08
#define SPIBacklightOn          0x10
#define SPICardBufferOff        0x20
#define SPICardSlotPwrOff       0x40
#define SPIBusWidth8            0x80

static SpiValue UnlatchedVal = 0;
static SpiValue LatchedVal   = 0;

/***********************************************************************
 *
 * FUNCTION:    SpiInitialize
 *
 * DESCRIPTION: This function is called to initialize the SPI functions
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 ***********************************************************************/
void SpiInitialize(void)
{
	UnlatchedVal = LatchedVal = 0;
}

/***********************************************************************
 *
 * FUNCTION:    SpiSetUnlatchedVal
 *
 * DESCRIPTION: This function is called to store an unlatched value for
 *              the SPI ... it will be used later only if SPILatch is
 *              called
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 ***********************************************************************/
void SpiSetUnlatchedVal(SpiValue val)
{
	UnlatchedVal = val;
}

/***********************************************************************
 *
 * FUNCTION:    SpiLatch
 *
 * DESCRIPTION: This function is called if the Port D Latch bit is set
 *              it latches the current SPI value
 *
 * PARAMETERS:  None
 *
 * RETURNED:    None
 *
 ***********************************************************************/
void SpiLatch(void)
{
	LatchedVal = UnlatchedVal;
}

/***********************************************************************
 *
 * FUNCTION:    SpiIsBacklightOn
 *
 * DESCRIPTION: This function returns TRUE if the backlight is currently
 *              turned on
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Backlight state
 *
 ***********************************************************************/
Boolean	SpiIsBacklightOn(void)
{
	return(LatchedVal & SPIBacklightOn) != 0;
}

/***********************************************************************
 *
 * FUNCTION:    SpiGetKeyRows
 *
 * DESCRIPTION: This function returns the state of the key rows
 *
 * PARAMETERS:  'keyRows' has the returned state of the key rows
 *
 * RETURNED:    None
 *
 ***********************************************************************/
void SpiGetKeyRows(KeyRowsType * keyRows)
{
	keyRows->val[0] = (LatchedVal & SPIKeyRow0); 
	keyRows->val[1] = (LatchedVal & SPIKeyRow1); 
	keyRows->val[2] = (LatchedVal & SPIKeyRow2); 
}

/***********************************************************************
 *
 * FUNCTION:    SpiGetBusState
 *
 * DESCRIPTION: This function returns the bus state indicating whether
 *              the slot is enabled, in 8 or 16-bit mode, and whether
 *              swap is enabled
 *
 * PARAMETERS:  None
 *
 * RETURNED:    bus state
 *
 ***********************************************************************/
BusState SpiGetBusState(void)
{
	BusState retVal = 0;
 
	if ((LatchedVal & (SPICardBufferOff |
	                   SPICardSlotPwrOff)) == 0)
		retVal |= BUS_ENABLED;
	if ((LatchedVal & SPIBusSwapOff) == 0)
		retVal |= BUS_SWAP;
	if ((LatchedVal & SPIBusWidth8) == 0)
		retVal |= BUS_16BIT;
	return(retVal);
}
