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
#include "MetaMemory.h"

#include "DebugMgr.h"			// Debug::GetRoutineName
#include "EmBankSRAM.h"			// gRAMBank_Size
#include "EmHAL.h"				// EmHAL
#include "EmLowMem.h"			// LowMem_SetGlobal, LowMem_GetGlobal
#include "EmMemory.h"			// CEnableFullAccess, EmMemGetMetaAddress
#include "EmPalmFunction.h"		// InFoo functions
#include "EmPalmOS.h"			// StackRange, GetBootStack
#include "EmPalmStructs.h"		// EmAliasWindowType, EmAliasFormType
#include "EmSession.h"			// gSession->ScheduleDeferredError
#include "Logging.h"			// ReportUIMgrDataAccess
#include "Miscellaneous.h"		// FindFunctionName
#include "ROMStubs.h"			// SysKernelInfo
#include "TrapPatches.h"		// IsInSysBinarySearch, OSMajorMinorVersion

struct EmTaggedPalmChunk : public EmPalmChunk
{
	EmTaggedPalmChunk (void) {}
	EmTaggedPalmChunk (const EmPalmChunk& chunk, Bool isSystemChunk) :
		EmPalmChunk (chunk),
		fIsSystemChunk (isSystemChunk)
		{}

	Bool	fIsSystemChunk;
};

typedef vector<EmTaggedPalmChunk>	EmTaggedPalmChunkList;

static EmTaggedPalmChunkList	gTaggedChunks;
static Bool						gHaveLastChunk;
static EmTaggedPalmChunk		gLastChunk;

#pragma mark -

// ---------------------------------------------------------------------------
//		� MetaMemory::Initialize
// ---------------------------------------------------------------------------

void MetaMemory::Initialize (void)
{
}


// ---------------------------------------------------------------------------
//		� MetaMemory::Reset
// ---------------------------------------------------------------------------

void MetaMemory::Reset (void)
{
	gTaggedChunks.clear ();
	gHaveLastChunk = false;
}


// ---------------------------------------------------------------------------
//		� MetaMemory::Save
// ---------------------------------------------------------------------------

void MetaMemory::Save (SessionFile&)
{
}


// ---------------------------------------------------------------------------
//		� MetaMemory::Load
// ---------------------------------------------------------------------------

void MetaMemory::Load (SessionFile&)
{
	gTaggedChunks.clear ();
	gHaveLastChunk = false;
}


// ---------------------------------------------------------------------------
//		� MetaMemory::Dispose
// ---------------------------------------------------------------------------

void MetaMemory::Dispose (void)
{
}


#if FOR_LATER
// ---------------------------------------------------------------------------
//		� MetaMemory::MarkUninitialized
// ---------------------------------------------------------------------------

void MetaMemory::MarkUninitialized (emuptr begin, emuptr end)
{
	MarkRange (begin, end, kUninitialized);
}
#endif


#if FOR_LATER
// ---------------------------------------------------------------------------
//		� MetaMemory::MoveUninitialized
// ---------------------------------------------------------------------------
//	Transfer the initialized/uninitialized state of a range of bytes to a
//	new location.  Called during MemMove, so that the state of the bytes
//	being moved can be preserved.

void MetaMemory::MoveUninitialized (emuptr source, emuptr dest, uint32 size)
{
	source = MASK (source);
	dest = MASK (dest);

	if (source > fgMetaMemorySize)
		return;

	if (dest > fgMetaMemorySize)
		return;

	uint8*	p0 = fgMetaMemory + source;
	uint8*	p1 = fgMetaMemory + dest;

	while (size--)
	{
		uint8	dValue = *p0;
		uint8	sValue = *p1;

		dValue = (dValue & ~kUninitialized) | (sValue & kUninitialized);

		*p0 = dValue;

		++p0;
		++p1;
	}
}
#endif


#if FOR_LATER
// ---------------------------------------------------------------------------
//		� MetaMemory::MarkInitialized
// ---------------------------------------------------------------------------

void MetaMemory::MarkInitialized (emuptr begin, emuptr end)
{
	UnmarkRange (begin, end, kUninitialized);
}
#endif


// ---------------------------------------------------------------------------
//		� MetaMemory::SyncHeap
// ---------------------------------------------------------------------------
//	Find chunk headers and mark them as memory manager structures.  Mark
//	unlocked relocatable chunks as "unlocked" and "initialized" (as we don't
//	have any idea any more what their state was before they were moved).
//	Mark free blocks as "uninitialized".

// ---------------------------------------------------------------------------
//		� MetaMemory::SyncOneChunk
// ---------------------------------------------------------------------------
//	Mark the chunk header, free chunks, and unlocked chunks.

void MetaMemory::SyncOneChunk (const EmPalmChunk& chunk)
{
	// Set the access for the chunk header.

	MarkChunkHeader (chunk.HeaderStart (), chunk.HeaderEnd ());

	// Set the access for the body of the chunk.

	// Check for free chunks.

	if (chunk.Free ())
	{
		MarkFreeChunk (chunk.BodyStart (), chunk.BodyEnd ());
	}

	// It's not a free chunk; see if it's unlocked.

	else if (chunk.LockCount () == 0)
	{
		MarkUnlockedChunk (chunk.BodyStart (), chunk.BodyEnd ());
	}

	// It's an allocated, locked chunk.

	else
	{
		MarkTotalAccess (chunk.BodyStart (), chunk.BodyEnd ());
	}

	// Set the access for any unallocated trailing bytes.

	if (chunk.TrailerSize () > 0)
	{
		MarkChunkTrailer (chunk.TrailerStart (), chunk.TrailerEnd ());
	}
}


// ---------------------------------------------------------------------------
//		� MetaMemory::Resync
// ---------------------------------------------------------------------------

