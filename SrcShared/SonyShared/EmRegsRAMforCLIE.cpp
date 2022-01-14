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
#include "EmRegsRAMforCLIE.h"

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
// EmRegsRAMforCLIE Class
//*******************************************************************

// Macro to return the Dragonball address of the specified register

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrRAMforCLIEType, x))

#define INSTALL_HANDLER(read, write, reg)			\
					this->SetHandler ((ReadFunction) &EmRegsRAMforCLIE::read,		\
									  (WriteFunction) &EmRegsRAMforCLIE::write,		\
									  addressof (reg),			\
									  sizeof (fRegs.reg))
#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::EmRegsRAMforCLIE
// ---------------------------------------------------------------------------

EmRegsRAMforCLIE::EmRegsRAMforCLIE ()
{
}

EmRegsRAMforCLIE::EmRegsRAMforCLIE (emuptr baseAddr) :
	fBaseAddr (baseAddr)
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::~EmRegsRAMforCLIE
// ---------------------------------------------------------------------------

EmRegsRAMforCLIE::~EmRegsRAMforCLIE (void)
{
}

/***********************************************************************
 *
 * FUNCTION:	EmRegsRAMforCLIE::Initialize
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

void EmRegsRAMforCLIE::Initialize (void)
{
	EmRegs::Initialize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsRAMforCLIE::Reset
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

void EmRegsRAMforCLIE::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);
	if (hardwareReset)
	{
		memset (&fRegs, 0, sizeof(fRegs));
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsRAMforCLIE::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsRAMforCLIE::Save (SessionFile& f)
{
	EmRegs::Save (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsRAMforCLIE::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsRAMforCLIE::Load (SessionFile& f)
{
	EmRegs::Load (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsRAMforCLIE::Dispose
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

void EmRegsRAMforCLIE::Dispose (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::SetSubBankHandlers
// ---------------------------------------------------------------------------
void EmRegsRAMforCLIE::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (ReadFromDummy,		WrireToDummy,		_filler01);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsRAMforCLIE::GetRealAddress (emuptr address)
{
	uint8*	loc = ((uint8*) &fRegs) + (address - this->GetAddressStart ());
	return loc;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsRAMforCLIE::GetAddressStart (void)
{
	return fBaseAddr; 
}


// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsRAMforCLIE::GetAddressRange (void)
{
	return sizeof(fRegs);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::Write
// ---------------------------------------------------------------------------
void EmRegsRAMforCLIE::Write(emuptr address, int size, uint32 value)
{
	this->StdWriteBE (address, size, value);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::Read
// ---------------------------------------------------------------------------
uint32 EmRegsRAMforCLIE::Read(emuptr address, int size)
{
	uint32	rstValue = this->StdReadBE (address, size);
	return	rstValue;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::WrireToDummy
// ---------------------------------------------------------------------------
void EmRegsRAMforCLIE::WrireToDummy (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

//	this->StdWriteBE (address, size, value);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::ReadFromDummy
// ---------------------------------------------------------------------------

uint32 EmRegsRAMforCLIE::ReadFromDummy (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	return	this->StdReadBE (address, size);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::ValidAddress
// ---------------------------------------------------------------------------

int EmRegsRAMforCLIE::ValidAddress (emuptr address, uint32 iSize)
{
	UNUSED_PARAM(iSize)
	return 1;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::InvalidAccess
// ---------------------------------------------------------------------------

void EmRegsRAMforCLIE::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsRAMforCLIE::AddressError
// ---------------------------------------------------------------------------

void EmRegsRAMforCLIE::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		¥ USBSonyReg::UnsupportedWrite
// ---------------------------------------------------------------------------

void EmRegsRAMforCLIE::UnsupportedWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(value)

#ifdef SONY_ROM
	return ;
#endif

	if (!CEnableFullAccess::AccessOK ())
	{
		EmRegsRAMforCLIE::InvalidAccess (address, size, false);
	}
}

#pragma mark -

