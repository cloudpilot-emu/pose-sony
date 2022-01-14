#include "EmCommon.h"
#include "EmPatchState.h"
#include "EmPalmFunction.h"

#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"			// PARAM_VAL, etc.

#include "UAE.h"

#include "EmPatchModule_ExpMgr.h"		// g_nCardInserted, SetCardInfo()
#include "EmPatchModule_SlotDrvLib.h"	
#include "SonyShared\FSLib.h"
#include "SonyShared\VFSMgr.h"
#include "SlotDrvLib.h"				// CardMetricsType

// ======================================================================
//	Proto patch table for Slot-Driver-Library functions.  
//  This array will be used to create a sparse array at runtime.
// ======================================================================

static void SlotDrvLibReternValueChecker(void);

static ProtoPatchTableEntry	gProtoSlotDrvLibPatchTable[] =
{
	{sysLibTrapOpen,		NULL,								NULL},
	{sysLibTrapClose,		NULL,								NULL},
	{sysLibTrapSleep,		NULL,								NULL},
	{sysLibTrapWake,		NULL,								NULL},

	{4,						NULL,								NULL},
	{5,						NULL,								NULL},
	{6,						NULL,								NULL},

	{7,						NULL,								NULL},
	{8,						SlotDrvLibHeadpatch::Selecter8,		NULL},
	{9,						NULL,								NULL},
	{10,					NULL,								NULL},
	{11,					SlotDrvLibHeadpatch::Selecter11,	NULL},
	{12,					NULL,								NULL},
	{13,					NULL,								NULL},	
	{14,					NULL,								NULL},
	{15,					SlotDrvLibHeadpatch::Selecter15,	NULL},
	{16,					NULL,								NULL},
	{17,					NULL,								NULL},
	{18,					NULL,								NULL},
	{19,					NULL,								NULL},
	{20,					NULL,								NULL},

	{0xFFFF,				NULL,								NULL}
};

static char	gszVolumeLabel[256];	// Volume Label

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

ProtoPatchTableEntry* SlotDrvLibHeadpatch::GetSlotDrvLibPatchesTable()
{
	return gProtoSlotDrvLibPatchTable;
}

/***********************************************************************
 *
 * FUNCTION:	EmPatchModule_SlotDrvlib::EmPatchModule_SlotDrvlib
 *
 * DESCRIPTION:	Constructor
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/
EmPatchModule_SlotDrvLib::EmPatchModule_SlotDrvLib() :
	EmPatchModule("~SlotDrvLib"/*不明*/, gProtoSlotDrvLibPatchTable)
{
}

/***********************************************************************
 * FUNCTION:	Selecter8()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType SlotDrvLibHeadpatch::Selecter8 (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("Selecter8");

	CALLED_SETUP("Err", "UInt16 libRefNum, UInt16 slotRefNum, ExpCardInfoType* infoP");

	CALLED_GET_PARAM_VAL(UInt16, libRefNum);
	CALLED_GET_PARAM_VAL(UInt16, slotRefNum);
	CALLED_GET_PARAM_REF(ExpCardInfoType,	infoP,		Marshal::kInOut);

	m68k_dreg (regs, 0) = ExpMgrLibHeadpatch::SetCardInfo(slotRefNum, infoP);
	
	infoP.Put();
	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	Selecter11()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType SlotDrvLibHeadpatch::Selecter11 (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("Selecter11");

	CALLED_SETUP("Err", "UInt16	libRefNum, UInt16 slotRefNum, CardMetricsType* metricsP");

	CALLED_GET_PARAM_REF(CardMetricsType,	metricsP,		Marshal::kInOut);
	
	CardMetricsType*		pMetrics = metricsP;

	pMetrics->bytesPerSector = 512;
	pMetrics->sectorsPerCluster = 16;		// MSサイズが8Mの場合16
	pMetrics->headsPerCylinder = 0;
	pMetrics->numReservedRanges = 0;
	pMetrics->numSectors = 0;
	pMetrics->physicalSectorStart = 0;
	pMetrics->reservedRangesP = NULL;
	pMetrics->sectorsPerHead = 0;
	pMetrics->reserved = 0;

	Preference<Configuration>	prefConfiguration (kPrefKeyLastConfiguration);
	Configuration				cfg = *prefConfiguration;
	if (cfg.fMSSize >= 16)
		pMetrics->sectorsPerCluster = 32;	// MSサイズが16M以上の場合32
	
	m68k_dreg (regs, 0) = errNone;
	
	metricsP.Put();
	return kSkipROM;
}


/***********************************************************************
 * FUNCTION:	Selecter15()
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	kSkipROM
 *
 ***********************************************************************/
CallROMType SlotDrvLibHeadpatch::Selecter15 (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("Selecter15");

	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter5 (void)
{
#ifdef _DEBUG
	OutputDebugString("SlotDrvLib Call -- Selecter No.5\n");
#endif
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter6 (void)
{
#ifdef _DEBUG
	OutputDebugString("SlotDrvLib Call -- Selecter No.6\n");
#endif
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter7 (void)
{
#ifdef _DEBUG
	OutputDebugString("SlotDrvLib Call -- Selecter No.7\n");
#endif
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter9 (void)
{
#ifdef _DEBUG
	OutputDebugString("SlotDrvLib Call -- Selecter No.9\n");
#endif
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter10 (void)
{
#ifdef _DEBUG
	OutputDebugString("SlotDrvLib Call -- Selecter No.10\n");
#endif
	return kExecuteROM;
}


#include "PalmPackPop.h"
