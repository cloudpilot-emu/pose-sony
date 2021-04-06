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
#include "EmMemory.h"

#include "EmBankDRAM.h"			// EmBankDRAM::Initialize
#include "EmBankDummy.h"		// EmBankDummy::Initialize
#include "EmBankMapped.h"		// EmBankMapped::Initialize
#include "EmBankRegs.h"			// EmBankRegs::Initialize
#include "EmBankROM.h"			// EmBankROM::Initialize
#include "EmBankSRAM.h"			// EmBankSRAM::Initialize
#include "EmSession.h"			// gSession, GetDevice
#include "MetaMemory.h"			// MetaMemory::Initialize


#if HAS_PROFILING
#include "Profiling.h"			// gProfilingCounted
#endif


/*
	Hitchhiker's Guide To Accessing Memory

	When emulating CPU instructions, all memory access must be emulated as
	well.  Additionally, the memory location that the emulated instruction
	is accessing is not necessarily the same as the memory location in the
	emulator's address space.  That is, memory location 0x00001234 in the
	emulated process's address space is not the same as memory location
	0x00001234 in the emulator's address space.  Instead, the emulated
	0x00001234 is actually 0x00001234 bytes into some buffer that we've
	allocate for the purpose of holding the emulated address space's contents.

	There are several ranges of Palm OS memory that we need to emulate:

		- Low-memory RAM (for the dynamic heap)
		- High-memory RAM (for the storage heap)
		- ROM
		- Dragonball and other memory-mapped registers
		- Everything else

	Each of these ranges (except for the last one) is managed by a set of
	functions for reading from and writing to that memory and a chunk of
	memory that we allocate as a "backing store" for that memory.

	Thus, for each of these ranges, we need to quickly determine what set of
	functions need to be called.  The method for doing this is to divide the
	entire 4GB address space into 64K banks, each 64K long.

	Each 64K bank is managed by a set of functions.  These functions handle
	reading from and writing to that 64K bank, handling 1-byte, 2-byte, and
	4-byte accesses.  They also manage mapping from the emulated addres space
	to the physical space as viewed by the emulator, and validate memory
	addresses.

	Because there are 64K of these 64K banks, the functions that manage each
	bank are stored in an array of 64K elements.  When we need to invoke a
	function for a particular bank, the upper word of the prospective memory
	address is used as a "bank index" into this array.  From that, the right
	set of functions is found, and the desired function from that set of
	functions is called.

	This 64K array is very sparse.  There are one to four entries for managing
	the dynamic heap (depending on whether the dynamic heap is 64K, 96K, 128K,
	or 256K), 2 to 128 entries for the storage heap (depending on whether the
	storage heap is 128K to 8Meg), 8 to 32 entries for the ROM (again,
	depending on its size), and 1 for the Dragonball registers. That leaves
	the remaining 64K - 12 entries to be managed by a set of functions that do
	little but signal an address error if the memory they manage is being
	accessed.

	Pictorally, it looks like this:

	Emulated 4G space:				Memory function array							Address Bank functions

	0x00000000	+--------------+	+--------------------------------+
				|      64K     |<---| Ptr to fns that mng this range |------+
	0x00010000	+--------------+	+--------------------------------+		|
				|      64K     |<---| Ptr to fns that mng this range |------+
	0x00020000	+--------------+	+--------------------------------+		|
				|      64K     |											+----->	EmBankDRAM::GetLong, EmBankDRAM::SetLong
				\/\/\/\/\/\/\/\/													EmBankDRAM::GetWord, EmBankDRAM::SetWord
				\/\/\/\/\/\/\/\/													EmBankDRAM::GetByte, EmBankDRAM::SetByte
				|      64K     |													etc.
	0x10000000	+--------------+	+--------------------------------+
				|      64K     |<---| Ptr to fns that mng this range |------+
	0x10010000	+--------------+	+--------------------------------+		|
				|      64K     |<---| Ptr to fns that mng this range |------+
	0x10020000	+--------------+	+--------------------------------+		|
				|      64K     |											+----->	EmBankSRAM::GetLong, EmBankSRAM::SetLong
				\/\/\/\/\/\/\/\/													EmBankSRAM::GetWord, EmBankSRAM::SetWord
				\/\/\/\/\/\/\/\/													EmBankSRAM::GetByte, EmBankSRAM::SetByte
				|      64K     |													etc.
	0x10C00000	+--------------+	+--------------------------------+
				|      64K     |<---| Ptr to fns that mng this range |------+
	0x10C10000	+--------------+	+--------------------------------+		|
				|      64K     |<---| Ptr to fns that mng this range |------+
	0x10C20000	+--------------+	+--------------------------------+		|
				|      64K     |											+----->	EmBankROM::GetLong, EmBankROM::SetLong
				\/\/\/\/\/\/\/\/													EmBankROM::GetWord, EmBankROM::SetWord
				\/\/\/\/\/\/\/\/													EmBankROM::GetByte, EmBankROM::SetByte
				|      64K     |													etc.
	0xFFFE0000	+--------------+	+--------------------------------+
				|      64K     |<---| Ptr to fns that mng this range |
	0xFFFF0000	+--------------+	+--------------------------------+
				|      64K     |<---| Ptr to fns that mng this range |------+
	0xFFFFFFFF	+--------------+	+--------------------------------+		|
																			+----->	EmRegs::GetLong, EmRegs::SetLong
																					EmRegs::GetWord, EmRegs::SetWord
																					EmRegs::GetByte, EmRegs::SetByte
																					etc.

	All of the remaining memory function array entries are initialized with
	pointers to the EmBankDummy::GetLong, EmBankDummy::SetLong, etc., set of
	functions.

	Using EmBankMapped accessors, we also have the facility for mapping memory
	from the emulator's address space directly into the emulated address
	space.  This memory appears as additional RAM that magically appears out
	of nowhere.  This mechanism makes it easy for us to "inject" data into the
	emulated address space.  It also makes it easier to call emulated ROM
	functions as subroutines.  Some ROM functions take as parameters pointers
	to memory locations that are to receive values (for example, pointers to
	local variables).  For those functions, we just map the emulator's stack
	into the emulated address space.  After that's done, we can pass pointers
	to stack-based variables with impunity.

	A 64K bank can be further sub-divided into 1, 2, or 4 byte chunks
	spanning any range of the 64K chunk.  EmBankRegs manages a collection
	of EmRegs objects.  The EmRegs abstract base class is sub-classed into
	classes that specify the sub-range of the 64K for which they are
	responsible and dispatch memory accesses to individual functions
	that handle accesses at more-or-less a byte level.

	Mapping from emulated addresses to "real" addresses (that is, the
	corresponding memory locations within the buffers we've allocated to
	"stand in" for the emulated memory) is performed via macros and inline
	functions, described below.

	Executive summary:

		- use EmMemGetFoo if you want the address checked for even/oddness.
		- use EmMemDoGetFoo to actually get the value using a physical address.

	EmMemGet32, EmMemGet16, EmMemGet8
	EmMemPut32, EmMemPut16, EmMemPut8
		Inline functions that dispatch to the right bank of functions
		using EmMemCallGetFunc, EmMemGetBank, and EmMemBankIndex.

	EmMemBankIndex
		Returns a bank index for the given address.

	EmMemGetBank
		Gets the right EmAddressBank object.

	EmMemCallGetFunc
		Merely calls the given function through the EmAddressBank fn ptr.

	EmMemDoGet32, EmMemDoGet16, EmMemDoGet8
	EmMemDoPut32, EmMemDoPut16, EmMemDoPut8
		Very low-level memory access.  They return the value at the
		given physical location, byte-swapping if needed.
*/

