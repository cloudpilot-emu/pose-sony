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

#ifndef EmMemory_h
#define EmMemory_h

// Normally, I'd assume that these includes were pulled in
// by EmCommon.h.  However, EmMemory.h gets included by UAE,
// which doesn't pull in EmCommon.h.  So I have to explicitly
// make sure they're included.

#include "Switches.h"			// WORDSWAP_MEMORY, UNALIGNED_LONG_ACCESS
#include "EmTypes.h"			// uint32, etc.
#include "EmAssert.h"			// EmAssert

#include "sysconfig.h"			// STATIC_INLINE

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
//		� EmAddressBank
// ---------------------------------------------------------------------------

typedef uint32	(*EmMemGetFunc)				(emuptr);
typedef void	(*EmMemPutFunc)				(emuptr, uint32);
typedef uint8*	(*EmMemTranslateFunc)		(emuptr);
typedef int		(*EmMemCheckFunc)			(emuptr, uint32);
typedef void	(*EmMemCycleFunc)			(void);
typedef uint8*	(*EmMemTranslateMetaFunc)	(emuptr);

typedef struct EmAddressBank
{
	/* These ones should be self-explanatory... */
	EmMemGetFunc			lget, wget, bget;
	EmMemPutFunc			lput, wput, bput;

	/* Use xlateaddr to translate an Amiga address to a uae_u8 * that can
	 * be used to address memory without calling the wget/wput functions.
	 * This doesn't work for all memory banks, so this function may call
	 * abort(). */
	EmMemTranslateFunc		xlateaddr;

	/* To prevent calls to abort(), use check before calling xlateaddr.
	 * It checks not only that the memory bank can do xlateaddr, but also
	 * that the pointer points to an area of at least the specified size.
	 * This is used for example to translate bitplane pointers in custom.c */
	EmMemCheckFunc			check;

	EmMemTranslateMetaFunc	xlatemetaaddr;
	EmMemCycleFunc			EmMemAddOpcodeCycles;
} EmAddressBank;


extern EmAddressBank*	gEmMemBanks[65536];


// ---------------------------------------------------------------------------
//		� Support macros
// ---------------------------------------------------------------------------

#define EmMemBankIndex(addr)			(((emuptr)(addr)) >> 16)

#define EmMemGetBank(addr)				(*gEmMemBanks[EmMemBankIndex(addr)])
#define EmMemPutBank(addr, b)			(gEmMemBanks[EmMemBankIndex(addr)] = (b))

#define EmMemCallGetFunc(func, addr)	((*EmMemGetBank(addr).func)(addr))
#define EmMemCallPutFunc(func, addr, v)	((*EmMemGetBank(addr).func)(addr, v))


// ---------------------------------------------------------------------------
//		� EmMemGet32
// ---------------------------------------------------------------------------

STATIC_INLINE uint32 EmMemGet32(emuptr addr)
{
    return EmMemCallGetFunc(lget, addr);
}

// ---------------------------------------------------------------------------
//		� EmMemGet16
// ---------------------------------------------------------------------------

STATIC_INLINE uint32 EmMemGet16(emuptr addr)
{
    return EmMemCallGetFunc(wget, addr);
}

// ---------------------------------------------------------------------------
//		� EmMemGet8
// ---------------------------------------------------------------------------

STATIC_INLINE uint32 EmMemGet8(emuptr addr)
{
    return EmMemCallGetFunc(bget, addr);
}

// ---------------------------------------------------------------------------
//		� EmMemPut32
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemPut32(emuptr addr, uint32 l)
{
    EmMemCallPutFunc(lput, addr, l);
}

// ---------------------------------------------------------------------------
//		� EmMemPut16
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemPut16(emuptr addr, uint32 w)
{
    EmMemCallPutFunc(wput, addr, w);
}

// ---------------------------------------------------------------------------
//		� EmMemPut8
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemPut8(emuptr addr, uint32 b)
{
    EmMemCallPutFunc(bput, addr, b);
}

// ---------------------------------------------------------------------------
//		� EmMemGetRealAddress
// ---------------------------------------------------------------------------

