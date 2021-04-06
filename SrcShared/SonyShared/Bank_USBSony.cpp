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
#include "Bank_USBSony.h"

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
// EmRegsUsbCLIE Class
//*******************************************************************

// Macro to return the Dragonball address of the specified register

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrUsbCLIEType, x))

#define INSTALL_HANDLER(read, write, reg)			\
					this->SetHandler ((ReadFunction) &EmRegsUsbCLIE::read,		\
									  (WriteFunction) &EmRegsUsbCLIE::write,		\
									  addressof (reg),			\
									  sizeof (fRegs.reg))
#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::EmRegsUsbCLIE
// ---------------------------------------------------------------------------

EmRegsUsbCLIE::EmRegsUsbCLIE ()
{
	fOffsetAddr = 0;
	fBaseAddr = 0;
}

EmRegsUsbCLIE::EmRegsUsbCLIE (uint32 offset)
{
	fOffsetAddr = offset;
	fBaseAddr = 0;
}

EmRegsUsbCLIE::EmRegsUsbCLIE (emuptr baseAddr, uint32 offset)
{
	fOffsetAddr = offset;
	fBaseAddr = baseAddr;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::~EmRegsUsbCLIE
// ---------------------------------------------------------------------------

EmRegsUsbCLIE::~EmRegsUsbCLIE (void)
{
}



/***********************************************************************
 *
 * FUNCTION:	EmRegsUsbCLIE::Initialize
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

void EmRegsUsbCLIE::Initialize (void)
{
	EmRegs::Initialize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUsbCLIE::Reset
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

void EmRegsUsbCLIE::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);
	if (hardwareReset)
	{
		memset (&fRegs, 0, sizeof(fRegs));
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUsbCLIE::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsUsbCLIE::Save (SessionFile& f)
{
	EmRegs::Save (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUsbCLIE::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsUsbCLIE::Load (SessionFile& f)
{
	EmRegs::Load (f);
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUsbCLIE::Dispose
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

void EmRegsUsbCLIE::Dispose (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::SetSubBankHandlers
// ---------------------------------------------------------------------------
void EmRegsUsbCLIE::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (Read,			Write,				dataWrite);
	INSTALL_HANDLER (Read,			Write,				dataRead);
	INSTALL_HANDLER (Read,			Write,				cmdWrite);
	INSTALL_HANDLER (Read,			Write,				cmdRead);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsUsbCLIE::GetRealAddress (emuptr address)
{
	uint8*	loc = ((uint8*) &fRegs) + (address - this->GetAddressStart ());
	return loc;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsUsbCLIE::GetAddressStart (void)
{
	emuptr	startAddr = fBaseAddr;
	if (!startAddr)
		startAddr = EmBankROM::GetMemoryStart() + EmHAL::GetROMSize();
	return  startAddr + fOffsetAddr; 
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsUsbCLIE::GetAddressRange (void)
{
	return sizeof(fRegs);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::Write
// ---------------------------------------------------------------------------
void EmRegsUsbCLIE::Write(emuptr address, int size, uint32 value)
{
#ifdef SONY_ROM
	if ((value & 0x000000FF) == 0xFB && address == (this->GetAddressStart() + 2))
	{
		extern uint8* 		gRAM_Memory;
		fRegs.dataRead = 0xC0;
	}
#endif

	this->StdWriteBE (address, size, value);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::Read
// ---------------------------------------------------------------------------
uint32 EmRegsUsbCLIE::Read(emuptr address, int size)
{
	uint32	rstValue = this->StdReadBE (address, size);
	return	rstValue;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::ValidAddress
// ---------------------------------------------------------------------------

int EmRegsUsbCLIE::ValidAddress (emuptr address, uint32 iSize)
{
	UNUSED_PARAM(iSize)
	return 1;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::InvalidAccess
// ---------------------------------------------------------------------------

void EmRegsUsbCLIE::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUsbCLIE::AddressError
// ---------------------------------------------------------------------------

void EmRegsUsbCLIE::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		¥ USBSonyReg::UnsupportedWrite
// ---------------------------------------------------------------------------

void EmRegsUsbCLIE::UnsupportedWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(value)

#ifdef SONY_ROM
	return ;
#endif

	if (!CEnableFullAccess::AccessOK ())
	{
		EmRegsUsbCLIE::InvalidAccess (address, size, false);
	}
}


//*******************************************************************
// EmRegsUSBforPegN700C Class
//*******************************************************************

// Macro to return the Dragonball address of the specified register
#ifdef  addressof
#undef	addressof
#undef	INSTALL_HANDLER

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrUSBforPegN700C, x))

#define INSTALL_HANDLER(read, write, reg)			\
	this->SetHandler ((ReadFunction) &EmRegsUSBforPegN700C::read,		\
					  (WriteFunction) &EmRegsUSBforPegN700C::write,		\
					   addressof (reg),			\
					   sizeof (fRegs.reg))
#endif

#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::EmRegsUSBforPegN700C
// ---------------------------------------------------------------------------

EmRegsUSBforPegN700C::EmRegsUSBforPegN700C (emuptr baseAddr) :
	fBaseAddr (baseAddr)
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::~EmRegsUSBforPegN700C
// ---------------------------------------------------------------------------

EmRegsUSBforPegN700C::~EmRegsUSBforPegN700C (void)
{
}



/***********************************************************************
 *
 * FUNCTION:	EmRegsUSBforPegN700C::Initialize
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

void EmRegsUSBforPegN700C::Initialize (void)
{
#if 1
	EmRegs::Initialize ();
#else	
	//	Auto-detect USB hardware base address.  The USB hardware is at CSA1,
	//	which resides just after the ROM (CSA0) in memory.  

	kMemoryStart = EmBankROM::GetMemoryStart() + EmHAL::GetROMSize () + 0x00100000; 

	// Memory banks starting at kMemoryStart are managed by the functions in USBBank for Sony.

	long	numBanks = (sizeof (SED1375RegsType) + 0x0FFFF) / 0x10000;
	Memory::InitializeBanks (gAddressBank, bankindex (kMemoryStart), numBanks);

	// Install the handlers for each register.

	EmRegsUsbCLIE::InstallHandlers ();
#endif
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUSBforPegN700C::Reset
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

void EmRegsUSBforPegN700C::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);
	if (hardwareReset)
	{
		memset (&fRegs, 0, sizeof(fRegs));
		fRegs.USB0C06 = 0xFC;
		fRegs.USB0C07 = 0x00;
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUSBforPegN700C::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsUSBforPegN700C::Save (SessionFile& f)
{
#if 1
	EmRegs::Save (f);
#else
	StWordSwapper	swapper (&gUSBSonyRegs, sizeof(gUSBSonyRegs));
	f.WriteUSBSonyRegsType (gUSBSonyRegs);
#endif
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUSBforPegN700C::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmRegsUSBforPegN700C::Load (SessionFile& f)
{
#if 1
	EmRegs::Load (f);
#else
	// Read in the USB registers, and then byteswap them.

	if (f.ReadUSBSonyRegsType (gUSBSonyRegs))
	{
		ByteswapWords (&gUSBSonyRegs, sizeof(gUSBSonyRegs));
	}
	else
	{
		f.SetCanReload (false);
	}
#endif
}


/***********************************************************************
 *
 * FUNCTION:	EmRegsUSBforPegN700C::Dispose
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

void EmRegsUSBforPegN700C::Dispose (void)
{
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::SetSubBankHandlers
// ---------------------------------------------------------------------------
void EmRegsUSBforPegN700C::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (Read,		Write,				dataWrite);
	INSTALL_HANDLER (Read,		Write,				dataRead);
	INSTALL_HANDLER (Read,		Write,				cmdWrite);
	INSTALL_HANDLER (Read,		Write,				cmdRead);
	INSTALL_HANDLER (Read,		Write,				USB0200);
	INSTALL_HANDLER (Read,		Write,				USB0201);
	INSTALL_HANDLER (Read,		Write,				USB0202);
	INSTALL_HANDLER (Read,		Write,				USB0203);
	INSTALL_HANDLER (Read,		Write,				USB0204);
	INSTALL_HANDLER (Read,		Write,				USB0205);
	INSTALL_HANDLER (Read,		Write,				USB0206);
	INSTALL_HANDLER (Read,		Write,				USB0207);
	INSTALL_HANDLER (Read,		Write,				USB0208);
	INSTALL_HANDLER (Read,		Write,				USB0209);

	INSTALL_HANDLER (Read,		Write,				USB0C00);
	INSTALL_HANDLER (Read,		Write,				USB0C01);
	INSTALL_HANDLER (Read,		Write,				USB0C02);
	INSTALL_HANDLER (Read,		Write,				USB0C03);
	INSTALL_HANDLER (Read,		Write,				USB0C04);
	INSTALL_HANDLER (Read,		Write,				USB0C05);
	INSTALL_HANDLER (Read,		Write,				USB0C06);
	INSTALL_HANDLER (Read,		Write,				USB0C07);
	INSTALL_HANDLER (Read,		Write,				USB0C08);
	INSTALL_HANDLER (Read,		Write,				USB0C09);
	INSTALL_HANDLER (Read,		Write,				USB0C0A);
	INSTALL_HANDLER (Read,		Write,				USB0C0B);
	INSTALL_HANDLER (Read,		Write,				USB0C0C);
	INSTALL_HANDLER (Read,		Write,				USB0C0D);
	INSTALL_HANDLER (Read,		Write,				USB0C0E);
	INSTALL_HANDLER (Read,		Write,				USB0C0F);

	INSTALL_HANDLER (Read,		Write,				_filler01);
	INSTALL_HANDLER (Read,		Write,				_filler02);
	INSTALL_HANDLER (Read,		Write,				_filler03);
	INSTALL_HANDLER (Read,		Write,				_filler04);
	INSTALL_HANDLER (Read,		Write,				_filler05);
	
	INSTALL_HANDLER (ReadFromAddr0x8000,		WrireToAddr0x8000,	USB8000);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsUSBforPegN700C::GetRealAddress (emuptr address)
{
	uint8*	loc = ((uint8*) &fRegs) + (address - this->GetAddressStart ());

	return loc;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsUSBforPegN700C::GetAddressStart (void)
{
	return fBaseAddr;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsUSBforPegN700C::GetAddressRange (void)
{
	return sizeof(fRegs);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsCFMemCard::Write
// ---------------------------------------------------------------------------
void EmRegsUSBforPegN700C::Write(emuptr address, int size, uint32 value)
{
	if (address == 0x10800C06 && size == 2)
	{
		if (this->StdReadBE (address, size) == 0xFC00)
			value = 0x0001;
	}

	this->StdWriteBE (address, size, value);
}

// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::Read
// ---------------------------------------------------------------------------
uint32 EmRegsUSBforPegN700C::Read(emuptr address, int size)
{
	uint32	rstValue = this->StdReadBE (address, size);

	return	rstValue;
}

void EmRegsUSBforPegN700C::WrireToAddr0x8000 (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	// Always set the vertical non-display status high since in the real
	// hardware, the ROM will check this flag in order to write the CLUT
	// registers.
	this->StdWriteBE (address, size, value);

	value &= 0x0FF;
}

uint32 EmRegsUSBforPegN700C::ReadFromAddr0x8000 (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	// Always set the vertical non-display status high since in the real
	// hardware, the ROM will check this flag in order to write the CLUT
	// registers.
	return	this->StdReadBE (address, size);
}



// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::ValidAddress
// ---------------------------------------------------------------------------

int EmRegsUSBforPegN700C::ValidAddress (emuptr address, uint32 iSize)
{
	UNUSED_PARAM(iSize)
	return 1;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::InvalidAccess
// ---------------------------------------------------------------------------

void EmRegsUSBforPegN700C::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsUSBforPegN700C::AddressError
// ---------------------------------------------------------------------------

void EmRegsUSBforPegN700C::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}

// ---------------------------------------------------------------------------
//		¥ USBSonyReg::UnsupportedWrite
// ---------------------------------------------------------------------------

void EmRegsUSBforPegN700C::UnsupportedWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(value)

#ifdef SONY_ROM
	return ;
#endif

	if (!CEnableFullAccess::AccessOK ())
	{
		EmRegsUSBforPegN700C::InvalidAccess (address, size, false);
	}
}

#pragma mark -