// Types.

#pragma mark Types

// Globals.

#pragma mark Globals

EmAddressBank*	gEmMemBanks[65536];		// (normally defined in memory.c)

MemAccessFlags	gMemAccessFlags =
{
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,
	MASTER_RUNTIME_VALIDATE_SWITCH,

	// User prevention flags.

//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
	false,	// SRAM-get
	true,	// SRAM-set
	false,	// ROM-get
	true,	// ROM-set (HACK: We really want it to be true!)
	MASTER_RUNTIME_PREVENT_SWITCH,
	MASTER_RUNTIME_PREVENT_SWITCH,

//	false,	// fCheck_UserChunkGet
//	false,	// fCheck_UserChunkSet
//	false,	// fCheck_SysChunkGet
//	false,	// fCheck_SysChunkSet

	// System prevention flags.

//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	false,	// SRAM-get
//	true,	// SRAM-set
	false,	// ROM-get
	true,	// ROM-set (HACK: We really want it to be true!)
//	MASTER_RUNTIME_PREVENT_SWITCH,
//	MASTER_RUNTIME_PREVENT_SWITCH
};

MemAccessFlags	kZeroMemAccessFlags;


#if PROFILE_MEMORY
/*
	After 12 Million instructions executed...

	kDRAMLongRead,		// 2,797,111
	kDRAMLongRead2,		//
	kDRAMWordRead,		// 1,334,295
	kDRAMByteRead,		//   427,936

	kDRAMLongWrite,		// 1,960,933
	kDRAMLongWrite2,	//
	kDRAMWordWrite,		//   800,780
	kDRAMByteWrite,		//   213,029

	kSRAMLongRead,		//     4,167
	kSRAMLongRead2,		//     4,167
	kSRAMWordRead,		//    10,768
	kSRAMByteRead,		//     6,295 (jumped WAY up (5-fold) after playing with Address Book / 20 Million instructions)

	kSRAMLongWrite,		//   516,246 (stable after boot?)
	kSRAMLongWrite2,	//
	kSRAMWordWrite,		//     5,112
	kSRAMByteWrite,		//     2,183

	kROMLongRead,		//    10,156
	kROMLongRead2,		//
	kROMWordRead,		//   557,062
	kROMByteRead,		//    15,516

	kROMLongWrite,		//         0
	kROMLongWrite2,		//
	kROMWordWrite,		//         0
	kROMByteWrite,		//         0

	kREGLongRead,		//     16,638
	kREGLongRead2,		//
	kREGWordRead,		//     30,477
	kREGByteRead,		//      3,553

	kREGLongWrite,		//      9,472
	kREGLongWrite2,		//
	kREGWordWrite,		//     20,709
	kREGByteWrite,		//     12,656


	After 32,439,154 instructions:

	kDRAMLongRead,		// 6,466,064
	kDRAMWordRead,		// 3,342,826
	kDRAMByteRead,		//   940,345

	kDRAMLongWrite,		// 4,136,635
	kDRAMWordWrite,		// 1,462,995
	kDRAMByteWrite,		//   509,189

	kSRAMLongRead,		//    24,355
	kSRAMWordRead,		//    32,190
	kSRAMByteRead,		//    75,917

	kSRAMLongWrite,		// 2,074,137 (8-Meg ROM)
	kSRAMWordWrite,		//    42,050
	kSRAMByteWrite,		//    11,292

	kROMLongRead,		//    25,806
	kROMWordRead,		// 1,302,897
	kROMByteRead,		//    63,590

	kROMLongWrite,		//         0
	kROMWordWrite,		//         0
	kROMByteWrite,		//         0

	kREGLongRead,		//    32,037
	kREGWordRead,		//    74,299
	kREGByteRead,		//    91,930

	kREGLongWrite,		//    20,358
	kREGWordWrite,		//    57,879
	kREGByteWrite,		//    22,592
*/

