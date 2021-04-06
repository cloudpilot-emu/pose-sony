/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1999-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "TrapPatches.h"

#include "EmBankMapped.h"		// MappedBank::GetEmulatedAddress
#include "EmLowMem.h"			// EmLowMem_GetGlobal
#include "EmMemory.h"			// CEnableFullAccess
#include "EmPalmOS.h"			// ForgetStack
#include "EmPalmHeap.h"			// EmPalmHeap::MemMgrInit, etc.
#include "EmSession.h"			// ScheduleDeferredError
#include "Logging.h"			// LogAppendMsg
#include "MetaMemory.h"			// MetaMemory mark functions
#include "Platform.h"			// GetShortVersionString
#include "ROMStubs.h"			// MemHandleLock, MemHandleUnlock


/* ===========================================================================
	The following macros are used to help us generate patches to all the
	Memory Manager functions.  We patch the entire Memory Manager in order
	to track when we are "in" the Memory Manager.  We want to know this so
	that we can extend special privileges to the Memory Manager when it
	accesses memory; the Memory Manager can touch more of RAM than any other
	executing code.
	
	Since our patches mostly just increment and decrement a counter, most of
	them are generated automatically with macros.  I call these "simple"
	head- and tailpatches.  However, we patch a few other the other Memory
	Manager functions for other reasons.  We can't let the macros generate
	simple patches for these functions and instead have to write them
	ourselves, making sure to perform the same incrementing and decrementing
	the simple functions would do.  I call these patches "complex" head- and
	tailpatches.
	
	Therefore, there are four "categories" of patchings: functions for which
	we want simple head- and tailpatches, functions for which we have a
	complex headpatch but need a simple tailpatch, functions for which we
	have a complex tailpatch but need a simple headpatch, and functions for
	which we already have complex head- and tailpatches and don't need
	simple patches.
	
	(Actually, there is a fifth category: Memory Manager functions that we
	don't want to treat as Memory Manager functions.  The functions in this
	category are MemSet, MemMove, and MemCmp.  We don't want to extend to
	these functions the same privileges that other Memory Manager functions
	get.)
	
	(And there's kind of a sixth category as well: non-Memory Manager
	functions that we want to treat as Memory Manager functions. For
	instance, DmWriteCheck grovels over Memory Manager data structures, so
	we have to give it Memory Manager function privileges. 
	PrvFindMemoryLeaks is also a function like this, but since it's not in
	the trap table, we can't patch it, and have to handle it differently. 
	See MetaMemory.cpp for this.)
	
	The macros below divide the set of Memory Manager functions into the
	four categories.  Any function on any of the four lists will
	automatically have generated for it declarations for head- and
	tailpatches.  Additionally, depending on which list it's on, it will
	optionally have simple head- and tailpatch functions generated for it.
=========================================================================== */
	
// List of functions for which we want to automatically generate simple
// head- and tailpatches.
//
// DmWriteCheck is on this list because it grovels over memory, making sure
// everything is OK before it writes to the storage heap.  ScrInit is on
// this list because it calls MemMove to move the contents of the temporary
// LCD buffer to the real one after it's been allocated; the source of this
// move is a faux buffer out in the middle of an unallocated block.