STATIC_INLINE uint8* EmMemGetRealAddress(emuptr addr)
{
    return EmMemGetBank(addr).xlateaddr(addr);
}

// ---------------------------------------------------------------------------
//		� EmMemCheckAddress
// ---------------------------------------------------------------------------

STATIC_INLINE int EmMemCheckAddress(emuptr addr, uint32 size)
{
    return EmMemGetBank(addr).check(addr, size);
}

// ---------------------------------------------------------------------------
//		� EmMemAddOpcodeCycles
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemAddOpcodeCycles(emuptr addr)
{
	EmAssert (EmMemGetBank(addr).EmMemAddOpcodeCycles);
    EmMemGetBank(addr).EmMemAddOpcodeCycles();
}

// ---------------------------------------------------------------------------
//		� EmMemGetMetaAddress
// ---------------------------------------------------------------------------

STATIC_INLINE uint8* EmMemGetMetaAddress(emuptr addr)
{
	EmAssert(EmMemGetBank(addr).xlatemetaaddr);
    return EmMemGetBank(addr).xlatemetaaddr(addr);
}

// ---------------------------------------------------------------------------
//		� EmMemDoGet32
// ---------------------------------------------------------------------------

STATIC_INLINE uint32 EmMemDoGet32 (void* a)
{
#if WORDSWAP_MEMORY || !UNALIGNED_LONG_ACCESS
	return	(((uint32) *(((uint16*) a) + 0)) << 16) |
			(((uint32) *(((uint16*) a) + 1)));
#else
	return *(uint32*) a;
#endif
}

// ---------------------------------------------------------------------------
//		� EmMemDoGet16
// ---------------------------------------------------------------------------

STATIC_INLINE uint16 EmMemDoGet16 (void* a)
{
	return *(uint16*) a;
}

// ---------------------------------------------------------------------------
//		� EmMemDoGet8
// ---------------------------------------------------------------------------

STATIC_INLINE uint8 EmMemDoGet8 (void* a)
{
#if WORDSWAP_MEMORY
	return *(uint8*) ((long) a ^ 1);
#else
	return *(uint8*) a;
#endif
}

// ---------------------------------------------------------------------------
//		� EmMemDoPut32
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemDoPut32 (void* a, uint32 v)
{
#if WORDSWAP_MEMORY || !UNALIGNED_LONG_ACCESS
	*(((uint16*) a) + 0) = (uint16) (v >> 16);
	*(((uint16*) a) + 1) = (uint16) (v);
#else
	*(uint32*) a = v;
#endif
}

// ---------------------------------------------------------------------------
//		� EmMemDoPut16
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemDoPut16 (void* a, uint16 v)
{
	*(uint16*) a = v;
}

// ---------------------------------------------------------------------------
//		� EmMemDoPut8
// ---------------------------------------------------------------------------

STATIC_INLINE void EmMemDoPut8 (void* a, uint8 v)
{
#if WORDSWAP_MEMORY
	*(uint8*) ((long) a ^ 1) = v;
#else
	*(uint8*) a = v;
#endif
}


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

class EmStream;


// Types.

// This struct is used to control access to memory.  The first set of fields
// are booleans which, when set to true, turn on address validation for the
// various ranges of memory.  The second second set of fields are booleans
// which, when set to true, prevent access by user applications to the various
// ranges of memory.

