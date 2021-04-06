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
#include "EmRegsFMSound.h"

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
// EmRegsFMSound Class
//*******************************************************************

// Macro to return the Dragonball address of the specified register

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrFMSoundType, x))

#define INSTALL_HANDLER(read, write, reg)			\
					this->SetHandler ((ReadFunction) &EmRegsFMSound::read,		\
									  (WriteFunction) &EmRegsFMSound::write,		\
									  addressof (reg),			\
									  sizeof (fRegs.reg))
#pragma mark -

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::EmRegsFMSound
// ---------------------------------------------------------------------------

EmRegsFMSound::EmRegsFMSound ()
{
}

EmRegsFMSound::EmRegsFMSound (emuptr baseAddr) :
	fBaseAddr (baseAddr)
{
}


// ---------------------------------------------------------------------------
//		� EmRegsFMSound::~EmRegsFMSound
// ---------------------------------------------------------------------------

EmRegsFMSound::~EmRegsFMSound (void)
{
}

/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSound::Initialize
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

void EmRegsFMSound::Initialize (void)
{
	EmRegs::Initialize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSound::Reset
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

void EmRegsFMSound::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);
	if (hardwareReset)
	{
		memset (&fRegs, 0, sizeof(fRegs));
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSound::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsFMSound::Save (SessionFile& f)
{
	EmRegs::Save (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSound::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsFMSound::Load (SessionFile& f)
{
	EmRegs::Load (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsFMSound::Dispose
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

void EmRegsFMSound::Dispose (void)
{
}

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::SetSubBankHandlers
// ---------------------------------------------------------------------------
void EmRegsFMSound::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (ReadFromDummy,	WrireToDummy,		_filler01);
}

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsFMSound::GetRealAddress (emuptr address)
{
	uint8*	loc = ((uint8*) &fRegs) + (address - this->GetAddressStart ());
	return loc;
}

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsFMSound::GetAddressStart (void)
{
	return fBaseAddr; 
}


// ---------------------------------------------------------------------------
//		� EmRegsFMSound::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsFMSound::GetAddressRange (void)
{
	return sizeof(fRegs);
}


// ---------------------------------------------------------------------------
//		� EmRegsFMSound::Write
// ---------------------------------------------------------------------------
void EmRegsFMSound::Write(emuptr address, int size, uint32 value)
{
	this->StdWriteBE (address, size, value);
}

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::Read
// ---------------------------------------------------------------------------
uint32 EmRegsFMSound::Read(emuptr address, int size)
{
	uint32	rstValue = this->StdReadBE (address, size);
	return	rstValue;
}

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::WrireToDummy
// ---------------------------------------------------------------------------
void EmRegsFMSound::WrireToDummy (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

//	this->StdWriteBE (address, size, value);
}


// ---------------------------------------------------------------------------
//		� EmRegsFMSound::ReadFromDummy
// ---------------------------------------------------------------------------
uint32 EmRegsFMSound::ReadFromDummy (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	return	this->StdReadBE (address, size);
}

// ---------------------------------------------------------------------------
//		� EmRegsFMSound::ValidAddress
// ---------------------------------------------------------------------------

int EmRegsFMSound::ValidAddress (emuptr address, uint32 iSize)
{
	UNUSED_PARAM(iSize)
	return 1;
}


// ---------------------------------------------------------------------------
//		� EmRegsFMSound::InvalidAccess
// ---------------------------------------------------------------------------

void EmRegsFMSound::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		� EmRegsFMSound::AddressError
// ---------------------------------------------------------------------------

void EmRegsFMSound::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		� USBSonyReg::UnsupportedWrite
// ---------------------------------------------------------------------------

void EmRegsFMSound::UnsupportedWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(value)

#ifdef SONY_ROM
	return ;
#endif

	if (!CEnableFullAccess::AccessOK ())
	{
		EmRegsFMSound::InvalidAccess (address, size, false);
	}
}

#pragma mark -