#define FOR_EACH_STUB_BOTHPATCH_FUNCTION(DO_TO_FUNCTION)	\
	DO_TO_FUNCTION(MemInit)					\
	DO_TO_FUNCTION(MemNumCards)				\
	DO_TO_FUNCTION(MemCardFormat)			\
	DO_TO_FUNCTION(MemCardInfo)				\
	DO_TO_FUNCTION(MemStoreInfo)			\
	DO_TO_FUNCTION(MemStoreSetInfo)			\
	DO_TO_FUNCTION(MemNumHeaps)				\
	DO_TO_FUNCTION(MemNumRAMHeaps)			\
	DO_TO_FUNCTION(MemHeapID)				\
	DO_TO_FUNCTION(MemHeapDynamic)			\
	DO_TO_FUNCTION(MemHeapFreeBytes)		\
	DO_TO_FUNCTION(MemHeapSize)				\
	DO_TO_FUNCTION(MemHeapFlags)			\
	DO_TO_FUNCTION(MemPtrRecoverHandle)		\
	DO_TO_FUNCTION(MemPtrFlags)				\
	DO_TO_FUNCTION(MemPtrSize)				\
	DO_TO_FUNCTION(MemPtrOwner)				\
	DO_TO_FUNCTION(MemPtrHeapID)			\
	DO_TO_FUNCTION(MemPtrDataStorage)		\
	DO_TO_FUNCTION(MemPtrCardNo)			\
	DO_TO_FUNCTION(MemPtrToLocalID)			\
	DO_TO_FUNCTION(MemPtrSetOwner)			\
	DO_TO_FUNCTION(MemHandleFlags)			\
	DO_TO_FUNCTION(MemHandleSize)			\
	DO_TO_FUNCTION(MemHandleOwner)			\
	DO_TO_FUNCTION(MemHandleLockCount)		\
	DO_TO_FUNCTION(MemHandleHeapID)			\
	DO_TO_FUNCTION(MemHandleDataStorage)	\
	DO_TO_FUNCTION(MemHandleCardNo)			\
	DO_TO_FUNCTION(MemHandleToLocalID)		\
	DO_TO_FUNCTION(MemHandleSetOwner)		\
	DO_TO_FUNCTION(MemLocalIDToGlobal)		\
	DO_TO_FUNCTION(MemLocalIDKind)			\
	DO_TO_FUNCTION(MemLocalIDToPtr)			\
	DO_TO_FUNCTION(MemDebugMode)			\
	DO_TO_FUNCTION(MemSetDebugMode)			\
	DO_TO_FUNCTION(MemHeapCheck)			\
	DO_TO_FUNCTION(MemHeapPtr)				\
	DO_TO_FUNCTION(MemStoreSearch)			\
	DO_TO_FUNCTION(MemStoreInit)			\
	DO_TO_FUNCTION(MemNVParams)				\
	DO_TO_FUNCTION(DmWriteCheck)			\
	DO_TO_FUNCTION(WinScreenInit)

// List of functions for which we want to automatically
// generate simple headpatches only.

#define FOR_EACH_STUB_HEADPATCH_FUNCTION(DO_TO_FUNCTION)	\
	DO_TO_FUNCTION(MemKernelInit)			\
	DO_TO_FUNCTION(MemInitHeapTable)		\
	DO_TO_FUNCTION(MemHeapInit)				\
	DO_TO_FUNCTION(MemHeapCompact)			\
	DO_TO_FUNCTION(MemHeapFreeByOwnerID)	\
	DO_TO_FUNCTION(MemChunkNew)				\
	DO_TO_FUNCTION(MemPtrNew)				\
	DO_TO_FUNCTION(MemPtrResize)			\
	DO_TO_FUNCTION(MemPtrUnlock)			\
	DO_TO_FUNCTION(MemPtrResetLock)			\
	DO_TO_FUNCTION(MemHandleNew)			\
	DO_TO_FUNCTION(MemHandleResize)			\
	DO_TO_FUNCTION(MemHandleLock)			\
	DO_TO_FUNCTION(MemHandleUnlock)			\
	DO_TO_FUNCTION(MemHandleResetLock)		\
	DO_TO_FUNCTION(MemLocalIDToLockedPtr)	\
	DO_TO_FUNCTION(MemHeapScramble)

// List of functions for which we want to automatically
// generate simple tailpatches only.

#define FOR_EACH_STUB_TAILPATCH_FUNCTION(DO_TO_FUNCTION)	\
	DO_TO_FUNCTION(MemSemaphoreReserve)		\
	DO_TO_FUNCTION(MemSemaphoreRelease)

// List of functions for which we have compex head- and tailpatches.

#define FOR_EACH_STUB_NOPATCH_FUNCTION(DO_TO_FUNCTION)	\
	DO_TO_FUNCTION(MemChunkFree)			\
	DO_TO_FUNCTION(MemHandleFree)

// This macro contains _all_ memory manager functions.

#define FOR_EACH_MEMMGR_FUNCTION(DO_TO_FUNCTION)		\
	FOR_EACH_STUB_BOTHPATCH_FUNCTION(DO_TO_FUNCTION)	\
	FOR_EACH_STUB_HEADPATCH_FUNCTION(DO_TO_FUNCTION)	\
	FOR_EACH_STUB_TAILPATCH_FUNCTION(DO_TO_FUNCTION)	\
	FOR_EACH_STUB_NOPATCH_FUNCTION(DO_TO_FUNCTION)


