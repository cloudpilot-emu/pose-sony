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

#ifndef EmCommon_h
#define EmCommon_h

// Common header file included by all Palm OS Emulator files.

// ==================================================================================
// Configuration
// ==================================================================================

// Pull in switches that define our configuration.  We require that the following
// symbols be defined externally, but all the rest are defined in Switches.h.
//
//		_DEBUG or NDEBUG
//		HAS_PROFILING (0 or 1)
//		INCLUDE_SECRET_STUFF (0 or 1)

#include "Switches.h"


// ==================================================================================
// Some very handy macros.
// ==================================================================================

#define countof(a)	(sizeof(a) / sizeof (a[0]))

#define UNUSED_PARAM(x)	((void) x);

#define COMPILE_TIME_ASSERT(x)	do { char _dummy[(x) ? 1 : -1]; UNUSED_PARAM (_dummy)} while (0)


// ==================================================================================
// Platform-independent definitions
// ==================================================================================

#include "EmTypes.h"
//#include "EmStructs.h"


// ==================================================================================
// Platform-specific definitions
// ==================================================================================

#if PLATFORM_MAC
	#include "EmCommonMac.h"
#endif

#if PLATFORM_UNIX
	#include "EmCommonUnix.h"
#endif

#if PLATFORM_WINDOWS
	#include "EmCommonWin.h"
#endif

using namespace std;

// Everyone gets EmAssert!

#include "EmAssert.h"


// Compile-time checks to see that the sizes are right.

extern char check1	[sizeof(int8) == 1 ? 1 : 0];
extern char check2	[sizeof(int16) == 2 ? 1 : 0];
extern char check3	[sizeof(int32) == 4 ? 1 : 0];
extern char check4	[sizeof(int64) == 8 ? 1 : 0];

extern char check5	[sizeof(uint8) == 1 ? 1 : 0];
extern char check6	[sizeof(uint16) == 2 ? 1 : 0];
extern char check7	[sizeof(uint32) == 4 ? 1 : 0];
extern char check8	[sizeof(uint64) == 8 ? 1 : 0];

extern char check9	[sizeof(emuptr) == 4 ? 1 : 0];


#endif	/* EmCommon_h */
