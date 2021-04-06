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
#include "TrapPatches.h"

#include "Byteswapping.h"		// Canonical
#include "CGremlinsStubs.h" 	// StubAppEnqueueKey
#include "DebugMgr.h"			// Debug::ConnectedToTCPDebugger
#include "EmFileImport.h"		// InstallExgMgrLib
#include "EmLowMem.h"			// EmLowMem::GetEvtMgrIdle, EmLowMem::TrapExists, EmLowMem_SetGlobal, EmLowMem_GetGlobal
#include "EmMemory.h"			// CEnableFullAccess
#include "EmPalmFunction.h"		// InEggOfInfiniteWisdom
#include "EmPalmOS.h"			// EmPalmOS::RememberStackRange
#include "EmPalmStructs.h"		// EmAliasErr
#include "EmRPC.h"				// RPC::SignalWaiters
#include "EmSession.h"			// GetDevice
#include "ErrorHandling.h"		// Errors::SysFatalAlert
#include "Hordes.h"				// Hordes::IsOn, Hordes::PostFakeEvent, Hordes::CanSwitchToApp
#include "HostControlPrv.h" 	// HandleHostControlCall
#include "Logging.h"			// LogEvtAddEventToQueue, etc.
#include "MetaMemory.h" 		// MetaMemory mark functions
#include "Miscellaneous.h"		// SetHotSyncUserName, DateToDays, SystemCallContext
#include "Platform.h"			// Platform::SndDoCmd, GetString
#include "PreferenceMgr.h"		// Preference (kPrefKeyUserName)
#include "Profiling.h"			// StDisableAllProfiling
#include "ROMStubs.h"			// FtrSet, FtrUnregister, EvtWakeup, ...
#include "SessionFile.h"		// SessionFile
#include "SLP.h"				// SLP
#include "Startup.h"			// Startup::GetAutoLoads
#include "Strings.r.h"			// kStr_ values
#include "SystemPacket.h"		// SystemPacket::SendMessage
#include "UAE_Utils.h"			// uae_memcpy, uae_strcpy, uae_strlen

#if PLATFORM_MAC
#include <errno.h>				// ENOENT, errno
#include <sys/types.h>			// u_short, ssize_t, etc.
#include <sys/socket.h>			// sockaddr
#include <sys/errno.h>			// Needed for error translation.
#include <sys/time.h>			// fd_set
#include <netdb.h>				// hostent
#include <unistd.h>				// close
#include <sys/filio.h>			// FIONBIO
#include <sys/ioctl.h>			// ioctl
#include <netinet/in.h>			// sockaddr_in
#include <netinet/tcp.h>		// TCP_NODELAY
#include <arpa/inet.h>			// inet_ntoa
#endif

#if PLATFORM_UNIX
#include <errno.h>				// ENOENT, errno
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>			// timeval
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>				// close
#include <arpa/inet.h>			// inet_ntoa
#endif

#include "PalmPack.h"
#define NON_PORTABLE
#include "HwrROMToken.h"		// hwrROMTokenIrda
#undef NON_PORTABLE
#include "PalmPackPop.h"

#ifdef SONY_ROM
#include "EmCommands.h"
#include "EmWindow.h"
#include "EmWindowWin.h"		// GetHostWindow
#endif

// ======================================================================
// Patches for system functions.
// ======================================================================

class SysHeadpatch
{
	public:
		static CallROMType		UnmarkUIObjects			(void); // 	CtlNewControl, FldNewField, FrmInitForm, FrmNewBitmap, FrmNewGadget, FrmNewGsi, FrmNewLabel, LstNewList, WinAddWindow, WinRemoveWindow
		static CallROMType		RecordTrapNumber		(void); // EvtGetEvent & EvtGetPen

		static CallROMType		ClipboardGetItem		(void);
		static CallROMType		DbgMessage				(void);
		static CallROMType		DmInit					(void);
		static CallROMType		ErrDisplayFileLineMsg	(void);
		static CallROMType		EvtAddEventToQueue		(void);
		static CallROMType		EvtAddUniqueEventToQueue(void);
		static CallROMType		EvtEnqueueKey			(void);
		static CallROMType		EvtEnqueuePenPoint		(void);
		static CallROMType		ExgDoDialog				(void);
		static CallROMType		FrmCustomAlert			(void);
		static CallROMType		FrmDrawForm				(void);
		static CallROMType		HostControl 			(void);
		static CallROMType		HwrBatteryLevel 		(void);
		static CallROMType		HwrBattery		 		(void);
		static CallROMType		HwrDockStatus			(void);
		static CallROMType		HwrGetROMToken			(void);
		static CallROMType		HwrSleep				(void);
		static CallROMType		KeyCurrentState 		(void);
		static CallROMType		PenOpen 				(void);
		static CallROMType		PrefSetAppPreferences	(void);
		static CallROMType		SndDoCmd				(void);
		static CallROMType		SysAppExit				(void);
		static CallROMType		SysAppLaunch			(void);
		static CallROMType		SysBinarySearch 		(void);
		static CallROMType		SysDoze					(void);
		static CallROMType		SysEvGroupWait			(void);
		static CallROMType		SysFatalAlert			(void);
		static CallROMType		SysLaunchConsole		(void);
		static CallROMType		SysReset				(void);
		static CallROMType		SysSemaphoreWait		(void);
		static CallROMType		SysUIAppSwitch			(void);
		static CallROMType		TblHandleEvent			(void);

#ifdef SONY_ROM
		static CallROMType		SysLibLoad				(void);
		static CallROMType		HwrIRQ4Handler			(void);
		static CallROMType		RomSkip					(void);
#endif
};


class SysTailpatch
{
	public:
		static void		MarkUIObjects			(void); // 	CtlNewControl, FldNewField, FrmInitForm, FrmNewBitmap, FrmNewGadget, FrmNewGsi, FrmNewLabel, LstNewList, WinAddWindow, WinRemoveWindow

		static void		ClipboardAddItem		(void);
		static void		ClipboardAppendItem		(void);
		static void 	DmGetNextDatabaseByTypeCreator	(void);
		static void 	DmGetResource 			(void);
		static void 	EvtGetEvent 			(void);
		static void 	EvtGetPen				(void);
		static void 	EvtGetSysEvent			(void);
		static void		EvtSysEventAvail		(void);
		static void		ExgReceive				(void);
		static void		ExgSend					(void);
		static void 	FtrInit 				(void);
		static void 	FtrSet	 				(void);
		static void 	HwrMemReadable			(void);
		static void 	HwrSleep				(void);
		static void 	SysAppStartup			(void);
		static void 	SysBinarySearch 		(void);
		static void		SysTaskCreate			(void);
		static void 	TblHandleEvent	 		(void);
		static void 	TimInit 				(void);
		static void 	UIInitialize			(void);
		static void 	UIReset					(void);

#ifdef SONY_ROM
		static void		WinScreenMode			(void);
#endif
};


// ======================================================================
//	Proto patch table for the system functions.  This array will be used
//	to create a sparse array at runtime.
// ======================================================================

ProtoPatchTableEntry	gProtoSysPatchTable[] =
{
	{sysTrapClipboardGetItem, 		SysHeadpatch::ClipboardGetItem,			NULL},
	{sysTrapDbgMessage, 			SysHeadpatch::DbgMessage,				NULL},
	{sysTrapClipboardAddItem,		NULL,									SysTailpatch::ClipboardAddItem},
	{sysTrapClipboardAppendItem,	NULL,									SysTailpatch::ClipboardAppendItem},
	{sysTrapDmGetNextDatabaseByTypeCreator,	NULL,							SysTailpatch::DmGetNextDatabaseByTypeCreator},
	{sysTrapDmInit, 				SysHeadpatch::DmInit,					NULL},
	{sysTrapDmGetResource,			NULL,									SysTailpatch::DmGetResource},
	{sysTrapErrDisplayFileLineMsg,	SysHeadpatch::ErrDisplayFileLineMsg,	NULL},
	{sysTrapEvtAddEventToQueue, 	SysHeadpatch::EvtAddEventToQueue,		NULL},
	{sysTrapEvtAddUniqueEventToQueue,SysHeadpatch::EvtAddUniqueEventToQueue,NULL},
	{sysTrapEvtEnqueueKey,			SysHeadpatch::EvtEnqueueKey,			NULL},
	{sysTrapEvtEnqueuePenPoint, 	SysHeadpatch::EvtEnqueuePenPoint,		NULL},
	{sysTrapEvtGetEvent,			SysHeadpatch::RecordTrapNumber, 		SysTailpatch::EvtGetEvent},
	{sysTrapEvtGetPen,				SysHeadpatch::RecordTrapNumber, 		SysTailpatch::EvtGetPen},
	{sysTrapEvtGetSysEvent, 		NULL,									SysTailpatch::EvtGetSysEvent},
	{sysTrapEvtSysEventAvail,		NULL,									SysTailpatch::EvtSysEventAvail},
	{sysTrapExgReceive,				NULL,									SysTailpatch::ExgReceive},
	{sysTrapExgSend,				NULL,									SysTailpatch::ExgSend},
	{sysTrapExgDoDialog,			SysHeadpatch::ExgDoDialog,				NULL},
	{sysTrapFrmCustomAlert, 		SysHeadpatch::FrmCustomAlert,			NULL},
	{sysTrapFrmDrawForm,			SysHeadpatch::FrmDrawForm,				NULL},
	{sysTrapFtrInit,				NULL,									SysTailpatch::FtrInit},
	{sysTrapFtrSet,					NULL,									SysTailpatch::FtrSet},
	{sysTrapHostControl,			SysHeadpatch::HostControl,				NULL},
	{sysTrapHwrBatteryLevel,		SysHeadpatch::HwrBatteryLevel,			NULL},
	{sysTrapHwrBattery,				SysHeadpatch::HwrBattery,				NULL},
	{sysTrapHwrDockStatus,			SysHeadpatch::HwrDockStatus,			NULL},
	{sysTrapHwrGetROMToken, 		SysHeadpatch::HwrGetROMToken,			NULL},
	{sysTrapHwrMemReadable, 		NULL,									SysTailpatch::HwrMemReadable},
	{sysTrapHwrSleep,				SysHeadpatch::HwrSleep, 				SysTailpatch::HwrSleep},
	{sysTrapKeyCurrentState,		SysHeadpatch::KeyCurrentState,			NULL},
	{sysTrapPenOpen,				SysHeadpatch::PenOpen,					NULL},
	{sysTrapPrefSetAppPreferences,	SysHeadpatch::PrefSetAppPreferences,	NULL},
	{sysTrapSndDoCmd,				SysHeadpatch::SndDoCmd,					NULL},
	{sysTrapSysAppExit, 			SysHeadpatch::SysAppExit,				NULL},
	{sysTrapSysAppLaunch,			SysHeadpatch::SysAppLaunch, 			NULL},
	{sysTrapSysAppStartup,			NULL,									SysTailpatch::SysAppStartup},
	{sysTrapSysBinarySearch,		SysHeadpatch::SysBinarySearch,			SysTailpatch::SysBinarySearch},
	{sysTrapSysDoze,				SysHeadpatch::SysDoze, 					NULL},
	{sysTrapSysEvGroupWait, 		SysHeadpatch::SysEvGroupWait,			NULL},
	{sysTrapSysFatalAlert,			SysHeadpatch::SysFatalAlert,			NULL},
	{sysTrapSysLaunchConsole,		SysHeadpatch::SysLaunchConsole, 		NULL},
	{sysTrapSysReset,				SysHeadpatch::SysReset,			 		NULL},
	{sysTrapSysSemaphoreWait,		SysHeadpatch::SysSemaphoreWait, 		NULL},
	{sysTrapSysTaskCreate,			NULL,							 		SysTailpatch::SysTaskCreate},
	{sysTrapSysUIAppSwitch, 		SysHeadpatch::SysUIAppSwitch,			NULL},
	{sysTrapTblHandleEvent,			SysHeadpatch::TblHandleEvent,			SysTailpatch::TblHandleEvent},
	{sysTrapTimInit,				NULL,									SysTailpatch::TimInit},
	{sysTrapUIInitialize,			NULL,									SysTailpatch::UIInitialize},
	{sysTrapUIReset,				NULL,									SysTailpatch::UIReset},

	{sysTrapCtlNewControl,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapFldNewField,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapFrmInitForm,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapFrmNewBitmap,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapFrmNewGadget,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapFrmNewGsi,				SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapFrmNewLabel,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapLstNewList,				SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapWinAddWindow,			SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},
	{sysTrapWinRemoveWindow,		SysHeadpatch::UnmarkUIObjects,			SysTailpatch::MarkUIObjects},

#ifdef SONY_ROM
	{sysTrapHwrIRQ4Handler,			SysHeadpatch::HwrIRQ4Handler,			NULL},
	{sysTrapSysLibLoad,				SysHeadpatch::SysLibLoad,				NULL},
	{sysTrapWinScreenMode,			NULL,									SysTailpatch::WinScreenMode},
#endif

	{0, 							NULL,									NULL}
};


// ======================================================================
//	Patches for HtalLib functions
// ======================================================================

class HtalLibHeadpatch
{
	public:
		static CallROMType		HtalLibSendReply			(void);
};


#pragma mark -

// ===========================================================================
//		¥ ModulePatchTable
// ===========================================================================
//	A simple class for managing patches on a module.  A "module" is defined
//	a the main set of system functions (dispatch numbers 0xA000 - 0xA7FFF) or
//	a library (dispatch numbers 0xA800 - 0xAFFF, with a unique refnum to
//	select which library).

class ModulePatchTable
{
	public:
		void Clear ();

		void AddProtoPatchTable (ProtoPatchTableEntry protoPatchTable[]);

		// B.S. operators for VC++ so that we can put objects of
		// this class into STL collections.

		bool operator < (const ModulePatchTable& other) const
		{
			return this < &other;
		}

		bool operator == (const ModulePatchTable& other) const
		{
			return this == &other;
		}

		// Return the patch function for the given module function.  The given
		// module function *must* be given as a zero-based index.  If there is
		// no patch function for the modeule function, return NULL.

		HeadpatchProc GetHeadpatch (uint16 index) const
		{
			if (index < fHeadpatches.size ())
			{
				return fHeadpatches[index];
			}

			return NULL;
		}

		TailpatchProc GetTailpatch (uint16 index) const
		{
			if (index < fTailpatches.size ())
			{
				return fTailpatches[index];
			}

			return NULL;
		}

	private:
		vector<HeadpatchProc>	fHeadpatches;
		vector<TailpatchProc>	fTailpatches;
};


void ModulePatchTable::Clear (void)
{
	fHeadpatches.clear ();
	fTailpatches.clear ();
}


void ModulePatchTable::AddProtoPatchTable (ProtoPatchTableEntry protoPatchTable[])
{
	// Create a fast dispatch table for the managed module.  A "fast
	// dispatch table" is a table with a headpatch and tailpatch entry
	// for each possible function in the module.  If the function is
	// not head or tailpatched, the corresponding entry is NULL.  When
	// a patch function is needed, the trap dispatch number is used as
	// an index into the table in order to get the right patch function.
	//
	// For simplicity, "fast patch tables" are created from "proto patch
	// tables".  A proto patch table is a compact table containing the
	// information needed to create a fast patch table.  Each entry in
	// the proto patch table is a trap-number/headpatch/tailpatch tupple.
	// Each tuple is examined in turn.	If there is a head or tail patch
	// function for the indicated module function, that patch function
	// is entered in the fast dispatch table, using the trap number as
	// the index.

	for (long ii = 0; protoPatchTable[ii].fTrapWord; ++ii)
	{
		// If there is a headpatch function...

		if (protoPatchTable[ii].fHeadpatch)
		{
			// Get the trap number.

			uint16 index = SysTrapIndex (protoPatchTable[ii].fTrapWord);

			// If the trap number is 0xA800-based, make it zero based.

			if (IsLibraryTrap (index))
				index -= SysTrapIndex (sysLibTrapBase);

			// Resize the fast patch table, if necessary.

			if (index >= fHeadpatches.size ())
			{
				fHeadpatches.resize (index + 1);
			}

			// Add the headpatch function.

			fHeadpatches[index] = protoPatchTable[ii].fHeadpatch;
		}

		// If there is a tailpatch function...

		if (protoPatchTable[ii].fTailpatch)
		{
			// Get the trap number.

			uint16 index = SysTrapIndex (protoPatchTable[ii].fTrapWord);

			// If the trap number is 0xA800-based, make it zero based.

			if (IsLibraryTrap (index))
				index -= SysTrapIndex (sysLibTrapBase);

			// Resize the fast patch table, if necessary.

			if (index >= fTailpatches.size ())
			{
				fTailpatches.resize (index + 1);
			}

			// Add the tailpatch function.

			fTailpatches[index] = protoPatchTable[ii].fTailpatch;
		}
	}
}