class MemMgrHeadpatch
{
	public:
		// Declare headpatches for all the memory manager functions.

		#define DECLARE_HEAD_PATCH(fn) static CallROMType fn (void);
		FOR_EACH_MEMMGR_FUNCTION(DECLARE_HEAD_PATCH)
};


class MemMgrTailpatch
{
	public:
		// Declare tailpatches for all the memory manager functions.

		#define DECLARE_TAIL_PATCH(fn) static void fn (void);
		FOR_EACH_MEMMGR_FUNCTION(DECLARE_TAIL_PATCH)
};


// ======================================================================
//	Proto patch table for the system functions.  This array will be used
//	to create a sparse array at runtime.
// ======================================================================

ProtoPatchTableEntry	gProtoMemMgrPatchTable[] =
{
	#undef INSTALL_PATCH
	#define INSTALL_PATCH(fn) {sysTrap##fn, MemMgrHeadpatch::fn, MemMgrTailpatch::fn},

	FOR_EACH_MEMMGR_FUNCTION(INSTALL_PATCH)

	{0,	NULL,	NULL}
};


const int			kChunkNewValue		= 0x31;
const int			kChunkResizeValue	= 0x33;
const int			kChunkFreeValue		= 0x35;
const int			kHandleNewValue		= 0x71;
const int			kHandleResizeValue	= 0x73;
const int			kHandleFreeValue	= 0x75;
const int			kPtrNewValue		= 0x91;
const int			kPtrResizeValue		= 0x93;
const int			kPtrFreeValue		= 0x95;

long				gMemMgrCount;
long				gMemSemaphoreCount;
unsigned long		gMemSemaphoreReserveTime;
UInt32				gResizeOrigSize;
UInt16				gHeapID;

map<emuptr, EmPalmHeap*>	gRememberedHeaps;

static void			PrvRememberHeapAndPtr (EmPalmHeap* h, emuptr p);
static EmPalmHeap*	PrvGetRememberedHeap (emuptr p);


// ========================================================================
// Macros for extracting parameters from the emulated stack
// Note that use of these has been superceded by the PARAM_FOO
// macros;  we should eventually move over to those macros.
// ========================================================================

#define PARAMETER_SIZE(x)	\
	(sizeof (((StackFrame*) 0)->x))

#define PARAMETER_OFFSET(x)	\
	(m68k_areg (regs, 7) + offsetof (StackFrame, x))

#define GET_PARAMETER(x)		\
	((PARAMETER_SIZE(x) == sizeof (char)) ? EmMemGet8 (PARAMETER_OFFSET(x)) :	\
	 (PARAMETER_SIZE(x) == sizeof (short)) ? EmMemGet16 (PARAMETER_OFFSET(x)) :	\
											EmMemGet32 (PARAMETER_OFFSET(x)))

#define SET_PARAMETER(x, v)		\
	((PARAMETER_SIZE(x) == sizeof (char)) ? EmMemPut8 (PARAMETER_OFFSET(x), v) :	\
	 (PARAMETER_SIZE(x) == sizeof (short)) ? EmMemPut16 (PARAMETER_OFFSET(x), v) :	\
											EmMemPut32 (PARAMETER_OFFSET(x), v))


// ========================================================================
// The following functions define a bunch of StackFrame structs.
// These structs need to mirror the format of parameters pushed
// onto the stack by the emulated code, and so need to be packed
// to 2-byte boundaries.
//
// The pragmas are reversed at the end of the file.
// ========================================================================

#include "PalmPack.h"

// Define a bunch of wrapper head- and tailpatches.
// These are defined via macros for those functions that don't have a
// complex implementation.

#define DEFINE_HEAD_PATCH(fn) CallROMType MemMgrHeadpatch::fn (void) { Patches::EnterMemMgr (#fn); return kExecuteROM; }
#define DEFINE_TAIL_PATCH(fn) void MemMgrTailpatch::fn (void) { Patches::ExitMemMgr (#fn); }

FOR_EACH_STUB_HEADPATCH_FUNCTION(DEFINE_HEAD_PATCH)
FOR_EACH_STUB_BOTHPATCH_FUNCTION(DEFINE_HEAD_PATCH)

