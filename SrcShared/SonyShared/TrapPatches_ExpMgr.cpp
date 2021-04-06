/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.
\* ===================================================================== */

#include "EmCommon.h"
#include "TrapPatches.h"
#include "EmPalmFunction.h"

#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"			// PARAM_VAL, etc.

#include "SonyShared\ExpansionMgr.h"
#include "TrapPatches_ExpMgr.h"	
#include "SonyWin\Platform_MsfsLib.h"	
#include "SonyWin\Platform_ExpMgrLib.h"

void	LCD_SetStateJogButton(SkinElementType witch, Bool bPress, Bool bEnabled);
void	LCD_DrawButtonForPEG(HDC hDC, SkinElementType witch);

// ======================================================================
//	Globals and constants
// ======================================================================
static	UInt16					g_lstSlotNum[SLOT_COUNT] = { 1 };	// スロット番号リスト

// ======================================================================
//	Proto patch table for ExpansionManager functions.  
//  This array will be used to create a sparse array at runtime.
// ======================================================================

static ProtoPatchTableEntry	gProtoExpMgrPatchTable[] =
{
	{expInit,						ExpMgrLibHeadpatch::Init,				ExpMgrLibTailpatch::ExpInit},
	{expSlotDriverInstall,			NULL,									NULL},
	{expSlotDriverRemove,			NULL,									NULL},
	{expSlotLibFind,				NULL,									NULL},
	{expSlotRegister,				NULL,									NULL},
	{expSlotUnregister,				NULL,									NULL},
	{expCardInserted,				ExpMgrLibHeadpatch::CardInserted,		NULL},
	{expCardRemoved,				ExpMgrLibHeadpatch::CardRemoved,		NULL},
	{expCardPresent,				ExpMgrLibHeadpatch::CardPresent,		NULL},
	{expCardInfo,					ExpMgrLibHeadpatch::CardInfo,			NULL},
	{expSlotEnumerate,				ExpMgrLibHeadpatch::SlotEnumerate,		NULL},
	{0xFFFF,						NULL,									NULL}
};

static	char*	pExpMgrFuncName[] =
{
	"ExpInit",
	"ExpSlotDriverInstall",
	"ExpSlotDriverRemove",
	"ExpSlotLibFind",
	"ExpSlotRegister",
	"ExpSlotUnregister",
	"ExpCardInserted",
	"ExpCardRemoved",
	"ExpCardPresent",
	"ExpCardInfo",
	"ExpSlotEnumerate",
	NULL
};

#define PARAMETER_SIZE(x)	\
	(sizeof (((StackFrame*) 0)->x))

#define PARAMETER_OFFSET(x) \
	(m68k_areg (regs, 7) + offsetof (StackFrame, x))

#define GET_PARAMETER(x)		\
	((PARAMETER_SIZE(x) == sizeof (char)) ? get_byte (PARAMETER_OFFSET(x)) :	\
	 (PARAMETER_SIZE(x) == sizeof (short)) ? get_word (PARAMETER_OFFSET(x)) :	\
											get_long (PARAMETER_OFFSET(x)))
#define SET_PARAMETER(x, v) 	\
	((PARAMETER_SIZE(x) == sizeof (char)) ? put_byte (PARAMETER_OFFSET(x), v) : \
	 (PARAMETER_SIZE(x) == sizeof (short)) ? put_word (PARAMETER_OFFSET(x), v) :	\
											put_long (PARAMETER_OFFSET(x), v))

// ======================================================================
//	Private functions
// ======================================================================

#define PRINTF	if (1) ; else LogAppendMsg


// ========================================================================
// The following functions define a bunch of StackFrame structs.
// These structs need to mirror the format of parameters pushed
// onto the stack by the emulated code, and so need to be packed
// to 2-byte boundaries.
//
// The pragmas are reversed at the end of the file.
// ========================================================================

#include "PalmPack.h"

#pragma mark -

Bool ExpMgrLibHeadpatch::GetExpMgrPatches(	
		const SystemCallContext& context,
		HeadpatchProc& hp,
		TailpatchProc& tp)
{
	if (context.fTrapWord != sysTrapExpansionMgr)
		return false;

	if (context.fExtra > expMaxSelector)
		return false;

	hp = NULL;
	tp = NULL;
	for (int i=0; gProtoExpMgrPatchTable[i].fTrapWord!=0xFFFF; i++)
	{
		if (gProtoExpMgrPatchTable[i].fTrapWord == context.fExtra)
		{
			hp = gProtoExpMgrPatchTable[i].fHeadpatch;
			tp = gProtoExpMgrPatchTable[i].fTailpatch;
			break;
		}
	}
	return true;
}