long	gMemoryAccess[kLastEnum];
#endif


#pragma mark -


// ===========================================================================
//		¥ Memory
// ===========================================================================


// ---------------------------------------------------------------------------
//		¥ Memory::Initialize
// ---------------------------------------------------------------------------
// Initializes the RAM, ROM, and special memory areas of the emulator. Takes
// a stream handle to the ROM.

void Memory::Initialize (	EmStream&		hROM,
							RAMSizeType		ramSize)
{
	// Clear everything out.
	
	memset (gEmMemBanks, 0, sizeof (gEmMemBanks));

	// Initialize the valid memory banks.

	EmBankDummy::Initialize ();

	// Initialize the Hardware registers memory bank.  Do this
	// *before* initializing the other RAM banks so that we
	// know how memory is laid out from the chip selects.

	EmBankRegs::Initialize ();

	// Initialize EmBankDRAM after initializing the EmBankSRAM. The order is
	// important for DragonballEZ. On Dragonball devices, DRAM is
	// at 0x00000000, and SRAM is at 0x10000000. But on EZ devices,
	// both start at 0x00000000. By calling EmBankDRAM::Initialize
	// second, we allow it to overwrite the EmAddressBank handlers
	// for the part of memory where they overlap.

	EmBankSRAM::Initialize (ramSize);
	EmBankDRAM::Initialize ();

	EmBankROM::Initialize (hROM);

	EmBankMapped::Initialize ();

	EmAssert (gSession);
	if (gSession->GetDevice ().HasFlash ())
		EmBankFlash::Initialize ();

	MetaMemory::Initialize ();

//	Memory::ResetBankHandlers ();	// Can't do this yet.  We can't set the
									// bank handlers until we know how memory
									// is laid out, and that information isn't
									// determined until reset.
}