struct MemAccessFlags
{
	Bool				fValidate_DummyGet;
	Bool				fValidate_DummySet;
	Bool				fValidate_RegisterGet;
	Bool				fValidate_RegisterSet;
	Bool				fValidate_DRAMGet;
	Bool				fValidate_DRAMSet;
	Bool				fValidate_SRAMGet;
	Bool				fValidate_SRAMSet;
	Bool				fValidate_ROMGet;
	Bool				fValidate_ROMSet;

//	Bool				fProtect_LowMemGet;
//	Bool				fProtect_LowMemSet;
//	Bool				fProtect_GlobalGet;
//	Bool				fProtect_GlobalSet;
//	Bool				fProtect_ScreenGet;
//	Bool				fProtect_ScreenSet;
	Bool				fProtect_SRAMGet;
	Bool				fProtect_SRAMSet;
	Bool				fProtect_ROMGet;
	Bool				fProtect_ROMSet;
	Bool				fProtect_RegisterGet;
	Bool				fProtect_RegisterSet;

//	Bool				fCheck_UserChunkGet;
//	Bool				fCheck_UserChunkSet;
//	Bool				fCheck_SysChunkGet;
//	Bool				fCheck_SysChunkSet;

//	Bool				fProtect_SysLowMemGet;
//	Bool				fProtect_SysLowMemSet;
//	Bool				fProtect_SysGlobalGet;
//	Bool				fProtect_SysGlobalSet;
//	Bool				fProtect_SysScreenGet;
//	Bool				fProtect_SysScreenSet;
//	Bool				fProtect_SysSRAMGet;
//	Bool				fProtect_SysSRAMSet;
	Bool				fProtect_SysROMGet;
	Bool				fProtect_SysROMSet;
//	Bool				fProtect_SysRegisterGet;
//	Bool				fProtect_SysRegisterSet;
};


// Globals.

extern MemAccessFlags	gMemAccessFlags;

#if PROFILE_MEMORY
enum
{
	kDRAMLongRead,	kDRAMLongRead2,		kDRAMWordRead,	kDRAMByteRead,
	kDRAMLongWrite,	kDRAMLongWrite2,	kDRAMWordWrite,	kDRAMByteWrite,
	kSRAMLongRead,	kSRAMLongRead2,		kSRAMWordRead,	kSRAMByteRead,
	kSRAMLongWrite,	kSRAMLongWrite2,	kSRAMWordWrite,	kSRAMByteWrite,
	kROMLongRead,	kROMLongRead2,		kROMWordRead,	kROMByteRead,
	kROMLongWrite,	kROMLongWrite2,		kROMWordWrite,	kROMByteWrite,
	kREGLongRead,	kREGLongRead2,		kREGWordRead,	kREGByteRead,
	kREGLongWrite,	kREGLongWrite2,		kREGWordWrite,	kREGByteWrite,

	kSED1375LongRead,	kSED1375LongRead2,		kSED1375WordRead,	kSED1375ByteRead,
	kSED1375LongWrite,	kSED1375LongWrite2,		kSED1375WordWrite,	kSED1375ByteWrite,
	
	kLastEnum
};
extern long				gMemoryAccess[];
#endif

struct EmAddressBank;
class SessionFile;

// Function prototypes.

class Memory
{
	public:
		static void				Initialize			(EmStream& hROM,
													 RAMSizeType ramSize);
		static void				Reset				(Bool hardwareReset);
		static void				Save				(SessionFile&);
		static void				Load				(SessionFile&);
		static void				Dispose				(void);

		static void				InitializeBanks		(EmAddressBank& iBankInitializer,
													 int32 iStartingBankIndex,
													 int32 iNumberOfBanks);

		static void				ResetBankHandlers	(void);

		static void				MapPhysicalMemory	(const void*, uint32);
		static void				UnmapPhysicalMemory	(const void*);
		static void				GetMappingInfo		(emuptr, void**, uint32*);
};


// There are places within the emulator where we'd like to access low-memory
// and/or Dragonball registers.  If the PC happens to be in RAM, then
// the checks implied by the above booleans and switches will flag our
// access as an error.  Before making such accesses, create an instance
// of CEnableFullAccess to suspend and restore the checks.
//
// Since such accesses are typically "meta" accesses where the emulator is
// accessing memory outside the normal execution of an opcode, we also
// turn off the profiling variable that controls whether or not cycles
// spent accessing memory are counted.

class CEnableFullAccess
{
	public:
								CEnableFullAccess (void);
								~CEnableFullAccess (void);

		static Bool				AccessOK (void);

	private:
		MemAccessFlags			fOldMemAccessFlags;

#if HAS_PROFILING
		Bool					fOldProfilingCounted;
#endif

		static long				fgAccessCount;
};

#endif	// __cplusplus

#endif /* EmMemory_h */
