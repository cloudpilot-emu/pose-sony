/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmBankSRAM.h"

#include "Byteswapping.h"		// ByteswapWords
#include "DebugMgr.h"			// Debug::CheckStepSpy
#include "EmCPU68K.h"			// ProcessException
#include "EmMemory.h"			// gRAMBank_Size, gRAM_Memory, gMemoryAccess
#include "EmSession.h"			// GetDevice
#include "Miscellaneous.h"		// StWordSwapper
#include "SessionFile.h"		// WriteRAMImage

#ifdef SONY_ROM
#include "EmScreen.h"			// EmScreen::MarkDirty
#endif

// ===========================================================================
//		¥ SRAM Bank Accessors
// ===========================================================================
// These functions provide fetch and store access to the emulator's random
// access memory.

static EmAddressBank	gAddressBank =
{
	EmBankSRAM::GetLong,
	EmBankSRAM::GetWord,
	EmBankSRAM::GetByte,
	EmBankSRAM::SetLong,
	EmBankSRAM::SetWord,
	EmBankSRAM::SetByte,
	EmBankSRAM::GetRealAddress,
	EmBankSRAM::ValidAddress,
	EmBankSRAM::GetMetaAddress,
	EmBankSRAM::AddOpcodeCycles
};

	// Note: I'd've used hwrCardBase0 here, except that that
	// changes on different hardware. It's 0x10000000 in some
	// cases, 0x00000000 in others.

static const emuptr	kMemoryStart	= 0x10000000;
static const emuptr	kMemoryStartEZ	= 0x00000000;
static const emuptr	kMemoryStartVZ	= 0x00000000;

emuptr 		gMemoryStart;

uint32 		gRAMBank_Size;
uint32 		gRAMBank_Mask;
uint8* 		gRAM_Memory;
uint8* 		gRAM_MetaMemory;

#if defined (_DEBUG)

	// In debug mode, define a global variable that points to the
	// Palm ROM's low-memory globals.  That makes it easier to find
	// out what's going wrong when something goes wrong.
	//
	// Keep in mind that on Windows, all values are "wordswapped"
	// (each pair of bytes is byteswapped).

	LowMemHdrType*	gLowMemory;

#endif


/***********************************************************************
 *
 * FUNCTION:	EmBankSRAM::Initialize
 *
 * DESCRIPTION: Standard initialization function.  Responsible for
 *				initializing this sub-system when a new session is
 *				created.  Will be followed by at least one call to
 *				Reset or Load.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmBankSRAM::Initialize (RAMSizeType ramSize)
{
	EmAssert (gRAM_Memory == NULL);
	EmAssert (gRAM_MetaMemory == NULL);

	if (ramSize > 0)
	{
		gRAMBank_Size	= ramSize * 1024;
		gRAMBank_Mask	= gRAMBank_Size - 1;
		gRAM_Memory 	= (uint8*) Platform::AllocateMemoryClear (gRAMBank_Size);
		gRAM_MetaMemory = (uint8*) Platform::AllocateMemoryClear (gRAMBank_Size);

#if defined (_DEBUG)
		// In debug mode, define a global variable that points to the
		// Palm ROM's low-memory globals.  That makes it easier to find
		// out what's going wrong when something goes wrong.

		gLowMemory		= (LowMemHdrType*) gRAM_Memory;
#endif
	}

	EmAssert (gSession);

	if (gSession->GetDevice ().Supports68EZ328 ())
	{
		gMemoryStart = kMemoryStartEZ;
	}
	else if (gSession->GetDevice ().Supports68VZ328 ())
	{
		gMemoryStart = kMemoryStartVZ;
	}
	else
	{
		gMemoryStart = kMemoryStart;
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmBankSRAM::Reset
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

void EmBankSRAM::Reset (Bool /*hardwareReset*/)
{
	memset (gRAM_MetaMemory, 0, gRAMBank_Size);
}