/***********************************************************************
 *
 * FUNCTION:	Memory::Reset
 *
 * DESCRIPTION:	Standard reset function.  Sets the sub-system to a
 *				default state.  This occurs not only on a Reset (as
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

void Memory::Reset (Bool hardwareReset)
{
	EmBankDummy::Reset (hardwareReset);
	EmBankRegs::Reset (hardwareReset);
	EmBankSRAM::Reset (hardwareReset);
	EmBankDRAM::Reset (hardwareReset);
	EmBankROM::Reset (hardwareReset);
	EmBankMapped::Reset (hardwareReset);

	EmAssert (gSession);
	if (gSession->GetDevice ().HasFlash ())
		EmBankFlash::Reset (hardwareReset);

	Memory::ResetBankHandlers ();

	MetaMemory::Reset ();
}


/***********************************************************************
 *
 * FUNCTION:	Memory::Save
 *
 * DESCRIPTION:	Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Memory::Save (SessionFile& f)
{
	EmBankDummy::Save (f);
	EmBankRegs::Save (f);
	EmBankSRAM::Save (f);
	EmBankDRAM::Save (f);
	EmBankROM::Save (f);
	EmBankMapped::Save (f);

	EmAssert (gSession);
	if (gSession->GetDevice ().HasFlash ())
		EmBankFlash::Save (f);

	MetaMemory::Save (f);
}


/***********************************************************************
 *
 * FUNCTION:	Memory::Load
 *
 * DESCRIPTION:	Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Memory::Load (SessionFile& f)
{
	EmBankDummy::Load (f);
	EmBankRegs::Load (f);
	EmBankSRAM::Load (f);
	EmBankDRAM::Load (f);
	EmBankROM::Load (f);
	EmBankMapped::Load (f);

	EmAssert (gSession);
	if (gSession->GetDevice ().HasFlash ())
		EmBankFlash::Load (f);

	MetaMemory::Load (f);

	Memory::ResetBankHandlers ();
}


/***********************************************************************
 *
 * FUNCTION:	Memory::Dispose
 *
 * DESCRIPTION:	Standard dispose function.  Completely release any
 *				resources acquired or allocated in Initialize and/or
 *				Load.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Memory::Dispose (void)
{
	EmBankDummy::Dispose ();
	EmBankRegs::Dispose ();
	EmBankSRAM::Dispose ();
	EmBankDRAM::Dispose ();
	EmBankROM::Dispose ();
	EmBankMapped::Dispose ();

	// We can't reliably call GetDevice here.  That's because the
	// session may not have been initialized (we could be disposing
	// of everything because an error condition occurred), and so
	// there may be no assigned device yet.  So we can't ask that
	// device if there's flash.  However, EmBankFlash::Dispose
	// doesn't do anything, so we don'thave to worry about it
	// being called.

//	EmAssert (gSession);
//	if (gSession->GetDevice ().HasFlash ())
//		EmBankFlash::Dispose ();

	MetaMemory::Dispose ();
}


// ---------------------------------------------------------------------------
//		¥ Memory::InitializeBanks
// ---------------------------------------------------------------------------
// Initializes the specified memory banks with the given data.

void Memory::InitializeBanks (	EmAddressBank&	iBankInitializer,
								int32			iStartingBankIndex,
								int32			iNumberOfBanks)
{
	for (int32 aBankIndex = iStartingBankIndex;
		aBankIndex < iStartingBankIndex + iNumberOfBanks;
		aBankIndex++)
	{
		gEmMemBanks[aBankIndex] = &iBankInitializer;
	}
}


// ---------------------------------------------------------------------------
//		¥ Memory::ResetBankHandlers
// ---------------------------------------------------------------------------
// Reset the memory bank handlers in response to the chip selects changing.
// Also called on Initialize, Reset, and Load, just to make sure everything's
// nailed down.

void Memory::ResetBankHandlers (void)
{
	EmBankDummy::SetBankHandlers ();
	EmBankRegs::SetBankHandlers ();
	EmBankSRAM::SetBankHandlers ();
	EmBankDRAM::SetBankHandlers ();
	EmBankROM::SetBankHandlers ();
	EmBankMapped::SetBankHandlers ();

	if (gSession->GetDevice ().HardwareID () == 0x0a /*halModelIDVisorPrism*/)
	{
		// Run this one again.  Visor Prism has this thing where the USB
		// controller is at 0x10800000, but it sets up the ROM chip select
		// for 0x10000000 to 0x10FFFFFF.  In order to not have EmBankROM
		// take control over the USB chip, have EmBankRegs install *after*
		// EmBankROM.
		EmBankRegs::SetBankHandlers ();
	}

	EmAssert (gSession);
	if (gSession->GetDevice ().HasFlash ())
		EmBankFlash::SetBankHandlers ();
}


