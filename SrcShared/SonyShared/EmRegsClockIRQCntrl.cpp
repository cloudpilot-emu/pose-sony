/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmRegsClockIRQCntrl.h"

#include "EmBankRegs.h"			// RegsBank::GetROMSize
#include "EmBankROM.h"			// ROMBank::IsPCInRAM
#include "EmHAL.h"
#include "Byteswapping.h"		// ByteswapWords
#include "DebugMgr.h"			// Debug::CheckStepSpy
#include "Profiling.h"			// WAITSTATES_USB
#include "EmMemory.h"			// gMemoryAccess
#include "SessionFile.h"		// WriteUSBSonyRegsType, ReadUSBSonyRegsType, etc.


#define WAITSTATES_USB 0

typedef uint32 	(*ReadFunction)(emuptr address, int size);
typedef void	(*WriteFunction)(emuptr address, int size, uint32 value);


//*******************************************************************
// EmRegsClockIRQCntrl Class
//*******************************************************************

// Macro to return the Dragonball address of the specified register

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrClockIRQCntrlType, x))

#define INSTALL_HANDLER(read, write, reg)			\
					this->SetHandler ((ReadFunction) &EmRegsClockIRQCntrl::read,		\
									  (WriteFunction) &EmRegsClockIRQCntrl::write,		\
									  addressof (reg),			\
									  sizeof (fRegs.reg))
#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::EmRegsClockIRQCntrl
// ---------------------------------------------------------------------------

EmRegsClockIRQCntrl::EmRegsClockIRQCntrl ()
{
}

EmRegsClockIRQCntrl::EmRegsClockIRQCntrl (emuptr baseAddr) :
	fBaseAddr (baseAddr)
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::~EmRegsClockIRQCntrl
// ---------------------------------------------------------------------------

EmRegsClockIRQCntrl::~EmRegsClockIRQCntrl (void)
{
}

/***********************************************************************
 *
 * FUNCTION:	EmRegsClockIRQCntrl::Initialize
 *
 * DESCRIPTION: Standard initialization function.  Responsible for
 *				initializing this sub-system when a new session is
 *				created.  May also be called from the Load function
 *				to share common functionality.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsClockIRQCntrl::Initialize (void)
{
	EmRegs::Initialize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsClockIRQCntrl::Reset
 *
 * DESCRIPTION: Standard reset function.  Sets the sub-system to a
 *				default state.	This occurs not only on a Reset (as
 *				from the menu item), but also when the sub-system
 *				is first initialized (Reset is called after Initialize)
 *				as well as when the system is re-loaded from an
 *				insufficient session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsClockIRQCntrl::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);
	if (hardwareReset)
	{
		memset (&fRegs, 0, sizeof(fRegs));
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsClockIRQCntrl::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsClockIRQCntrl::Save (SessionFile& f)
{
	EmRegs::Save (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsClockIRQCntrl::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsClockIRQCntrl::Load (SessionFile& f)
{
	EmRegs::Load (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsClockIRQCntrl::Dispose
 *
 * DESCRIPTION: Standard dispose function.	Completely release any
 *				resources acquired or allocated in Initialize and/or
 *				Load.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsClockIRQCntrl::Dispose (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::SetSubBankHandlers
// ---------------------------------------------------------------------------
void EmRegsClockIRQCntrl::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (ReadFromDummy,	WrireToDummy,		_filler01);

	INSTALL_HANDLER (Read,			Write,				dataWrite);
	INSTALL_HANDLER (Read,			Write,				dataRead);
	INSTALL_HANDLER (Read,			Write,				cmdWrite);
	INSTALL_HANDLER (Read,			Write,				cmdRead);

	INSTALL_HANDLER (ReadFromDummy,	WrireToDummy,		_filler02);

	INSTALL_HANDLER (Read,			Write,				ExpCard0008);
	INSTALL_HANDLER (Read,			Write,				ExpCard0009);

	INSTALL_HANDLER (ReadFromDummy,	WrireToDummy,		_filler03);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsClockIRQCntrl::GetRealAddress (emuptr address)
{
	uint8*	loc = ((uint8*) &fRegs) + (address - this->GetAddressStart ());
	return loc;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsClockIRQCntrl::GetAddressStart (void)
{
	return fBaseAddr; 
}


// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsClockIRQCntrl::GetAddressRange (void)
{
	return sizeof(fRegs);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::Write
// ---------------------------------------------------------------------------
void EmRegsClockIRQCntrl::Write(emuptr address, int size, uint32 value)
{
	this->StdWriteBE (address, size, value);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::Read
// ---------------------------------------------------------------------------
uint32 EmRegsClockIRQCntrl::Read(emuptr address, int size)
{
	uint32	rstValue = this->StdReadBE (address, size);
	return	rstValue;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::WrireToDummy
// ---------------------------------------------------------------------------
void EmRegsClockIRQCntrl::WrireToDummy (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

//	this->StdWriteBE (address, size, value);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::ReadFromDummy
// ---------------------------------------------------------------------------
uint32 EmRegsClockIRQCntrl::ReadFromDummy (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	return	this->StdReadBE (address, size);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::ValidAddress
// ---------------------------------------------------------------------------

int EmRegsClockIRQCntrl::ValidAddress (emuptr address, uint32 iSize)
{
	UNUSED_PARAM(iSize)
	return 1;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::InvalidAccess
// ---------------------------------------------------------------------------

void EmRegsClockIRQCntrl::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsClockIRQCntrl::AddressError
// ---------------------------------------------------------------------------

void EmRegsClockIRQCntrl::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		¥ USBSonyReg::UnsupportedWrite
// ---------------------------------------------------------------------------

void EmRegsClockIRQCntrl::UnsupportedWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(value)

#ifdef SONY_ROM
	return ;
#endif

	if (!CEnableFullAccess::AccessOK ())
	{
		EmRegsClockIRQCntrl::InvalidAccess (address, size, false);
	}
}

#pragma mark -

