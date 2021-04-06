#include "EmCommon.h"
#include "TrapPatches.h"
#include "EmPalmFunction.h"

#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"			// PARAM_VAL, etc.

#include "TrapPatches_ExpMgr.h"		// g_nCardInserted, SetCardInfo()
#include "TrapPatches_SlotDrvLib.h"	
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

	struct StackFrame
	{
		UInt16				libRefNum;
		UInt16				slotRefNum;
		ExpCardInfoType*	infoP;
	};

	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(UInt16, slotRefNum);
	PARAM_REF(ExpCardInfoType,	infoP,		Marshal::kInOut);

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

	struct StackFrame
	{
		UInt16				libRefNum;
		UInt16				slotRefNum;
		CardMetricsType*	metricsP;
	};

	PARAM_REF(CardMetricsType,	metricsP,		Marshal::kInOut);
	
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
	OutputDebugString("SlotDrvLib Call -- Selecter No.5\n");
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter6 (void)
{
	OutputDebugString("SlotDrvLib Call -- Selecter No.6\n");
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter7 (void)
{
	OutputDebugString("SlotDrvLib Call -- Selecter No.7\n");
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter9 (void)
{
	OutputDebugString("SlotDrvLib Call -- Selecter No.9\n");
	return kExecuteROM;
}

CallROMType SlotDrvLibHeadpatch::Selecter10 (void)
{
	OutputDebugString("SlotDrvLib Call -- Selecter No.10\n");
	return kExecuteROM;
}


#include "PalmPackPop.h"