FOR_EACH_STUB_TAILPATCH_FUNCTION(DEFINE_TAIL_PATCH)
FOR_EACH_STUB_BOTHPATCH_FUNCTION(DEFINE_TAIL_PATCH)


static const char	kPrvCredits[] =
		"Palm OS Emulator"
		"%By:"
		"%Keith Rollin"
		"%Phillip Shoemaker"
		"%Bob Ebert"
		"%Ben Manuto"
		"%Ben Williamson"
		"%David Creemer"
		"%Jerry Lin"
		"%Patrick Porlan"
		"%Regis Nicolas"
		"%Kenneth Albanowski"

		"%Steve Haneman"
		"%Kevin Munroe"
		"%Sushama D."
		"%Minh Nguyen"
		"%Larry Bolton"
		"%Derek Johnson"

		"%Greg Hewgill"
		"%Craig Schofield"
		"%Bernd Schmidt"
		"%Bill Spitzak"
		"%Matthias Neeracher"
		"%John C. Daub"
		"%Quinn \"The Eskimo\""
		"%Neil Rhodes"
		"%Ron Marianetti"
		"%Adam Dingle"
		"%Jean-loup Gailly"
		"%The Independent..."
		"%...JPEG Group"
		"%Jeff Prosise"
		"%Catherine White"
		"%Stuart Malone"
		"%Jon Fo"
		"%Frank Yellin"
		"%Stefan Hoffmeister"
		"%Red Dutta"
		"%Ben Darnell"
		"%John Ludwig"
		;

static string gPrvVersion;