// ===========================================================================
//		¥ TailpatchType
// ===========================================================================
// Structure used to hold tail-patch information.

struct TailpatchType
{
	SystemCallContext	fContext;
	int32 				fCount;
	TailpatchProc		fTailpatch;

	// I hate VC++...really I do...
	bool operator< (const TailpatchType&) const {return false;}
	bool operator> (const TailpatchType&) const {return false;}
	bool operator== (const TailpatchType&) const {return false;}
	bool operator!= (const TailpatchType&) const {return false;}
};


// ===========================================================================
//		¥ Patches
// ===========================================================================

// ======================================================================
//	Globals and constants
// ======================================================================

const UInt16						kMagicRefNum		= 0x666;	// See comments in HtalLibSendReply.
const ModulePatchTable* 			kNoPatchTable		= (ModulePatchTable*) -1;

static bool 						gUIInitialized;
static bool 						gUIReset;
static bool 						gHeapInitialized;
static bool							gEvtGetEventCalled;
static UInt32						gEncoding;
static long 						gSysBinarySearchCount;

extern long 						gMemMgrCount;
extern long 						gMemSemaphoreCount;
extern unsigned long				gMemSemaphoreReserveTime;
extern UInt32						gResizeOrigSize;
extern UInt16 						gHeapID;
extern map<emuptr, EmPalmHeap*>		gRememberedHeaps;

static UInt16 						gNextAppCardNo;
static LocalID						gNextAppDbID;
static UInt16 						gQuitAppCardNo;
static LocalID						gQuitAppDbID;
static Bool							gTimeToQuit;

static EmuAppInfoList				gCurAppInfo;

static ModulePatchTable 			gSysPatchTable;
static ModulePatchTable 			gNetLibPatchTable;

#ifdef SONY_ROM
static ModulePatchTable 			gMsfsLibPatchTable;     // MS FileSystem Lib for Sony's ROM
static ModulePatchTable 			gSlotDrvLibPatchTable;	// Slot driver Lib for Sony's ROM
#endif

typedef vector<const ModulePatchTable*> PatchTableType;
static PatchTableType				gLibPatches;

typedef vector<TailpatchType>		TailpatchTableType;
static TailpatchTableType			gInstalledTailpatches;

static uint16						gLastEvtTrap;
static UInt32						gOSVersion;

static long							gSaveDrawStateStackLevel;
static Bool							gAutoAcceptBeamDialogs;

static Bool							gDontPatchClipboardAddItem;
static Bool							gDontPatchClipboardGetItem;

static const UInt32					kOSUndeterminedVersion = ~0;


// ======================================================================
//	Private functions
// ======================================================================

static string		PrvToString 				(emuptr s);
static void 		PrvAutoload					(void);
static void 		PrvSetCurrentDate			(void);
static long			PrvGetDrawStateStackLevel	(void);
static void			PrvCopyPalmClipboardToHost	(void);
static void			PrvCopyHostClipboardToPalm	(void);


// ========================================================================
// Macros for extracting parameters from the emulated stack
// Note that use of these has been superceded by the PARAM_FOO
// macros;	we should eventually move over to those macros.
// ========================================================================

#define PARAMETER_SIZE(x)	\
	(sizeof (((StackFrame*) 0)->x))

#define PARAMETER_OFFSET(x) \
	(m68k_areg (regs, 7) + offsetof (StackFrame, x))

#define GET_PARAMETER(x)		\
	((PARAMETER_SIZE(x) == sizeof (char)) ? EmMemGet8 (PARAMETER_OFFSET(x)) :	\
	 (PARAMETER_SIZE(x) == sizeof (short)) ? EmMemGet16 (PARAMETER_OFFSET(x)) :	\
											EmMemGet32 (PARAMETER_OFFSET(x)))