void MetaMemory::Resync (const EmPalmChunkList& delta)
{
	if (delta.size () == 0)
		return;

	// Get the heap that was changed. Assume that all chunks in the
	// delta list are in the same heap for now.

	EmPalmChunkList::const_iterator	iter = delta.begin ();
	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr (iter->Start ());
	if (!heap)
		return;

	// Only support syncing with dynamic heap for now.

	if (heap->HeapID () != 0)
		return;

	while (iter != delta.end ())
	{
		SyncOneChunk (*iter);
		++iter;
	}

	// This process has just wiped out any access bits we've set
	// for UI objects.  Re-establish those.

	MetaMemory::MarkUIObjects ();

	// Mark the master pointer tables.  Mark the MPT headers as
	// for use by the Memory Manager.  However, mark the tables
	// themselves as usable by any of the OS; a lot of the OS
	// uses a MemMgr macro to deref handles directly.

	ITERATE_MPTS(*heap, mpt_iter, end)
	{
		MarkMPT (mpt_iter->Start (), mpt_iter->TableStart ());
		MarkMPT (mpt_iter->TableStart (), mpt_iter->End ());
		++mpt_iter;
	}

	// Hack for startup time.  When MemInit is called, it creates
	// a free block spanning the entire dynamic heap.  However,
	// that's where the current stack happens to be.  We still need
	// access to that, so free it up.

	StackRange	range = EmPalmOS::GetBootStack ();
	MarkTotalAccess (range.fBottom, range.fTop);
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GetWhatHappened
// ---------------------------------------------------------------------------
//	Some memory access violation was detected.  Nail down more closely what
//	it was.  These checks can be expensive, as they only occur when we're
//	pretty sure we're about to report an error in a dialog.

Errors::EAccessType MetaMemory::GetWhatHappened (emuptr address, long size, Bool forRead)
{
	// If  we've whisked away all memory access checking, return now.

	if (CEnableFullAccess::AccessOK ())
		return Errors::kOKAccess;

	CEnableFullAccess	munge;	// Remove blocks on memory access.

	Errors::EAccessType	whatHappened = Errors::kUnknownAccess;

	// Now figure out what just happened.

	if (MetaMemory::IsLowMemory (address, size))
	{
		whatHappened = Errors::kLowMemAccess;
	}

	else if (MetaMemory::IsSystemGlobal (address, size))
	{
		whatHappened = Errors::kGlobalVarAccess;
	}

	else if (MetaMemory::IsScreenBuffer (address, size))
	{
		whatHappened = Errors::kScreenAccess;
	}

	else if (MetaMemory::IsLowStack (address, size))
	{
		whatHappened = Errors::kLowStackAccess;
	}

#if FOR_LATER
	else if (MetaMemory::IsUninitialized (address, size))
	{
		if (MetaMemory::IsStack (address, size))
		{
			whatHappened = Errors::kUninitializedStack;
		}

		else if (MetaMemory::IsAllocatedChunk (address, size))
		{
			whatHappened = Errors::kUninitializedChunk;
		}
	}
#endif

	else
	{
		Bool	inUIObject, butItsOK;
		CheckUIObjectAccess (address, size, forRead, inUIObject, butItsOK);

		if (inUIObject)
		{
			// We don't really need to do anything else (like return an
			// error or check "butItsOK", since if an error occured,
			// an error object will be scheduled with the EmSession.

			whatHappened = Errors::kOKAccess;
		}
		else
		{
			WhatHappenedData	info;
			info.result			= Errors::kUnknownAccess;
			info.address		= address;
			info.size			= size;
			info.forRead		= forRead;

			const EmPalmHeap*	heap = EmPalmHeap::GetHeapByID (0);

			if (heap)
				GWH_ExamineHeap (*heap, info);

			if (info.result != Errors::kUnknownAccess)
			{
				whatHappened = info.result;
			}
		}
	}

	whatHappened = AllowForBugs (address, size, forRead, whatHappened);

	return whatHappened;
}


// ---------------------------------------------------------------------------
//		� MetaMemory::AllowForBugs
// ---------------------------------------------------------------------------

Errors::EAccessType MetaMemory::AllowForBugs (emuptr address, long size, Bool forRead, Errors::EAccessType whatHappened)
{
	if (whatHappened == Errors::kOKAccess)
	{
		return whatHappened;
	}

	if (forRead)
	{
		// Let PrvFindMemoryLeaks have the run of the show.

		if (::InPrvFindMemoryLeaks ())
		{
			return Errors::kOKAccess;
		}

		// SecPrvRandomSeed calls Crc16CalcBlock (NULL, 0x1000, 0).

		emuptr	a6 = m68k_areg (regs, 6);
		emuptr	rtnAddr = EmMemGet32 (a6 + 4);

		if (address < 0x1000 &&
			::InCrc16CalcBlock () &&
			::InSecPrvRandomSeed (rtnAddr))
		{
			return Errors::kOKAccess;
		}

		if (whatHappened == Errors::kLowMemAccess)
		{
			// See if the the low-memory checksummer is at work.

			if (size == 4 &&
				::InPrvSystemTimerProc ())
			{
				return Errors::kOKAccess;
			}

			// There's a bug in BackspaceChar (pre-3.2) that accesses the word at 0x0000.

			if (Patches::HasBackspaceCharBug () &&
				address == 0x0000 && size == 2 &&
				InBackspaceChar ())
			{
				return Errors::kOKAccess;
			}

			// There's a bug in FldDelete (pre-3.2) that accesses the word at 0x0000.

			if (Patches::HasFldDeleteBug () &&
				address == 0x0000 && size == 2 &&
				::InFldDelete ())
			{
				return Errors::kOKAccess;
			}

			// There's a bug in GrfProcessStroke (pre-3.1) that accesses the word at 0x0002.

			if (Patches::HasGrfProcessStrokeBug () &&
				address == 0x0002 && size == 2 &&
				::InGrfProcessStroke ())
			{
				return Errors::kOKAccess;
			}

			// There's a bug in NetPrvTaskMain (pre-3.2) that accesses low-memory.

			if (Patches::HasNetPrvTaskMainBug () &&
				::InNetPrvTaskMain ())
			{
				return Errors::kOKAccess;
			}
		}

		//	There's a bug in pre-3.0 versions of SysBinarySearch that cause it to
		//	call the user callback function with a pointer just past the array
		//	to search.

		if (Patches::HasSysBinarySearchBug () &&
			Patches::IsInSysBinarySearch ())
		{
			return Errors::kOKAccess;
		}

		// The Certicom Encryption library snags 20 bytes of data from some
		// unspecified place in RAM to help randomize its keys.  Allow this
		// access.  (Note: the test for the function that performs the access
		// is fragile.  An alternative may be to allow full-memory access
		// while SecLibEncryptBegin is active, which appears to be the only
		// place where the Certicom function is called from.

		if (::In_CerticomMemCpy ())
		{
			return Errors::kOKAccess;
		}

		// dns_decode_name appears to walk off the end of an input buffer.
		// I'm not sure why that happens, yet, but let's allow it for now.

		if (::Indns_decode_name () &&
			whatHappened == Errors::kMemMgrAccess)
		{
			return Errors::kOKAccess;
		}
	}

	if (whatHappened == Errors::kLowMemAccess)
	{
		// Visor replaces the exception vector at 0x0008;
		// allow for that.

		if (size == 4 && address == 0x0008 && (::InHsPrvInit () || ::InHsPrvInitCard ()))
		{
			return Errors::kOKAccess;
		}
	}

	if (whatHappened == Errors::kGlobalVarAccess)
	{
		// Let TSM glue routines (linked with applications) access
		// the TSM global vars.


		if ((address == EmLowMem_AddressOf (tsmFepLibStatusP) ||
			 address == EmLowMem_AddressOf (tsmFepLibRefNum)) &&
			 ::InTsmGlueGetFepGlobals ())
		{
			return Errors::kOKAccess;
		}

		// Let locale modules access the Intl Mgr global pointer.
		
		if ((address == EmLowMem_AddressOf (intlMgrGlobalsP)) &&
			(::InPrvGetIntlMgrGlobalsP() ||
			 ::InPrvSetIntlMgrGlobalsP()))
		{
			return Errors::kOKAccess;
		}

		// Always allow access to this for our Test Harness.

		if (address == EmLowMem_AddressOf (testHarnessGlobalsP))
		{
			return Errors::kOKAccess;
		}
	}

	return whatHappened;
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GetLowMemoryBegin
// ---------------------------------------------------------------------------

emuptr MetaMemory::GetLowMemoryBegin (void)
{
	return 0;
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GetLowMemoryEnd
// ---------------------------------------------------------------------------

emuptr MetaMemory::GetLowMemoryEnd (void)
{
	return offsetof (LowMemHdrType, globals);
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GetSysGlobalsBegin
// ---------------------------------------------------------------------------

emuptr MetaMemory::GetSysGlobalsBegin (void)
{
	return offsetof (LowMemHdrType, globals);
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GetSysGlobalsEnd
// ---------------------------------------------------------------------------

emuptr MetaMemory::GetSysGlobalsEnd (void)
{
	CEnableFullAccess	munge;	// Remove blocks on memory access.

	return EmLowMem_GetGlobal (sysDispatchTableP) + 
			EmLowMem_GetGlobal (sysDispatchTableSize) * (sizeof (void*));
}
// ---------------------------------------------------------------------------
//		� MetaMemory::GetHeapHdrBegin
// ---------------------------------------------------------------------------

emuptr MetaMemory::GetHeapHdrBegin (UInt16 heapID)
{
	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByID (heapID);
	if (!heap)
		return EmMemNULL;

	return heap->HeaderStart ();
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GetHeapHdrEnd
// ---------------------------------------------------------------------------

emuptr MetaMemory::GetHeapHdrEnd (UInt16 heapID)
{
	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByID (heapID);
	if (!heap)
		return EmMemNULL;

	return heap->HeaderEnd ();
}


Bool MetaMemory::IsLowMemory (emuptr testAddress, uint32 size)
{
	return testAddress >= GetLowMemoryBegin () && testAddress + size <= GetLowMemoryEnd ();
}


Bool MetaMemory::IsSystemGlobal (emuptr testAddress, uint32 size)
{
	return testAddress >= GetSysGlobalsBegin () && testAddress + size <= GetSysGlobalsEnd ();
}


Bool MetaMemory::IsScreenBuffer (emuptr testAddress, uint32 size)
{
	return IsScreenBuffer (EmMemGetMetaAddress (testAddress), size);
}


Bool MetaMemory::IsMemMgrData (emuptr testAddress, uint32 size)
{
	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr (testAddress);
	
	if (heap)
	{
		ITERATE_CHUNKS (*heap, iter, end)
		{
			if ((iter->HeaderContains (testAddress) &&
				iter->HeaderContains (testAddress + size)) ||
				(iter->TrailerContains (testAddress) &&
				iter->TrailerContains (testAddress + size)))
			{
				return true;
			}

			++iter;
		}
	}
	
	return false;
}


Bool MetaMemory::IsUnlockedChunk (emuptr testAddress, uint32 size)
{
	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr (testAddress);
	
	if (heap)
	{
		ITERATE_CHUNKS (*heap, iter, end)
		{
			if (iter->BodyContains (testAddress) &&
				iter->BodyContains (testAddress + size) &&
				iter->LockCount () == 0)
			{
				return true;
			}

			++iter;
		}
	}
	
	return false;
}


Bool MetaMemory::IsFreeChunk (emuptr testAddress, uint32 size)
{
	const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr (testAddress);
	
	if (heap)
	{
		ITERATE_CHUNKS (*heap, iter, end)
		{
			if (iter->BodyContains (testAddress) &&
				iter->BodyContains (testAddress + size) &&
				iter->Free () == 0)
			{
				return true;
			}

			++iter;
		}
	}
	
	return false;
}


Bool MetaMemory::IsUninitialized (emuptr testAddress, uint32 size)
{
#if FOR_LATER
	return (fgMetaMemory [MASK(testAddress)] & kUninitialized) != 0;
#else
	UNUSED_PARAM(testAddress)
	UNUSED_PARAM(size)

	return false;
#endif
}


Bool MetaMemory::IsStack (emuptr testAddress, uint32 size)
{
	UNUSED_PARAM(testAddress)
	UNUSED_PARAM(size)

	return false;
}


Bool MetaMemory::IsLowStack (emuptr testAddress, uint32 size)
{
	UNUSED_PARAM(testAddress)
	UNUSED_PARAM(size)

	return false;
}


Bool MetaMemory::IsAllocatedChunk (emuptr testAddress, uint32 size)
{
	return !IsFreeChunk (testAddress, size);
}


// ---------------------------------------------------------------------------
//		� MetaMemory::MarkRange
// ---------------------------------------------------------------------------

void MetaMemory::MarkRange (emuptr start, emuptr end, uint8 v)
{
	// If there's no meta-memory (not needed for dedicated framebuffers)
	// just leave.

	if (EmMemGetBank(start).xlatemetaaddr == NULL)
		return;

	// If the beginning and end of the buffer are not in the same address
	// space, just leave.  This can happen while initializing the Dragonball's
	// LCD -- for a while, the LCD framebuffer range falls off the end
	// of the dynamic heap.

	if (start >= 0 && start < 0 + gRAMBank_Size
		&& end >= 0 + gRAMBank_Size)
	{
		end = gRAMBank_Size - 1;
	}

	if (start >= gMemoryStart && start < gMemoryStart + gRAMBank_Size
		&& end >= gMemoryStart + gRAMBank_Size)
	{
		end = gMemoryStart + gRAMBank_Size - 1;
	}

	uint8*	startP	= EmMemGetMetaAddress (start);
	uint8*	endP	= startP + (end - start);	// EmMemGetMetaAddress (end);
	uint8*	end4P	= (uint8*) (((uint32) endP) & ~3);
	uint8*	p		= startP;

	EmAssert (end >= start);
	EmAssert (endP >= startP);
	EmAssert (endP - startP == (ptrdiff_t) (end - start));

#if 1
	// Optimization: if there are no middle longs to fill, just
	// do everything a byte at a time.

	if (end - start < 12)
	{
		while (p < endP)
		{
			*p++ &= v;
		}
	}
	else
	{
		uint32	longValue = v;
		longValue |= (longValue << 8);
		longValue |= (longValue << 16);

		while (((uint32) p) & 3)		// while there are leading bytes
		{
			*p++ |= v;
		}

		while (p < end4P)				// while there are middle longs
		{
			*(uint32*) p |= longValue;
			p += sizeof (uint32);
		}

		while (p < endP)				// while there are trailing bytes
		{
			*p++ |= v;
		}
	}
#else
	for (p = startP; p < endP; ++p)
	{
		*p |= v;
	}
#endif
}


// ---------------------------------------------------------------------------
//		� MetaMemory::UnmarkRange
// ---------------------------------------------------------------------------

void MetaMemory::UnmarkRange (emuptr start, emuptr end, uint8 v)
{
	// If there's no meta-memory (not needed for dedicated framebuffers)
	// just leave.

	if (EmMemGetBank(start).xlatemetaaddr == NULL)
		return;

	// If the beginning and end of the buffer are not in the same address
	// space, just leave.  This can happen while initializing the Dragonball's
	// LCD -- for a while, the LCD framebuffer range falls off the end
	// of the dynamic heap.

	if (start >= 0 && start < 0 + gRAMBank_Size
		&& end >= 0 + gRAMBank_Size)
	{
		end = gRAMBank_Size - 1;
	}

	if (start >= gMemoryStart && start < gMemoryStart + gRAMBank_Size
		&& end >= gMemoryStart + gRAMBank_Size)
	{
		end = gMemoryStart + gRAMBank_Size - 1;
	}

	uint8*	startP	= EmMemGetMetaAddress (start);
	uint8*	endP	= startP + (end - start);	// EmMemGetMetaAddress (end);
	uint8*	end4P	= (uint8*) (((uint32) endP) & ~3);
	uint8*	p		= startP;

	EmAssert (end >= start);
	EmAssert (endP >= startP);
	EmAssert (endP - startP == (ptrdiff_t) (end - start));

	v = ~v;

#if 1
	// Optimization: if there are no middle longs to fill, just
	// do everything a byte at a time.

	if (end - start < 12)
	{
		while (p < endP)
		{
			*p++ &= v;
		}
	}
	else
	{
		uint32	longValue = v;
		longValue |= (longValue << 8);
		longValue |= (longValue << 16);

		while (((uint32) p) & 3)		// while there are leading bytes
		{
			*p++ &= v;
		}

		while (p < end4P)				// while there are middle longs
		{
			*(uint32*) p &= longValue;
			p += sizeof (uint32);
		}

		while (p < endP)				// while there are trailing bytes
		{
			*p++ &= v;
		}
	}
#else
	for (p = startP; p < endP; ++p)
	{
		*p &= v;
	}
#endif
}


// ---------------------------------------------------------------------------
//		� MetaMemory::MarkUnmarkRange
// ---------------------------------------------------------------------------

void MetaMemory::MarkUnmarkRange (emuptr start, emuptr end,
							uint8 andValue, uint8 orValue)
{
	// If there's no meta-memory (not needed for dedicated framebuffers)
	// just leave.

	if (EmMemGetBank(start).xlatemetaaddr == NULL)
		return;

	// If the beginning and end of the buffer are not in the same address
	// space, just leave.  This can happen while initializing the Dragonball's
	// LCD -- for a while, the LCD framebuffer range falls off the end
	// of the dynamic heap.

	if (start >= 0 && start < 0 + gRAMBank_Size
		&& end >= 0 + gRAMBank_Size)
	{
		end = gRAMBank_Size - 1;
	}

	if (start >= gMemoryStart && start < gMemoryStart + gRAMBank_Size
		&& end >= gMemoryStart + gRAMBank_Size)
	{
		end = gMemoryStart + gRAMBank_Size - 1;
	}

	uint8*	startP	= EmMemGetMetaAddress (start);
	uint8*	endP	= startP + (end - start);	// EmMemGetMetaAddress (end);
	uint8*	end4P	= (uint8*) (((uint32) endP) & ~3);
	uint8*	p		= startP;

	EmAssert (end >= start);
	EmAssert (endP >= startP);
	EmAssert (endP - startP == (ptrdiff_t) (end - start));

	if (andValue == 0xFF)
	{
		while (p < endP)
		{
			*p++ |= orValue;
		}
	}
	else if (orValue == 0x00)
	{
		while (p < endP)
		{
			*p++ &= andValue;
		}
	}
	else
	{
#if 1
		// Optimization: if there are no middle longs to fill, just
		// do everything a byte at a time.

		if (end - start < 12)
		{
			while (p < endP)				// while there are trailing bytes
			{
				*p = (*p & andValue) | orValue;
				p += sizeof (char);
			}
		}
		else
		{
			uint32	longAnd = andValue;
			longAnd |= (longAnd << 8);
			longAnd |= (longAnd << 16);

			uint32	longOr = orValue;
			longOr |= (longOr << 8);
			longOr |= (longOr << 16);

			while (((uint32) p) & 3)		// while there are leading bytes
			{
				*p = (*p & andValue) | orValue;
				p += sizeof (char);
			}

			while (p < end4P)				// while there are middle longs
			{
				*(uint32*) p = ((*(uint32*) p) & longAnd) | longOr;
				p += sizeof (long);
			}

			while (p < endP)				// while there are trailing bytes
			{
				*p = (*p & andValue) | orValue;
				p += sizeof (char);
			}
		}
#else
		for (p = startP; p < endP; ++p)
		{
			*p = (*p & andValue) | orValue;
		}
#endif
	}
}


// ---------------------------------------------------------------------------
//		� MetaMemory::WhatHappenedCallback
// ---------------------------------------------------------------------------
//	Check to see if there was an access to memory manager data, an unlocked
//	block, or a free block.

void MetaMemory::GWH_ExamineHeap (	const EmPalmHeap& heap,
									WhatHappenedData& info)
{
	// Bail out if the address to test is not even in this heap.

	if (!heap.Contains (info.address))
	{
		return;
	}

	// See if we touched a memory manager structure.
	// Let's start with the headers.

	// Is it in the heap header?

	if (heap.HeaderContains (info.address))
	{
		info.result = Errors::kMemMgrAccess;
		return;
	}

	// Is it in any of the master pointer tables?

	{
		ITERATE_MPTS(heap, iter, end)
		{
			if (iter->Contains (info.address))
			{
				info.result = Errors::kMemMgrAccess;
				return;
			}

			++iter;
		}
	}

	// Check against all of the memory manager chunks.

	{
		ITERATE_CHUNKS (heap, iter, end)
		{
			GWH_ExamineChunk (*iter, info);
			if (info.result != Errors::kUnknownAccess)
				return;

			++iter;
		}
	}
}


// ---------------------------------------------------------------------------
//		� MetaMemory::GWH_ExamineChunk
// ---------------------------------------------------------------------------
//	Check to see if there was an access to memory manager data, an unlocked
//	block, or a free block.

void MetaMemory::GWH_ExamineChunk (	const EmPalmChunk& chunk,
									WhatHappenedData& info)
{
	emuptr	addrStart	= info.address;
	emuptr	addrEnd		= info.address + info.size;

	// Sort out some ranges we'll be comparing against.

	emuptr	hdrStart	= chunk.HeaderStart ();
	emuptr	bodyStart	= chunk.BodyStart ();
	emuptr	trlStart	= chunk.BodyEnd ();
	emuptr	chunkEnd	= chunk.End ();

	/*
		A note on the comparison below (I had to draw this on the whiteboard
		before I could figure it out).  I need to see if one range of memory
		(the test address, extending for 1, 2, or 4 byte) overlaps another
		range of memory (the chunk header, the chunk body, etc.).  I started
		by drawing the second range as follows:

			+---------+---------+---------+
			|    A    |    B    |    C    |
			+---------+---------+---------+

		"B" is the range I'm interested in testing against, "A" is everything
		before it, and "C" is everything after it.

		The range of memory I want to test to see if it overlaps can be thought
		of as starting in region A, B, or C, and ending in A, B, or C.  We can
		identify all possible ranges by labelling each range "xy", where x is
		the starting section and y is the ending section.  Thus, all the possible
		ranges are AA, AB, AC, BB, BC, and CC.  From inspection, we can see that
		only segments AA and CC don't overlap B in any way.  Looking at it
		another way, any segment ending in A or starting in C will not overlap
		B.  All other ranges will.
	*/

	// See if we hit this chunk at all.  If not, leave.

	if (addrEnd > hdrStart && addrStart < chunkEnd)
	{
		// See if the access was to the body of the block.

		if (addrEnd > bodyStart && addrStart < trlStart)
		{
			// See if this is a free (unallocated) block.

			if (chunk.Free ())
			{
				info.result = Errors::kFreeChunkAccess;
			}

			// See if this is an unlocked block.

			else if (chunk.LockCount () == 0)
			{
				info.result = Errors::kUnlockedChunkAccess;
			}
		}

		// See if the access was to the chunk header.

		if (addrEnd > hdrStart && addrStart < bodyStart)
		{
			info.result = Errors::kMemMgrAccess;
		}

		// See if the access was to the chunk trailer.

		if (addrEnd > trlStart && addrStart < chunkEnd)
		{
			info.result = Errors::kMemMgrAccess;
		}


		// ============================== ALLOW FOR BUGS ==============================

		// Allow the screen driver some liberty.  Some BitBlt functions get a running
		// start or come to a skidding halt before or after an allocated chunk.  They
		// read these two bytes, but the logic is such that those bits get masked out
		// or shifted away.

		if (info.result == Errors::kMemMgrAccess &&
			info.forRead &&
		//	info.size == sizeof (UInt16) &&	// Can be 1-byte access in cases
			(::InPrvCompressedInnerBitBlt () ||
			 ::InPrvMisAlignedForwardInnerBitBlt () ||
			 ::InPrvMisAlignedBackwardInnerBitBlt ()))
		{
			goto HideBug;
		}


		if (Patches::HasConvertDepth1To2BWBug () &&
			info.result == Errors::kMemMgrAccess &&
			info.forRead &&
			info.size == sizeof (UInt8) &&
			::InPrvConvertDepth1To2BW ())
		{
			goto HideBug;
		}


		// Similarly, allow NetLibBitMove some liberty.  It will occassionally
		// scoot off the end of the source buffer, but making sure that it
		// masks off the data that it reads.

		if (info.result == Errors::kMemMgrAccess &&
			info.forRead &&
			info.size == sizeof (UInt8) &&
			::InNetLibBitMove ())
		{
			goto HideBug;
		}


		// There's a bug in MenuHandleEvent (pre-3.1 only) that causes it to read a
		// random long (actually, the random location is the result of using a -1 to index
		// into a menu array).

		if (Patches::HasMenuHandleEventBug () &&
			info.forRead &&
			info.size == sizeof (WinHandle) &&	// MenuPullDownType.menuWin
			::InMenuHandleEvent ())
		{
			goto HideBug;
		}


		// There's a bug in NetPrvSettingSet that causes it to read 0x020C bytes
		// from the beginning of its globals buffer when that buffer is only 8 bytes long.

		if (Patches::HasNetPrvSettingSetBug () &&
			info.forRead &&
			info.size == 4 &&
			::InNetPrvSettingSet ())
		{
			goto HideBug;
		}


		// SysAppExit deletes the block of memory containing the current task's stack
		// and TCB before the task has been deleted.  When SysTaskDelete is called,
		// there are many accesses to both the stack (as functions are called) and
		// the TCB (as tasks are scheduled and as the task to be deleted is finally
		// deleted).
		//
		// We need to detect when any of these SysTaskDelete accesses occurs and
		// allow them.  One hueristic that gets most of them is to see if there
		// are any TCBs with a pointer into the deleted memory chunk.  That will
		// work up until the point the TCB is marked as deleted.  This marking
		// occurs in the AMX function cj_kptkdelete.  At that point, we can just
		// check to see if the access occured in that function and allow it.

		if (info.result == Errors::kFreeChunkAccess &&
			Patches::HasDeletedStackBug ())
		{
			// First see if there is an active TCB with a stack pointer
			// pointing into this deleted memory chunk.

			SysKernelInfoType	taskInfo;

			taskInfo.selector	= sysKernelInfoSelTaskInfo;
			taskInfo.id			= 0;	// Ask for the first task

			// Get the first task.  Remember the task IDs so that we can
			// detect when we've looped through them all.  Remembering *all*
			// the IDs (instead of just the first one) is necessary in case
			// we're called with the linked list of TCBs is inconsistant
			// (that is, it's in the process of being updated -- when that's
			// happening, we may find a loop that doesn't necessarily involve
			// the first TCB).

			vector<UInt32>	taskIDList;
			Err				err = SysKernelInfo (&taskInfo);

			while (err == 0)
			{
				// See if we've seen this task ID before.  If so, leave.

				vector<UInt32>::iterator	iter = taskIDList.begin ();
				while (iter != taskIDList.end ())
				{
					if (*iter++ == taskInfo.param.task.id)
					{
						goto PlanB;	// I'd really like a break(2) here... :-)
					}
				}

				// We haven't looked at this task before; check the stack.
				// Check against "stackStart" instead of stackP, as the
				// latter sometimes contains values outside of the current
				// stack range (such as the faux stacks we set up when
				// doing ATrap calls).

				if (taskInfo.param.task.stackStart >= bodyStart &&
					taskInfo.param.task.stackStart < trlStart)
				{
					goto HideBug;
				}

				// Save this task ID so we can see if we've visited this
				// record before.

				taskIDList.push_back (taskInfo.param.task.id);

				// Get the next TCB.

				taskInfo.selector	= sysKernelInfoSelTaskInfo;
				taskInfo.id			= taskInfo.param.task.nextID;
				err					= SysKernelInfo (&taskInfo);
			}

			// Plan B...
PlanB:
			// If there is no TCB pointing into the deleted chunk, see if the current
			// function is cj_kptkdelete.  If not, see if it looks like we're in a
			// function called by cj_kptkdelete.

			if (	m68k_areg (regs, 7) >= bodyStart &&
					m68k_areg (regs, 7) < trlStart)
			{
				// See if we're currently in cj_kptkdelete.

				if (::Incj_kptkdelete ())
				{
					info.result = Errors::kOKAccess;
				}

				// If not, see if it's in the current call chain.  Normally (!)
				// I'd walk the a6 stack frame links, but there's a lot of
				// assembly stuff going on here, and not all of them set up
				// stack frames.  a6 *should* point to a valid stack frame
				// somewhere up the line, but it may be cj_kptkdelete's
				// stack frame, resulting in our skipping over it.
				//
				// It's important that gcj_kptkdelete.InRange() has returned
				// true at least once before entering this section of code.
				// That's because if it hasn't, the cached range of the
				// function will be empty, causing InRange to look for the
				// beginning and ending of the function containing the address
				// passed to it.  Since we're passing random contents of the
				// stack, it's very unlikely that there is a function range
				// corresponding to the values passed to InRange, sending it
				// on wild goose chases.  Fortunately, "Plan B" goes into effect
				// only after the TCB has been marked as deleted, which occurs
				// in cj_kptkdelete.  The very next thing cj_kptkdelete does is
				// push a parameter on the stack, causing us to get here while
				// the PC is in cj_kptkdelete.  This will cause the call to InRange
				// immediately above to return true, satisfying the precondition
				// we need here.

				else
				{
					// Get the current stack pointer and use it to iterate
					// over the contents of the stack.  We examine every
					// longword on every even boundary to see if it looks
					// like a return address into cj_kptkdelete.

					emuptr	a7 = m68k_areg (regs, 7);

					while (a7 >= bodyStart && a7 < trlStart)
					{
						if (::Incj_kptkdelete (EmMemGet32 (a7)))
						{
							goto HideBug;
						}

						a7 += 2;
					}
				}	// end: checking the stack for cj_kptkdelete
			}	// end: A7 is in the deleted chunk
		}	// end: accessed a deleted chunk


		// There's a bug in FindShowResults (pre-3.0) that accesses
		// a byte in an unlocked block.

		if (info.result == Errors::kUnlockedChunkAccess &&
			Patches::HasFindShowResultsBug () &&
			info.forRead &&
			info.size == sizeof (Boolean) &&
			addrStart == bodyStart + sizeof (UInt16) * 2 /*offsetof (FindParamsType, more)*/ &&
			::InFindShowResults ())
		{
			goto HideBug;
		}


		// There's a bug in FindSaveFindStr that causes it to possibly read
		// off the end of the source handle.  The invalid access will be
		// generated in MemMove, which is called by DmWrite, which is called
		// by FindSaveFindStr.  So look up the stack a bit to see who's calling us.

		emuptr	a6_0 = m68k_areg (regs, 6);	// MemMove's A6 (points to caller's A6 and return address to caller)
		emuptr	a6_1 = EmMemGet32 (a6_0);		// DmWrite's (points to caller's A6 and return address to caller)
		
		if (Patches::HasFindSaveFindStrBug () &&
			info.forRead &&
			::InMemMove () &&							// See if we're in MemMove
			::InDmWrite (EmMemGet32 (a6_0 + 4)) &&		// See if DmWrite is MemMove's caller
			::InFindSaveFindStr (EmMemGet32 (a6_1 + 4)))	// See if FindSaveFindStr is DmWrite's caller
		{
			goto HideBug;
		}

		// There's a bug in FntDefineFont that causes it to possibly read
		// off the end of the source handle.  The invalid access will be
		// generated in MemMove, which is called by FntDefineFont.  So look
		// up the stack a bit to see who's calling us.

		if (Patches::HasFntDefineFontBug () &&
			info.forRead &&
			::InMemMove () &&							// See if we're in MemMove
			::InFntDefineFont (EmMemGet32 (a6_0 + 4)))	// See if FntDefineFont is MemMove's caller
		{
			goto HideBug;
		}
	}

	return;

HideBug:
	info.result = Errors::kOKAccess;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		� CheckUIObjectAccess
// ---------------------------------------------------------------------------

#define WindowFlagsType_format			0x8000	// window format:  0=screen mode; 1=generic mode
#define WindowFlagsType_offscreen		0x4000	// offscreen flag: 0=onscreen ; 1=offscreen
#define WindowFlagsType_modal			0x2000	// modal flag:     0=modeless window; 1=modal window
#define WindowFlagsType_focusable		0x1000	// focusable flag: 0=non-focusable; 1=focusable
#define WindowFlagsType_enabled			0x0800	// enabled flag:   0=disabled; 1=enabled
#define WindowFlagsType_visible			0x0400	// visible flag:   0-invisible; 1=visible
#define WindowFlagsType_dialog			0x0200	// dialog flag:    0=non-dialog; 1=dialog
#define WindowFlagsType_freeBitmap		0x0100	// free bitmap w/window: 0=don't free, 1=free

#define FormAttrType_usable				0x8000	// Set if part of ui 
#define FormAttrType_enabled			0x4000	// Set if interactable (not grayed out)
#define FormAttrType_visible			0x2000	// Set if drawn, used internally
#define FormAttrType_dirty				0x1000	// Set if dialog has been modified
#define FormAttrType_saveBehind			0x0800	// Set if bits behind form are save when form ids drawn
#define FormAttrType_graffitiShift		0x0400	// Set if graffiti shift indicator is supported
#define FormAttrType_globalsAvailable	0x0200	// Set by Palm OS if globals are available for the form event handler
#define FormAttrType_doingDialog		0x0100	// FrmDoDialog is using for nested event loop
#define FormAttrType_exitDialog			0x0080	// tells FrmDoDialog to bail out and stop using this form

#define ControlAttrType_usable			0x8000	// set if part of ui 
#define ControlAttrType_enabled			0x4000	// set if interactable (not grayed out)
#define ControlAttrType_visible			0x2000	// set if drawn (set internally)
#define ControlAttrType_on				0x1000	// set if on (checked)
#define ControlAttrType_leftAnchor		0x0800	// set if bounds expand to the right, clear if bounds expand to the left
#define ControlAttrType_frame			0x0700
#define ControlAttrType_drawnAsSelected	0x0080	// support for old-style graphic controls where control overlaps a bitmap
#define ControlAttrType_graphical		0x0040	// set if images are used instead of text
#define ControlAttrType_vertical		0x0020	// true for vertical sliders

// Here are some symbols to search for when looking for more things
// to which to prohibit access:
//
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_CLIPBOARDS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_CONTROLS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_FIELDS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_FINDPARAMS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_FORMS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_LISTS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_MENUS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_PROGRESS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_SCROLLBARS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_TABLES
//	
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_FONTS
//	#define ALLOW_ACCESS_TO_INTERNALS_OF_WINDOWS


// ---------------------------------------------------------------------------
//		� PrvTrapsAvailable
// ---------------------------------------------------------------------------

static Bool PrvTrapsAvailable (void)
{
	// If the OS is 4.0 or later, the API should be used.

	Bool	result = Patches::OSMajorVersion () >= 4;

	return result;
}


// ---------------------------------------------------------------------------
//		� PrvAccessorTrapAvailable
// ---------------------------------------------------------------------------

static Bool PrvAccessorTrapAvailable (void)
{
	// Magic Accessor Trap is available.  Defined but not implemented
	// in 4.0, but magically implemented in the Timulator.

	#ifndef sysFtrNumAccessorTrapPresent
	#define	sysFtrNumAccessorTrapPresent	25	// If accessor trap exists (PalmOS 4.0)
	#endif

	UInt32	value;
	Bool	result = (FtrGet (sysFtrCreator, sysFtrNumAccessorTrapPresent, &value) == errNone) && (value != 0);

	return result;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedFieldObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedFieldObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		FieldType
			UInt16					id;					// FrmGetObjectId
			RectangleType			rect;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition, FldGetBounds, FldSetBounds
			FieldAttrType			attr;				// FldGetAttributes, FldSetAttributes
										usable			// FrmShowObject, FrmHideObject, FldSetUsable
										visible
										editable
										singleLine
										hasFocus
										dynamicSize
										insPtVisible
										dirty			// FldSetDirty
										underlined
										justification
										autoShift
										hasScrollBar
										numeric
			Char*					text;				// FldGetTextPtr, FldSetTextPtr, FldSetText
			MemHandle				textHandle;			// FldGetTextHandle, FldSetTextHandle, FldSetText
			LineInfoPtr				lines;				// (FldGetScrollPosition, FldSetScrollPosition, FldGetTextHeight, FldGetVisibleLines)
			UInt16					textLen;			// FldGetTextLength, FldSetText
			UInt16					textBlockSize;		// FldGetTextAllocatedSize, FldSetTextAllocatedSize
			UInt16					maxChars;			// FldGetMaxChars, FldSetMaxChars
			UInt16					selFirstPos;		// FldGetSelection, FldSetSelection
			UInt16					selLastPos;			// FldGetSelection, FldSetSelection
			UInt16					insPtXPos;			// FldGetInsPtPosition, FldSetInsPtPosition, FldSetInsertionPoint
			UInt16					insPtYPos;			// FldGetInsPtPosition, FldSetInsPtPosition, FldSetInsertionPoint
			FontID					fontID;				// FldGetFont, FldSetFont
			UInt8					maxVisibleLines;	// FldSetMaxVisibleLines
	*/

	// Allow read access to "lines" if Emu68KAccessorTrapAvailable / AccFldGetLineInfo
	// not available

	if (forRead)
	{
		size_t			offset			= address - objectP;
		const size_t	lines_offset	= EmAliasFieldType<PAS>::offsetof_lines ();
		const size_t	lines_size		= EmAliasemuptr<PAS>::GetSize ();

		if (offset >= lines_offset && offset < lines_offset + lines_size)
		{
			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read/write access to "attr" before Palm OS 3.3.  Catherine
	// says there's a bug before those versions they need to workaround

//	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasFieldType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasFieldAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (Patches::OSMajorMinorVersion () < 33)
			{
				return true;
			}
		}
	}

	// Don't allow access to any other fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedControlObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedControlObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		ControlType
			UInt16					id;					// FrmGetObjectId
			RectangleType			bounds;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
			Char*					text;
			ControlAttrType			attr;
										usable			// FrmShowObject, FrmHideObject, CtlSetUsable
										enabled
										visible
										on				// FrmGetControlValue, CtlGetValue, FrmSetControlValue, CtlSetValue
										leftAnchor
										frame
										drawnAsSelected
										graphical
										vertical
										reserved
			ControlStyleType		style;
			FontID					font;
			UInt8					group;				// FrmGetControlGroupSelection
			UInt8 					reserved;

		GraphicControlType
			UInt16					id;					// FrmGetObjectId
			RectangleType			bounds;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
			DmResID					bitmapID;
			DmResID					selectedBitmapID;
			ControlAttrType			attr;
			ControlStyleType		style;
			FontID					unused;
			UInt8					group;
			UInt8 					reserved;

		SliderControlType
			UInt16					id;					// FrmGetObjectId
			RectangleType			bounds;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
			DmResID					thumbID;
			DmResID					backgroundID;
			ControlAttrType			attr;
			ControlStyleType		style;
			UInt8					reserved;		
			Int16					minValue;
			Int16					maxValue;
			Int16					pageSize;
			Int16					value;				// FrmGetControlValue, CtlGetValue, FrmSetControlValue, CtlSetValue
			MemPtr					activeSliderP;
	*/
	
	// Allow read access to "attr" and "style" if Emu68KAccessorTrapAvailable / AccFrmGetObjectUsable
	// not available

	if (forRead)
	{
		size_t			offset		= address - objectP;

		const size_t	attr_offset	= EmAliasControlType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasControlAttrType<PAS>::GetSize ();

		const size_t	style_offset= EmAliasControlType<PAS>::offsetof_style ();
		const size_t	style_size	= EmAliasUInt8<PAS>::GetSize ();

		if ((offset >= attr_offset && offset < attr_offset + attr_size) ||
			(offset >= style_offset && offset < style_offset + style_size))
		{
			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read/write access to "font" field.  Developers may need
	// to set the font to something other than what Constructor allows
	// or knows about.
	
	// Actually, Roger says that developers should call FrmRemoveObject
	// and FooNewFoo to create an object with the right font.

	// But, on the third hand, FooNewFoo can run into the
	// PrvFixupPoints bug.

	{
		size_t			offset		= address - objectP;

		const size_t	font_offset	= EmAliasControlType<PAS>::offsetof_font ();
		const size_t	font_size	= EmAliasFontID<PAS>::GetSize ();

		if ((offset >= font_offset && offset < font_offset + font_size))
		{
			return true;
		}
	}

	// Don't allow access to any other fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedListObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedListObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (forRead)

	/*
		ListType
			UInt16					id;					// FrmGetObjectId
			RectangleType			bounds;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition, LstSetHeight, LstSetPosition
			ListAttrType			attr;				// FrmShowObject, FrmHideObject
										usable			// FrmShowObject, FrmHideObject
										enabled			// (not used)
										visible			// LstDrawList, LstEraseList
										poppedUp		// LstPopupList
										hasScrollBar	// (Tested, but what sets it?)
										search			// (Tested, but what sets it?)
			Char**					itemsText;			// LstGetSelectionText, LstSetListChoices
			Int16					numItems;			// LstGetNumberOfItems
			Int16					currentItem;		// LstGetSelection, LstSetSelection
			Int16					topItem;			// LstGetTopItem, LstSetTopItem
			FontID					font;				// ?
			UInt8 					reserved;			//
			WinHandle				popupWin;			// (Used internally)
			ListDrawDataFuncPtr		drawItemsCallback;	// LstSetDrawFunction
	*/
	
	// Allow read access to "attr" if Emu68KAccessorTrapAvailable / AccFrmGetObjectUsable
	// not available

	// Actually, always allow full read/write access to attr.  Developers have a need
	// to access the "search" attribute, but there's no API for it.

//	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasListType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasListAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
//			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read access to topItem before 4.0.  In 4.0 and later, use LstGetTopItem.

	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	topItem_offset	= EmAliasListType<PAS>::offsetof_topItem ();
		const size_t	topItem_size	= EmAliasUInt16<PAS>::GetSize ();

		if (offset >= topItem_offset && offset < topItem_offset + topItem_size)
		{
			if (!::PrvTrapsAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read/write access to "font" field.  Developers may need
	// to set the font to something other than what Constructor allows
	// or knows about.

	// Actually, Roger says that developers should call FrmRemoveObject
	// and FooNewFoo to create an object with the right font.

	// But, on the third hand, FooNewFoo can run into the
	// PrvFixupPoints bug.

	{
		size_t			offset		= address - objectP;

		const size_t	font_offset	= EmAliasListType<PAS>::offsetof_font ();
		const size_t	font_size	= EmAliasFontID<PAS>::GetSize ();

		if ((offset >= font_offset && offset < font_offset + font_size))
		{
			return true;
		}
	}

	// Don't allow access to any other fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedTableObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedTableObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		TableType
			UInt16					id;
			RectangleType			bounds;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition, TblGetBounds, TblSetBounds
			TableAttrType			attr;
										visible			// TblEraseTable
										editable
										editing			// TblEditing, TblGrabFocus
										selected		// TblEraseTable
										hasScrollBar	// TblHasScrollBar(sets)
										usable			// FrmShowObject, FrmHideObject [Added later]
			Int16					numColumns;			// TblGetNumberOfColumns [4.0]
			Int16					numRows;			// TblGetNumberOfRows
			Int16					currentRow;			// TblGetSelection, TblSelectItem, TblGrabFocus, TblSetSelection [4.0]
			Int16					currentColumn;		// TblGetSelection, TblSelectItem, TblGrabFocus, TblSetSelection [4.0]
			Int16					topRow;				// TblGetTopRow [4.0]
			TableColumnAttrType*	columnAttrs;		// 
										width;					// TblGetColumnWidth, TblSetColumnWidth
										reserved1 		: 5;
										masked			: 1;	// TblSetColumnMasked
										editIndicator	: 1;	// TblSetColumnEditIndicator
										usable 			: 1;	// TblSetColumnUsable
										reserved2		: 8;
										spacing;				// TblGetColumnSpacing, TblSetColumnSpacing
										drawCallback;			// TblSetCustomDrawProcedure
										loadDataCallback;		// TblSetLoadDataProcedure
										saveDataCallback;		// TblSetSaveDataProcedure
			TableRowAttrType*		rowAttrs;			// 
										id;						// TblFindRowID, TblGetRowID, TblSetRowID
										height;					// TblGetRowHeight, TblSetRowHeight
										data;					// TblFindRowData, TblGetRowData, TblSetRowData
										reserved1		: 7;
										usable			: 1;	// TblRowUsable, TblSetRowUsable
										reserved2		: 4;
										masked			: 1;	// TblRowMasked, TblSetRowMasked
										invalid			: 1;	// TblRowInvalid, TblMarkRowInvalid, TblMarkTableInvalid
										staticHeight	: 1;	// TblSetRowStaticHeight
										selectable		: 1;	// TblRowSelectable, TblSetRowSelectable
										reserved3;
			TableItemPtr			items;				// 
										itemType;				// TblSetItemStyle
										fontID;					// TblGetItemFont, TblSetItemFont
										intValue;				// TblGetItemInt, TblSetItemInt
										ptr;					// TblGetItemPtr, TblSetItemPtr
			FieldType				currentField;		// TblGetCurrentField
	*/

	// TblGlueGetNumberOfColumns
	//	Give read access to numColumns before 4.0

	// TblGlueGetTopRow
	//	Give read access to topRow before 4.0

	// TblGlueSetSelection
	//	Give read access to numColumns before 4.0
	//	Give read access to numRows before 4.0
	//	Give r/w access to attr
	//		* need r/w access to "selected" if !TrapsAvailable because of code in TblGlueSetSelection
	//		* need r/w access to "usable" if TrapsAvailable (actually, if Palm OS > 4.0 for now)
	//	Give read access to rowAttrs before 4.0
	//	Give r/w access to currentRow before 4.0
	//	Give r/w access to currentColumn before 4.0

	if (forRead)
	{
		size_t			offset				= address - objectP;
		const size_t	numRows_offset		= EmAliasTableType<PAS>::offsetof_numRows ();
		const size_t	numRows_size		= EmAliasUInt16<PAS>::GetSize ();

		const size_t	numColumns_offset	= EmAliasTableType<PAS>::offsetof_numColumns ();
		const size_t	numColumns_size		= EmAliasUInt16<PAS>::GetSize ();

		const size_t	topRow_offset		= EmAliasTableType<PAS>::offsetof_topRow ();
		const size_t	topRow_size			= EmAliasUInt16<PAS>::GetSize ();

		if ((offset >= numRows_offset && offset < numRows_offset + numRows_size) ||
			(offset >= numColumns_offset && offset < numColumns_offset + numColumns_size) ||
			(offset >= topRow_offset && offset < topRow_offset + topRow_size))
		{
			if (!::PrvTrapsAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read access to these fields so that applications can get
	// to sub-fields that have no APIs.

	if (forRead)
	{
		size_t			offset				= address - objectP;

		const size_t	items_offset		= EmAliasTableType<PAS>::offsetof_items ();
		const size_t	items_size			= EmAliasemuptr<PAS>::GetSize ();

		const size_t	rowAttrs_offset		= EmAliasTableType<PAS>::offsetof_rowAttrs ();
		const size_t	rowAttrs_size		= EmAliasemuptr<PAS>::GetSize ();

		const size_t	columnAttrs_offset	= EmAliasTableType<PAS>::offsetof_columnAttrs ();
		const size_t	columnAttrs_size	= EmAliasemuptr<PAS>::GetSize ();

		if ((offset >= items_offset && offset < items_offset + items_size) ||
			(offset >= rowAttrs_offset && offset < rowAttrs_offset + rowAttrs_size) ||
			(offset >= columnAttrs_offset && offset < columnAttrs_offset + columnAttrs_size))
		{
			return true;
		}
	}

	{
		size_t			offset				= address - objectP;

		const size_t	currentRow_offset	= EmAliasTableType<PAS>::offsetof_currentRow ();
		const size_t	currentRow_size		= EmAliasUInt16<PAS>::GetSize ();

		const size_t	currentColumn_offset= EmAliasTableType<PAS>::offsetof_currentColumn ();
		const size_t	currentColumn_size	= EmAliasUInt16<PAS>::GetSize ();

		if ((offset >= currentRow_offset && offset < currentRow_offset + currentRow_size) ||
			(offset >= currentColumn_offset && offset < currentColumn_offset + currentColumn_size))
		{
			if (!::PrvTrapsAvailable ())
			{
				return true;
			}
		}
	}

	{
		size_t			offset				= address - objectP;
		const size_t	attr_offset			= EmAliasTableType<PAS>::offsetof_attr ();
		const size_t	attr_size			= EmAliasTableAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			return true;
		}
	}

	// Chain to the function that checks FieldType field access.

	return PrvAllowedFieldObjectAccess (
		objectP + EmAliasTableType<PAS>::offsetof_currentField (),
		address, forRead);
}


// ---------------------------------------------------------------------------
//		� PrvAllowedBitmapObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedBitmapObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		FormBitmapType
			FormObjAttrType			attr;
										usable			// FrmShowObject, FrmHideObject
			PointType				pos;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
			UInt16					rscID;				// FrmGetObjectId
	*/

	// Allow read access to "attr" if Emu68KAccessorTrapAvailable / AccFrmGetObjectUsable
	// not available

	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasFormBitmapType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasFormObjAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read/write access to "attr" before 3.2 -- FrmHideObject didn't
	// change the usable bit before then.

	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasFormBitmapType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasFormObjAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (Patches::OSMajorMinorVersion () < 32)
			{
				return true;
			}
		}
	}


	// Don't allow access to any other fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedLineObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedLineObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (objectP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FormLineType
			FormObjAttrType			attr;
			PointType				point1;
			PointType				point2;
	*/

	// No access to any fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedFrameObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedFrameObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (objectP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FormFrameType
			UInt16					id;
			FormObjAttrType			attr;
			RectangleType			rect;
			UInt16					frameType;
	*/

	// No access to any fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedRectangleObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedRectangleObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (objectP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FormRectangleType
			FormObjAttrType			attr;
			RectangleType			rect;
	*/
	
	// No access to any fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedLabelObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedLabelObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		FormLabelType
			UInt16					id;					// FrmGetObjectId
			PointType				pos;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
			FormObjAttrType			attr;
										usable			// FrmShowObject, FrmHideObject
			FontID					fontID;				//
			UInt8					reserved;			//
			Char*					text;				// FrmGetLabel, FrmCopyLabel
	*/

	// Allow read access to "attr" if Emu68KAccessorTrapAvailable / AccFrmGetObjectUsable
	// not available

	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasFormLabelType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasFormObjAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow read/write access to "font" field.  Developers may need
	// to set the font to something other than what Constructor allows
	// or knows about.

	// Actually, Roger says that developers should call FrmRemoveObject
	// and FooNewFoo to create an object with the right font.
	
	// But, on the third hand, FooNewFoo can run into the
	// PrvFixupPoints bug.

	{
		size_t			offset		= address - objectP;

		const size_t	font_offset	= EmAliasFormLabelType<PAS>::offsetof_fontID ();
		const size_t	font_size	= EmAliasFontID<PAS>::GetSize ();

		if ((offset >= font_offset && offset < font_offset + font_size))
		{
			return true;
		}
	}

	// Don't allow access to any other fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedTitleObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedTitleObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (objectP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FormTitleType
			RectangleType			rect;				// FrmSetObjectBounds
			Char*					text;				// FrmGetTitle, FrmSetTitle, FrmCopyTitle
	*/

	// No access to any fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedPopupObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedPopupObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (objectP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FormPopupType
			UInt16					controlID;			//
			UInt16					listID;				//
	*/

	// No access to any fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedGraffitiStateObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedGraffitiStateObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (objectP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FrmGraffitiStateType
			PointType				pos;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
	*/

	// No access to any fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedGadgetObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedGadgetObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		FormGadgetType
			UInt16					id;					// FrmGetObjectId
			FormGadgetAttrType		attr;
										usable			// FrmShowObject, FrmHideObject
										extended		//
										visible			//
			RectangleType			rect;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, FrmSetObjectPosition
			const void*				data;				// FrmGetGadgetData, FrmSetGadgetData
			FormGadgetHandlerType*	handler;			// FrmSetGadgetHandler
	*/

#if 1
	UNUSED_PARAM (objectP);
	UNUSED_PARAM (address);
	UNUSED_PARAM (forRead);

	// Return "true" for all gadget fields.  Gadget callback functions
	// provide just a pointer to the gadget, with no reference to the
	// form through which to make access calls.  So the only way to
	// access fields is directly.

	return true;

#else
	// Allow read access to "attr" if Emu68KAccessorTrapAvailable / AccFrmGetObjectUsable
	// not available.

	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasFormGadgetType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasFormGadgetAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow write access to "attr" before 3.5, as that version added support for
	// showing and hiding gadgets.  And write-access implies read-access.

//	if (!forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasFormGadgetType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasFormGadgetAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (Patches::OSMajorMinorVersion () < 35)
			{
				return true;
			}
		}
	}

	// Don't allow access to any other fields.

	return false;
#endif
}


// ---------------------------------------------------------------------------
//		� PrvAllowedScrollBarObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedScrollBarObjectAccess (emuptr objectP, emuptr address, Bool forRead)
{
	/*
		ScrollBarType
			RectangleType			bounds;				// FrmGetObjectBounds, FrmGetObjectPosition, FrmSetObjectBounds, , FrmSetObjectPosition
			UInt16					id;					// FrmGetObjectId
			ScrollBarAttrType		attr;
										usable			// FrmShowObject, FrmHideObject
										visible			//
										hilighted		//
										shown			//
										activeRegion	//
			Int16					value;				// SclGetScrollBar, SclSetScrollBar
			Int16					minValue;			// SclGetScrollBar, SclSetScrollBar
			Int16					maxValue;			// SclGetScrollBar, SclSetScrollBar
			Int16					pageSize;			// SclGetScrollBar, SclSetScrollBar
			Int16					penPosInCar;		//
			Int16					savePos;			//
	*/

	// Allow read access to "attr" if Emu68KAccessorTrapAvailable / AccFrmGetObjectUsable
	// not available

	if (forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasScrollBarType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasScrollBarAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (!::PrvAccessorTrapAvailable ())
			{
				return true;
			}
		}
	}

	// Allow write access to "attr" before 3.5, as that version added support for
	// showing and hiding scrollbars.  And write-access implies read-access.

//	if (!forRead)
	{
		size_t			offset		= address - objectP;
		const size_t	attr_offset	= EmAliasScrollBarType<PAS>::offsetof_attr ();
		const size_t	attr_size	= EmAliasScrollBarAttrType<PAS>::GetSize ();

		if (offset >= attr_offset && offset < attr_offset + attr_size)
		{
			if (Patches::OSMajorMinorVersion () < 35)
			{
				return true;
			}
		}
	}

	// Don't allow access to any other fields.

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedFormObjectAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form object is allowed
// on the current platform.

static Bool PrvAllowedFormObjectAccess (emuptr thisObject, emuptr address, Bool forRead)
{
	EmAliasFormObjListType<PAS>	object (thisObject);
	FormObjectKind				kind = object.objectType;

	Bool (*check_function) (emuptr, emuptr, Bool) = NULL;

	switch (kind)
	{
		case frmFieldObj:			check_function = PrvAllowedFieldObjectAccess;			break;
		case frmControlObj:			check_function = PrvAllowedControlObjectAccess;			break;
		case frmListObj:			check_function = PrvAllowedListObjectAccess;			break;
		case frmTableObj:			check_function = PrvAllowedTableObjectAccess;			break;
		case frmBitmapObj:			check_function = PrvAllowedBitmapObjectAccess;			break;
		case frmLineObj:			check_function = PrvAllowedLineObjectAccess;			break;
		case frmFrameObj:			check_function = PrvAllowedFrameObjectAccess;			break;
		case frmRectangleObj:		check_function = PrvAllowedRectangleObjectAccess;		break;
		case frmLabelObj:			check_function = PrvAllowedLabelObjectAccess;			break;
		case frmTitleObj:			check_function = PrvAllowedTitleObjectAccess;			break;
		case frmPopupObj:			check_function = PrvAllowedPopupObjectAccess;			break;
		case frmGraffitiStateObj:	check_function = PrvAllowedGraffitiStateObjectAccess;	break;
		case frmGadgetObj:			check_function = PrvAllowedGadgetObjectAccess;			break;
		case frmScrollBarObj:		check_function = PrvAllowedScrollBarObjectAccess;		break;
	}

	if (check_function)
	{
		return check_function (object.object, address, forRead);
	}

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedWindowAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given window is allowed on
// the current platform.

static Bool PrvAllowedWindowAccess (emuptr windowP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (forRead)

	size_t					offset = address - windowP;
	EmAliasWindowType<PAS>	window (windowP);

#undef ACCESSED
#define ACCESSED(fieldName)	\
	((offset >= window.offsetof_##fieldName ()) &&	\
	 (offset < window.offsetof_##fieldName () + window.fieldName.GetSize ()))

	// Allow access to the following fields before 2.0:
	//
	//		displayWidthV20
	//		displayHeightV20

	if (forRead && Patches::OSMajorMinorVersion () < 20)
	{
		if (ACCESSED (displayWidthV20) ||
			ACCESSED (displayHeightV20))
		{
			return true;
		}
	}

	// Allow access to the following fields before 3.5:
	//
	//		displayAddrV20
	//		bitmapP

	if (forRead && Patches::OSMajorMinorVersion () < 35)
	{
		if (ACCESSED (displayAddrV20) ||
			ACCESSED (bitmapP))
		{
			return true;
		}
	}

	// Allow access to the following fields.

	{
		if (ACCESSED (frameType))
		{
			return true;
		}
	}

	return false;
}


// ---------------------------------------------------------------------------
//		� PrvAllowedFormAccess
// ---------------------------------------------------------------------------
// Return whether or not the given access to the given form is allowed on the
// current platform.

static Bool PrvAllowedFormAccess (emuptr formP, emuptr address, Bool forRead)
{
	UNUSED_PARAM (formP)
	UNUSED_PARAM (address)
	UNUSED_PARAM (forRead)

	/*
		FormType
			WindowType				window;
			UInt16					formId;					// FrmGetFormId
			FormAttrType			attr;
										usable				// 
										enabled				//
										visible				// FrmVisible
										dirty				// FrmGetUserModifiedState, FrmSetNotUserModified
										saveBehind			//
										graffitiShift		//
										globalsAvailable	//
										doingDialog			//
										exitDialog			//
			WinHandle	       		bitsBehindForm;			//
			FormEventHandlerType*	handler;				// FrmSetEventHandler
			UInt16					focus;					// FrmGetFocus, FrmSetFocus
			UInt16					defaultButton;			//
			UInt16					helpRscId;				//
			UInt16					menuRscId;				// FrmSetMenu
			UInt16					numObjects;				// FrmGetNumberOfObjects
			FormObjListType*		objects;				// FrmGetObjectPtr, FrmGetObjectType
	*/

	// Allow read access to "handler".  Some well-meaning application may
	// try to chain to it if they change it.  Or they may want to restore
	// when they're done with their own hook.

	if (forRead)
	{
		size_t			offset			= address - formP;
		const size_t	handler_offset	= EmAliasFormType<PAS>::offsetof_handler ();
		const size_t	handler_size	= EmAliasemuptr<PAS>::GetSize ();

		if (offset >= handler_offset && offset < handler_offset + handler_size)
		{
			return true;
		}
	}

	// Allow full access to defaultButton, menuRscId, and helpRscId.  There's no API
	// for them, and developers do access them.

	{
		size_t			offset					= address - formP;
		const size_t	defaultButton_offset	= EmAliasFormType<PAS>::offsetof_defaultButton ();
		const size_t	defaultButton_size		= EmAliasUInt16<PAS>::GetSize ();
		const size_t	helpRscId_offset		= EmAliasFormType<PAS>::offsetof_helpRscId ();
		const size_t	helpRscId_size			= EmAliasUInt16<PAS>::GetSize ();
		const size_t	menuRscId_offset		= EmAliasFormType<PAS>::offsetof_menuRscId ();
		const size_t	menuRscId_size			= EmAliasUInt16<PAS>::GetSize ();

		if ((offset >= defaultButton_offset && offset < defaultButton_offset + defaultButton_size) ||
			(offset >= helpRscId_offset && offset < helpRscId_offset + helpRscId_size) ||
			(offset >= menuRscId_offset && offset < menuRscId_offset + menuRscId_size))
		{
			return true;
		}
	}

	// Chain to the function that checks WindowType field access.

	return PrvAllowedWindowAccess (formP, address, forRead);
}


// ---------------------------------------------------------------------------
//		� CheckUIObjectAccess
// ---------------------------------------------------------------------------
// Check to see if the given access is in a proscribed area of memory.  Will
// first check to see if:
//
//	* this check is turned off
//	* the system is initialized enough to make this check
//	* the access is made from a RAM-based system component
//	* full memory access is allowed
//
// Only if checking seems indicated is the full-winded check made.  If
// a proscribed access is indeed made, a "deferred error" object is posted
// to the session.

void MetaMemory::CheckUIObjectAccess (emuptr address, size_t size, Bool forRead,
									  Bool& isInUIObject, Bool& butItsOK)
{
	isInUIObject	= false;
	butItsOK		= true;

	// If the UI is not initialized for this application, the FirstWindow
	// pointer in uiGlobals will be non-NULL, but invalid (it will point
	// to the FirstWindow for the previous application, which has been
	// disposed of by now).  Wait until UIReset has been called.

	if (!Patches::UIReset ())
		return;

	// Walk the window list, looking for windows and forms

	emuptr	w = EmLowMem_GetGlobal (uiGlobals.firstWindow);
	while (w)
	{
		// See if this window is a plain window, or a dialog.

		EmAliasWindowType<PAS>	window (w);
		uint16	flags = window.windowFlags.flags;

		if (flags & WindowFlagsType_dialog)
		{
			// It's a dialog.

			EmAliasFormType<PAS>	dialog (w);

			// Get the number of objects in the dialog.  Use this value to determine
			// the range of memory the dialog spans.

			uint16	numObjects	= dialog.numObjects;
			emuptr	firstObject	= dialog.objects;
			emuptr	lastObject	= firstObject + numObjects * EmAliasFormObjListType<PAS>::GetSize ();

			// If the access was to the dialog, flag an error.

			if (address >= w && address < w + EmAliasFormType<PAS>::GetSize ())
			{
				isInUIObject = true;

				if (!::PrvAllowedFormAccess (w, address, forRead))
				{
					gSession->ScheduleDeferredError (new EmDeferredErrFormAccess (w, address, size, forRead));
					butItsOK = false;
				}

				return;
			}

			// If the access was to the dialog item list, flag an error.

			if (address >= firstObject && address < lastObject)
			{
				isInUIObject = true;

				if (!::PrvAllowedFormAccess (w, address, forRead))
				{
					gSession->ScheduleDeferredError (new EmDeferredErrFormAccess (w, address, size, forRead));
					butItsOK = false;
				}

				return;
			}

			// Walk the objects in the dialog.  Check each one to see
			// if the access was made to one of them.

			emuptr	thisObject = firstObject;

			for (int ii = 0; ii < numObjects; ++ii)
			{
				// Get the form object kind, so that we can determine its size.

				EmAliasFormObjListType<PAS>	object (thisObject);
				FormObjectKind				kind		= object.objectType;
				emuptr						objectAddr	= object.object;

				static int	kSizeArray[] =
				{
					EmAliasFieldType<PAS>::GetSize (),				// frmFieldObj
					-1,												// frmControlObj
					EmAliasListType<PAS>::GetSize (),				// frmListObj
					EmAliasTableType<PAS>::GetSize (),				// frmTableObj
					EmAliasFormBitmapType<PAS>::GetSize (),			// frmBitmapObj
					EmAliasFormLineType<PAS>::GetSize (),			// frmLineObj
					EmAliasFormFrameType<PAS>::GetSize (),			// frmFrameObj
					EmAliasFormRectangleType<PAS>::GetSize (),		// frmRectangleObj
					EmAliasFormLabelType<PAS>::GetSize (),			// frmLabelObj
					EmAliasFormTitleType<PAS>::GetSize (),			// frmTitleObj
					EmAliasFormPopupType<PAS>::GetSize (),			// frmPopupObj
					EmAliasFrmGraffitiStateType<PAS>::GetSize (),	// frmGraffitiStateObj
					EmAliasFormGadgetType<PAS>::GetSize (),			// frmGadgetObj
					EmAliasScrollBarType<PAS>::GetSize ()			// frmScrollBarObj
				};

				// If the access was to a form object, flag an error.

				int		itsSize	= kSizeArray[kind];

				if (itsSize < 0)	// Special cases
				{
					// Some controls have special sizes.  Determine and use those.

					EmAliasControlType<PAS>	control (objectAddr);
					uint16	attr	= control.attr.flags;
					uint8	style	= control.style;

					itsSize	= control.GetSize ();	// Standard ControlType size
					
					if (attr & ControlAttrType_graphical)
					{
						// It's a GraphicControlType.

						itsSize = EmAliasGraphicControlType<PAS>::GetSize ();
					}
					else if (style == sliderCtl || style == feedbackSliderCtl)
					{
						// It's a SliderControlType.

						itsSize = EmAliasSliderControlType<PAS>::GetSize ();
					}
				}

				// Now check the size.

				if (address >= objectAddr && address < objectAddr + itsSize)
				{
					isInUIObject = true;

					if (!::PrvAllowedFormObjectAccess (thisObject, address, forRead))
					{
						gSession->ScheduleDeferredError (new EmDeferredErrFormObjectAccess (thisObject, w, address, size, forRead));
						butItsOK = false;
					}

					return;
				}

				// Go to the next form object.

				thisObject += object.GetSize ();
			}
		}
		else
		{
			// Plain old window.  Check the access against the window size.
			// While a couple of the fields of WindowTyp have changed from
			// Palm OS 1.0 to 4.0 (viewOrigin has changed to bitMapP, gstate
			// has changed to drawStateP, and compressed has changed to
			// freeBitmap), the size is still the same.

			if (address >= w && address < w + window.GetSize ())
			{
				isInUIObject = true;

				if (!::PrvAllowedWindowAccess (w, address, forRead))
				{
					gSession->ScheduleDeferredError (new EmDeferredErrWindowAccess (w, address, size, forRead));
					butItsOK = false;
				}

				return;
			}
		}

		// Go to the next window.

		w = window.nextWindow;
	}
}

typedef void (*MarkerFn) (emuptr begin, emuptr end);

static void PrvForEachWindow (MarkerFn fn)
{
	// If the UI is not initialized for this application, the FirstWindow
	// pointer in uiGlobals will be non-NULL, but invalid (it will point
	// to the FirstWindow for the previous application, which has been
	// disposed of by now).  Wait until UIReset has been called.

	if (!Patches::UIReset ())
		return;

	// Give us full access to memory.

	CEnableFullAccess	munge;

	// Walk the window list, looking for windows and forms

	emuptr	w = EmLowMem_GetGlobal (uiGlobals.firstWindow);
	while (w)
	{
		// Get the heap the resource is in.  Use that to get information about
		// the chunk the resource is in.

		const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr ((MemPtr) w);

		// Can't find the heap?  Aip!

		EmAssert (heap != NULL);

		if (!heap)
			return;

		// Get information on this chunk so that we can get its size.

		EmPalmChunk	chunk (*heap, w - heap->ChunkHeaderSize ());

		fn (w, w + chunk.BodySize ());

		// Go to the next window.

		EmAliasWindowType<PAS>	window (w);
		w = window.nextWindow;
	}
}


void MetaMemory::MarkUIObjects (void)
{
	::PrvForEachWindow (&MarkUIObject);
}


void MetaMemory::UnmarkUIObjects (void)
{
	::PrvForEachWindow (&UnmarkUIObject);
}


#pragma mark -

// ---------------------------------------------------------------------------
//		� PrvLocalIDToPtr
// ---------------------------------------------------------------------------
// Local/native version of MemLocalIDToPtr.  Implemented natively instead
// of calling emulated version for speed.

static emuptr PrvLocalIDToPtr (LocalID local)
{
	emuptr						memCardInfoP = EmLowMem_GetGlobal (memCardInfoP);
	EmAliasCardInfoType<PAS>	cardInfo (memCardInfoP);
	emuptr						p = (local & 0xFFFFFFFE) + cardInfo.baseP;

	// If it's a handle, dereference it

	if (local & 0x01) 
		p = EmMemGet32 (p);

	return p;
}


// ---------------------------------------------------------------------------
//		� PrvGetRAMDatabaseDirectory
// ---------------------------------------------------------------------------
// Return a pointer to the database directory for RAM-based databases.

static emuptr PrvGetRAMDatabaseDirectory(void)
{
	emuptr							memCardInfoP = EmLowMem_GetGlobal (memCardInfoP);
	EmAliasCardInfoType<PAS>		cardInfo (memCardInfoP);
	emuptr							storeP = cardInfo.ramStoreP;

	EmAliasStorageHeaderType<PAS>	store (storeP);
	LocalID							databaseDirID = store.databaseDirID;

	return ::PrvLocalIDToPtr (databaseDirID);
}


// ---------------------------------------------------------------------------
//		� PrvIsLowerCaseCreator
// ---------------------------------------------------------------------------
// Return whether or not the given creator is composed of all lower-case
// letters (as defined by the islower macro in ctypes.h).

static Bool PrvIsLowerCaseCreator (UInt32 creator)
{
	const char*	p = (const char*) &creator;

	return islower (p[0]) && islower (p[1]) && islower (p[2]) && islower (p[3]);
}


// ---------------------------------------------------------------------------
//		� PrvAddTaggedChunk
// ---------------------------------------------------------------------------

static void PrvAddTaggedChunk (const EmTaggedPalmChunk& chunk)
{
	gTaggedChunks.push_back (chunk);
}


// ---------------------------------------------------------------------------
//		� PrvLoadTaggedChunk
// ---------------------------------------------------------------------------
// Return whether or not the given memory address is in a chunk on our cache
// of RAM-based system components.

static void PrvLoadTaggedChunk (emuptr pc)
{
	EmTaggedPalmChunkList::iterator	iter = gTaggedChunks.begin ();
	while (iter != gTaggedChunks.end ())
	{
		if (iter->BodyContains (pc))
		{
			gHaveLastChunk = true;
			gLastChunk = *iter;
			return;
		}

		++iter;
	}
}


// ---------------------------------------------------------------------------
//		� MetaMemory::ChunkUnlocked
// ---------------------------------------------------------------------------
// If a chunk is unlocked, see if that chunk is on our cache of RAM-based
// system components.  If so, remove that chunk from our cache.

void MetaMemory::ChunkUnlocked (emuptr addr)
{
	EmTaggedPalmChunkList::iterator	iter = gTaggedChunks.begin ();
	while (iter != gTaggedChunks.end ())
	{
		if (iter->BodyContains (addr))
		{
			gTaggedChunks.erase (iter);

			if (gLastChunk.BodyContains (addr))
			{
				gHaveLastChunk = false;
			}

			return;
		}

		++iter;
	}
}


// ---------------------------------------------------------------------------
//		� PrvIsSystemDatabase
// ---------------------------------------------------------------------------
// See if the database is one that we want to treat as a RAM-based system
// component.

static Bool PrvIsSystemDatabase (UInt32 type, UInt32 creator)
{
	Bool	isSystemDatabase = false;

	if ((type == sysFileTExtension) ||
		(type == sysFileTLibrary) ||
		(type == sysFileTPanel) ||
		(type == sysFileTSystemPatch))
	{
		isSystemDatabase = ::PrvIsLowerCaseCreator (creator);
	}
	else if (type == sysFileTApplication)
	{
		// I'd just check for all-lower-case letters here, but I'm
		// dubious as to how strictly 3rd party applications adhere
		// to that requirement.  Checking a directory full of 3rd
		// party applications on our server, 9 of the 250 or so
		// application had all-lower-case creators (and more than
		// one of those was 'memo'!).

		isSystemDatabase = (creator == sysFileCClipper);
	}

	// Handspring has RAM-based extensions that access system globals
	// low-memory, and hardware registers.  From Bob Petersen:
	//
	//		HsHal.prc and HsExtensions.prc are the only Handspring
	//		databases that are allowed to perform "system-only" functions
	//		like accessing low memory or touching the Dragonball.  On
	//		Prism, we copy HsExtensions.prc to RAM.  Potentially either
	//		PRC could be run from RAM.
	//
	// From James Phillips:
	//
	//		The HsExtensions.prc is type 'HsPt' creator 'HsEx' and
	//		the Hal.prc is type 'HwAl' creator 'HsEx'.

	else if (creator == 'HsEx')
	{
		isSystemDatabase = (type == 'HsPt' || type == 'HsEx');
	}

	return isSystemDatabase;
}


// ---------------------------------------------------------------------------
//		� PrvSearchForCodeChunk
// ---------------------------------------------------------------------------

static void PrvSearchForCodeChunk (emuptr pc)
{
	// We don't already know about the code chunk containing the given PC.
	// Iterate over the RAM-based databases, looking for one containing a
	// resource containing the given PC.  When we find it, cache it for
	// subsequent searches.

	// Give us full access to memory.

	CEnableFullAccess			munge;

	// Get the directory for the RAM store.  Assumes Card 0.

	emuptr						dirP = ::PrvGetRAMDatabaseDirectory ();
	EmAliasDatabaseDirType<PAS>	dir (dirP);

	// Iterate over all the entries.

	UInt16	numDatabases = dir.numDatabases;
	for (UInt16 ii = 0; ii < numDatabases; ++ii)
	{
		// Get the database header for the current entry.

		LocalID						dbID = dir.databaseID[ii].baseID;
		emuptr						hdrP = ::PrvLocalIDToPtr (dbID);
		EmAliasDatabaseHdrType<PAS>	hdr (hdrP);

		// Skip this if it's a record database, not a resource database.

		if ((hdr.attributes & dmHdrAttrResDB) == 0)
			continue;

		// Get a reference to the list of resources in the database.

		EmAliasRecordListType<PAS>	recList (hdr.recordList);

		// Grovel over all of the resources.

		UInt16	numRecords = recList.numRecords;
		for (UInt16 jj = 0; jj < numRecords; ++jj)
		{
			EmAliasRsrcEntryType<PAS>	entry (recList.resources[jj]);

			// Convert the resource's LocalID into a pointer to the resource data.

			LocalID	entryLocalID = entry.localChunkID;
			emuptr	resourceP = ::PrvLocalIDToPtr (entryLocalID);

			// Get the heap the resource is in.  Use that to get information about
			// the chunk the resource is in.

			const EmPalmHeap*	heap = EmPalmHeap::GetHeapByPtr ((MemPtr) resourceP);

			// Can't find the heap?  Aip!

			EmAssert (heap != NULL);

			if (!heap)
				continue;

			// If not in this resource, move to the next one.

			EmPalmChunk	chunk (*heap, resourceP - heap->ChunkHeaderSize ());

			if (!chunk.BodyContains (pc))
				continue;

			// The PC is in this resource.  Tag it as a system resource
			// or not and add it to our cache of tagged resources.

			UInt32	type	= hdr.type;
			UInt32	creator	= hdr.creator;
			Bool	okType	= ::PrvIsSystemDatabase (type, creator);

			gHaveLastChunk	= true;
			gLastChunk		= EmTaggedPalmChunk (chunk, okType);

			::PrvAddTaggedChunk (gLastChunk);

			// We found what we were looking for, so we can leave now.

			return;
		}
	}
}


// ---------------------------------------------------------------------------
//		� MetaMemory::InRAMOSComponent
// ---------------------------------------------------------------------------
// Determine if the given memory address is in a RAM-based system component.

Bool MetaMemory::InRAMOSComponent (emuptr pc)
{
	// If any access is OK, return true.

	if (CEnableFullAccess::AccessOK ())
		return true;

	// See if we have cached the last known code chunk.  If so, see if it
	// contains the given pc.

	if (!(gHaveLastChunk && gLastChunk.BodyContains (pc)))
	{
		// No, either there's no cached chunk or it doesn't contain the pc.
		// Check our cache of known code chunks.

		::PrvLoadTaggedChunk (pc);

		if (!(gHaveLastChunk && gLastChunk.BodyContains (pc)))
		{
			// No, it's not in our cached list of chunks.  Walk the databases
			// looking for a new code chunk to add to our list.

			::PrvSearchForCodeChunk (pc);

			// If we don't have it by now, it means that the PC is not in
			// a database code resource!  That can happen with some system
			// function patch stubs.  If it does, assume it's not system code.

			if (!(gHaveLastChunk && gLastChunk.BodyContains (pc)))
			{
				return false;
			}
		}
	}

	// Return whether or not it's a system code chunk.

	return gLastChunk.fIsSystemChunk;
}
