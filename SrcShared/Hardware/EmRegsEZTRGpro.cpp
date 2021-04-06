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

// This file provides emulation at the register level for TRG's
// TRGpro handheld computer, which uses the EZ processor


#include "EmCommon.h"
#include "EmRegsEZTRGpro.h"
#include "EmTRGSPI.h"
#include "EmRegsEZPrv.h"

#include "EmSPISlaveADS784x.h"	// EmSPISlaveADS784x

#include "PalmPack.h"
#define NON_PORTABLE
	#include "EZSumo/IncsPrv/HardwareEZ.h"			// hwrEZPortCLCDEnableOn, etc.
#undef NON_PORTABLE
#include "PalmPackPop.h"

// TRGpro Defines
#define hwrEZPortDCFInit          0x40
#define hwrEZPortGDO_LATCH        0x08  // SPI DO_LATCH enable
#define hwrEZPortBLCDVccOff       0x02  // LCD VCC off
#define hwrEZPortD232EnableTRGpro 0x20  // RS 232 enable

const int		kNumButtonRows = 3;
const int		kNumButtonCols = 4;

const uint16	kButtonMap[kNumButtonRows][kNumButtonCols] =
{
	{ keyBitHard1,	keyBitHard2,	keyBitHard3,	keyBitHard4 },
	{ keyBitPageUp,	keyBitPageDown,	0,				0 },
	{ keyBitPower,	keyBitContrast,	keyBitHard2,	0 }
};

// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::EmRegsEZTRGpro
// ---------------------------------------------------------------------------

EmRegsEZTRGpro::EmRegsEZTRGpro (void) :
	EmRegsEZ (),
	fSPISlaveADC (new EmSPISlaveADS784x (kChannelSet2))
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::~EmRegsEZTRGpro
// ---------------------------------------------------------------------------

EmRegsEZTRGpro::~EmRegsEZTRGpro (void)
{
	delete fSPISlaveADC;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::Initialize
// ---------------------------------------------------------------------------

void EmRegsEZTRGpro::Initialize(void)
{
	EmRegsEZ::Initialize();
	SpiInitialize();
}

// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmRegsEZTRGpro::GetLCDScreenOn (void)
{
	return (READ_REGISTER (portCData) & hwrEZPortCLCDEnableOn) != 0;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetLCDBacklightOn
// ---------------------------------------------------------------------------

Bool EmRegsEZTRGpro::GetLCDBacklightOn (void)
{
	return (SpiIsBacklightOn());
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmRegsEZTRGpro::GetSerialPortOn (int /*uartNum*/)
{
	return (READ_REGISTER (portDData) & hwrEZPortD232EnableTRGpro ) != 0;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetPortInputValue
// ---------------------------------------------------------------------------
// Return the GPIO values for the pins on the port.  These values are used
// if the select pins are high.

uint8 EmRegsEZTRGpro::GetPortInputValue (int port)
{
	uint8	result = EmRegsEZ::GetPortInputValue (port);

	return result;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetPortInternalValue
// ---------------------------------------------------------------------------
// Return the dedicated values for the pins on the port.  These values are
// used if the select pins are low.

uint8 EmRegsEZTRGpro::GetPortInternalValue (int port)
{
	uint8	result = EmRegsEZ::GetPortInternalValue (port);

	if (port == 'D')
	{
		// Make sure that hwrEZPortDPowerFail is set.  If it's clear,
		// the battery code will make the device go to sleep immediately.

		result |= (hwrEZPortDCFInit | hwrEZPortDPowerFail);
	}

	return result;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetKeyInfo
// ---------------------------------------------------------------------------

void EmRegsEZTRGpro::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	KeyRowsType keyRows;

	*numRows = kNumButtonRows;
	*numCols = kNumButtonCols;

	memcpy (keyMap, kButtonMap, sizeof (kButtonMap));

	// Determine what row is being asked for.
	SpiGetKeyRows(&keyRows);
	rows[0]	= !(keyRows.val[0]);
	rows[1]	= !(keyRows.val[1]);
	rows[2]	= !(keyRows.val[2]);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetKeyInfo
// ---------------------------------------------------------------------------

void EmRegsEZTRGpro::PortDataChanged (int port, uint8 oldValue, uint8 newValue)
{
	if (port == 'G')
	{
		// If the the DO_LATCH bit is set, then the recent value
		// written to the SPI controller is for us, and not
		// the touchscreen

		if (newValue & hwrEZPortGDO_LATCH)
			SpiLatch();
	}

	EmRegsEZ::PortDataChanged(port, oldValue, newValue);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::spiWrite
//
// We need to share SPI writes with the touchscreen ...
// ---------------------------------------------------------------------------

void EmRegsEZTRGpro::spiWrite(emuptr address, int size, uint32 value)
{
	SpiSetUnlatchedVal(value);
	EmRegsEZ::spiMasterControlWrite (address, size, value);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::SetSubBankHandlers
// ---------------------------------------------------------------------------

void EmRegsEZTRGpro::SetSubBankHandlers(void)
{
	EmRegsEZ::SetSubBankHandlers();
	this->SetHandler((ReadFunction)&EmRegs::StdRead,
                         (WriteFunction)&EmRegsEZTRGpro::spiWrite,
	                 addressof(spiMasterData),
	                 sizeof(f68EZ328Regs.spiMasterData));
}


// ---------------------------------------------------------------------------
//		¥ EmRegsEZTRGpro::GetSPISlave
// ---------------------------------------------------------------------------

EmSPISlave* EmRegsEZTRGpro::GetSPISlave (void)
{
	if ((READ_REGISTER (portGData) & hwrEZPortGADCOff) == 0)
	{
		return fSPISlaveADC;
	}

	return NULL;
}