CallROMType ExpMgrLibHeadpatch::HandleExpMgrCall(long selector)
{
	EmAssert(selector <= expMaxSelector);

	if (gProtoExpMgrPatchTable[selector].fHeadpatch)
		return gProtoExpMgrPatchTable[selector].fHeadpatch ();
	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	Init()
 *
 * DESCRIPTION:	ExpInit のエミュレート関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType ExpMgrLibHeadpatch::Init (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("Init");

	return kExecuteROM;
}

void ExpMgrLibTailpatch::ExpInit (void)
{
	struct StackFrame
	{
	};

	if (g_nCardInserted == MSSTATE_UNKNOWN)
		g_nCardInserted = MSSTATE_REQ_INSERT;

	Platform_ExpMgrLib::CalledExpInit();
}


/***********************************************************************
 *
 * FUNCTION:	CardPresent()
 *
 * DESCRIPTION:	ExpCardPresentのエミュレート関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/

CallROMType ExpMgrLibHeadpatch::CardPresent (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("CardPresent");

	struct StackFrame
	{
		UInt16			slotNumber;
	};

	PARAM_VAL(UInt16, slotNumber);
	
	UInt16	retval = errNone;
	if (slotNumber != 1)
		retval = expErrInvalidSlotRefNumber;

	if (g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE)
		retval = expErrCardNotPresent;

	m68k_dreg (regs, 0) = retval;

	return kSkipROM;
}

/***********************************************************************
 *
 * FUNCTION:	CardRemoved()
 *
 * DESCRIPTION:	ExpCardRemovedのエミュレート関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kExecuteROM
 *
 ***********************************************************************/

CallROMType ExpMgrLibHeadpatch::CardRemoved (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("CardRemoved");

	struct StackFrame
	{
		UInt16			slotRefNum;
	};

	PARAM_VAL(UInt16, slotRefNum);
	
	g_nCardInserted = MSSTATE_REMOVED;

	LCD_SetStateJogButton(kElement_MS_InOut, false, true);
	LCD_DrawButtonForPEG(NULL, kElement_MS_InOut);

	return kExecuteROM;
}

/***********************************************************************
 *
 * FUNCTION:	CardInserted()
 *
 * DESCRIPTION:	ExpCardInserted のエミュレート関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kExecuteROM
 *
 ***********************************************************************/
CallROMType ExpMgrLibHeadpatch::CardInserted (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("CardInserted");

	struct StackFrame
	{
		UInt16			slotRefNum;
	};

	PARAM_VAL(UInt16, slotRefNum);
	
	g_nCardInserted = slotRefNum;

	LCD_SetStateJogButton(kElement_MS_InOut, true, true);
	LCD_DrawButtonForPEG(NULL, kElement_MS_InOut);

	return kExecuteROM;
}

/***********************************************************************
 *
 * FUNCTION:	CardInfo()
 *
 * DESCRIPTION:	ExpCardInfo のエミュレート関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType ExpMgrLibHeadpatch::CardInfo(void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("CardInfo");

	struct StackFrame
	{
		UInt16				slotNumber;
		ExpCardInfoType*	infoP;
	};

	PARAM_VAL(UInt16, slotNumber);
	PARAM_REF(ExpCardInfoType,	infoP,		Marshal::kInOut);

	Err retval = errNone;
	if (g_nCardInserted != MSSTATE_REMOVED && g_nCardInserted != MSSTATE_REQ_REMOVE)
		SetCardInfo(slotNumber, infoP);
	else
		retval = expErrCardNotPresent;
	
	m68k_dreg (regs, 0) = retval;

	if (retval == errNone)
		infoP.Put();
	return kSkipROM;
}

Err	ExpMgrLibHeadpatch::SetCardInfo(UInt16 nSlotRefNum, ExpCardInfoType* pCardInf)
{
	BOOL	bFound = false;
	for (int i=0; i<SLOT_COUNT && !bFound; i++)
	{
		if (g_lstSlotNum[i] == nSlotRefNum)
			bFound = true;
	}

	if (!bFound)
		return expErrInvalidSlotRefNumber;

	if (g_nCardInserted != MSSTATE_REMOVED && g_nCardInserted != MSSTATE_REQ_REMOVE)
	{
		pCardInf->capabilityFlags = expCapabilityHasStorage;
		if (Platform_MsfsLib::CardIsWriteProtect(g_nCardInserted))
			pCardInf->capabilityFlags |= expCapabilityReadOnly;
		strcpy(pCardInf->manufacturerStr, "");
		strcpy(pCardInf->productStr, "");
		strcpy(pCardInf->deviceClassStr, "Memory Stick");
		strcpy(pCardInf->deviceUniqueIDStr, "");
		return errNone;
	}
	else
		return expErrCardNotPresent;
}


/***********************************************************************
 *
 * FUNCTION:	SlotEnumerate()
 *
 * DESCRIPTION:	ExpSlotEnumerate のエミュレート関数
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType ExpMgrLibHeadpatch::SlotEnumerate (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NumSlots");

	struct StackFrame
	{
		UInt16*			slotRefNumP;
		UInt32*			slotIteratorP;
	};
	PARAM_REF(UInt16,		slotRefNumP,	Marshal::kInOut);
	PARAM_REF(UInt32,		slotIteratorP,	Marshal::kInOut);

	UInt16*	pRefNum = slotRefNumP;
	*pRefNum = g_lstSlotNum[0];
	UInt32*	pIterator = slotIteratorP;
	*pIterator = expIteratorStop;

	m68k_dreg (regs, 0) = errNone;

	slotRefNumP.Put();
	slotIteratorP.Put();
	return kSkipROM;
}

/*
その他、ExpansionMgrライブラリの関数のエミュレート関数です。
必要に応じて使用してください。
CallROMType ExpMgrLibHeadpatch::SlotDriverInstall (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("SlotDriverInstall");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

CallROMType ExpMgrLibHeadpatch::SlotDriverRemove (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("SlotDriverRemove");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

CallROMType ExpMgrLibHeadpatch::SlotLibFind (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("SlotLibFind");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

CallROMType ExpMgrLibHeadpatch::SlotPollingProc (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("SlotPollingProc");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

CallROMType ExpMgrLibHeadpatch::SlotUnregister (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("SlotUnregister");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

CallROMType ExpMgrLibHeadpatch::SlotRegister (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("SlotRegister");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}
*/
#include "PalmPackPop.h"
