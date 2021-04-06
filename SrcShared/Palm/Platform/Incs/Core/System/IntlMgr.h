/******************************************************************************
 *
 * Copyright (c) 1998-1999 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: IntlMgr.h
 *
 * Description:
 *	  This file defines public Int'l Mgr structures and routines.
 *
 * History:
 *		03/21/98	kwk	Created by Ken Krugler.
 *		10/14/98	kwk	Added intlIntlGetRoutineAddress selector and
 *							IntlGetRoutineAddress routine declaration.
 *		08/05/99	kwk	Added intlIntlHandleEvent selector and the
 *							IntlHandleEvent routine declaration.
 *		09/22/99	kwk	Added intlTxtParamString selector.
 *		10/20/99	kwk	Moved private stuff to IntlPrv.h
 *
 *****************************************************************************/

#ifndef __INTLMGR_H__
#define __INTLMGR_H__

#include <PalmTypes.h>
#include <CoreTraps.h>

// If nobody has explicitly specified that we should or should not
// use our international trap dispatcher, set it based on the emulation
// level.

#ifndef USE_INTL_TRAPS
	#if EMULATION_LEVEL == EMULATION_NONE
		#define	USE_INTL_TRAPS	1
	#else
		#define	USE_INTL_TRAPS	0
	#endif
#endif

/***********************************************************************
 * Public constants
 ***********************************************************************/

// Bits set for the Intl Mgr feature.

#define	intlMgrExists	0x00000001

// International Manager trap macros.

#if USE_INTL_TRAPS
	#define INTL_TRAP(sel) \
		_SYSTEM_API(_CALL_WITH_SELECTOR)(_SYSTEM_TABLE, sysTrapIntlDispatch, sel)
#else
	#define INTL_TRAP(intlSelectorNum)
#endif

// Selectors for routines found in the international manager. The order
// of these selectors MUST match the jump table in IntlDispatch.c.

typedef enum {
	intlIntlInit = 0,
	intlTxtByteAttr,
	intlTxtCharAttr,
	intlTxtCharXAttr,
	intlTxtCharSize,
	intlTxtGetPreviousChar,
	intlTxtGetNextChar,
	intlTxtGetChar,
	intlTxtSetNextChar,
	intlTxtCharBounds,
	intlTxtPrepFindString,
	intlTxtFindString,
	intlTxtReplaceStr,
	intlTxtWordBounds,
	intlTxtCharEncoding,
	intlTxtStrEncoding,
	intlTxtEncodingName,
	intlTxtMaxEncoding,
	intlTxtTransliterate,
	intlTxtCharIsValid,
	intlTxtCompare,
	intlTxtCaselessCompare,
	intlTxtCharWidth,
	intlTxtGetTruncationOffset,
	intlIntlGetRoutineAddress,
	intlIntlHandleEvent,
	intlTxtParamString,
	
	intlMaxSelector = intlTxtParamString,
	intlBigSelector = 0x7FFF	// Force IntlSelector to be 16 bits.
} IntlSelector;

/***********************************************************************
 * Public routines
 ***********************************************************************/

#ifdef REMOVE_FOR_EMULATOR
#ifdef __cplusplus
	extern "C" {
#endif

// Return back the address of the routine indicated by <inSelector>. If
// <inSelector> isn't a valid routine selector, return back NULL.
void *IntlGetRoutineAddress(IntlSelector inSelector)
		INTL_TRAP(intlIntlGetRoutineAddress);

#ifdef __cplusplus
	}
#endif
#endif

#endif // __INTLMGR_H__