MemHandle	gVersionResource;
MemHandle	gCreditsResource;


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	MemMgrHeadpatch::MemChunkFree
 *
 * DESCRIPTION:	If the user wants disposed blocks to be filled with a
 *				special value, handle that here.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType MemMgrHeadpatch::MemChunkFree (void)
{
	// Err MemChunkFree(MemPtr chunkDataP)

	Patches::EnterMemMgr ("MemChunkFree");

	struct StackFrame
	{
		MemPtr chunkDataP;
	};

	MemPtr	p = (MemPtr) GET_PARAMETER (chunkDataP);

#ifdef FILL_BLOCKS

	gHeapID = ::MemPtrHeapID (p);

	if (p && gPrefs->FillDisposedBlocks ())
	{
		UInt32	size = ::MemPtrSize (p);

		CEnableFullAccess	munge;
		uae_memset (p, kChunkFreeValue, size);
	}

#endif

	// Remember what heap the chunk was in for later when
	// we need to resync with that heap.

	EmPalmHeap*	heap = const_cast<EmPalmHeap*>(EmPalmHeap::GetHeapByPtr (p));
	::PrvRememberHeapAndPtr (heap, (emuptr) p);

	// In case this chunk contained a stack, forget all references
	// to that stack (or those stacks).

	if (heap)
	{
		const EmPalmChunk*	chunk = heap->GetChunkBodyContaining ((emuptr) p);
		if (chunk)
		{
			EmPalmOS::ForgetStacksIn (chunk->BodyStart (), chunk->BodySize ());
		}
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrHeadpatch::MemHandleFree
 *
 * DESCRIPTION:	If the user wants disposed blocks to be filled with a
 *				special value, handle that here.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType MemMgrHeadpatch::MemHandleFree (void)
{
	// Err MemHandleFree(MemHandle h)

	Patches::EnterMemMgr ("MemHandleFree");

	struct StackFrame
	{
		MemHandle h;
	};

	MemHandle	h = (MemHandle) GET_PARAMETER (h);

#ifdef FILL_BLOCKS

	gHeapID = ::MemHandleHeapID (h);

	if (h && gPrefs->FillDisposedBlocks ())
	{
		UInt32	size = ::MemHandleSize (h);
		if (p)
		{
			{
				CEnableFullAccess	munge;
				uae_memset (p, kHandleFreeValue, size);
			}
		}
	}

#endif

	// Remember what heap the chunk was in for later when
	// we need to resync with that heap.

	EmPalmHeap*	heap = const_cast<EmPalmHeap*>(EmPalmHeap::GetHeapByHdl (h));
	::PrvRememberHeapAndPtr (heap, (emuptr) h);

	// In case this chunk contained a stack, forget all references
	// to that stack (or those stacks).

	if (heap)
	{
		const EmPalmChunk*	chunk = heap->GetChunkReferencedBy (h);
		if (chunk)
		{
			EmPalmOS::ForgetStacksIn (chunk->BodyStart (), chunk->BodySize ());
		}
	}

	// In case this chunk contained system code, invalid our cache
	// of valid system code chunks.

	if (h)
	{
		CEnableFullAccess	munge;	// Remove blocks on memory access.

		emuptr	pp	= (emuptr) memHandleUnProtect(h);
		emuptr	p	= EmMemGet32 (pp);
		MetaMemory::ChunkUnlocked (p);
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrHeadpatch::MemSemaphoreRelease
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType MemMgrHeadpatch::MemSemaphoreRelease (void)
{
	// Err MemSemaphoreRelease (Boolean writeAccess)

	Patches::EnterMemMgr ("MemSemaphoreRelease");

	struct StackFrame
	{
		Boolean writeAccess;
	};

	Boolean	writeAccess = GET_PARAMETER (writeAccess);

	if (writeAccess
		&& --gMemSemaphoreCount == 0
		&& Patches::HasWellBehavedMemSemaphoreUsage ())
	{
		const unsigned long	kMaxElapsedSeconds	= 60 /* seconds */ * 100 /* ticks per second */;

		CEnableFullAccess	munge;

		unsigned long	curCount		= EmLowMem_GetGlobal (hwrCurTicks);
		unsigned long	elapsedTicks	= curCount - gMemSemaphoreReserveTime;

		if (elapsedTicks > kMaxElapsedSeconds)
		{
			EmAssert (gSession);
			gSession->ScheduleDeferredError (new EmDeferredErrMemMgrSemaphore);
		}
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrHeadpatch::MemSemaphoreReserve
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType MemMgrHeadpatch::MemSemaphoreReserve (void)
{
	// Err MemSemaphoreReserve (Boolean writeAccess)

	Patches::EnterMemMgr ("MemSemaphoreReserve");

	struct StackFrame
	{
		Boolean writeAccess;
	};

	Boolean	writeAccess = GET_PARAMETER (writeAccess);

	if (writeAccess
		&& gMemSemaphoreCount++ == 0
		&& Patches::HasWellBehavedMemSemaphoreUsage ())
	{
		CEnableFullAccess	munge;
		gMemSemaphoreReserveTime = EmLowMem_GetGlobal (hwrCurTicks);
	}

	return kExecuteROM;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemChunkFree
 *
 * DESCRIPTION:	If the user wants disposed blocks to be filled with a
 *				special value, handle that here.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemChunkFree (void)
{
	// Err MemChunkFree(MemPtr chunkDataP)

	struct StackFrame
	{
		MemPtr chunkDataP;
	};

	emuptr	p = GET_PARAMETER (chunkDataP);

	EmPalmHeap*	heap = ::PrvGetRememberedHeap (p);

	EmPalmChunkList	delta;
	EmPalmHeap::MemChunkFree (heap, &delta);
	MetaMemory::Resync (delta);

	{
		CEnableFullAccess	munge;

		emuptr	w = EmLowMem_GetGlobal (uiGlobals.firstWindow);
		if (w == p)
		{
			Patches::SetUIReset (false);
		}
	}

	Patches::ExitMemMgr ("MemChunkFree");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemChunkNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemChunkNew (void)
{
	// MemPtr MemChunkNew(UInt16 heapID, UInt32 size, UInt16 attr)

	struct StackFrame
	{
		UInt16 heapID;
		UInt32 size;
		UInt16 attr;
	};

	uint32	heapID	= GET_PARAMETER (heapID);
//	uint32	size	= GET_PARAMETER (size);
	uint16	attr	= GET_PARAMETER (attr);
	emuptr	c		= m68k_areg (regs, 0);

#ifdef FILL_BLOCKS

	if (gPrefs->FillNewBlocks ())
	{
		if (c)
		{

			if (attr & memNewChunkFlagNonMovable)
			{
				CEnableFullAccess	munge;
				uae_memset (c, kChunkNewValue, size);
			}
			else
			{
				emuptr	p = (emuptr) ::MemHandleLock ((MemHandle) c);
				if (p)
				{
					{
						CEnableFullAccess	munge;
						uae_memset (p, kChunkNewValue, size);
					}

					::MemHandleUnlock ((MemHandle) c);
				}
			}
		}
	}

#endif

	EmPalmChunkList	delta;
	EmPalmHeap::MemChunkNew (heapID, (MemPtr) c, attr, &delta);
	MetaMemory::Resync (delta);

#define TRACK_BOOT_ALLOCATION 0
#if TRACK_BOOT_ALLOCATION
	if ((attr & memNewChunkFlagNonMovable) != 0)
	{
		CEnableFullAccess	munge;	// Remove blocks on memory access.
		emuptr	pp = (emuptr) memHandleUnProtect(c);
		c = EmMemGet32 (pp);
	}

	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr ((MemPtr) c);
	if (heap)
	{
		const EmPalmChunk*	chunk = heap->GetChunkContaining (c);
		if (chunk)
		{
			LogAppendMsg ("Chunk allocated, loc = 0x%08X, size = 0x%08X",
				(int) chunk->Start (), (int) chunk->Size ());
		}
	}
#endif

	Patches::ExitMemMgr ("MemChunkNew");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHandleFree
 *
 * DESCRIPTION:	If the user wants disposed blocks to be filled with a
 *				special value, handle that here.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHandleFree (void)
{
	// Err MemHandleFree(MemHandle h)

	struct StackFrame
	{
		MemHandle h;
	};

	emuptr	h = GET_PARAMETER (h);

	EmPalmHeap*	heap = ::PrvGetRememberedHeap (h);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHandleFree (heap, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemHandleFree");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHandleNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHandleNew (void)
{
	// MemHandle MemHandleNew(UInt32 size)

	struct StackFrame
	{
		UInt32 size;
	};

//	uint32	size = GET_PARAMETER (size);
	emuptr	h = m68k_areg (regs, 0);


#ifdef FILL_BLOCKS

	if (h)
	{
		if (size && gPrefs->FillNewBlocks ())
		{
			emuptr	p = (emuptr) ::MemHandleLock ((MemHandle) h);
			if (p)
			{
				{
					CEnableFullAccess	munge;
					uae_memset (p, kHandleNewValue, size);
				}

				::MemHandleUnlock ((MemHandle) h);
			}
		}
	}

#endif

	EmPalmChunkList	delta;
	EmPalmHeap::MemHandleNew ((MemHandle) h, &delta);
	MetaMemory::Resync (delta);

#if TRACK_BOOT_ALLOCATION
	emuptr	c;

	if (1)
	{
		CEnableFullAccess	munge;	// Remove blocks on memory access.
		emuptr	pp = (emuptr) memHandleUnProtect(h);
		c = EmMemGet32 (pp);
	}

	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr ((MemPtr) c);
	if (heap)
	{
		const EmPalmChunk*	chunk = heap->GetChunkContaining (c);
		if (chunk)
		{
			LogAppendMsg ("Handle allocated, loc = 0x%08X, size = 0x%08X",
				(int) chunk->Start (), (int) chunk->Size ());
		}
	}
#endif

	Patches::ExitMemMgr ("MemHandleNew");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHandleLock
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHandleLock (void)
{
	// Err MemHandleLock(MemHandle h) 

	struct StackFrame
	{
		MemHandle h;
	};

	MemHandle	h = (MemHandle) GET_PARAMETER (h);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHandleLock (h, &delta);
	MetaMemory::Resync (delta);

	const char*	remap = NULL;
	if (h == gVersionResource)
	{
		gPrvVersion = Platform::GetShortVersionString ();

		// If there's a space in there (from "3.0 Prerelease"), get rid
		// of the extra part -- it doesn't fit.

		string::size_type	index = gPrvVersion.find (' ');
		if (index != string::npos)
		{
			gPrvVersion = gPrvVersion.substr (0, index);
		}

		remap = gPrvVersion.c_str ();
	}
	else if (h == gCreditsResource)
	{
		remap = kPrvCredits;
	}

	if (remap)
	{
		EmBankMapped::MapPhysicalMemory (remap, strlen (remap) + 1);
		m68k_areg (regs, 0) = EmBankMapped::GetEmulatedAddress (remap);
	}

	Patches::ExitMemMgr ("MemHandleLock");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHandleResetLock
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHandleResetLock (void)
{
	// Err MemHandleResetLock(MemHandle h) 

	struct StackFrame
	{
		MemHandle h;
	};

	MemHandle	h = (MemHandle) GET_PARAMETER (h);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHandleResetLock (h, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemHandleResetLock");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHandleResize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHandleResize (void)
{
	// Err MemHandleResize(MemHandle h,  UInt32 newSize) 

	struct StackFrame
	{
		MemHandle h;
		UInt32 newSize;
	};

	emuptr	h = GET_PARAMETER (h);
//	uint32	newSize = GET_PARAMETER (newSize);

#ifdef FILL_BLOCKS

	if (h)
	{
		if (newSize > gResizeOrigSize && gPrefs->FillResizedBlocks ())
		{
			emuptr	p = (emuptr) ::MemHandleLock ((MemHandle) h);
			if (p)
			{
				{
					CEnableFullAccess	munge;
					uae_memset (p + gResizeOrigSize, kHandleResizeValue, newSize - gResizeOrigSize);
				}

				::MemHandleUnlock ((MemHandle) h);
			}
		}
	}

#endif

	EmPalmChunkList	delta;
	EmPalmHeap::MemHandleResize ((MemHandle) h, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemHandleResize");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHandleUnlock
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHandleUnlock (void)
{
	// Err MemHandleUnlock(MemHandle h) 

	struct StackFrame
	{
		MemHandle h;
	};

	MemHandle	h = (MemHandle) GET_PARAMETER (h);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHandleUnlock (h, &delta);
	MetaMemory::Resync (delta);

	const char* remap = NULL;
	if (h == gVersionResource)
	{
		remap = gPrvVersion.c_str ();
	}
	else if (h == gCreditsResource)
	{
		remap = kPrvCredits;
	}

	if (remap)
	{
		EmBankMapped::UnmapPhysicalMemory (remap);
	}

	Patches::ExitMemMgr ("MemHandleUnlock");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHeapCompact
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHeapCompact (void)
{
	// Err MemHeapCompact(UInt16 heapID)

	struct StackFrame
	{
		UInt16 heapID;
	};

	UInt16	heapID = GET_PARAMETER (heapID);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHeapCompact (heapID, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemHeapCompact");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHeapFreeByOwnerID
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

//	!!! This should probably walk the heap itself so that it can just mark
//	the chunks that are about to be freed.

void MemMgrTailpatch::MemHeapFreeByOwnerID (void)
{
	// Err MemHeapFreeByOwnerID(UInt16 heapID, UInt16 ownerID)

	struct StackFrame
	{
		UInt16 heapID;
		UInt16 ownerID;
	};

	UInt16	heapID = GET_PARAMETER (heapID);
	UInt16	ownerID = GET_PARAMETER (ownerID);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHeapFreeByOwnerID (heapID, ownerID, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemHeapFreeByOwnerID");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemInitHeapTable
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemInitHeapTable (void)
{
	// Err MemInitHeapTable(UInt16 cardNo)

	struct StackFrame
	{
		UInt16 cardNo;
	};

	UInt16	cardNo			= GET_PARAMETER (cardNo);

	EmPalmHeap::MemInitHeapTable (cardNo);

	Patches::ExitMemMgr ("MemInitHeapTable");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHeapInit
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHeapInit (void)
{
	// Err MemHeapInit(UInt16 heapID, Int16 numHandles, Boolean initContents)

	struct StackFrame
	{
		UInt16 heapID;
		Int16 numHandles;
		Boolean initContents;
	};

	UInt16	heapID			= GET_PARAMETER (heapID);
	Int16	numHandles		= GET_PARAMETER (numHandles);
	Boolean	initContents	= GET_PARAMETER (initContents);

	EmPalmHeap::MemHeapInit (heapID, numHandles, initContents);

	Patches::ExitMemMgr ("MemHeapInit");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemHeapScramble
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemHeapScramble (void)
{
	// Err MemHeapScramble(UInt16 heapID)

	struct StackFrame
	{
		UInt16 heapID;
	};

	UInt16	heapID = GET_PARAMETER (heapID);

	EmPalmChunkList	delta;
	EmPalmHeap::MemHeapScramble (heapID, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemHeapScramble");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemKernelInit
 *
 * DESCRIPTION:	MemKernelInit is called just after AMXDriversInit, which
 *				is where the exception vectors are set up.  After those
 *				vectors are installed, there really shouldn't be any
 *				more memory accesses to that range of memory. Thus,
 *				memory access there is flagged as invalid.
 *
 *				While we're here, let's mark some other memory
 *				locations, too.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemKernelInit (void)
{
	MetaMemory::MarkLowMemory (		MetaMemory::GetLowMemoryBegin (),
									MetaMemory::GetLowMemoryEnd ());
	MetaMemory::MarkSystemGlobals (	MetaMemory::GetSysGlobalsBegin (),
									MetaMemory::GetSysGlobalsEnd ());
	MetaMemory::MarkHeapHeader (	MetaMemory::GetHeapHdrBegin (0),
									MetaMemory::GetHeapHdrEnd (0));

	// On the Visor, these are fair game.
	MetaMemory::MarkSystemGlobals (offsetof (M68KExcTableType, unassigned[12]),
								 offsetof (M68KExcTableType, unassigned[16]));

	Patches::ExitMemMgr ("MemKernelInit");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemLocalIDToLockedPtr
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemLocalIDToLockedPtr (void)
{
	// MemPtr MemLocalIDToLockedPtr(LocalID local,  UInt16 cardNo)

	MemPtr	p = (MemPtr) m68k_areg (regs, 0);

	EmPalmChunkList	delta;
	EmPalmHeap::MemLocalIDToLockedPtr (p, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemLocalIDToLockedPtr");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemPtrNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemPtrNew (void)
{
	// MemPtr MemPtrNew(UInt32 size) 

	struct StackFrame
	{
		UInt32 size;
	};

//	uint32	size = GET_PARAMETER (size);
	emuptr	p = m68k_areg (regs, 0);

#ifdef FILL_BLOCKS

	if (p)
	{
		if (gPrefs->FillNewBlocks ())
		{
			CEnableFullAccess	munge;
			uae_memset (p, kPtrNewValue, size);
		}
	}

#endif

	EmPalmChunkList	delta;
	EmPalmHeap::MemPtrNew ((MemPtr) p, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemPtrNew");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemPtrResetLock
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemPtrResetLock (void)
{
	// Err MemPtrResetLock(MemPtr p) 

	struct StackFrame
	{
		MemPtr p;
	};

	MemPtr	p = (MemPtr) GET_PARAMETER (p);

	EmPalmChunkList	delta;
	EmPalmHeap::MemPtrResetLock (p, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemPtrResetLock");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemPtrResize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemPtrResize (void)
{
	// Err MemPtrResize(MemPtr p, UInt32 newSize)

	struct StackFrame
	{
		MemPtr p;
		UInt32 newSize;
	};

	emuptr	p = GET_PARAMETER (p);
//	uint32	newSize = GET_PARAMETER (newSize);

#ifdef FILL_BLOCKS

	if (p)
	{
		if (newSize > gResizeOrigSize && gPrefs->FillResizedBlocks ())
		{
			CEnableFullAccess	munge;
			uae_memset (p + gResizeOrigSize, kPtrResizeValue, newSize - gResizeOrigSize);
		}
	}

#endif

	EmPalmChunkList	delta;
	EmPalmHeap::MemPtrResize ((MemPtr) p, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemPtrResize");
}


/***********************************************************************
 *
 * FUNCTION:	MemMgrTailpatch::MemPtrUnlock
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MemMgrTailpatch::MemPtrUnlock (void)
{
	// Err MemPtrUnlock(MemPtr p) 

	struct StackFrame
	{
		MemPtr p;
	};

	MemPtr	p = (MemPtr) GET_PARAMETER (p);

	EmPalmChunkList	delta;
	EmPalmHeap::MemPtrUnlock (p, &delta);
	MetaMemory::Resync (delta);

	Patches::ExitMemMgr ("MemPtrUnlock");
}


void PrvRememberHeapAndPtr (EmPalmHeap* h, emuptr p)
{
	EmAssert (gRememberedHeaps.find (p) == gRememberedHeaps.end ());

	gRememberedHeaps[p] = h;
}

EmPalmHeap* PrvGetRememberedHeap (emuptr p)
{
	EmPalmHeap*	result = NULL;

	map<emuptr, EmPalmHeap*>::iterator	iter = gRememberedHeaps.find (p);
	if (iter != gRememberedHeaps.end ())
	{
		result = iter->second;
		gRememberedHeaps.erase (iter);
	}

	EmAssert (result);

	return result;
}