/***********************************************************************
 *
 * FUNCTION:	EmBankSRAM::Save
 *
 * DESCRIPTION: Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmBankSRAM::Save (SessionFile& f)
{
	StWordSwapper	swapper1 (gRAM_Memory, gRAMBank_Size);
	f.WriteRAMImage (gRAM_Memory, gRAMBank_Size);

	StWordSwapper	swapper2 (gRAM_MetaMemory, gRAMBank_Size);
	f.WriteMetaRAMImage (gRAM_MetaMemory, gRAMBank_Size);
}


/***********************************************************************
 *
 * FUNCTION:	EmBankSRAM::Load
 *
 * DESCRIPTION: Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmBankSRAM::Load (SessionFile& f)
{
	EmAssert (gRAM_Memory);
	EmAssert (gRAM_MetaMemory);

	if (f.ReadRAMImage (gRAM_Memory))
	{
		ByteswapWords (gRAM_Memory, gRAMBank_Size);
	}
	else
	{
		f.SetCanReload (false);
	}

	if (f.ReadMetaRAMImage (gRAM_MetaMemory))
	{
		ByteswapWords (gRAM_MetaMemory, gRAMBank_Size);
	}
	else
	{
		f.SetCanReload (false);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmBankSRAM::Dispose
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

void EmBankSRAM::Dispose (void)
{
	Platform::DisposeMemory (gRAM_Memory);
	Platform::DisposeMemory (gRAM_MetaMemory);
}


/***********************************************************************
 *
 * FUNCTION:    EmBankSRAM::SetBankHandlers
 *
 * DESCRIPTION: Set the bank handlers UAE uses to dispatch memory
 *				access operations.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmBankSRAM::SetBankHandlers (void)
{
	// Memory banks 0x1000 to <mumble> are managed by the functions
	// in EmBankSRAM.  <mumble> is based on the amount of RAM being emulated.

	long	numBanks = EmMemBankIndex (gMemoryStart + gRAMBank_Size - 1) -
									EmMemBankIndex (gMemoryStart) + 1;
	Memory::InitializeBanks (gAddressBank, EmMemBankIndex (gMemoryStart), numBanks);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::GetLong
// ---------------------------------------------------------------------------

uint32 EmBankSRAM::GetLong (emuptr address)
{
#if PROFILE_MEMORY
	gMemoryAccess[kSRAMLongRead]++;
	if (address & 2)
		gMemoryAccess[kSRAMLongRead2]++;
#endif

#if (CHECK_FOR_ADDRESS_ERROR)
	if ((address & 1) != 0)
	{
		AddressError (address, sizeof (uae_u32), true);
	}
#endif

#if (PREVENT_USER_SRAM_GET)
	if (gMemAccessFlags.fProtect_SRAMGet)
	{
		ProtectedAccess (address, sizeof (uae_u32), true);
	}
#endif

#if (VALIDATE_SRAM_GET)
	if (gMemAccessFlags.fValidate_SRAMGet && !ValidAddress (address, sizeof (uae_u32)))
	{
		InvalidAccess (address, sizeof (uae_u32), true);
	}
#endif

#if HAS_PROFILING
	CYCLE_GETLONG (WAITSTATES_SRAM);
#endif

	address &= gRAMBank_Mask;

	return EmMemDoGet32 (gRAM_Memory + address);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::GetWord
// ---------------------------------------------------------------------------

uint32 EmBankSRAM::GetWord (emuptr address)
{

#if PROFILE_MEMORY
	gMemoryAccess[kSRAMWordRead]++;
#endif

#if (CHECK_FOR_ADDRESS_ERROR)
	if ((address & 1) != 0)
	{
		AddressError (address, sizeof (uae_u16), true);
	}
#endif

#if (PREVENT_USER_SRAM_GET)
	if (gMemAccessFlags.fProtect_SRAMGet)
	{
		ProtectedAccess (address, sizeof (uae_u16), true);
	}
#endif

#if (VALIDATE_SRAM_GET)
	if (gMemAccessFlags.fValidate_SRAMGet && !ValidAddress (address, sizeof (uae_u16)))
	{
		InvalidAccess (address, sizeof (uae_u16), true);
	}
#endif

#if HAS_PROFILING
	CYCLE_GETWORD (WAITSTATES_SRAM);
#endif

	address &= gRAMBank_Mask;

	return EmMemDoGet16 (gRAM_Memory + address);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::GetByte
// ---------------------------------------------------------------------------

uint32 EmBankSRAM::GetByte (emuptr address)
{

#if PROFILE_MEMORY
	gMemoryAccess[kSRAMByteRead]++;
#endif

#if (PREVENT_USER_SRAM_GET)
	if (gMemAccessFlags.fProtect_SRAMGet)
	{
		ProtectedAccess (address, sizeof (uae_u8), true);
	}
#endif

#if (VALIDATE_SRAM_GET)
	if (gMemAccessFlags.fValidate_SRAMGet && !ValidAddress (address, sizeof (uae_u8)))
	{
		InvalidAccess (address, sizeof (uae_u8), true);
	}
#endif

#if HAS_PROFILING
	CYCLE_GETBYTE (WAITSTATES_SRAM);
#endif

	address &= gRAMBank_Mask;

	return EmMemDoGet8 (gRAM_Memory + address);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::SetLong
// ---------------------------------------------------------------------------

void EmBankSRAM::SetLong (emuptr address, uint32 value)
{
#if PROFILE_MEMORY
	gMemoryAccess[kSRAMLongWrite]++;
	if (address & 2)
		gMemoryAccess[kSRAMLongWrite2]++;
#endif

#if (CHECK_FOR_ADDRESS_ERROR)
	if ((address & 1) != 0)
	{
		AddressError (address, sizeof (uae_u32), false);
	}
#endif

#ifndef SONY_ROM
#if (PREVENT_USER_SRAM_SET)
	if (gMemAccessFlags.fProtect_SRAMSet)
	{
		ProtectedAccess (address, sizeof (uae_u32), false);
	}
#endif
#endif // SONY_ROM

#if (VALIDATE_SRAM_SET)
	if (gMemAccessFlags.fValidate_SRAMSet && !ValidAddress (address, sizeof (uae_u32)))
	{
		InvalidAccess (address, sizeof (uae_u32), false);
	}
#endif

#if HAS_PROFILING
	CYCLE_PUTLONG (WAITSTATES_SRAM);
#endif

	emuptr phyAddress = address;
	phyAddress &= gRAMBank_Mask;

	EmMemDoPut32 (gRAM_Memory + phyAddress, value);

	// See if any interesting memory locations have changed.  If so,
	// CheckStepSpy will report it.

	Debug::CheckStepSpy (address, sizeof (uae_u32));

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGT400) {
		emuptr begin, end;
		EmHAL::GetLCDBeginEnd (begin, end);
		if (begin && begin != 0x00003B00 && address >= begin && address <= end) {
			EmScreen::MarkDirty (address, 4);
		}
	}
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::SetWord
// ---------------------------------------------------------------------------

void EmBankSRAM::SetWord (emuptr address, uint32 value)
{
#if PROFILE_MEMORY
	gMemoryAccess[kSRAMWordWrite]++;
#endif

#if (CHECK_FOR_ADDRESS_ERROR)
	if ((address & 1) != 0)
	{
		AddressError (address, sizeof (uae_u16), false);
	}
#endif

#ifndef SONY_ROM
#if (PREVENT_USER_SRAM_SET)
	if (gMemAccessFlags.fProtect_SRAMSet)
	{
		ProtectedAccess (address, sizeof (uae_u16), false);
	}
#endif
#endif // SONY_ROM

#if (VALIDATE_SRAM_SET)
	if (gMemAccessFlags.fValidate_SRAMSet && !ValidAddress (address, sizeof (uae_u16)))
	{
		InvalidAccess (address, sizeof (uae_u16), false);
	}
#endif

#if HAS_PROFILING
	CYCLE_PUTWORD (WAITSTATES_SRAM);
#endif

	emuptr phyAddress = address;
	phyAddress &= gRAMBank_Mask;

	EmMemDoPut16 (gRAM_Memory + phyAddress, value);

	// See if any interesting memory locations have changed.  If so,
	// CheckStepSpy will report it.

	Debug::CheckStepSpy (address, sizeof (uae_u16));

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGT400) {
		emuptr begin, end;
		EmHAL::GetLCDBeginEnd (begin, end);
		if (begin && begin != 0x00003B00 && address >= begin && address <= end) {
			EmScreen::MarkDirty (address, 2);
		}
	}
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::SetByte
// ---------------------------------------------------------------------------

void EmBankSRAM::SetByte (emuptr address, uint32 value)
{
#if PROFILE_MEMORY
	gMemoryAccess[kSRAMByteWrite]++;
#endif

#ifndef SONY_ROM
#if (PREVENT_USER_SRAM_SET)
	if (gMemAccessFlags.fProtect_SRAMSet)
	{
		ProtectedAccess (address, sizeof (uae_u8), false);
	}
#endif
#endif // SONY_ROM

#if (VALIDATE_SRAM_SET)
	if (gMemAccessFlags.fValidate_SRAMSet && !ValidAddress (address, sizeof (uae_u8)))
	{
		InvalidAccess (address, sizeof (uae_u8), false);
	}
#endif

#if HAS_PROFILING
	CYCLE_PUTBYTE (WAITSTATES_SRAM);
#endif

	emuptr phyAddress = address;
	phyAddress &= gRAMBank_Mask;

	EmMemDoPut8 (gRAM_Memory + phyAddress, value);

	// See if any interesting memory locations have changed.  If so,
	// CheckStepSpy will report it.

	Debug::CheckStepSpy (address, sizeof (uae_u8));

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGT400) {
		emuptr begin, end;
		EmHAL::GetLCDBeginEnd (begin, end);
		if (begin && begin != 0x00003B00 && address >= begin && address <= end) {
			EmScreen::MarkDirty (address, 1);
		}
	}
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::ValidAddress
// ---------------------------------------------------------------------------

int EmBankSRAM::ValidAddress (emuptr address, uint32 size)
{
	address -= gMemoryStart;
	int	result = (address + size) <= (gMemoryStart + gRAMBank_Size);

	return result;
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmBankSRAM::GetRealAddress (emuptr address)
{
	// Strip the uppermost bit of the address.

	address &= gRAMBank_Mask;

	return (uint8*) &(gRAM_Memory[address]);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::GetMetaAddress
// ---------------------------------------------------------------------------

uint8* EmBankSRAM::GetMetaAddress (emuptr address)
{
	// Strip the uppermost bit of the address.

	address &= gRAMBank_Mask;

	return (uint8*) &(gRAM_MetaMemory[address]);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::AddOpcodeCycles
// ---------------------------------------------------------------------------

void EmBankSRAM::AddOpcodeCycles (void)
{
#if (HAS_PROFILING)
	CYCLE_GETWORD (WAITSTATES_SRAM);
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::AddressError
// ---------------------------------------------------------------------------

void EmBankSRAM::AddressError (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->AddressError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::InvalidAccess
// ---------------------------------------------------------------------------

void EmBankSRAM::InvalidAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}


// ---------------------------------------------------------------------------
//		¥ EmBankSRAM::ProtectedAccess
// ---------------------------------------------------------------------------

void EmBankSRAM::ProtectedAccess (emuptr address, long size, Bool forRead)
{
	EmAssert (gCPU68K);
	gCPU68K->BusError (address, size, forRead);
}

