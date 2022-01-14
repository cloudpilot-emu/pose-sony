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
#include "EmRegsFMSoundforSZ.h"

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
// EmRegsFMSoundforSZ Class
//*******************************************************************

// Macro to return the Dragonball address of the specified register

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrFMSoundforSZType, x))

#define INSTALL_HANDLER(read, write, reg)			\
					this->SetHandler ((ReadFunction) &EmRegsFMSoundforSZ::read,		\
									  (WriteFunction) &EmRegsFMSoundforSZ::write,		\
									  addressof (reg),			\
									  sizeof (fRegs.reg))
#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::EmRegsFMSoundforSZ
// ---------------------------------------------------------------------------

EmRegsFMSoundforSZ::EmRegsFMSoundforSZ ()
{
}

EmRegsFMSoundforSZ::EmRegsFMSoundforSZ (emuptr baseAddr) :
	fBaseAddr (baseAddr)
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::~EmRegsFMSoundforSZ
// ---------------------------------------------------------------------------

EmRegsFMSoundforSZ::~EmRegsFMSoundforSZ (void)
{
}

/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSoundforSZ::Initialize
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

void EmRegsFMSoundforSZ::Initialize (void)
{
	EmRegs::Initialize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSoundforSZ::Reset
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

void EmRegsFMSoundforSZ::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);
	if (hardwareReset)
	{
		memset (&fRegs, 0, sizeof(fRegs));
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSoundforSZ::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsFMSoundforSZ::Save (SessionFile& f)
{
	EmRegs::Save (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSoundforSZ::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsFMSoundforSZ::Load (SessionFile& f)
{
	EmRegs::Load (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSoundforSZ::Dispose
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

void EmRegsFMSoundforSZ::Dispose (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::SetSubBankHandlers
// ---------------------------------------------------------------------------
void EmRegsFMSoundforSZ::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (ReadFromDummy,	WrireToDummy,		_filler01);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsFMSoundforSZ::GetRealAddress (emuptr address)
{
	uint8*	loc = ((uint8*) &fRegs) + (address - this->GetAddressStart ());
	return loc;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsFMSoundforSZ::GetAddressStart (void)
{
	return fBaseAddr; 
}


// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsFMSoundforSZ::GetAddressRange (void)
{
	return sizeof(fRegs);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::Write
// ---------------------------------------------------------------------------
void EmRegsFMSoundforSZ::Write(emuptr address, int size, uint32 value)
{
	this->StdWriteBE (address, size, value);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::Read
// ---------------------------------------------------------------------------
uint32 EmRegsFMSoundforSZ::Read(emuptr address, int size)
{
	uint32	rstValue = this->StdReadBE (address, size);
	return	rstValue;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::WrireToDummy
// ---------------------------------------------------------------------------
void EmRegsFMSoundforSZ::WrireToDummy (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

//	this->StdWriteBE (address, size, value);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::ReadFromDummy
// ---------------------------------------------------------------------------
uint32 EmRegsFMSoundforSZ::ReadFromDummy (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	return	this->StdReadBE (address, size);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::ValidAddress
// ---------------------------------------------------------------------------

int EmRegsFMSoundforSZ::ValidAddress (emuptr address, uint32 iSize)
{
	UNUSED_PARAM(iSize)
	return 1;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::InvalidAccess
// ---------------------------------------------------------------------------

void EmRegsFMSoundforSZ::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsFMSoundforSZ::AddressError
// ---------------------------------------------------------------------------

void EmRegsFMSoundforSZ::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		¥ USBSonyReg::UnsupportedWrite
// ---------------------------------------------------------------------------

void EmRegsFMSoundforSZ::UnsupportedWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(value)

#ifdef SONY_ROM
	return ;
#endif

	if (!CEnableFullAccess::AccessOK ())
	{
		EmRegsFMSoundforSZ::InvalidAccess (address, size, false);
	}
}

#pragma mark -