#define SET_PARAMETER(x, v) 	\
	((PARAMETER_SIZE(x) == sizeof (char)) ? EmMemPut8 (PARAMETER_OFFSET(x), v) : \
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


/***********************************************************************
 *
 * FUNCTION:	Patches::Initialize
 *
 * DESCRIPTION: Standard initialization function.  Responsible for
 *				initializing this sub-system when a new session is
 *				created.  Will be followed by at least one call to
 *				Reset or Load.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/
void Patches::Initialize (void)
{
	EmAssert (gSession);
	gSession->AddInstructionBreakHandlers (
				InstallInstructionBreaks,
				RemoveInstructionBreaks,
				HandleInstructionBreak);

	gSysPatchTable.Clear ();
	gSysPatchTable.AddProtoPatchTable (gProtoSysPatchTable);
	gSysPatchTable.AddProtoPatchTable (gProtoMemMgrPatchTable);

	gNetLibPatchTable.Clear ();
	gNetLibPatchTable.AddProtoPatchTable (gProtoNetLibPatchTable);

#ifdef SONY_ROM		
	gMsfsLibPatchTable.Clear ();
	gMsfsLibPatchTable.AddProtoPatchTable (MsfsLibHeadpatch::GetMsfsLibPatchesTable());
	
	gSlotDrvLibPatchTable.Clear ();
	gSlotDrvLibPatchTable.AddProtoPatchTable (SlotDrvLibHeadpatch::GetSlotDrvLibPatchesTable());
#endif
}


/***********************************************************************
 *
 * FUNCTION:	Patches::Reset
 *
 * DESCRIPTION:	Standard reset function.  Sets the sub-system to a
 *				default state.	This occurs not only on a Reset (as
 *				from the menu item), but also when the sub-system
 *				is first initialized (Reset is called after Initialize)
 *				as well as when the system is re-loaded from an
 *				insufficient session file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::Reset (void)
{
	gLastEvtTrap	= 0;
	gOSVersion		= kOSUndeterminedVersion;

	gInstalledTailpatches.clear ();

	gUIInitialized				= false;
	gUIReset					= false;
	gHeapInitialized			= false;
	gEvtGetEventCalled			= false;
	gEncoding					= charEncodingUnknown;
	gSysBinarySearchCount		= 0;
	gMemMgrCount				= 0;
	gMemSemaphoreCount			= 0;
	gMemSemaphoreReserveTime	= 0;
	gResizeOrigSize 			= 0;
	gHeapID 					= 0;

	gRememberedHeaps.clear ();

	Patches::SetSwitchApp (0, 0);
	Patches::SetQuitApp (0, 0);

	gLibPatches.clear ();

	// Clear out everything we know about the current applications.

	gCurAppInfo.clear ();
}


/***********************************************************************
 *
 * FUNCTION:	Patches::Save
 *
 * DESCRIPTION:	Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::Save (SessionFile& f)
{
	const long	kCurrentVersion = 3;

	Chunk			chunk;
	EmStreamChunk	s (chunk);

	s << kCurrentVersion;

	s << gUIInitialized;
	s << gHeapInitialized;
	s << gSysBinarySearchCount;

	s << gMemMgrCount;
	s << gMemSemaphoreCount;
	s << gMemSemaphoreReserveTime;
	s << gResizeOrigSize;
	s << gHeapID;

	s << gNextAppCardNo;
	s << gNextAppDbID;
	s << (long) 0;	// was gQuitStage;

	s << (long) gCurAppInfo.size ();
	EmuAppInfoList::iterator	iter1;
	for (iter1 = gCurAppInfo.begin (); iter1 != gCurAppInfo.end (); ++iter1)
	{
		s << iter1->fCmd;
		s << (long) iter1->fDB;
		s << iter1->fCardNo;
		s << iter1->fDBID;
		s << iter1->fMemOwnerID;
		s << iter1->fStackP;
		s << iter1->fStackEndP;
		s << iter1->fStackSize;
		s << iter1->fName;
		s << iter1->fVersion;
	}

//	s << gSysPatchTable;
//	s << gNetLibPatchTable;

//	s << gLibPatches;

	s << (long) gInstalledTailpatches.size ();
	TailpatchTableType::iterator	iter2;
	for (iter2 = gInstalledTailpatches.begin (); iter2 != gInstalledTailpatches.end (); ++iter2)
	{
		s << iter2->fContext.fDestPC1;	// !!! Need to support fDestPC2, too.  But since only fNextPC seems to be used, it doesn't really matter.
		s << iter2->fContext.fExtra;
		s << iter2->fContext.fNextPC;
		s << iter2->fContext.fPC;
		s << iter2->fContext.fTrapIndex;
		s << iter2->fContext.fTrapWord;
		s << iter2->fCount;
//		s << iter2->fTailpatch; // Patched up in ::Load
	}

	s << gLastEvtTrap;
	s << gOSVersion;

	// Added in version 2.

	s << false;	// was: gJapanese

	// Added in version 3.

	s << gEncoding;

	// Added in version 4.

	s << gUIReset;

	f.WritePatchInfo (chunk);
}


/***********************************************************************
 *
 * FUNCTION:	Patches::Load
 *
 * DESCRIPTION:	Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::Load (SessionFile& f)
{
	Chunk	chunk;
	if (f.ReadPatchInfo (chunk))
	{
		long			temp;
		long			version;
		EmStreamChunk	s (chunk);

		s >> version;

		if (version >= 1)
		{
			s >> gUIInitialized;
			s >> gHeapInitialized;
			gEvtGetEventCalled = false;
			s >> gSysBinarySearchCount;

			s >> gMemMgrCount;
			s >> gMemSemaphoreCount;
			s >> gMemSemaphoreReserveTime;
			s >> gResizeOrigSize;
			s >> gHeapID;

			s >> gNextAppCardNo;
			s >> gNextAppDbID;
			s >> temp;		// was gQuitStage;

			gCurAppInfo.clear ();

			long	numApps;
			s >> numApps;

			long	ii;
			for (ii = 0; ii < numApps; ++ii)
			{
				EmuAppInfo	info;

				s >> info.fCmd;
				s >> temp;	info.fDB = (DmOpenRef) temp;
				s >> info.fCardNo;
				s >> info.fDBID;
				s >> info.fMemOwnerID;
				s >> info.fStackP;
				s >> info.fStackEndP;
				s >> info.fStackSize;
				s >> info.fName;
				s >> info.fVersion;

				gCurAppInfo.push_back (info);
			}

			gLibPatches.clear ();

			gInstalledTailpatches.clear ();

			long	numTailpatches;
			s >> numTailpatches;

			for (ii = 0; ii < numTailpatches; ++ii)
			{
				TailpatchType	patch;

				s >> patch.fContext.fDestPC1;	// !!! Need to support fDestPC2, too.  But since only fNextPC seems to be used, it doesn't really matter.
				patch.fContext.fDestPC2 = patch.fContext.fDestPC1;
				s >> patch.fContext.fExtra;
				s >> patch.fContext.fNextPC;
				s >> patch.fContext.fPC;
				s >> patch.fContext.fTrapIndex;
				s >> patch.fContext.fTrapWord;
				s >> patch.fCount;

				// Patch up the tailpatch proc.

				HeadpatchProc	dummy;
				GetPatches (patch.fContext, dummy, patch.fTailpatch);

				gInstalledTailpatches.push_back (patch);
			}

			s >> gLastEvtTrap;
			s >> gOSVersion;
		}

		if (version >= 2)
		{
			bool	dummy;
			s >> dummy;	// was: gJapanese
		}

		if (version >= 3)
		{
			s >> gEncoding;
		}
		else
		{
			gEncoding = charEncodingUnknown;
		}

		if (version >= 4)
		{
			s >> gUIReset;
		}
	}
	else
	{
		f.SetCanReload (false);
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::Dispose
 *
 * DESCRIPTION:	Standard dispose function.	Completely release any
 *				resources acquired or allocated in Initialize and/or
 *				Load.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::Dispose (void)
{
	gUIInitialized				= false;
	gUIReset					= false;
	gHeapInitialized			= false;
	gEvtGetEventCalled			= false;
	gEncoding					= charEncodingUnknown;

	gSysPatchTable.Clear ();
	gNetLibPatchTable.Clear ();

#ifdef SONY_ROM
	gMsfsLibPatchTable.Clear ();
	gSlotDrvLibPatchTable.Clear ();
#endif
}


/***********************************************************************
 *
 * FUNCTION:	Patches::PostLoad
 *
 * DESCRIPTION:	Do some stuff that is normally taken care of during the
 *				process of resetting the device (autoloading
 *				applications, setting the device date, installing the
 *				HotSync user-name, and setting the 'gdbS' feature).
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::PostLoad (void)
{
	if (Patches::UIInitialized ())
	{
		// If we're listening on a socket, install the 'gdbS' feature.	The
		// existance of this feature causes programs written with the prc tools
		// to enter the debugger when they're launched.

		if (Debug::ConnectedToTCPDebugger ())
		{
			FtrSet ('gdbS', 0, 0x12BEEF34);
		}
		else
		{
			FtrUnregister ('gdbS', 0);
		}

		// Install the HotSync user-name.

		// Actually, let's not do that.  From Scott Maxwell:
		//
		//	Would it be possible to save the HotSync user name with each session? This
		//	would be very convenient for working on multiple projects because each
		//	session could have a different user name.
		//
		// To which I said:
		//	I think that what you're seeing is Poser (re-)establishing the user preference
		//	from the Properties/Preferences dialog box after the session is reloaded.  I
		//	could see this way of working as being valuable, too, so I'm not sure which way
		//	to go: keep things the way they are or change them.
		//
		// To which he said:
		//
		//	How about having the preferences dialog grab the name from the Palm RAM?
		//	That way you could easily maintain it per session.
		//
		// Sounds good to me...

//		Preference<string>	userNamePref (kPrefKeyUserName);
//		::SetHotSyncUserName (userNamePref->c_str ());

		CEnableFullAccess	munge;

		if (EmLowMem::TrapExists (sysTrapDlkGetSyncInfo))
		{
			char	userName[dlkUserNameBufSize];
			Err 	err = ::DlkGetSyncInfo (NULL, NULL, NULL, userName, NULL, NULL);
			if (!err)
			{
				Preference<string>	userNamePref (kPrefKeyUserName);
				userNamePref = string (userName);
			}
		}

		// Auto-load any files in the Autoload[Foo] directories.

		::PrvAutoload ();

		// Install the current date.

		::PrvSetCurrentDate ();

		// Wake up any current application so that they can respond
		// to events we pump in at EvtGetEvent time.

		::EvtWakeup ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::GetLibPatchTable
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

const ModulePatchTable* Patches::GetLibPatchTable (uint16 refNum)
{
	string	libName = ::GetLibraryName (refNum);

	if (libName == "Net.lib")
	{
		return &gNetLibPatchTable;
	}

#ifdef SONY_ROM
	else if (libName == MsFsLibName
		  || libName == MsFsLibNameForPEGN700C)
	{
		return &gMsfsLibPatchTable;
	}

	else if (libName == SlotDriverLibName
		  || libName == SlotDriverLibNameForPEGN700C)
	{
		return &gSlotDrvLibPatchTable;
	}
#endif

	return kNoPatchTable;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::HandleSystemCall
 *
 * DESCRIPTION:	If this is a trap we could possibly have head- or
 *				tail-patched, handle those cases.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType Patches::HandleSystemCall (const SystemCallContext& context)
{
	EmAssert (gSession);
	if (gSession->GetNeedPostLoad ())
	{
		gSession->SetNeedPostLoad (false);
		Patches::PostLoad ();
	}

	HeadpatchProc	hp;
	TailpatchProc	tp;
	Patches::GetPatches (context, hp, tp);

	CallROMType handled = Patches::HandlePatches (context, hp, tp);

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::GetPatches
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::GetPatches (	const SystemCallContext& context,
							HeadpatchProc& hp,
							TailpatchProc& tp)
{
	const ModulePatchTable* patchTable = NULL;

#ifdef SONY_ROM
	if (ExpMgrLibHeadpatch::GetExpMgrPatches(context, hp, tp))
		return;
	if (VfsMgrLibHeadpatch::GetVfsMgrPatches(context, hp, tp))
		return;
#endif

	// If this is in the system function range, check our table of
	// system function patches.

	if (::IsSystemTrap (context.fTrapWord))
	{
		patchTable = &gSysPatchTable;
	}

	// Otherwise, see if this is a call to a library function

	else
	{
		if (context.fExtra == kMagicRefNum) // See comments in HtalLibSendReply.
		{
			hp = HtalLibHeadpatch::HtalLibSendReply;
			tp = NULL;
			return;
		}

		if (context.fExtra >= gLibPatches.size ())
		{
			gLibPatches.resize (context.fExtra + 1);
		}

		patchTable = gLibPatches[context.fExtra];

		if (patchTable == NULL)
		{
			patchTable = gLibPatches[context.fExtra] = GetLibPatchTable (context.fExtra);
		}
	}

	// Now that we've got the right patch table for this module, see if
	// that patch table has head- or tailpatches for this function.

	if (patchTable != kNoPatchTable)
	{
		hp = patchTable->GetHeadpatch (context.fTrapIndex);
		tp = patchTable->GetTailpatch (context.fTrapIndex);
	}
	else
	{
		EmAssert (patchTable == kNoPatchTable);

#ifdef SONY_ROM
		if (gSession->GetDevice().GetDeviceType() == kDevicePEGT600 
		 || gSession->GetDevice().GetDeviceType() == kDevicePEGT400) 
		{
			string	libName = ::GetLibraryName (context.fExtra);
			if (libName == SoundLibName || libName == IrcLibName) 
			{
				hp = SysHeadpatch::RomSkip;
			}
		}
		else
		{
			hp = NULL;
		}
#endif
		tp = NULL;
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::HandlePatches
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType Patches::HandlePatches (const SystemCallContext& context,
									HeadpatchProc hp,
									TailpatchProc tp)
{
	CallROMType handled = kExecuteROM;

	// First, see if we have a SysHeadpatch for this function. If so, call
	// it. If it returns true, then that means that the head patch
	// completely handled the function.

	// !!! May have to mess with PC here in case patches do something
	// to enter the debugger.

	if (hp)
	{
		handled = CallHeadpatch (hp);
	}

	// Next, see if there's a SysTailpatch function for this trap. If
	// so, install the TRAP that will cause us to regain control
	// after the trap function has executed.

	if (tp)
	{
		if (handled == kExecuteROM)
		{
			SetupForTailpatch (tp, context);
		}
		else
		{
			CallTailpatch (tp);
		}
	}

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::HandleInstructionBreak
 *
 * DESCRIPTION:	Handle a tail patch, if any is registered for this
 *				memory location.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::HandleInstructionBreak (void)
{
	// Get the address of the tailpatch to call.  May return NULL if
	// there is no tailpatch for this memory location.

	TailpatchProc	tp = RecoverFromTailpatch (m68k_getpc ());

	// Call the tailpatch handler for the trap that just returned.

	CallTailpatch (tp);
}


/***********************************************************************
 *
 * FUNCTION:	Patches::InstallInstructionBreaks
 *
 * DESCRIPTION:	Set the MetaMemory bit that tells the CPU loop to stop
 *				when we get to the desired locations.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::InstallInstructionBreaks (void)
{
	TailpatchTableType::iterator	iter = gInstalledTailpatches.begin ();

	while (iter != gInstalledTailpatches.end ())
	{
		MetaMemory::MarkInstructionBreak (iter->fContext.fNextPC);
		++iter;
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::RemoveInstructionBreaks
 *
 * DESCRIPTION:	Clear the MetaMemory bit that tells the CPU loop to stop
 *				when we get to the desired locations.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::RemoveInstructionBreaks (void)
{
	TailpatchTableType::iterator	iter = gInstalledTailpatches.begin ();

	while (iter != gInstalledTailpatches.end ())
	{
		MetaMemory::UnmarkInstructionBreak (iter->fContext.fNextPC);
		++iter;
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::SetupForTailpatch
 *
 * DESCRIPTION:	Set up the pending TRAP $F call to be tailpatched.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::SetupForTailpatch (TailpatchProc tp, const SystemCallContext& context)
{
	// See if this function is already tailpatched.  If so, merely increment
	// the use-count field.

	TailpatchTableType::iterator	iter = gInstalledTailpatches.begin ();

	while (iter != gInstalledTailpatches.end ())
	{
		if (iter->fContext.fNextPC == context.fNextPC)
		{
			++(iter->fCount);
			return;
		}

		++iter;
	}

	// This function is not already tailpatched, so add a new entry
	// for the the PC/opcode we want to save.

	EmAssert (gSession);
	gSession->RemoveInstructionBreaks ();

	TailpatchType	newTailpatch;

	newTailpatch.fContext	= context;
	newTailpatch.fCount 	= 1;
	newTailpatch.fTailpatch = tp;

	gInstalledTailpatches.push_back (newTailpatch);

	gSession->InstallInstructionBreaks ();
}


/***********************************************************************
 *
 * FUNCTION:	Patches::RecoverFromTailpatch
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

TailpatchProc Patches::RecoverFromTailpatch (emuptr startPC)
{
	// Get the current PC so that we can find the record for this tailpatch.

	emuptr patchPC = startPC;

	// Find the PC.

	TailpatchTableType::iterator	iter = gInstalledTailpatches.begin ();

	while (iter != gInstalledTailpatches.end ())
	{
		if (iter->fContext.fNextPC == patchPC)
		{
			TailpatchProc	result = iter->fTailpatch;

			// Decrement the use-count.  If it reaches zero, remove the
			// patch from our list.

			if (--(iter->fCount) == 0)
			{
				EmAssert (gSession);
				gSession->RemoveInstructionBreaks ();

				gInstalledTailpatches.erase (iter);

				gSession->InstallInstructionBreaks ();
			}

			return result;
		}

		++iter;
	}

	return NULL;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::CallHeadpatch
 *
 * DESCRIPTION:	If the given system function is head patched, then call
 *				the headpatch.	Return "handled" (which means whether
 *				or not to call the ROM function after this one).
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType Patches::CallHeadpatch (HeadpatchProc hp)
{
	CallROMType handled = kExecuteROM;

	if (hp)
	{
		if (hp != &SysHeadpatch::HostControl)
		{
			// Stop all profiling activities. Stop cycle counting and stop the
			// recording of function entries and exits.  We want our trap patches
			// to be as transparent as possible.

			StDisableAllProfiling	stopper;

			handled = hp ();
		}
		else
		{
			handled = hp ();
		}
	}

	return handled;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::CallTailpatch
 *
 * DESCRIPTION:	If the given function is tail patched, then call the
 *				tailpatch.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::CallTailpatch (TailpatchProc tp)
{
	if (tp)
	{
		// Stop all profiling activities. Stop cycle counting and stop the
		// recording of function entries and exits.  We want our trap patches
		// to be as transparent as possible.

		StDisableAllProfiling	stopper;

		tp ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::OSVersion
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

UInt32 Patches::OSVersion (void)
{
	EmAssert (gOSVersion != kOSUndeterminedVersion);

	return gOSVersion;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::OSMajorMinorVersion
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

UInt32 Patches::OSMajorMinorVersion (void)
{
	return OSMajorVersion () * 10 + OSMinorVersion ();
}


/***********************************************************************
 *
 * FUNCTION:	Patches::OSMajorVersion
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

UInt32 Patches::OSMajorVersion (void)
{
	EmAssert (gOSVersion != kOSUndeterminedVersion);

	return sysGetROMVerMajor (gOSVersion);
}


/***********************************************************************
 *
 * FUNCTION:	Patches::OSMinorVersion
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

UInt32 Patches::OSMinorVersion (void)
{
	EmAssert (gOSVersion != kOSUndeterminedVersion);

	return sysGetROMVerMinor (gOSVersion);
}


/***********************************************************************
 *
 * FUNCTION:	SetSwitchApp
 *
 * DESCRIPTION:	Sets an application or launchable document to switch to
 *				the next time the system can manage it.
 *
 * PARAMETERS:	cardNo - the card number of the app to switch to.
 *
 *				dbID - the database id of the app to switch to.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::SetSwitchApp (UInt16 cardNo, LocalID dbID)
{
	gNextAppCardNo = cardNo;
	gNextAppDbID = dbID;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::SwitchToApp
 *
 * DESCRIPTION:	Switches to the given application or launchable document.
 *
 * PARAMETERS:	cardNo - the card number of the app to switch to.
 *
 *				dbID - the database id of the app to switch to.
 *
 * RETURNED:	Err number of any errors that occur
 *
 ***********************************************************************/

Err Patches::SwitchToApp (UInt16 cardNo, LocalID dbID)
{
	UInt16	dbAttrs;
	UInt32	type, creator;

	Err err = ::DmDatabaseInfo (
				cardNo,
				dbID,
				NULL,				/*name*/
				&dbAttrs,
				NULL,				/*version*/
				NULL,				/*create date*/
				NULL,				/*modDate*/
				NULL,				/*backup date*/
				NULL,				/*modNum*/
				NULL,				/*appInfoID*/
				NULL,				/*sortInfoID*/
				&type,
				&creator);

	if (err)
		return err;

	//---------------------------------------------------------------------
	// If this is an executable, call SysUIAppSwitch
	//---------------------------------------------------------------------
	if (::IsExecutable (type, creator, dbAttrs))
	{
		err = ::SysUIAppSwitch (cardNo, dbID,
						sysAppLaunchCmdNormalLaunch, NULL);

		if (err)
			return err;
	}

	//---------------------------------------------------------------------
	// else, this must be a launchable data database. Find it's owner app
	//	and launch it with a pointer to the data database name.
	//---------------------------------------------------------------------
	else
	{
		DmSearchStateType	searchState;
		UInt16				appCardNo;
		LocalID 			appDbID;

		err = ::DmGetNextDatabaseByTypeCreator (true, &searchState,
						sysFileTApplication, creator,
						true, &appCardNo, &appDbID);
		if (err)
			return err;

		// Create the param block

		emuptr cmdPBP = (emuptr) ::MemPtrNew (sizeof (SysAppLaunchCmdOpenDBType));
		if (cmdPBP == EmMemNULL)
			return memErrNotEnoughSpace;

		// Fill it in

		::MemPtrSetOwner ((MemPtr) cmdPBP, 0);
		EmMemPut16 (cmdPBP + offsetof (SysAppLaunchCmdOpenDBType, cardNo), cardNo);
		EmMemPut32 (cmdPBP + offsetof (SysAppLaunchCmdOpenDBType, dbID), dbID);

		// Switch now

		err = ::SysUIAppSwitch (appCardNo, appDbID, sysAppLaunchCmdOpenDB, (MemPtr) cmdPBP);
		if (err)
			return err;
	}

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::SetQuitApp
 *
 * DESCRIPTION:	Tells our patching mechanisms that the emulator should
 *				quit once the given application quits.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::SetQuitApp (UInt16 cardNo, LocalID dbID)
{
	gTimeToQuit		= false;
	gQuitAppCardNo	= cardNo;
	gQuitAppDbID	= dbID;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::TimeToQuit
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::TimeToQuit (void)
{
	// Clear out the flag when returning it.  If not, then if the
	// emulator presents a "Save this session?" dialog in response
	// to quitting, the dialog will run the idle loop again, which
	// will cause this function to be called again, and we don't
	// want it to return true again.

	Bool	result = gTimeToQuit;
	gTimeToQuit = false;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::PuppetString
 *
 * DESCRIPTION:	Puppet stringing function for inserting events into
 *				the system.  We want to insert events when:
 *
 *				- Gremlins is running
 *				- The user types characters
 *				- The user clicks with the mouse
 *				- We need to trigger a switch another application
 *
 *				This function is called from headpatches to
 *				SysEvGroupWait and SysSemaphoreWait.  When the Palm OS
 *				needs an event, it calls EvtGetEvent.  EvtGetEvent
 *				looks in all the usual places for events to return. If
 *				it doesn't find any, it puts the system to sleep by
 *				calling SysEvGroupWait (or SysSemaphoreWait on 1.0
 *				systems).  SysEvGroupWait will wake up and return when
 *				an event is posted via something like EvtEnqueuePenPoint,
 *				EvtEnqueueKey, or KeyHandleInterrupt.
 *
 *				To puppet-string Palm OS, we therefore patch those
 *				functions and post events, preventing them from actually
 *				going to sleep.
 *
 * PARAMETERS:	callROM - return here whether or not the original ROM
 *					function still needs to be called.	Normally, the
 *					answer is "yes".
 *
 *				clearTimeout - set to true if the "timeout" parameter
 *					of the function we've patched needs to be prevented
 *					from being "infinite".
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void PrvForceNilEvent (void)
{
	// No event was posted.  What we'd like right now is to force
	// EvtGetEvent to return a nil event.  We can do that by returning
	// a non-zero result code from SysEvGroupWait.  EvtGetEvent doesn't
	// look too closely at the result, but let's try to be as close to
	// reality as possible. SysEvGroupWait currently returns "4"
	// (CJ_WRTMOUT) to indicate a timeout condition.  It should
	// probably get turned into sysErrTimeout somewhere along the way,
	// but that translation doesn't seem to occur.

	m68k_dreg(regs, 0) = 4;
}

void Patches::PuppetString (CallROMType& callROM, Bool& clearTimeout)
{
	callROM = kExecuteROM;
	clearTimeout = false;

	// Set the return value (Err) to zero in case we return
	// "true" (saying that we handled the trap).

	m68k_dreg (regs, 0) = 0;

	// If the low-memory global "idle" is true, then we're being
	// called from EvtGetEvent or EvtGetPen, in which case we
	// need to check if we need to post some events.

	if (EmLowMem::GetEvtMgrIdle ())
	{
		// If there's an RPC request waiting for a nilEvent,
		// let it know that it happened.

		if (gLastEvtTrap == sysTrapEvtGetEvent)
		{
			RPC::SignalWaiters (hostSignalIdle);
		}

		// If we're in the middle of calling a Palm OS function ourself,
		// and we are somehow at the point where the system is about to
		// doze, then just return now.  Don't let it doze!  Interrupts are
		// off, and HwrDoze will never return!

		if (gSession->IsNested ())
		{
			::PrvForceNilEvent();
			callROM = kSkipROM;
			return;
		}

		EmAssert (gSession);
		if (gSession->GetBreakOnIdle ())
		{
			gSession->ScheduleSuspendIdle ();
		}

		else if (Hordes::IsOn ())
		{
			if (gLastEvtTrap == sysTrapEvtGetEvent)
			{
				if (!Hordes::PostFakeEvent ())
				{
					if (LogEnqueuedEvents ())
					{
						LogAppendMsg ("Hordes::PostFakeEvent did not post an event.");
					}

					::PrvForceNilEvent();
					callROM = kSkipROM;
					return;
				}
			}
			else if (gLastEvtTrap == sysTrapEvtGetPen)
			{
				Hordes::PostFakePenEvent ();
			}
			else
			{
				if (LogEnqueuedEvents ())
				{
					LogAppendMsg ("Last event was 0x%04X, so not posting event.", gLastEvtTrap);
				}
			}

			// Never let the timeout be infinite.  If the above event-posting
			// attempts failed (which could happen, for instance, if we attempted
			// to post a pen event with the same coordinates as the previous
			// pen event), we'd end up waiting forever.

			clearTimeout = true;
		}

		// Gremlins aren't on; let's see if the user has typed some
		// keys that we need to pass on to the Palm device.

		else if (gSession->HasKeyEvent ())
		{
			EmKeyEvent	event = gSession->GetKeyEvent ();

			UInt16	modifiers = 0;

			if (event.fShiftDown)
				modifiers |= shiftKeyMask;

			if (event.fCapsLockDown)
				modifiers |= capsLockMask;

			if (event.fNumLockDown)
				modifiers |= numLockMask;

			// We don't really want to set this one.  commandKeyMask
			// means something special in Palm OS
//			if (event.fCommandDown)
//				modifiers |= commandKeyMask;

			if (event.fOptionDown)
				modifiers |= optionKeyMask;

			if (event.fControlDown)
				modifiers |= controlKeyMask;

			if (event.fAltDown)
				modifiers |= 0;	// no bit defined for this

			if (event.fWindowsDown)
				modifiers |= 0;	// no bit defined for this

			::StubAppEnqueueKey (event.fKey, 0, modifiers);
		}

		// No key events, let's see if there are pen events.

		else if (gSession->HasPenEvent ())
		{
			EmPoint		pen (-1, -1);
			EmPenEvent	event = gSession->GetPenEvent ();
			if (event.fPenIsDown)
			{
				pen = event.fPenPoint;
			}

			PointType	palmPen = pen;
			StubAppEnqueuePt (&palmPen);
		}

		// E. None of the above.  Let's see if there's an app
		//	  we're itching to switch to.

		else if (gNextAppDbID != 0)
		{
			/*Err err =*/ SwitchToApp (gNextAppCardNo, gNextAppDbID);

			gNextAppCardNo = 0;
			gNextAppDbID = 0;

			clearTimeout = true;
		}
	}
	else
	{
		if (Hordes::IsOn () && LogEnqueuedEvents ())
		{
			LogAppendMsg ("Event Manager not idle, so not posting an event.");
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	Patches::HasWellBehavedMemSemaphoreUsage
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::HasWellBehavedMemSemaphoreUsage (void)
{
	// Palm OS 3.0 and later should not be holding the memory manager
	// semaphore for longer than 1 minute.	I imagine that older ROMs
	// don't hold the semaphore for longer than this, but Roger still
	// suggested testing for 3.0.

	return gOSVersion != kOSUndeterminedVersion && OSMajorMinorVersion () >= 30;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::EnterMemMgr
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::EnterMemMgr (const char* fnName)
{
	UNUSED_PARAM(fnName)

	++gMemMgrCount;
	EmAssert (gMemMgrCount < 10);
}


/***********************************************************************
 *
 * FUNCTION:	Patches::ExitMemMgr
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::ExitMemMgr (const char* fnName)
{
	UNUSED_PARAM(fnName)

	--gMemMgrCount;
	EmAssert (gMemMgrCount >= 0);

#if 0	// _DEBUG
	if (gMemMgrCount == 0)
	{
		EmPalmHeap::ValidateAllHeaps ();
	}
#endif
}


/***********************************************************************
 *
 * FUNCTION:	Patches::IsInSysBinarySearch
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::IsInSysBinarySearch (void)
{
	return gSysBinarySearchCount > 0;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::EnterSysBinarySearch
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::EnterSysBinarySearch (void)
{
	EmAssert (gSysBinarySearchCount < 10);
	++gSysBinarySearchCount;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::ExitSysBinarySearch
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::ExitSysBinarySearch (void)
{
	--gSysBinarySearchCount;
	EmAssert (gSysBinarySearchCount >= 0);
}


/***********************************************************************
 *
 * FUNCTION:	Patches::UIInitialized
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::UIInitialized (void)
{
	return gUIInitialized;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::SetUIInitialized
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::SetUIInitialized (Bool b)
{
	gUIInitialized = b != 0;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::UIReset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::UIReset (void)
{
	return gUIReset;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::SetUIReset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::SetUIReset (Bool b)
{
	gUIReset = b != 0;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::HeapInitialized
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::HeapInitialized (void)
{
	return gHeapInitialized;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::SetHeapInitialized
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Patches::SetHeapInitialized (Bool b)
{
	gHeapInitialized = b != 0;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::AutoAcceptBeamDialogs
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::AutoAcceptBeamDialogs (Bool b)
{
	Bool	oldState = gAutoAcceptBeamDialogs;
	gAutoAcceptBeamDialogs = b;
	return oldState;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::TurningJapanese
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::TurningJapanese (void)
{
	return gEncoding == charEncodingPalmSJIS;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::EvtGetEventCalled
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool Patches::EvtGetEventCalled (void)
{
	return gEvtGetEventCalled;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::CollectCurrentAppInfo
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

EmuAppInfo Patches::CollectCurrentAppInfo (emuptr appInfoP)
{
	EmuAppInfo	newAppInfo;
	memset (&newAppInfo, 0, sizeof (newAppInfo));

	// Scarf some information out of the app info block.

	newAppInfo.fDB			= (DmOpenRef) EmMemGet32 (appInfoP + offsetof (SysAppInfoType, dbP));
	newAppInfo.fStackP		= EmMemGet32 (appInfoP + offsetof (SysAppInfoType, stackP));
	newAppInfo.fMemOwnerID	= EmMemGet16 (appInfoP + offsetof (SysAppInfoType, memOwnerID));

	// Determine the current stack range.  Under Palm OS 3.0 and later, this information
	// is in the DatabaseInfo block.  Under earlier OSes, we only get the low-end of the stack
	// (that is, the address that was returned by MemPtrNew).  To get the high-end of
	// the stack, assume that stackP pointed to a chunk of memory allocated by MemPtrNew
	// and call MemPtrSize.

	if (DatabaseInfoHasStackInfo ())
	{
		if (newAppInfo.fStackP)
		{
			UInt32	stackSize = ::MemPtrSize ((MemPtr) newAppInfo.fStackP);
			if (stackSize)
			{
				newAppInfo.fStackEndP = newAppInfo.fStackP + stackSize;
			}
			else
			{
				newAppInfo.fStackEndP = EmMemNULL;
			}
		}
	}
	else
	{
		newAppInfo.fStackEndP = EmMemGet32 (appInfoP + offsetof (SysAppInfoType, stackEndP));
	}

	newAppInfo.fStackSize = newAppInfo.fStackEndP - newAppInfo.fStackP;

	// Remember the current application name and version information.  We use
	// this information when telling users that something has gone haywire.  Collect
	// this information now instead of later (on demand) as we can't be sure that
	// we can make the necessary DataMgr calls after an error occurs.
	//
	// If the database has a 'tAIN' resource, get the name from there.
	// Otherwise, use the database name.
	//
	// (Write the name into a temporary local variable.  The local variable is
	// on the stack, which will get "mapped" into the emulated address space so
	// that the emulated DmDatabaseInfo can get to it.)

	UInt16	cardNo;
	LocalID dbID;

	Err err = ::DmOpenDatabaseInfo (newAppInfo.fDB, &dbID, NULL, NULL, &cardNo, NULL);
	if (err)
		return newAppInfo;

	newAppInfo.fCardNo = cardNo;
	newAppInfo.fDBID = dbID;

	char	appName[dmDBNameLength] = {0};
	char	appVersion[256] = {0};	// <gulp> I hope this is big enough...

//	DmOpenRef	dbP = DmOpenDatabase (cardNo, dbID, dmModeReadOnly);

//	if (dbP)
	{
		MemHandle	strH;

		// Get the app name from the 'tAIN' resource.

		strH = ::DmGet1Resource (ainRsc, ainID);
		if (strH)
		{
			emuptr strP = (emuptr) ::MemHandleLock (strH);
			uae_strcpy (appName, strP);
			::MemHandleUnlock (strH);
			::DmReleaseResource (strH);
		}

		// Get the version from the 'tver' resource, using ID's 1 and 1000

		strH = ::DmGet1Resource (verRsc, appVersionID);
		if (strH == NULL)
			strH = ::DmGet1Resource (verRsc, appVersionAlternateID);
		if (strH)
		{
			emuptr strP = (emuptr) ::MemHandleLock (strH);
			uae_strcpy (appVersion, strP);
			::MemHandleUnlock (strH);
			::DmReleaseResource (strH);
		}

//		::DmCloseDatabase (dbP);
	}

	if (appName[0] == 0)	// No 'tAIN' resource, so use database name
	{
		::DmDatabaseInfo (cardNo, dbID,
						appName,
						NULL, NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL, NULL);
	}

	// Copy the strings from the stack to their permanent homes.

	strcpy (newAppInfo.fName, appName);
	strcpy (newAppInfo.fVersion, appVersion);

	return newAppInfo;
}

/***********************************************************************
 *
 * FUNCTION:	Patches::GetCurrentAppInfo
 *
 * DESCRIPTION:	Return information on the last application launched
 *				with SysAppLaunch (and that hasn't exited yet).
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/


EmuAppInfo	Patches::GetCurrentAppInfo		(void)
{
	EmuAppInfo	result;
	memset (&result, 0, sizeof (result));

	if (gCurAppInfo.size () > 0)
		result = *(gCurAppInfo.rbegin ());

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Patches::GetRootAppInfo
 *
 * DESCRIPTION:	Return information on the last application launched
 *				with SysAppLaunch and with the launch code of
 *				sysAppLaunchCmdNormalLaunch (and that hasn't exited yet).
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

EmuAppInfo	Patches::GetRootAppInfo 		(void)
{
	EmuAppInfo	result;
	memset (&result, 0, sizeof (result));

	EmuAppInfoList::reverse_iterator iter = gCurAppInfo.rbegin ();

	while (iter != gCurAppInfo.rend ())
	{
		if ((*iter).fCmd == sysAppLaunchCmdNormalLaunch)
		{
			result = *iter;
			break;
		}

		++iter;
	}

	return result;
}

#pragma mark -

// ===========================================================================
//		¥ SysHeadpatch
// ===========================================================================

/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::RecordTrapNumber
 *
 * DESCRIPTION:	Record the trap we're executing for our patch to
 *				SysEvGroupWait later.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::RecordTrapNumber (void)
{
	struct StackFrame
	{
	};

	uint8* realMem = EmMemGetRealAddress (m68k_getpc ());

	EmAssert (EmMemDoGet16 (realMem - 2) == (m68kTrapInstr + sysDispatchTrapNum));

	gLastEvtTrap = EmMemDoGet16 (realMem);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::UnmarkUIObjects
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::UnmarkUIObjects (void)
{
	MetaMemory::UnmarkUIObjects ();

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::ClipboardGetItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::ClipboardGetItem (void)
{
	if (!Hordes::IsOn () && !gDontPatchClipboardGetItem)
	{
		::PrvCopyHostClipboardToPalm ();
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::DbgMessage
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::DbgMessage (void)
{
	// void DbgMessage(const Char* aStr);

	struct StackFrame
	{
		const Char* aStr;
	};

	emuptr msg = GET_PARAMETER (aStr);

	if (msg)
	{
		string		msgCopy				= ::PrvToString (msg);
		CSocket*	debuggerSocket		= Debug::GetDebuggerSocket ();
		Bool		contactedDebugger	= false;
		
		if (debuggerSocket)
		{
			SLP		slp (debuggerSocket);
			ErrCode	err = SystemPacket::SendMessage (slp, msgCopy.c_str ());
			if (!err)
			{
				contactedDebugger = true;
			}
		}

		if (!contactedDebugger)
		{
			// Squelch these debugger messages.  Some functions in the ROM
			// (see Content.c and Progress.c) use DbgMessage for logging
			// instead of debugging.  I'm hoping this will get fixed after
			// Palm OS 4.0, so I'm only checking in versions before that.
			//
			// ("Transaction finished" actually appears only in 3.2 and
			//  3.3 ROMs, so that one's already taken care of.)
			//
			// Also note that I have the last character as "\x0A".  In
			// the ROM sources, this is "\n".  However, the value that
			// that turns into is compiler-dependent, so I'm using the
			// value that the Palm compiler is currently known to use.

			if ((msgCopy != "...Transaction cancelled\x0A" &&
				 msgCopy != "Transaction finished\x0A" &&
				 msgCopy != "c") ||
				Patches::OSMajorMinorVersion () > 40)
			{
				Errors::ReportErrDbgMessage (msgCopy.c_str ());
			}
		}
	}

	return kSkipROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::DmInit
 *
 * DESCRIPTION:	After MemInit is called, we need to sync up with the
 *				initial state of the heap(s).  However, MemInit is not
 *				called via the trap table, so we can't easily tailpatch
 *				it.  DmInit is the first such function called after
 *				MemInit, so we headpatch *it* instead of tailpatching
 *				MemInit.
 *
 *				(Actually, MemHeapCompact is called as one of the last
 *				 things MemInit does which makes it an interesting
 *				 candidate for patching in order to sync up with the
 *				 heap state.  However, if we were to do a full sync on
 *				 that call, a full sync would occur on *every* call to
 *				 MemHeapCompact, which we don't really want to do.)
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::DmInit (void)
{
//	MetaMemory::SyncAllHeaps ();

	Patches::SetHeapInitialized (true);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::ErrDisplayFileLineMsg
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

typedef Bool	(*VersionChecker)(void);
typedef Bool	(*FunctionChecker)(emuptr addr);

struct EmInvalidErrorMessageData
{
	VersionChecker	fVersionChecker;
	FunctionChecker	fFunctionChecker;
	const char*		fMessage;
};

EmInvalidErrorMessageData	fInvalidErrorMessages [] =
{
	{
		&Patches::HasECValidateFieldBug,
		&::InECValidateField,
		"Invalid insertion point position"
	},
	{
		&Patches::HasPrvDrawSliderControlBug,
		&::InPrvDrawSliderControl,
		"Background must be at least half as wide as slider."
	}
};


CallROMType SysHeadpatch::ErrDisplayFileLineMsg (void)
{
	// void ErrDisplayFileLineMsg(CharPtr filename, UInt16 lineno, CharPtr msg)

	struct StackFrame
	{
		Char*		filename;
		UInt16		lineno;
		Char*		msg;
	};

//	emuptr	filenameP	= GET_PARAMETER (filename);
//	UInt16	lineno		= GET_PARAMETER (lineno);
	emuptr	msgP		= GET_PARAMETER (msg);

	{
		CEnableFullAccess	munge;	// Remove blocks on memory access.

		// Force this guy to true.	If it's false, ErrDisplayFileLineMsg will
		// just try to enter the debugger.

		UInt16	sysMiscFlags = EmLowMem_GetGlobal (sysMiscFlags);
		EmLowMem_SetGlobal (sysMiscFlags, sysMiscFlags | sysMiscFlagUIInitialized);

		// Clear this low-memory flag so that we force the dialog to display.
		// If this flag is true, ErrDisplayFileLineMsg will just try to enter
		// the debugger.

		EmLowMem_SetGlobal (dbgWasEntered, false);
	}

	// Some ROMs incorrectly display error messages via ErrDisplayFileLineMsg.
	// Check to see if we are running on a ROM version with one of those
	// erroneous messages, check to see that we are in the function that
	// displays the message, and check to see if the text of the message
	// handed to us is one of the incorrect ones.  If all conditions are
	// true, squelch the message.

	for (size_t ii = 0; ii < countof (fInvalidErrorMessages); ++ii)
	{
		EmInvalidErrorMessageData*	d = &fInvalidErrorMessages[ii];
		if (d->fVersionChecker() && d->fFunctionChecker (m68k_getpc ()))
		{
			string	msg = ::PrvToString (msgP);
			if (msg == d->fMessage)
			{
				return kSkipROM;
			}
		}
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::EvtAddEventToQueue
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::EvtAddEventToQueue (void)
{
	// void EvtAddEventToQueue (const EventPtr event)

	struct StackFrame
	{
		const EventPtr	event;
	};

	emuptr event	= GET_PARAMETER (event);

	LogEvtAddEventToQueue (event);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::EvtAddUniqueEventToQueue
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::EvtAddUniqueEventToQueue (void)
{
	// void EvtAddUniqueEventToQueue(const EventPtr eventP, const UInt32 id, const Boolean inPlace)

	struct StackFrame
	{
		const EventPtr	event;
		const UInt32 	id;
		const Boolean	inPlace;
	};

	emuptr event	= GET_PARAMETER (event);
	UInt32	id		= GET_PARAMETER (id);
	Boolean inPlace = GET_PARAMETER (inPlace);

	LogEvtAddUniqueEventToQueue (event, id, inPlace);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::EvtEnqueueKey
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::EvtEnqueueKey (void)
{
	// Err EvtEnqueueKey(UInt16 ascii, UInt16 keycode, UInt16 modifiers)

	struct StackFrame
	{
		UInt16	ascii;
		UInt16	keycode;
		UInt16	modifiers;
	};

	UInt16	ascii		= GET_PARAMETER (ascii);
	UInt16	keycode 	= GET_PARAMETER (keycode);
	UInt16	modifiers	= GET_PARAMETER (modifiers);

	LogEvtEnqueueKey (ascii, keycode, modifiers);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::EvtEnqueuePenPoint
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::EvtEnqueuePenPoint (void)
{
	// Err EvtEnqueuePenPoint(PointType* ptP)

	struct StackFrame
	{
		PointType*	ptP;
	};

	emuptr ptP 	= GET_PARAMETER (ptP);

	LogEvtEnqueuePenPoint (ptP);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::ExgDoDialog
 *
 * DESCRIPTION:	Always accept the beam if we're beaming it in via our
 *				special ExgMgr "driver".
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::ExgDoDialog (void)
{
	// Boolean ExgDoDialog(ExgSocketPtr socketP, ExgDialogInfoType *infoP, Err *errP)

	struct StackFrame
	{
		emuptr	socketP;
		emuptr	infoP;
		emuptr	errP;
	};

	if (!gAutoAcceptBeamDialogs)
		return kExecuteROM;

	emuptr	socketP = GET_PARAMETER (socketP);
//	emuptr	infoP	= GET_PARAMETER (infoP);
	emuptr	errP	= GET_PARAMETER (errP);

	if (!socketP)
		return kExecuteROM;

	EmAliasErr<PAS>	err (errP);
	err = 0;

	m68k_dreg (regs, 0) = 1;

	return kSkipROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::FrmCustomAlert
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

#define replaceBeamAppAlert					2008
#define beamReplaceErrorAppIsRunning		2012

#define replaceBeamAppAlertYesButton		0

CallROMType SysHeadpatch::FrmCustomAlert (void)
{
	// UInt16 FrmCustomAlert (UInt16 alertId, const Char* s1, const Char* s2, const Char* s3)

	struct StackFrame
	{
		UInt16		alertId;
		emuptr		s1;
		emuptr		s2;
		emuptr		s3;
	};

#ifdef SONY_ROM
	emuptr unimplementedAddress	= EmLowMem::GetTrapAddress (sysTrapSysUnimplemented);
	emuptr result				= EmLowMem::GetTrapAddress (sysTrapFrmCustomAlert);

	if (unimplementedAddress == result)
		return kSkipROM;
#endif

	if (!gAutoAcceptBeamDialogs)
		return kExecuteROM;

	UInt16	alertId = GET_PARAMETER (alertId);

	// "App already exists, replace it?"

	if (alertId == replaceBeamAppAlert)
	{
		m68k_dreg (regs, 0) = replaceBeamAppAlertYesButton;
		return kSkipROM;
	}

	// "App is running. Switch first."

	if (alertId == beamReplaceErrorAppIsRunning)
	{
		m68k_dreg (regs, 0) = 0;	// Can be anything.
		return kSkipROM;
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::FrmDrawForm
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::FrmDrawForm (void)
{
	// void FrmDrawForm (const FormPtr frm)

	struct StackFrame
	{
		FormPtr	frm;
	};

	FormPtr frm 	= (FormPtr) GET_PARAMETER (frm);

	vector<UInt16>	okObjects;
	::CollectOKObjects (frm, okObjects, true);

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::HostControl
 *
 * DESCRIPTION:	This one's kind of odd.  Originally, there was
 *				SysGremlins, which was declared as follows:
 *
 *					UInt32 SysGremlins(GremlinFunctionType selector,
 *										GremlinParamsType *params)
 *
 *				Also originally, the only defined selector was
 *				GremlinIsOn.
 *
 *				Now, SysGremlins is extended to be SysHostControl,
 *				which allows the Palm environment to access host
 *				functions if it's actually running under the simulator
 *				or emulator.
 *
 *				Because of this extension, functions implemented via
 *				this trap are not limited to pushing a selector and
 *				parameter block on the stack. Now, they will all push
 *				on a selector, but what comes after is dependent on the
 *				selector.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::HostControl (void)
{
	return HandleHostControlCall ();
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::HwrBatteryLevel
 *
 * DESCRIPTION:	Return that the battery is always full.  HwrBatteryLevel
 *				is the bottleneck function called to determine the
 *				battery level.	By patching it this way, we don't have
 *				to emulate the hardware registers that report the
 *				battery level.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::HwrBatteryLevel (void)
{
	// On non-328 devices, we emulate the hardware that returns this
	// information, so we don't need to patch this function.

	// We don't currently emulate the Prism battery hardware, so we
	// allow the patch. Not sure if this is called by the Prism ROM,
	// but we patch it to be safe (better than dividing by zero).

	EmAssert (gSession);

#ifdef SONY_ROM
	if (!gSession->GetDevice().Supports68328 () 
	  && gSession->GetDevice().HardwareID () != 0x0a /*halModelIDVisorPrism*/
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS320
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS360
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGT400
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGT600
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGN600C
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGN700C
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS500C
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS300)
	{
		return kExecuteROM;
	}
#else 
	if (!gSession->GetDevice().Supports68328 () &&
		gSession->GetDevice().HardwareID () != 0x0a /*halModelIDVisorPrism*/)
	{
		return kExecuteROM;
	}
#endif
	
	// UInt16 HwrBatteryLevel(void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = 255;	// Hardcode a maximum level

	return kSkipROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::HwrBattery
 *
 * DESCRIPTION:	If queried for the battery level, return that the battery
 *				is always full.  This is equivalent to the HwrBatteryLevel
 *				patch, necessary because some newer devices call HwrBattery
 *				for this information. 
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 * History: 
 *   2000-05-10 Added by Bob Petersen 
 *
 ***********************************************************************/

CallROMType SysHeadpatch::HwrBattery (void)
{
	// On non-328 devices, we emulate the hardware that returns this
	// information, so we don't need to patch this function.

	// We don't currently emulate the Prism battery hardware, so we
	// allow the patch.

	EmAssert (gSession);

#ifdef SONY_ROM
	if (!gSession->GetDevice().Supports68328 () 
	  && gSession->GetDevice().HardwareID () != 0x0a /*halModelIDVisorPrism*/
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS320
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS360
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGT400
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGT600
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGN600C
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGN700C
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS500C
	  && gSession->GetDevice().GetDeviceType() != kDevicePEGS300)
	{
		return kExecuteROM;
	}
#else 
	if (!gSession->GetDevice().Supports68328 () &&
		gSession->GetDevice().HardwareID () != 0x0a /*halModelIDVisorPrism*/)
	{
		return kExecuteROM;
	}
#endif

	struct StackFrame
	{
		UInt16	cmd; /* HwrBatteryCmdEnum */
		void*	readParms;
	};

	UInt16 cmd = (UInt16) GET_PARAMETER (cmd);

	if (cmd == 2 /* hwrBatteryCmdMainRead */)
	{
		struct HwrBatCmdReadType	// from HwrBattery.h
		{
			UInt16	mVolts;	// level in millivolts (2500 = 2.5 volts)
			UInt16	abs;	// absolute level (0 -> 255)
		};

		emuptr	readParms = (emuptr) GET_PARAMETER (readParms);

		if (gSession->GetDevice().HardwareID () == 0x0a /*halModelIDVisorPrism*/)
		{
			// DOLATER: Battery voltage should differ by battery type.
			// 4.2 is returned to handle lithium ion batteries.  'mVolts'
			// is the value used by the Prism display, not 'abs'. 
			EmMemPut16 (readParms + offsetof (HwrBatCmdReadType, mVolts), (UInt16) 4200);	// 4.2 volts
			EmMemPut16 (readParms + offsetof (HwrBatCmdReadType, mVolts), (UInt16) 4000);	// 4.0 volts
		}
		else
		{
			EmMemPut16 (readParms + offsetof (HwrBatCmdReadType, mVolts), (UInt16) 2500);	// 2.5 volts
		}
		EmMemPut16 (readParms + offsetof (HwrBatCmdReadType, abs), (UInt16) 255);		// Hardcode a maximum level

		m68k_dreg (regs, 0) = errNone;
		return kSkipROM;
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::HwrDockStatus
 *
 * DESCRIPTION:	Always return hwrDockStatusUsingExternalPower.  We
 *				could fake this out by twiddling the right bits in the
 *				Dragonball and DragonballEZ emulation units, but those
 *				bits are different for almost every device.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::HwrDockStatus (void)
{
	// On non-328 devices, we emulate the hardware that returns this
	// information, so we don't need to patch this function.

	EmAssert (gSession);

	if (!gSession->GetDevice().Supports68328 ())
	{
		return kExecuteROM;
	}

	// hwrDockStatusState HwrDockStatus(void)
	//	(added in Palm OS 3.1)
	//	(changed later to return UInt16)

	struct StackFrame
	{
	};

	// Old enumerated values from Hardware.h:
	//
	//		DockStatusNotDocked = 0,
	//		DockStatusInModem,
	//		DockStatusInCharger,
	//		DockStatusUnknown = 0xFF

	// New defines from HwrDock.h
#define	hwrDockStatusUndocked			0x0000	// nothing is attached
#define	hwrDockStatusModemAttached		0x0001	// some type of modem is attached
#define	hwrDockStatusDockAttached		0x0002	// some type of dock is attached
#define	hwrDockStatusUsingExternalPower	0x0004	// using some type of external power source
#define	hwrDockStatusCharging			0x0008	// internal power cells are recharging

	m68k_dreg (regs, 0) = hwrDockStatusUsingExternalPower;

	return kSkipROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::HwrGetROMToken
 *
 * DESCRIPTION:	Patch this guy so that we never return the 'irda' token.
 *				We should take this out when some sort of IR support is
 *				added.
 *
 *				NOTE: This patch is useless for diverting the ROM. It
 *				calls HwrGetROMToken directly, which means that it will
 *				bypass this patch.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::HwrGetROMToken (void)
{
	//	Err HwrGetROMToken (UInt16 cardNo, UInt32 token, UInt8** dataP, UInt16* sizeP)

	struct StackFrame
	{
		UInt16 cardNo;
		UInt32 token;
		UInt8** dataP;
		UInt16* sizeP;
	};

	UInt16	cardNo = GET_PARAMETER (cardNo);
	UInt32	token = GET_PARAMETER (token);
	emuptr dataP = GET_PARAMETER (dataP);
	emuptr sizeP = GET_PARAMETER (sizeP);

	if (cardNo == 0 && token == hwrROMTokenIrda)
	{
		if (dataP)
			EmMemPut32 (dataP, 0);

		if (sizeP)
			EmMemPut32 (sizeP, 0);

		m68k_dreg (regs, 0) = ~0;		// token not found.

		return kSkipROM;
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::HwrSleep
 *
 * DESCRIPTION:	Record whether or not we are sleeping and update the
 *				boolean that determines if low-memory access is OK.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::HwrSleep (void)
{
	// void HwrSleep(Boolean untilReset, Boolean emergency)

	struct StackFrame
	{
		Boolean untilReset;
		Boolean dummy;
		Boolean emergency;
	};

	// HwrSleep changes the exception vectors for for the interrupts,
	// so temporarily unlock those.  We'll re-establish them in the
	// HwrSleep tailpatch.

	MetaMemory::MarkTotalAccess (offsetof (M68KExcTableType, busErr),
								 offsetof (M68KExcTableType, busErr) + sizeof (emuptr));

	MetaMemory::MarkTotalAccess (offsetof (M68KExcTableType, addressErr),
								 offsetof (M68KExcTableType, addressErr) + sizeof (emuptr));

	MetaMemory::MarkTotalAccess (offsetof (M68KExcTableType, illegalInstr),
								 offsetof (M68KExcTableType, illegalInstr) + sizeof (emuptr));

	MetaMemory::MarkTotalAccess (offsetof (M68KExcTableType, autoVec1),
								 offsetof (M68KExcTableType, trapN[0]));

	// On Palm OS 1.0, there was code in HwrSleep that would execute only if
	// memory location 0xC2 was set.  It looks like debugging code (that is,
	// Ron could change it from the debugger to try different things out),
	// but we still have to allow access to it.

	if (Patches::OSMajorVersion () == 1)
	{
		MetaMemory::MarkTotalAccess (0x00C2, 0x00C3);
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::KeyCurrentState
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

// From Roger:
//
//	I was thinking a bit more about Gremlins and games yesterday.  Games which
//	use the buttons as input have been a special case for Gremlins because
//	Gremlins only faked events and pen points.	Normally games have to fake
//	their own button mask, and they get better results if have some buttons
//	held down more often than others.
//
//	Now I'm thinking different.  With Poser, the KeyCurrentState call should
//	be stolen when Gremlins is running.  All of the keys possible should have
//	their button bits on half the time.  This will allow users to test games.
//	By not tuning how often buttons should be held down, the testing process
//	will take longer to excerise all app functionality, but it's better than
//	now.  App developers can override the default Gremlin values with their
//	own for better results.
//
//	To successfully test this, A game like SubHunt should play on it's own for
//	a least a while.  HardBall is an example of a game which would really
//	benefit from recognizing Gremlins is running and tune itself to make the
//	testing more effective.  It should grant infinite balls until after the
//	last level is finished.
//
//	I actually think this is important enough to be for Acorn because it
//	enables users to test a large class of apps which otherwise can't.	I
//	think it's easy to implement.  Basically just have KeyCurrentState return
//	the int from the random number generator.  Each bit should be on about
//	half the time.
//
// Follow-up commentary: it turns out that this patch is not having the
// effect we hoped.  SubHunt was so overwhelmed with events from Gremlins
// that it rarely had the chance to call KeyCurrentState.  We're thinking
// of Gremlins backing off on posting events if the SysGetEvent sleep time
// is small (i.e., not "forever"), but we'll have to think about the impact
// on other apps first.

CallROMType SysHeadpatch::KeyCurrentState (void)
{
	// UInt32 KeyCurrentState(void)

	struct StackFrame
	{
	};

#if 0
	// Remove this for now.  Its current implementation uses the Std C Lib
	// random number generator, which is not in sync with the Gremlins RNG.

	if (Hordes::IsOn ())
	{
		// Let's try setting each bit 1/4 of the time.

		uint32 bits = rand () & rand ();

		m68k_dreg (regs, 0) = bits;

		return kSkipROM;
	}
#endif

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::PenOpen
 *
 * DESCRIPTION:	This is where the pen calibration information is read.
 *				Preflight this call to add the calibration information
 *				to the preferences database if it doesn't exist.  That
 *				way, we don't have to continually calibrate the screen
 *				when we boot up.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::PenOpen (void)
{
	// Err PenOpen(void)

	struct StackFrame
	{
	};

	::InstallCalibrationInfo ();

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::PrefSetAppPreferences
 *
 * DESCRIPTION:	Change the default proxy server address.  If it looks
 *				like INet is establishing the old proxy server address,
 *				substitute the current one instead.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::PrefSetAppPreferences (void)
{
	// void PrefSetAppPreferences (UInt32 creator, UInt16 id, Int16 version, 
	//					const void *prefs, UInt16 prefsSize, Boolean saved)

	struct StackFrame
	{
		UInt32	creator;
		UInt16	id;
		Int16	version;
		emuptr	prefs;
		UInt16	prefsSize;
		Boolean	saved;
	};

	UInt32	creator		= GET_PARAMETER (creator);
//	UInt16	id			= GET_PARAMETER (id);
//	Int16	version		= GET_PARAMETER (version);
	emuptr	prefs		= GET_PARAMETER (prefs);
	UInt16	prefsSize	= GET_PARAMETER (prefsSize);
	Boolean	saved		= GET_PARAMETER (saved);

#define inetCreator	'inet'

	if ((creator == inetCreator) && saved && (prefsSize >= 3 + 64))
	{
		emuptr proxyP = prefs + 3;
		if ((uae_strcmp (proxyP, "207.240.80.136") == 0) ||
			(uae_strcmp (proxyP, "209.247.202.106") == 0))
		{
			static string	currentIPAddress;

			// See if we've looked up the address, yet.

			if (currentIPAddress.empty ())
			{
				// If not, get the host information.

				struct hostent*	host = gethostbyname ("content-dev.palm.net");
				if (!(host && host->h_addrtype == AF_INET))
				{
					host = gethostbyname ("content-dev2.palm.net");
				}

				if (host && host->h_addrtype == AF_INET)
				{
					// If the address family looks like one we can handle,
					// get and use the first one.

					struct in_addr*	addr_in = (struct in_addr*) host->h_addr_list[0];
					if (addr_in)
					{
						char*	as_string = inet_ntoa (*addr_in);
						currentIPAddress = as_string;
					}
				}
			}

			// If we have the address cached NOW, use it to replace
			// the hardcoded addresses.

			if (!currentIPAddress.empty ())
			{
				uae_strcpy (proxyP, currentIPAddress.c_str ());
			}
		}
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SndDoCmd
 *
 * DESCRIPTION:	Intercept calls to the Sound Manager and generate the
 *				equivalent host commands.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SndDoCmd (void)
{
	Preference<bool>	pref (kPrefKeyEnableSounds);

	if (!*pref)
		return kExecuteROM;

	struct StackFrame
	{
		void*			chanP;
		SndCommandPtr	cmdP;
		Boolean			noWait;
	};

	emuptr	cmdP = GET_PARAMETER (cmdP);

	SndCommandType cmd;

	uae_memcpy ((void*) &cmd, cmdP, sizeof cmd);
	Canonical(cmd);

	m68k_dreg (regs, 0) = errNone;	// Set default return value

	return Platform::SndDoCmd (cmd);
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysAppExit
 *
 * DESCRIPTION:	If the application calling SysAppExit was launched as a
 *				full application, then "forget" any information we have
 *				about it.  When the next application is launched, we'll
 *				collect information on it in SysAppStartup.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysAppExit (void)
{
	// Err SysAppExit(SysAppInfoPtr appInfoP, MemPtr prevGlobalsP, MemPtr globalsP)

	struct StackFrame
	{
		SysAppInfoPtr appInfoP;
		MemPtr prevGlobalsP;
		MemPtr globalsP;
	};

	emuptr appInfoP		= GET_PARAMETER (appInfoP);
//	emuptr prevGlobalsP	= GET_PARAMETER (prevGlobalsP);
//	emuptr globalsP		= GET_PARAMETER (globalsP);

	if (!appInfoP)
		return kExecuteROM;

	Int16 cmd = EmMemGet16 (appInfoP + offsetof (SysAppInfoType, cmd));

	if (false /*LogLaunchCodes ()*/)
	{
		emuptr	dbP		= EmMemGet32 (appInfoP + offsetof (SysAppInfoType, dbP));
		emuptr	openP	= EmMemGet32 (dbP + offsetof (DmAccessType, openP));
		LocalID	dbID	= EmMemGet32 (openP + offsetof (DmOpenInfoType, hdrID));
		UInt16	cardNo	= EmMemGet16 (openP + offsetof (DmOpenInfoType, cardNo));

		char	name[dmDBNameLength] = {0};
		UInt32	type = 0;
		UInt32	creator = 0;

		/* Err	err =*/ ::DmDatabaseInfo (
				cardNo, dbID, name,
				NULL, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL,
				&type, &creator);

		LogAppendMsg ("SysAppExit called:");
		LogAppendMsg ("	cardNo:			%ld",		(long) cardNo);
		LogAppendMsg ("	dbID:			0x%08lX",	(long) dbID);
		LogAppendMsg ("		name:		%s",		name);
		LogAppendMsg ("		type:		%04lX",		type);
		LogAppendMsg ("		creator:	%04lX",		creator);
	}

	if (cmd == sysAppLaunchCmdNormalLaunch)
	{
		EmuAppInfo	appInfo = Patches::CollectCurrentAppInfo (appInfoP);

		if (appInfo.fCardNo == gQuitAppCardNo && appInfo.fDBID == gQuitAppDbID)
		{
			gTimeToQuit = true;
		}
	}

	gCurAppInfo.pop_back ();	// !!! should probably make sure appInfoP matches
								// the top guy on the stack.

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysAppLaunch
 *
 * DESCRIPTION:	Log information app launches and action codes.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysAppLaunch (void)
{
	// Err SysAppLaunch(UInt16 cardNo, LocalID dbID, UInt16 launchFlags,
	//				UInt16 cmd, MemPtr cmdPBP, UInt32* resultP)

	struct StackFrame
	{
		UInt16	cardNo;
		LocalID dbID;
		UInt16	launchFlags;
		UInt16	cmd;
		MemPtr 	cmdPBP;
		UInt32*	resultP;
	};

	UInt16	cardNo		= GET_PARAMETER (cardNo);
	LocalID	dbID		= GET_PARAMETER (dbID);
	UInt16	launchFlags	= GET_PARAMETER (launchFlags);
	UInt16	cmd			= GET_PARAMETER (cmd);
//	emuptr	cmdPBP		= GET_PARAMETER (cmdPBP);
//	emuptr	resultP 	= GET_PARAMETER (resultP);

	if (false /*LogLaunchCodes ()*/)
	{
		const char* launchStr = ::LaunchCmdToString (cmd);

		char	name[dmDBNameLength] = {0};
		UInt32	type = 0;
		UInt32	creator = 0;

		/* Err	err =*/ ::DmDatabaseInfo (
				cardNo, dbID, name,
				NULL, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL,
				&type, &creator);

		LogAppendMsg ("SysAppLaunch called:");
		LogAppendMsg ("	cardNo:			%ld",		(long) cardNo);
		LogAppendMsg ("	dbID:			0x%08lX",	(long) dbID);
		LogAppendMsg ("		name:		%s",		name);
		LogAppendMsg ("		type:		%04lX",		type);
		LogAppendMsg ("		creator:	%04lX",		creator);
		LogAppendMsg ("	launchFlags:	0x%08lX",	(long) launchFlags);
		LogAppendMsg ("	cmd:			%ld (%s)",	(long) cmd, launchStr);

#if 0
		switch (cmd)
		{
			case sysAppLaunchCmdNormalLaunch:
				// No parameter block
				break;

			case sysAppLaunchCmdFind:
			{
				FindParamsType	parms;

				LogAppendMsg ("	FindParamsType:");
				LogAppendMsg ("		dbAccesMode:		%ld",	(long) parms.dbAccesMode);
				LogAppendMsg ("		recordNum:			%ld",	(long) parms.recordNum);
				LogAppendMsg ("		more:				%ld",	(long) parms.more);
				LogAppendMsg ("		strAsTyped:			%ld",	(long) parms.strAsTyped);
				LogAppendMsg ("		strToFind:			%ld",	(long) parms.strToFind);
				LogAppendMsg ("		numMatches:			%ld",	(long) parms.numMatches);
				LogAppendMsg ("		lineNumber:			%ld",	(long) parms.lineNumber);
				LogAppendMsg ("		continuation:		%ld",	(long) parms.continuation);
				LogAppendMsg ("		searchedCaller:		%ld",	(long) parms.searchedCaller);
				LogAppendMsg ("		callerAppDbID:		%ld",	(long) parms.callerAppDbID);
				LogAppendMsg ("		callerAppCardNo:	%ld",	(long) parms.callerAppCardNo);
				LogAppendMsg ("		appDbID:			%ld",	(long) parms.appCardNo);
				LogAppendMsg ("		newSearch:			%ld",	(long) parms.newSearch);
				LogAppendMsg ("		searchState:		%ld",	(long) parms.searchState);
				LogAppendMsg ("		match:				%ld",	(long) parms.match);

				break;
			}

			case sysAppLaunchCmdGoTo:
				// GoToParamsType
				break;

			case sysAppLaunchCmdSyncNotify:
				// No parameter block
				break;

			case sysAppLaunchCmdTimeChange:
				// No parameter block
				break;

			case sysAppLaunchCmdSystemReset:
			{
				// SysAppLaunchCmdSystemResetType
				SysAppLaunchCmdSystemResetType	parms;
				LogAppendMsg ("	SysAppLaunchCmdSystemResetType:");
				LogAppendMsg ("		hardReset:			%s",	parms.hardReset ? "TRUE" : "FALSE");
				LogAppendMsg ("		createDefaultDB:	%s",	parms.createDefaultDB ? "TRUE" : "FALSE");
				break;
			}

			case sysAppLaunchCmdAlarmTriggered:
				// SysAlarmTriggeredParamType
				break;

			case sysAppLaunchCmdDisplayAlarm:
				// SysDisplayAlarmParamType
				break;

			case sysAppLaunchCmdCountryChange:
				// Not sent?
				break;

			case sysAppLaunchCmdSyncRequestLocal:
//			case sysAppLaunchCmdSyncRequest:
				// No parameter block (I think...)
				break;

			case sysAppLaunchCmdSaveData:
				// SysAppLaunchCmdSaveDataType
				break;

			case sysAppLaunchCmdInitDatabase:
				// SysAppLaunchCmdInitDatabaseType
				break;

			case sysAppLaunchCmdSyncCallApplicationV10:
				// SysAppLaunchCmdSyncCallApplicationTypeV10
				break;

			case sysAppLaunchCmdPanelCalledFromApp:
				// Panel specific?
				//	SvcCalledFromAppPBType
				//	NULL
				break;

			case sysAppLaunchCmdReturnFromPanel:
				// No parameter block
				break;

			case sysAppLaunchCmdLookup:
				// App-specific (see AppLaunchCmd.h)
				break;

			case sysAppLaunchCmdSystemLock:
				// No parameter block
				break;

			case sysAppLaunchCmdSyncRequestRemote:
				// No parameter block (I think...)
				break;

			case sysAppLaunchCmdHandleSyncCallApp:
				// SysAppLaunchCmdHandleSyncCallAppType
				break;

			case sysAppLaunchCmdAddRecord:
				// App-specific (see AppLaunchCmd.h)
				break;

			case sysSvcLaunchCmdSetServiceID:
				// ServiceIDType
				break;

			case sysSvcLaunchCmdGetServiceID:
				// ServiceIDType
				break;

			case sysSvcLaunchCmdGetServiceList:
				// serviceListType
				break;

			case sysSvcLaunchCmdGetServiceInfo:
				// serviceInfoType
				break;

			case sysAppLaunchCmdFailedAppNotify:
				// SysAppLaunchCmdFailedAppNotifyType
				break;

			case sysAppLaunchCmdEventHook:
				// EventType
				break;

			case sysAppLaunchCmdExgReceiveData:
				// ExgSocketType
				break;

			case sysAppLaunchCmdExgAskUser:
				// ExgAskParamType
				break;

			case sysDialLaunchCmdDial:
				// DialLaunchCmdDialType
				break;

			case sysDialLaunchCmdHangUp:
				// DialLaunchCmdDialType
				break;

			case sysSvcLaunchCmdGetQuickEditLabel:
				// SvcQuickEditLabelInfoType
				break;

			case sysAppLaunchCmdURLParams:
				// Part of the URL
				break;

			case sysAppLaunchCmdNotify:
				// SysNotifyParamType
				break;

			case sysAppLaunchCmdOpenDB:
				// SysAppLaunchCmdOpenDBType
				break;

			case sysAppLaunchCmdAntennaUp:
				// No parameter block
				break;

			case sysAppLaunchCmdGoToURL:
				// URL
				break;
		}
#endif
	}

	// Prevent Symbol applications from running.

	if (cmd == sysAppLaunchCmdSystemReset)
	{
		UInt32	type;
		UInt32	creator;
		/*Err		err =*/ ::DmDatabaseInfo (
							cardNo, dbID, NULL,
							NULL, NULL, NULL, NULL,
							NULL, NULL, NULL, NULL,
							&type, &creator);

		if (type == sysFileTApplication &&
			(creator == 'SSRA' || creator == 'SYRA'))
		{
			m68k_dreg (regs, 0) = 0;
			return kSkipROM;
		}
	}

	// Work around a bug in Launcher where it tries to send
	// sysAppLaunchCmdSyncNotify to any received files that
	// aren't PQAs.  This code is pretty much the same as
	// the fixed version of Launcher.

	static Bool recursing;

	if (!recursing && cmd == sysAppLaunchCmdSyncNotify && Patches::HasSyncNotifyBug ())
	{

		UInt32	type;
		UInt32	creator;
		Err		err = ::DmDatabaseInfo (
							cardNo, dbID, NULL,
							NULL, NULL, NULL, NULL,
							NULL, NULL, NULL, NULL,
							&type, &creator);

		if (!err)
		{
			// If it's not an application, then get the application that owns it.

			UInt16	appCardNo = cardNo;
			LocalID	appDbID = dbID;

			if (type != sysFileTApplication)
			{
				DmSearchStateType	searchState;
				
				err = ::DmGetNextDatabaseByTypeCreator (true /*new search*/,
					&searchState, sysFileTApplication, creator, 
					true /*latest version*/, &appCardNo, &appDbID);
			}

			// If there's an error, pretend that everything went OK.

			if (err)
			{
				m68k_dreg (regs, 0) = 0;
				return kSkipROM;
			}

			// Otherwise, substitute the new cardNo and dbID for the
			// ones on the stack.

			SET_PARAMETER (cardNo, appCardNo);
			SET_PARAMETER (dbID, appDbID);
		}

		// Could not get information on the specified database.
		// Pass it off to the ROM as normal.
	}

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysBinarySearch
 *
 * DESCRIPTION:	There's a bug in pre-3.0 versions of SysBinarySearch
 *				that cause it to call the user callback function with a
 *				pointer just past the array to search.	Make a note of
 *				when we enter SysBinarySearch so that we can make
 *				allowances for that in MetaMemory's memory checking.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysBinarySearch (void)
{
	Patches::EnterSysBinarySearch ();

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysDoze
 *
 * DESCRIPTION:	There's a bug in pre-3.0 versions of SysBinarySearch
 *				that cause it to call the user callback function with a
 *				pointer just past the array to search.	Make a note of
 *				when we enter SysBinarySearch so that we can make
 *				allowances for that in MetaMemory's memory checking.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysDoze (void)
{
#if 0
	if (Hordes::IsOn () && gLastEvtTrap != sysTrapEvtGetPen && EmLowMem::GetEvtMgrIdle ())
	{
		LogDump ();
		EmAssert (false);
	}
#endif

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysEvGroupWait
 *
 * DESCRIPTION:	We patch SysEvGroupWait as the mechanism for feeding
 *				the Palm OS new events.  See comments in
 *				Patches::PuppetString.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysEvGroupWait (void)
{
	// Err SysEvGroupWait(UInt32 evID, UInt32 mask, UInt32 value, Int32 matchType,
	//						 Int32 timeout)

	struct StackFrame
	{
		UInt32 evID;
		UInt32 mask;
		UInt32 value;
		Int32 matchType;
		Int32 timeout;
	};

	// Only do this under 2.0 and later.  Under Palm OS 1.0, EvtGetSysEvent
	// called SysSemaphoreWait instead.  See our headpatch of that function
	// for a chunk of pretty similar code.

	if (Patches::OSMajorVersion () == 1)
	{
		return kExecuteROM;
	}

	CallROMType result;
	Bool		clearTimeout;

	Patches::PuppetString (result, clearTimeout);

	// If timeout is infinite, the kernel wants 0.
	// If timeout is none, the kernel wants -1.

	if (clearTimeout && GET_PARAMETER (timeout) == 0)
	{
		SET_PARAMETER (timeout, (uint32) -1);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysFatalAlert
 *
 * DESCRIPTION:	Intercept this and show the user a dialog.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysFatalAlert (void)
{
	// UInt16 SysFatalAlert (const Char* msg)

	struct StackFrame
	{
		const Char* msg;
	};

	Preference<bool>	pref (kPrefKeyInterceptSysFatalAlert);
	if (!*pref)
	{
		// Palm OS will display a dialog with just a Reset button
		// in it.  So *always* turn off the Gremlin, as the user
		// won't be able to select "Continue".

		Hordes::Stop ();
		return kExecuteROM;
	}

	emuptr msg = GET_PARAMETER (msg);

	string	msgString;

	if (msg)
	{
		msgString = PrvToString (msg);
	}
	else
	{
		msgString = Platform::GetString (kStr_UnknownFatalError);
	}

	EmDlgItemID button = Errors::ReportErrSysFatalAlert (msgString.c_str ());

	switch (button)
	{
		case kDlgItemDebug:			m68k_dreg (regs, 0) = fatalEnterDebugger;	break;
		case kDlgItemReset:			m68k_dreg (regs, 0) = fatalReset;			break;
		case kDlgItemContinue:		m68k_dreg (regs, 0) = fatalDoNothing;		break;
		case kDlgItemNextGremlin:	m68k_dreg (regs, 0) = fatalDoNothing;		break;
		default: break;
	}

	if (button == kDlgItemNextGremlin)
	{
		Hordes::ErrorEncountered();
	}
	else if (button != kDlgItemContinue)
	{
		Hordes::Stop ();
	}

	return kSkipROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysLaunchConsole
 *
 * DESCRIPTION:	Stub this function out so that it doesn't do anything.
 *				We completely handle the console in DebugMgr, so there's
 *				no need to get the ROM all heated up.  Also, there are
 *				problems with actually letting the ROM try to launch its
 *				console task, at least on the Mac.	It will try to open
 *				a serial port socket and do stuff with it.	That attempt
 *				will fail, as much of the serial port processing on the
 *				Mac is handled at idle time, and idle time processing is
 *				inhibited when handling debugger packets
 *				(SysLaunchConsole is usually called by a debugger via
 *				the RPC packet).  Since serial port processing doesn't
 *				occur, SysLaunchConsole hangs.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysLaunchConsole (void)
{
	// Err SysLaunchConsole(void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = 0;		// no error

	return kSkipROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysReset
 *
 * DESCRIPTION:	No longer used.  But I'm keeping it around because I
 *				could easily seeing needing it again someday..
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysReset (void)
{
#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGN700C) {
		::PostMessage(EmWindow::GetWindow()->GetHostData()->GetHostWindow(), WM_COMMAND, kCommandSoftReset, NULL);
	}
#endif
	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysSemaphoreWait
 *
 * DESCRIPTION:	We patch SysSemaphoreWait as the mechanism for feeding
 *				the Palm OS new events. See comments in
 *				Patches::PuppetString.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::SysSemaphoreWait (void)
{
	// Err SysSemaphoreWait(UInt32 smID, UInt32 priority, Int32 timeout)

	struct StackFrame
	{
		UInt32 smID;
		UInt32 priority;
		Int32 timeout;
	};

	// Only do this under 1.0.	Under Palm OS 2.0 and later, EvtGetSysEvent
	// calls SysEvGroupWait instead.  See our headpatch of that function
	// for a chunk of pretty similar code.

	if (Patches::OSMajorVersion () != 1)
	{
		return kExecuteROM;
	}

	CallROMType result;
	Bool		clearTimeout;

	Patches::PuppetString (result, clearTimeout);

	// If timeout is infinite, the kernel wants 0.
	// If timeout is none, the kernel wants -1.

	if (clearTimeout && GET_PARAMETER (timeout) == 0)
	{
		SET_PARAMETER (timeout, (uint32) -1);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::SysUIAppSwitch
 *
 * DESCRIPTION:	SysUIAppSwitch is called from the following locations
 *				for the given reasons.	When running Gremlins, we want
 *				to prevent SysUIAppSwitch from doing its job, which is
 *				to record information about the application to switch
 *				to and to then post an appStopEvent to the current
 *				application.
 *
 *				There are a couple of places where SysUIAppSwitch is
 *				called to quit and re-run the current application.
 *				Therefore, we want to stub out SysUIAppSwitch only when
 *				the application being switched to is not the currently
 *				running application.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

//	Places where this is called:
//
//	- LauncherMain.c (AppsViewSwitchApp) to launch new app.
//	- PrefApp (PilotMain) to launch a panel.
//	- SyncApp (prvLaunchExternalModule) to launch a "service owner"
//	- AppLaunchCmd.h (AppLaunchWithCommand) ???
//	- ModemPanel.h (ModemPanelLaunch) ???
//	- callApp.h (LaunchApp) ???
//	- ExgMgr.c (ExgNotifyReceive) to send a sysAppLaunchCmdGoTo message.
//	- GraffitiGlue.c (PrvLaunchGraffitiDemo) to launch Graffiti demo.
//	- Keyboard.c (PrvLaunchGraffitiDemo) to launch Graffiti demo.
//	- Launcher.c (LauncherFormHandleEvent) handles taps on launcher icons.
//	- SystemMgr.c (SysHandleEvent) to send sysAppLaunchCmdSystemLock
//	  in response to seeing a lockChr keyboard message.
//	- SystemMgr.c (SysHandleEvent) to switch apps in response to hard#Chr
//	  keyboard messages.
//	- Find.c (Find) to send sysAppLaunchCmdGoTo message.
//
//	- ButtonsPanel.c (ButtonsFormHandleEvent) switch to another panel.
//	- FormatsPanel.c (FormatsFormHandleEvent) switch to another panel.
//	- GeneralPanel.c (GeneralFormHandleEvent) switch to another panel.
//	- ModemPanel.c (ModemFormHandleEvent) switch to another panel.
//	- NetworkPanel.c (NetworkFormHandleEvent) switch to another panel.
//	- OwnerPanel.c (OwnerViewHandleEvent) switch to another panel.
//	- ShortCutsPanel.c (ShortCutFormHandleEvent) switch to another panel.

CallROMType SysHeadpatch::SysUIAppSwitch (void)
{
	// Err SysUIAppSwitch(UInt16 cardNo, LocalID dbID, UInt16 cmd, MemPtr cmdPBP)

	struct StackFrame
	{
		UInt16 cardNo;
		LocalID dbID;
		UInt16 cmd;
		MemPtr cmdPBP;
	};

	UInt16	cardNo	= (UInt16) GET_PARAMETER (cardNo);
	LocalID dbID	= (LocalID) GET_PARAMETER (dbID);
//	UInt16	cmd 	= (UInt16) GET_PARAMETER (cmd);

	if (false /*LogLaunchCodes ()*/)
	{
		char	name[dmDBNameLength] = {0};
		UInt32	type = 0;
		UInt32	creator = 0;

		/* Err	err =*/ ::DmDatabaseInfo (
				cardNo, dbID, name,
				NULL, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL,
				&type, &creator);

		LogAppendMsg ("SysUIAppSwitch called:");
		LogAppendMsg ("	cardNo:			%ld",		(long) cardNo);
		LogAppendMsg ("	dbID:			0x%08lX",	(long) dbID);
		LogAppendMsg ("		name:		%s",		name);
		LogAppendMsg ("		type:		%04lX",		type);
		LogAppendMsg ("		creator:	%04lX",		creator);
	}

	// We are headpatching SysUIAppSwitch; if we skip the ROM version, we
	// need to replicate at least this part of its functionality:
	//
	// If the last launch attempt failed, release the command parameter block, if
	//	any. When a launch succeeds, the UIAppShell will clear this global
	//	and free the chunk itself  when the app quits.
	{
		CEnableFullAccess	munge;	// Remove blocks on memory access.
		emuptr nextUIAppCmdPBP = EmLowMem_GetGlobal (nextUIAppCmdPBP);
		if (nextUIAppCmdPBP) {
			MemPtrFree((MemPtr) nextUIAppCmdPBP);
			EmLowMem_SetGlobal (nextUIAppCmdPBP, 0);
			}
	}

	// Get the current application card and db.  If we are attempting to switch
	// to the same app, allow it.

	UInt16	currentCardNo;
	LocalID currentDbID;
	Err 	err = SysCurAppDatabase (&currentCardNo, &currentDbID);

	// Got an error? Give up and let default handling occur.

	if (err != 0)
		return kExecuteROM;

	// Switching to current app; let default handling occur.

	if ((cardNo == currentCardNo) && (dbID == currentDbID))
		return kExecuteROM;

	// OK, we're switching to a different application.	If Gremlins is running
	// and running in "single app" mode, then stub out SysUIAppSwitch to do nothing.

	if (Hordes::IsOn () && !Hordes::CanSwitchToApp (cardNo, dbID))
	{
		m68k_dreg (regs, 0) = 0;
		return kSkipROM;
	}

	// Do the normal app switch.

	return kExecuteROM;
}


/***********************************************************************
 *
 * FUNCTION:	SysHeadpatch::TblHandleEvent
 *
 * DESCRIPTION:	See comments in SysTailpatch::TblHandleEvent.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType SysHeadpatch::TblHandleEvent (void)
{
	if (Patches::HasSelectItemBug ())
	{
		gSaveDrawStateStackLevel = ::PrvGetDrawStateStackLevel ();
	}

	return kExecuteROM;
}


#pragma mark -

// ===========================================================================
//		¥ SysTailpatch
// ===========================================================================

/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::MarkUIObjects
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::MarkUIObjects (void)
{
	MetaMemory::MarkUIObjects ();
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::ClipboardAddItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::ClipboardAddItem (void)
{
	if (!Hordes::IsOn () && !gDontPatchClipboardAddItem)
	{
		::PrvCopyPalmClipboardToHost ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::ClipboardAppendItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::ClipboardAppendItem (void)
{
	if (!Hordes::IsOn () && !gDontPatchClipboardAddItem)
	{
		::PrvCopyPalmClipboardToHost ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::DmGetNextDatabaseByTypeCreator
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::DmGetNextDatabaseByTypeCreator (void)
{
	static int	recursing;

	if (recursing)
		return;

	struct StackFrame
	{
		Boolean newSearch;
		DmSearchStatePtr stateInfoP;
		UInt32 type;
		UInt32 creator;
		Boolean onlyLatestVers;
		UInt16* cardNoP;
		LocalID* dbIDP;
	};

//	Boolean newSearch		= GET_PARAMETER (newSearch);
	emuptr	stateInfoP		= GET_PARAMETER (stateInfoP);
	UInt32	type			= GET_PARAMETER (type);
	UInt32	creator 		= GET_PARAMETER (creator);
	Boolean onlyLatestVers	= GET_PARAMETER (onlyLatestVers);
	emuptr	cardNoP 		= GET_PARAMETER (cardNoP);
	emuptr	dbIDP			= GET_PARAMETER (dbIDP);
	Err		result			= m68k_dreg (regs, 0);

	if (result == errNone && type == sysFileTExtension && creator == 0)
	{
		// Make sure we are called from within PrvLoadExtensions.

		// If so, get information on the returned cardNoP and dbIDP.

		UInt16	cardNo	= EmMemGet16 (cardNoP);
		LocalID dbID	= EmMemGet32 (dbIDP);

		UInt32	actualCreator;
		Err 	err = ::DmDatabaseInfo (cardNo, dbID, NULL, NULL, NULL, NULL,
							NULL, NULL, NULL, NULL, NULL, NULL, &actualCreator);

		// If the returned database has the creator of 'xxxx'
		// call DmGetNextDatabaseByTypeCreator again to skip
		// past this entry.

		if (err == errNone && actualCreator == 'xxxx')
		{
			recursing = true;

			err = ::DmGetNextDatabaseByTypeCreator (false,
						(DmSearchStatePtr) stateInfoP,
						type, creator, onlyLatestVers,
						(UInt16*) cardNoP, (LocalID*) dbIDP);
			m68k_dreg (regs, 0) = (uint32) err;

			recursing = false;
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::DmGetResource
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

extern MemHandle	gVersionResource;
extern MemHandle	gCreditsResource;

void SysTailpatch::DmGetResource (void)
{
	// MemHandle DmGetResource (UInt32 type, Int16 id)

	struct StackFrame
	{
		UInt32 type;
		Int16 id;
	};

	UInt32	type = GET_PARAMETER (type);
	Int16	id = GET_PARAMETER (id);

	emuptr	hdl = m68k_areg (regs, 0);
	if (hdl)
	{
		// (Calling InTheEgg is expensive, so hold off calling it
		//  until absolutely as late as possible).

		if (type == 'tSTR' && id == 1257 && ::InEggOfInfiniteWisdom ())
			gCreditsResource = (MemHandle) hdl;

		if (type == 'tver' && id == 10000 && gCreditsResource && ::InEggOfInfiniteWisdom ())
			gVersionResource = (MemHandle) hdl;
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::EvtGetEvent
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::EvtGetEvent (void)
{
	// void EvtGetEvent(const EventPtr event, Int32 timeout);

	struct StackFrame
	{
		const EventPtr event;
		Int32 timeout;
	};

	emuptr event = GET_PARAMETER (event);
	Int32	timeout = GET_PARAMETER (timeout);

	LogEvtGetEvent (event, timeout);

	gEvtGetEventCalled = true;
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::EvtGetPen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::EvtGetPen (void)
{
	// void EvtGetPen(Int16 *pScreenX, Int16 *pScreenY, Boolean *pPenDown)

	struct StackFrame
	{
		Int16 *pScreenX;
		Int16 *pScreenY;
		Boolean *pPenDown;
	};

	emuptr pScreenX = GET_PARAMETER (pScreenX);
	emuptr pScreenY = GET_PARAMETER (pScreenY);
	emuptr pPenDown = GET_PARAMETER (pPenDown);

	LogEvtGetPen (pScreenX, pScreenY, pPenDown);
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::EvtGetSysEvent
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::EvtGetSysEvent (void)
{
	// void EvtGetSysEvent(EventPtr eventP, Int32 timeout)

	struct StackFrame
	{
		EventPtr event;
		Int32 timeout;
	};

	emuptr	event = GET_PARAMETER (event);
	Int32	timeout = GET_PARAMETER (timeout);

	LogEvtGetSysEvent (event, timeout);
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::EvtSysEventAvail
 *
 * DESCRIPTION:	If there are no pending events in the real system,
 *				check to see if we have any pen events to insert.
 *				Set the function's return value to TRUE if so.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::EvtSysEventAvail (void)
{
	// Boolean EvtSysEventAvail(Boolean ignorePenUps)

	struct StackFrame
	{
		Boolean ignorePenUps;
	};

	Boolean	ignorePenUps = GET_PARAMETER (ignorePenUps);

	if (m68k_dreg (regs, 0) == 0)
	{
		EmAssert (gSession);
		if (gSession->HasPenEvent ())
		{
			EmPenEvent	event = gSession->PeekPenEvent ();

			if (event.fPenIsDown || !ignorePenUps)
			{
				m68k_dreg (regs, 0) = 1;
			}
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::ExgReceive
 *
 * DESCRIPTION:	Log Exchange Manager data.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::ExgReceive (void)
{
	// UInt32 ExgReceive (ExgSocketPtr exgSocketP, void* bufP, const UInt32 bufLen, Err* errP) = 0;

	struct StackFrame
	{
		void*	exgSocketP;
		void*	bufP;
		UInt32	bufLen;
		Err*	errP;
	};

	emuptr	bufP	= GET_PARAMETER (bufP);
	UInt32	bufLen	= GET_PARAMETER (bufLen);
	emuptr	errP	= GET_PARAMETER (errP);
	UInt32	received= m68k_dreg (regs, 0);

	EmAliasErr<PAS>	err (errP);

	if (LogExgMgr ())
	{
		LogAppendMsg ("ExgMgr: received %ld bytes, err = 0x%04X.", received, (Err) err);
	}
	else if (LogExgMgrData ())
	{
		StMemory	buffer (bufLen);
		uae_memcpy ((void*) buffer.Get (), bufP, bufLen);
		LogAppendData (buffer.Get (), bufLen, "ExgMgr: received %ld bytes, err = 0x%04X.", received, (Err) err);
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::ExgSend
 *
 * DESCRIPTION:	Log Exchange Manager data.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::ExgSend (void)
{
	// UInt32 ExgSend (ExgSocketPtr exgSocketP, void* bufP, const UInt32 bufLen, Err* errP) = 0;

	struct StackFrame
	{
		void*	exgSocketP;
		void*	bufP;
		UInt32	bufLen;
		Err*	errP;
	};

	emuptr	bufP	= GET_PARAMETER (bufP);
	UInt32	bufLen	= GET_PARAMETER (bufLen);
	emuptr	errP	= GET_PARAMETER (errP);
	UInt32	sent	= m68k_dreg (regs, 0);

	EmAliasErr<PAS>	err (errP);

	if (LogExgMgr ())
	{
		LogAppendMsg ("ExgMgr: sent %ld bytes, err = 0x%04X.", sent, (Err) err);
	}
	else if (LogExgMgrData ())
	{
		StMemory	buffer (bufLen);
		uae_memcpy ((void*) buffer.Get (), bufP, bufLen);
		LogAppendData (buffer.Get (), bufLen, "ExgMgr: sent %ld bytes, err = 0x%04X.", sent, (Err) err);
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::FtrInit
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::FtrInit (void)
{
	// void FtrInit(void)

	struct StackFrame
	{
	};

	// Get information about the current OS so that we know
	// what features are implemented (for those cases when we
	// can't make other tests, like above where we test for
	// the existance of a trap before calling it).

	// Read the version into a local variable; the ROM Stub facility
	// automatically maps local variables into Palm space so that ROM
	// functions can get to them.  If we were to pass &gOSVersion,
	// the DummyBank functions would complain about an invalid address.

	UInt32	value;
	Err 	err = ::FtrGet (sysFtrCreator, sysFtrNumROMVersion, &value);

	if (err == errNone)
	{
		gOSVersion = value;
	}
	else
	{
		gOSVersion = kOSUndeterminedVersion;
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::FtrSet
 *
 * DESCRIPTION:	Look for calls indicating that we're running a
 *				non-Roman system
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::FtrSet (void)
{
	// Err FtrSet (UInt32 creator, UInt16 featureNum, UInt32 newValue)

	struct StackFrame
	{
		UInt32	creator;
		UInt16	featureNum;
		UInt32	newValue;
	};

	// Look for calls indicating that we're running a non-Roman system.

	if (m68k_dreg (regs, 0) == errNone)
	{
		UInt32	creator		= GET_PARAMETER (creator);
		UInt16	featureNum	= GET_PARAMETER (featureNum);

		if (creator == sysFtrCreator && featureNum == sysFtrNumEncoding)
		{
			gEncoding = GET_PARAMETER (newValue);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::HwrMemReadable
 *
 * DESCRIPTION:	Patch this function so that it returns non-zero if the
 *				address is in the range of memory that we've mapped in
 *				to emulated space.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::HwrMemReadable (void)
{
	// UInt32 HwrMemReadable(MemPtr address)

	struct StackFrame
	{
		MemPtr address;
	};

	emuptr	address = GET_PARAMETER (address);
	UInt32	result = m68k_dreg (regs, 0);

	if (result == 0)
	{
		void*	addrStart;
		uint32	addrLen;

		Memory::GetMappingInfo (address, &addrStart, &addrLen);

		m68k_dreg (regs, 0) = addrLen;
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::HwrSleep
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::HwrSleep (void)
{
	// void HwrSleep(Boolean untilReset, Boolean emergency)

	struct StackFrame
	{
		Boolean untilReset;
		Boolean dummy;
		Boolean emergency;
	};

	MetaMemory::MarkLowMemory (offsetof (M68KExcTableType, busErr),
							   offsetof (M68KExcTableType, busErr) + sizeof (emuptr));

	MetaMemory::MarkLowMemory (offsetof (M68KExcTableType, addressErr),
							   offsetof (M68KExcTableType, addressErr) + sizeof (emuptr));

	MetaMemory::MarkLowMemory (offsetof (M68KExcTableType, illegalInstr),
							   offsetof (M68KExcTableType, illegalInstr) + sizeof (emuptr));

	MetaMemory::MarkLowMemory (offsetof (M68KExcTableType, autoVec1),
							   offsetof (M68KExcTableType, trapN[0]));

	// On Palm OS 1.0, there was code in HwrSleep that would execute only if
	// memory location 0xC2 was set.  It looks like debugging code (that is,
	// Ron could change it from the debugger to try different things out),
	// but we still have to allow access to it.

	if (Patches::OSMajorVersion () == 1)
	{
		MetaMemory::MarkLowMemory (0x00C2, 0x00C3);
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::SysAppStartup
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::SysAppStartup (void)
{
	// Err SysAppStartup(SysAppInfoPtr* appInfoPP, MemPtr* prevGlobalsP, MemPtr* globalsPtrP)

	struct StackFrame
	{
		SysAppInfoPtr* appInfoPP;
		MemPtr* prevGlobalsP;
		MemPtr* globalsPtrP;
	};

	if (m68k_dreg (regs, 0) != errNone)
		return;

	emuptr appInfoPP = GET_PARAMETER (appInfoPP);
	if (!appInfoPP)
		return;

	emuptr appInfoP = EmMemGet32 (appInfoPP);
	if (!appInfoP)
		return;

	if (false /*LogLaunchCodes ()*/)
	{
		emuptr	dbP		= EmMemGet32 (appInfoP + offsetof (SysAppInfoType, dbP));
		emuptr	openP	= EmMemGet32 (dbP + offsetof (DmAccessType, openP));
		LocalID	dbID	= EmMemGet32 (openP + offsetof (DmOpenInfoType, hdrID));
		UInt16	cardNo	= EmMemGet16 (openP + offsetof (DmOpenInfoType, cardNo));

		char	name[dmDBNameLength] = {0};
		UInt32	type = 0;
		UInt32	creator = 0;

		/* Err	err =*/ ::DmDatabaseInfo (
				cardNo, dbID, name,
				NULL, NULL, NULL, NULL,
				NULL, NULL, NULL, NULL,
				&type, &creator);

		LogAppendMsg ("SysAppStartup called:");
		LogAppendMsg ("	cardNo:			%ld",		(long) cardNo);
		LogAppendMsg ("	dbID:			0x%08lX",	(long) dbID);
		LogAppendMsg ("		name:		%s",		name);
		LogAppendMsg ("		type:		%04lX",		type);
		LogAppendMsg ("		creator:	%04lX",		creator);
	}

	Int16 cmd = EmMemGet16 (appInfoP + offsetof (SysAppInfoType, cmd));

	EmuAppInfo	newAppInfo = Patches::CollectCurrentAppInfo (appInfoP);
	newAppInfo.fCmd = cmd;
	gCurAppInfo.push_back (newAppInfo);

	if (cmd == sysAppLaunchCmdNormalLaunch)
	{
		// Clear the flags that tell us which warnings we've already issued.
		// We reset these flags any time a new application is launched.
		//
		// !!! Put these flags into EmuAppInfo?  That way, we can keep
		// separate sets for sub-launched applications.

//		Errors::ClearWarningFlags ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::SysBinarySearch
 *
 * DESCRIPTION:	There's a bug in pre-3.0 versions of SysBinarySearch
 *				that cause it to call the user callback function with a
 *				pointer just past the array to search.	Make a note of
 *				when we enter SysBinarySearch so that we can make
 *				allowances for that in MetaMemory's memory checking.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::SysBinarySearch (void)
{
	Patches::ExitSysBinarySearch ();
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::SysTaskCreate
 *
 * DESCRIPTION:	Track calls to SysTaskCreate to get information on any
 *				stacks created.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::SysTaskCreate (void)
{
	// Err		SysTaskCreate(UInt32 * taskIDP, UInt32 * creator, ProcPtr codeP,
	//						MemPtr stackP, UInt32 stackSize, UInt32 attr, UInt32 priority,
	//						UInt32 tSlice)

	struct StackFrame
	{
		emuptr	taskIDP;
		emuptr	creator;
		emuptr	codeP;
		emuptr	stackP;
		uint32	stackSize;
		uint32	attr;
		uint32	priority;
		uint32	tSlice;
	};

	if (m68k_dreg (regs, 0) == errNone)
	{
		emuptr	stackP		= GET_PARAMETER (stackP);
		uint32	stackSize	= GET_PARAMETER (stackSize);

		StackRange	range (stackP, stackP + stackSize);
		EmPalmOS::RememberStackRange (range);
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::TblHandleEvent
 *
 * DESCRIPTION:	There's a bug in a private Table Manager function called
 *				SelectItem (that's the name of the function, not the
 *				bug).  Under certain circumstances, the function will
 *				exit early without popping the draw state that it pushed
 *				onto the stack at the start of the function.  This
 *				tailpatch remedies that problem.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::TblHandleEvent (void)
{
	if (Patches::HasSelectItemBug ())
	{
		long	curDrawStateStackLevel = ::PrvGetDrawStateStackLevel ();
		long	levelDelta = curDrawStateStackLevel - gSaveDrawStateStackLevel;

		if (levelDelta == 1)
		{
			WinPopDrawState ();
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::TimInit
 *
 * DESCRIPTION:	TimInit is where a special boolean is set to trigger a
 *				bunch of RTC bug workarounds in the ROM (that is, there
 *				are RTC bugs in the Dragonball that the ROM needs to
 *				workaround).  We're not emulating those bugs, so turn
 *				off that boolean.  Otherwise, we'd have to add support
 *				for the 1-second, 1-minute, and 1-day RTC interrupts in
 *				order to get those workarounds to work.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::TimInit (void)
{
	// Err TimInit(void)

	struct StackFrame
	{
	};

	// Turn off the RTC bug workaround flag.

	CEnableFullAccess	munge;

	emuptr	timGlobalsP = EmLowMem_GetGlobal (timGlobalsP);
	EmAliasTimGlobalsType<PAS>	timGlobals (timGlobalsP);

	if (timGlobals.rtcBugWorkaround == 0x01)
	{
		timGlobals.rtcBugWorkaround = 0;
	}

	// Set the current date.

	::PrvSetCurrentDate ();
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::UIInitialize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::UIInitialize (void)
{
	// Void UIInitialize (void)

	struct StackFrame
	{
	};

	Patches::SetUIInitialized (true);

	// Prevent the device from going to sleep.

	::SysSetAutoOffTime (0);

	// Can't call PrefSetPreference on 1.0 devices....

	if (EmLowMem::TrapExists (sysTrapPrefSetPreference))
	{
		::PrefSetPreference (prefAutoOffDuration, 0);
	}

	// Install a 'pose' feature so that people can tell if they're running
	// under the emulator or not.  We *had* added a HostControl function
	// for this, but people can call that only under Poser or under 3.0
	// and later ROMs.	Which means that there's no way to tell under 2.0
	// and earlier ROMs when running on an actual device.
	//
	// Note that we do this here instead of in a tailpatch to FtrInit because
	// of a goofy inter-dependency.  FtrInit is called early in the boot process
	// before a valid stack has been allocated and switched to. During this time,
	// the ROM is using a range of memory that happens to reside in a free
	// memory chunk.  Routines in MetaMemory know about this and allow the use
	// of this unallocate range of memory.	However, in order to work, they need
	// to know the current stack pointer value.  If we were to call FtrSet from
	// the FtrInit tailpatch, the stack pointer will point to the stack set up
	// by the ATrap object, not the faux stack in use at the time of FtrInit.
	// Thus, the MetaMemory routines get confused and end up getting in a state
	// where accesses to the temporary OS stack are flagged as invalid.  Hence,
	// we defer adding these Features until much later in the boot process (here).

	::FtrSet ('pose', 0, 0);

	// If we're listening on a socket, install the 'gdbS' feature.	The
	// existance of this feature causes programs written with the prc tools
	// to enter the debugger when they're launched.

	if (Debug::ConnectedToTCPDebugger ())
	{
		::FtrSet ('gdbS', 0, 0x12BEEF34);
	}

	// Install the HotSync user-name.

	Preference<string>	userName (kPrefKeyUserName);
	::SetHotSyncUserName (userName->c_str ());

	// Auto-load any files in the Autoload[Foo] directories.

	::PrvAutoload ();

	// Install our ExgMgr driver

	// Don't do this for now.  We don't currently use it, and people get
	// confused seeing it show up in application lists, etc.

//	EmFileImport::InstallExgMgrLib ();
}


/***********************************************************************
 *
 * FUNCTION:	SysTailpatch::UIReset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void SysTailpatch::UIReset (void)
{
	// Void UIReset (void)

	struct StackFrame
	{
	};

	Patches::SetUIReset (true);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	HtalLibHeadpatch::HtalLibSendReply
 *
 * DESCRIPTION:	Ohhh...I'm going to Programmer Hell for this one...
 *				We call DlkDispatchRequest to install the user name in
 *				our UIInitialize patch.  DlkDispatchRequest will
 *				eventually call HtalLibSendReply to return a result
 *				code.  Well, we haven't fired up the Htal library, and
 *				wouldn't want it to send a response even if we had.
 *				Therefore, I'll subvert the whole process by setting
 *				the HTAL library refNum passed in to the Desktop Link
 *				Manager to an invalid value.  I'll look for this
 *				value in the SysTrap handling code and no-op the call
 *				by calling this stub.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType HtalLibHeadpatch::HtalLibSendReply (void)
{
	m68k_dreg (regs, 0) = errNone;

	return kSkipROM;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	PrvToString
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

string PrvToString (emuptr s)
{
	string	result;

	size_t	sLen = uae_strlen (s);
	if (sLen > 0)
	{
		result.resize (sLen);
		uae_strcpy (&result[0], s);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	PrvAutoload
 *
 * DESCRIPTION:	Install the files in the various Autoload directories.
 *				If there are any designated to be run, pick on to run.
 *				If the emulator should exit when the picked application
 *				quits, schedule it to do so.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void PrvAutoload (void)
{
	// Load all the files in one blow.

	EmFileRefList	fileList;
	Startup::GetAutoLoads (fileList);

	EmFileImport::LoadPalmFileList (fileList, kMethodHomebrew);

	// Get the application to switch to (if any);

	string	appToRun = Startup::GetAutoRunApp ();

	if (!appToRun.empty())
	{
		char	name[dmDBNameLength];
		strcpy (name, appToRun.c_str());

		UInt16	cardNo = 0;
		LocalID	dbID = ::DmFindDatabase (cardNo, name);

		if (dbID != 0)
		{
			Patches::SetSwitchApp (cardNo, dbID);

			// If we're supposed to quit after running this application,
			// start that process in motion.

			if (Startup::QuitOnExit ())
			{
				Patches::SetQuitApp (cardNo, dbID);
			}
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	PrvSetCurrentDate
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void PrvSetCurrentDate (void)
{
	CEnableFullAccess	munge;

	// Get the current date.

	long	year, month, day;
	::GetHostDate (&year, &month, &day);

	// Convert it to days -- and then hourse -- since 1/1/1904

	UInt32	rtcHours = ::DateToDays (year, month, day) * 24;

	// Update the "hours adjustment" value to contain the current date.

	emuptr	timGlobalsP = EmLowMem_GetGlobal (timGlobalsP);
	EmAliasTimGlobalsType<PAS>	timGlobals (timGlobalsP);

	timGlobals.rtcHours = rtcHours;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetDrawStateStackLevel
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

long PrvGetDrawStateStackLevel (void)
{
	CEnableFullAccess	munge;
	UInt16				level = EmLowMem_GetGlobal (uiGlobals.gStateV3.drawStateIndex);
	return level;
}


/***********************************************************************
 *
 * FUNCTION:	PrvCopyPalmClipboardToHost
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void PrvCopyPalmClipboardToHost (void)
{
	gDontPatchClipboardGetItem = true;

	UInt16		length;
	MemHandle	dataHdl = ::ClipboardGetItem (clipboardText, &length);

	gDontPatchClipboardGetItem = false;

	if (length > 0)
	{
		MemPtr	dataPtr = ::MemHandleLock (dataHdl);
		if (dataPtr)
		{
			void*	dataCopy = Platform::AllocateMemory (length);
			if (dataCopy)
			{
				uae_memcpy (dataCopy, (emuptr) dataPtr, length);
				Platform::CopyToClipboard (dataCopy, length);
				Platform::DisposeMemory (dataCopy);
			}

			::MemHandleUnlock (dataHdl);
		}
	}
	else
	{
		Platform::CopyToClipboard (NULL, 0);
	}
}


/***********************************************************************
 *
 * FUNCTION:	PrvCopyHostClipboardToPalm
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void PrvCopyHostClipboardToPalm (void)
{
	Chunk	hostData;
	Platform::CopyFromClipboard (hostData);

	gDontPatchClipboardAddItem = true;

	if (hostData.GetLength () > 0)
	{
		// Limit us to cbdMaxTextLength characters so as to not
		// overburden the dynamic heap.  This is the same limit used by
		// the Field Manager does when calling ClipboardAddItem (except
		// that the Field Manager puts up a dialog saying that it won't
		// copy the text to the clipboard instead of truncating).

		if (hostData.GetLength () > cbdMaxTextLength)
		{
			hostData.SetLength (cbdMaxTextLength);
		}

		// Set the Palm OS clipboard to the text data.

		::ClipboardAddItem (clipboardText, hostData.GetPointer (), hostData.GetLength ());
	}
	else
	{
		::ClipboardAddItem (clipboardText, NULL, 0);
	}

	gDontPatchClipboardAddItem = false;
}

#ifdef SONY_ROM
CallROMType SysHeadpatch::RomSkip (void)
{
	return kSkipROM;
}

CallROMType SysHeadpatch::SysLibLoad (void)
{
	struct StackFrame
	{
		UInt32	libType;
		UInt32	libCreator;
		UInt16*	refNumP;
	};

	UInt32	libType		= GET_PARAMETER (libType);
	UInt32	libCreator	= GET_PARAMETER (libCreator);

	if (libType == sysResTLibrary && libCreator == 'SlMa')
	{
		// unsupported Audio Library on POSE
		m68k_dreg (regs, 0) = sysErrLibNotFound;
		return kSkipROM;
	}

	return kExecuteROM;
}

CallROMType SysHeadpatch::HwrIRQ4Handler (void)
{
	emuptr unimplementedAddress	= EmLowMem::GetTrapAddress (sysTrapSysUnimplemented);
	emuptr result				= EmLowMem::GetTrapAddress (sysTrapHwrIRQ4Handler);

	if (unimplementedAddress == result)
		return kSkipROM;

	return kExecuteROM;
}

void SysTailpatch::WinScreenMode()
{
	struct StackFrame
	{
		UInt8	operation;
		UInt32 *widthP;
		UInt32 *heightP; 
		UInt32 *depthP; 
	};

	UInt8  operation = GET_PARAMETER (operation);
	emuptr depthP = GET_PARAMETER (depthP);

	if ((depthP && operation == winScreenModeSet) 
		&& gSession->GetDevice().GetDeviceType() == kDevicePEGN700C)
	{
		UInt32	depth;
		if (depthP)
			depth = get_long(depthP);
		else
			depth = 8;

		UInt32		data, colorIndex, i;
		WinHandle	winH = WinGetActiveWindow();

		if (winH)
		{
			DrawStateType	state;
			emuptr			stateP = get_long((emuptr)winH + offsetof (WindowType, drawStateP));
			uae_memcpy((void*)&state, stateP, sizeof(DrawStateType));
			colorIndex = state.backColor;
			for (i=0, data=0; i<32; i+=depth)
			{
				if (i)
					data = data << depth;
				data += colorIndex;
			}
			emuptr begin, end;
			EmHAL::GetLCDBeginEnd (begin, end);
			uae_memset(begin, data, (end-begin));
		}
	}
}
#endif


#include "PalmPackPop.h"