// ---------------------------------------------------------------------------
//		¥ Memory::MapPhysicalMemory
// ---------------------------------------------------------------------------
// Maps a range of physical memory to appear at the same location of the
// emulated Palm OS's virtual memory.

void Memory::MapPhysicalMemory (const void* addr, uint32 size)
{
	EmBankMapped::MapPhysicalMemory (addr, size);
}


// ---------------------------------------------------------------------------
//		¥ Memory::UnmapPhysicalMemory
// ---------------------------------------------------------------------------
// Unmaps a range of physical memory from appearing at the same location of
// the emulated Palm OS's virtual memory.

void Memory::UnmapPhysicalMemory (const void* addr)
{
	EmBankMapped::UnmapPhysicalMemory (addr);
}


// ---------------------------------------------------------------------------
//		¥ Memory::GetMappingInfo
// ---------------------------------------------------------------------------

void Memory::GetMappingInfo (emuptr addr, void** start, uint32* len)
{
	EmBankMapped::GetMappingInfo (addr, start, len);
}


#pragma mark -

// ===========================================================================
//		¥ CEnableFullAccess
// ===========================================================================

long	CEnableFullAccess::fgAccessCount = 0;


// ---------------------------------------------------------------------------
//		¥ CEnableFullAccess::CEnableFullAccess
// ---------------------------------------------------------------------------

CEnableFullAccess::CEnableFullAccess (void) :
	fOldMemAccessFlags (gMemAccessFlags)
#if HAS_PROFILING
	, fOldProfilingCounted (gProfilingCounted)
#endif
{
	gMemAccessFlags = kZeroMemAccessFlags;

#if HAS_PROFILING
	gProfilingCounted = false;
#endif

	++fgAccessCount;
}


// ---------------------------------------------------------------------------
//		¥ CEnableFullAccess::~CEnableFullAccess
// ---------------------------------------------------------------------------

CEnableFullAccess::~CEnableFullAccess (void)
{
	gMemAccessFlags = fOldMemAccessFlags;

#if HAS_PROFILING
	gProfilingCounted = fOldProfilingCounted;
#endif

	--fgAccessCount;
}


// ---------------------------------------------------------------------------
//		¥ CEnableFullAccess::AccessOK
// ---------------------------------------------------------------------------

Bool CEnableFullAccess::AccessOK (void)
{
	return fgAccessCount > 0;
}
