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

#ifndef EmCommonWin_h
#define EmCommonWin_h

// Common header file included by all Palm OS Emulator for Windows files.

// Turn off some warnings that we don't care about.

#pragma warning( disable : 4065 )	// warning C4065: switch statement contains default but no case labels
#pragma warning( disable : 4068 )	// warning C4068: unknown pragma
#pragma warning( disable : 4244 )	// warning C4244: initializing : conversion from unsigned long to unsigned short, possible loss of data
#pragma warning( disable : 4245 )	// warning C4245: 'argument' : conversion from 'int' to 'unsigned char', signed/unsigned mismatch
#pragma warning( disable : 4355 )	// warning C4355: 'this' : used in base member initializer list
#pragma warning( disable : 4660 )	// warning C4660: template-class specialization 'EmAliasEventType<class LAS>' is already instantiated
#pragma warning( disable : 4786 )	// warning C4786: '<foo>' : identifier was truncated to '255' characters in the browser information


// Palm headers

#include "Palm.h"


// Std C/C++ Library stuff

#include <ctype.h>				// isalpha, tolower
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// There is a bug in the string class in VC++ 5.0 (see KB: Q172398).
// We get around this by including a file with the fixed class.
// To make sure we get the fixed class, we must include this string2
// file *before* any other STL file can possibly bring it in.

#include "string2"

#include <algorithm>			// find, sort
#include <deque>				// deque
#include <list> 				// list
#include <utility>				// pair
#include <vector>
#include <map>

#include <malloc.h> 			// _alloca


// Windows headers

#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>			// InitCommonControls, etc.


// ============================================
// ========== Windows socket mapping ==========
// ============================================

typedef int				socklen_t;


#endif	/* EmCommonWin_h */
