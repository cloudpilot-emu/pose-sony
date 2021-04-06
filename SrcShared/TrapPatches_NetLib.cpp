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

#include "EmMemory.h"			// CEnableFullAccess
#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"			// PARAM_VAL, etc.
#include "Platform_NetLib.h"	// Platform_NetLib
#include "UAE.h"				// m68k_areg, m68k_dreg, regs

#if PLATFORM_UNIX || PLATFORM_MAC
#include <netinet/in.h>			// ntohs, ntohs
#endif

// ======================================================================
//	Patches for NetLib functions
// ======================================================================

class NetLibHeadpatch
{
	public:
		static CallROMType		NetLibOpen					(void);
		static CallROMType		NetLibClose					(void);
		static CallROMType		NetLibSleep					(void);
		static CallROMType		NetLibWake					(void);
		static CallROMType		NetLibAddrINToA				(void);
		static CallROMType		NetLibAddrAToIN				(void);
		static CallROMType		NetLibSocketOpen			(void);
		static CallROMType		NetLibSocketClose			(void);
		static CallROMType		NetLibSocketOptionSet		(void);
		static CallROMType		NetLibSocketOptionGet		(void);
		static CallROMType		NetLibSocketBind			(void);
		static CallROMType		NetLibSocketConnect			(void);
		static CallROMType		NetLibSocketListen			(void);
		static CallROMType		NetLibSocketAccept			(void);
		static CallROMType		NetLibSocketShutdown		(void);
		static CallROMType		NetLibSendPB				(void);
		static CallROMType		NetLibSend					(void);
		static CallROMType		NetLibReceivePB				(void);
		static CallROMType		NetLibReceive				(void);
		static CallROMType		NetLibDmReceive				(void);
		static CallROMType		NetLibSelect				(void);
		static CallROMType		NetLibMaster				(void);
		static CallROMType		NetLibGetHostByName			(void);
		static CallROMType		NetLibSettingGet			(void);
		static CallROMType		NetLibSettingSet			(void);
		static CallROMType		NetLibIFAttach				(void);
		static CallROMType		NetLibIFDetach				(void);
		static CallROMType		NetLibIFGet					(void);
		static CallROMType		NetLibIFSettingGet			(void);
		static CallROMType		NetLibIFSettingSet			(void);
		static CallROMType		NetLibIFUp					(void);
		static CallROMType		NetLibIFDown				(void);
		static CallROMType		NetLibGetHostByAddr			(void);
		static CallROMType		NetLibGetServByName			(void);
		static CallROMType		NetLibSocketAddr			(void);
		static CallROMType		NetLibFinishCloseWait		(void);
		static CallROMType		NetLibGetMailExchangeByName	(void);
		static CallROMType		NetLibOpenCount				(void);
		static CallROMType		NetLibTracePrintF			(void);
		static CallROMType		NetLibTracePutS				(void);
		static CallROMType		NetLibOpenIfCloseWait		(void);
		static CallROMType		NetLibHandlePowerOff		(void);
		static CallROMType		NetLibConnectionRefresh		(void);
		static CallROMType		NetLibOpenConfig			(void);
		static CallROMType		NetLibConfigMakeActive		(void);
		static CallROMType		NetLibConfigList			(void);
		static CallROMType		NetLibConfigIndexFromName	(void);
		static CallROMType		NetLibConfigDelete			(void);
		static CallROMType		NetLibConfigSaveAs			(void);
		static CallROMType		NetLibConfigRename			(void);
		static CallROMType		NetLibConfigAliasSet		(void);
		static CallROMType		NetLibConfigAliasGet		(void);
};


class NetLibTailpatch
{
	public:
		static void		NetLibOpen					(void);
		static void		NetLibClose					(void);
		static void		NetLibSleep					(void);
		static void		NetLibWake					(void);
		static void		NetLibAddrINToA				(void);
		static void		NetLibAddrAToIN				(void);
		static void		NetLibSocketOpen			(void);
		static void		NetLibSocketClose			(void);
		static void		NetLibSocketOptionSet		(void);
		static void		NetLibSocketOptionGet		(void);
		static void		NetLibSocketBind			(void);
		static void		NetLibSocketConnect			(void);
		static void		NetLibSocketListen			(void);
		static void		NetLibSocketAccept			(void);
		static void		NetLibSocketShutdown		(void);
		static void		NetLibSendPB				(void);
		static void		NetLibSend					(void);
		static void		NetLibReceivePB				(void);
		static void		NetLibReceive				(void);
		static void		NetLibDmReceive				(void);
		static void		NetLibSelect				(void);
		static void		NetLibMaster				(void);
		static void		NetLibGetHostByName			(void);
		static void		NetLibSettingGet			(void);
		static void		NetLibSettingSet			(void);
		static void		NetLibIFAttach				(void);
		static void		NetLibIFDetach				(void);
		static void		NetLibIFGet					(void);
		static void		NetLibIFSettingGet			(void);
		static void		NetLibIFSettingSet			(void);
		static void		NetLibIFUp					(void);
		static void		NetLibIFDown				(void);
		static void		NetLibGetHostByAddr			(void);
		static void		NetLibGetServByName			(void);
		static void		NetLibSocketAddr			(void);
		static void		NetLibFinishCloseWait		(void);
		static void		NetLibGetMailExchangeByName	(void);
		static void		NetLibOpenCount				(void);
		static void		NetLibTracePrintF			(void);
		static void		NetLibTracePutS				(void);
		static void		NetLibOpenIfCloseWait		(void);
		static void		NetLibHandlePowerOff		(void);
		static void		NetLibConnectionRefresh		(void);
		static void		NetLibOpenConfig			(void);
		static void		NetLibConfigMakeActive		(void);
		static void		NetLibConfigList			(void);
		static void		NetLibConfigIndexFromName	(void);
		static void		NetLibConfigDelete			(void);
		static void		NetLibConfigSaveAs			(void);
		static void		NetLibConfigRename			(void);
		static void		NetLibConfigAliasSet		(void);
		static void		NetLibConfigAliasGet		(void);
};


// ======================================================================
//	Proto patch table for NetLib functions.  This array will be used
//	to create a sparse array at runtime.
// ======================================================================

ProtoPatchTableEntry	gProtoNetLibPatchTable[] =
{
	{sysLibTrapOpen,				NetLibHeadpatch::NetLibOpen,					NetLibTailpatch::NetLibOpen},
	{sysLibTrapClose,				NetLibHeadpatch::NetLibClose,					NetLibTailpatch::NetLibClose},
	{sysLibTrapSleep,				NetLibHeadpatch::NetLibSleep,					NetLibTailpatch::NetLibSleep},
	{sysLibTrapWake,				NetLibHeadpatch::NetLibWake,					NetLibTailpatch::NetLibWake},
	{netLibTrapAddrINToA,			NetLibHeadpatch::NetLibAddrINToA,				NetLibTailpatch::NetLibAddrINToA},
	{netLibTrapAddrAToIN,			NetLibHeadpatch::NetLibAddrAToIN,				NetLibTailpatch::NetLibAddrAToIN},
	{netLibTrapSocketOpen,			NetLibHeadpatch::NetLibSocketOpen,				NetLibTailpatch::NetLibSocketOpen},
	{netLibTrapSocketClose,			NetLibHeadpatch::NetLibSocketClose,				NetLibTailpatch::NetLibSocketClose},
	{netLibTrapSocketOptionSet,		NetLibHeadpatch::NetLibSocketOptionSet,			NetLibTailpatch::NetLibSocketOptionSet},
	{netLibTrapSocketOptionGet,		NetLibHeadpatch::NetLibSocketOptionGet,			NetLibTailpatch::NetLibSocketOptionGet},
	{netLibTrapSocketBind,			NetLibHeadpatch::NetLibSocketBind,				NetLibTailpatch::NetLibSocketBind},
	{netLibTrapSocketConnect,		NetLibHeadpatch::NetLibSocketConnect,			NetLibTailpatch::NetLibSocketConnect},
	{netLibTrapSocketListen,		NetLibHeadpatch::NetLibSocketListen,			NetLibTailpatch::NetLibSocketListen},
	{netLibTrapSocketAccept,		NetLibHeadpatch::NetLibSocketAccept,			NetLibTailpatch::NetLibSocketAccept},
	{netLibTrapSocketShutdown,		NetLibHeadpatch::NetLibSocketShutdown,			NetLibTailpatch::NetLibSocketShutdown},
	{netLibTrapSendPB,				NetLibHeadpatch::NetLibSendPB,					NetLibTailpatch::NetLibSendPB},
	{netLibTrapSend,				NetLibHeadpatch::NetLibSend,					NetLibTailpatch::NetLibSend},
	{netLibTrapReceivePB,			NetLibHeadpatch::NetLibReceivePB,				NetLibTailpatch::NetLibReceivePB},
	{netLibTrapReceive,				NetLibHeadpatch::NetLibReceive,					NetLibTailpatch::NetLibReceive},
	{netLibTrapDmReceive,			NetLibHeadpatch::NetLibDmReceive,				NetLibTailpatch::NetLibDmReceive},
	{netLibTrapSelect,				NetLibHeadpatch::NetLibSelect,					NetLibTailpatch::NetLibSelect},
//	{netLibTrapPrefsGet,			NULL,											NULL},
//	{netLibTrapPrefsSet,			NULL,											NULL},
//	{netLibTrapDrvrWake,			NULL,											NULL},
//	{netLibTrapInterfacePtr,		NULL,											NULL},
	{netLibTrapMaster,				NetLibHeadpatch::NetLibMaster,					NetLibTailpatch::NetLibMaster},
	{netLibTrapGetHostByName,		NetLibHeadpatch::NetLibGetHostByName,			NetLibTailpatch::NetLibGetHostByName},
	{netLibTrapSettingGet,			NetLibHeadpatch::NetLibSettingGet,				NetLibTailpatch::NetLibSettingGet},
	{netLibTrapSettingSet,			NetLibHeadpatch::NetLibSettingSet,				NetLibTailpatch::NetLibSettingSet},
	{netLibTrapIFAttach,			NetLibHeadpatch::NetLibIFAttach,				NetLibTailpatch::NetLibIFAttach},
	{netLibTrapIFDetach,			NetLibHeadpatch::NetLibIFDetach,				NetLibTailpatch::NetLibIFDetach},
	{netLibTrapIFGet,				NetLibHeadpatch::NetLibIFGet,					NetLibTailpatch::NetLibIFGet},
	{netLibTrapIFSettingGet,		NetLibHeadpatch::NetLibIFSettingGet,			NetLibTailpatch::NetLibIFSettingGet},
	{netLibTrapIFSettingSet,		NetLibHeadpatch::NetLibIFSettingSet,			NetLibTailpatch::NetLibIFSettingSet},
	{netLibTrapIFUp,				NetLibHeadpatch::NetLibIFUp,					NetLibTailpatch::NetLibIFUp},
	{netLibTrapIFDown,				NetLibHeadpatch::NetLibIFDown,					NetLibTailpatch::NetLibIFDown},
//	{netLibTrapIFMediaUp,			NULL,											NULL},
//	{netLibTrapScriptExecute,		NULL,											NULL},
	{netLibTrapGetHostByAddr,		NetLibHeadpatch::NetLibGetHostByAddr,			NetLibTailpatch::NetLibGetHostByAddr},
	{netLibTrapGetServByName,		NetLibHeadpatch::NetLibGetServByName,			NetLibTailpatch::NetLibGetServByName},
	{netLibTrapSocketAddr,			NetLibHeadpatch::NetLibSocketAddr,				NetLibTailpatch::NetLibSocketAddr},
	{netLibTrapFinishCloseWait,		NetLibHeadpatch::NetLibFinishCloseWait,			NetLibTailpatch::NetLibFinishCloseWait},
	{netLibTrapGetMailExchangeByName,NetLibHeadpatch::NetLibGetMailExchangeByName,	NetLibTailpatch::NetLibGetMailExchangeByName},
//	{netLibTrapPrefsAppend,			NULL,											NULL},
//	{netLibTrapIFMediaDown,			NULL,											NULL},
	{netLibTrapOpenCount,			NetLibHeadpatch::NetLibOpenCount,				NetLibTailpatch::NetLibOpenCount},
	{netLibTrapTracePrintF,			NetLibHeadpatch::NetLibTracePrintF,				NetLibTailpatch::NetLibTracePrintF},
	{netLibTrapTracePutS,			NetLibHeadpatch::NetLibTracePutS,				NetLibTailpatch::NetLibTracePutS},
	{netLibTrapOpenIfCloseWait,		NetLibHeadpatch::NetLibOpenIfCloseWait,			NetLibTailpatch::NetLibOpenIfCloseWait},
	{netLibTrapHandlePowerOff,		NetLibHeadpatch::NetLibHandlePowerOff,			NetLibTailpatch::NetLibHandlePowerOff},
	{netLibTrapConnectionRefresh,	NetLibHeadpatch::NetLibConnectionRefresh,		NetLibTailpatch::NetLibConnectionRefresh},
//	{netLibTrapBitMove,				NULL,											NULL},
//	{netLibTrapBitPutFixed,			NULL,											NULL},
//	{netLibTrapBitGetFixed,			NULL,											NULL},
//	{netLibTrapBitPutUIntV,			NULL,											NULL},
//	{netLibTrapBitGetUIntV,			NULL,											NULL},
//	{netLibTrapBitPutIntV,			NULL,											NULL},
//	{netLibTrapBitGetIntV,			NULL,											NULL},
	{netLibOpenConfig,				NetLibHeadpatch::NetLibOpenConfig,				NetLibTailpatch::NetLibOpenConfig},
	{netLibConfigMakeActive,		NetLibHeadpatch::NetLibConfigMakeActive,		NetLibTailpatch::NetLibConfigMakeActive},
	{netLibConfigList,				NetLibHeadpatch::NetLibConfigList,				NetLibTailpatch::NetLibConfigList},
	{netLibConfigIndexFromName,		NetLibHeadpatch::NetLibConfigIndexFromName,		NetLibTailpatch::NetLibConfigIndexFromName},
	{netLibConfigDelete,			NetLibHeadpatch::NetLibConfigDelete,			NetLibTailpatch::NetLibConfigDelete},
	{netLibConfigSaveAs,			NetLibHeadpatch::NetLibConfigSaveAs,			NetLibTailpatch::NetLibConfigSaveAs},
	{netLibConfigRename,			NetLibHeadpatch::NetLibConfigRename,			NetLibTailpatch::NetLibConfigRename},
	{netLibConfigAliasSet,			NetLibHeadpatch::NetLibConfigAliasSet,			NetLibTailpatch::NetLibConfigAliasSet},
	{netLibConfigAliasGet,			NetLibHeadpatch::NetLibConfigAliasGet,			NetLibTailpatch::NetLibConfigAliasGet},

	{0,								NULL,											NULL}
};


// ======================================================================
//	Private functions
// ======================================================================

static const char*			PrvGetOptLevelString (NetSocketOptLevelEnum);
static const char*			PrvGetOptString (NetSocketOptLevelEnum, NetSocketOptEnum);
static const char*			PrvGetDottedIPString (const NetSocketAddrType& ipAddr);
static const char*			PrvGetPortString (const NetSocketAddrType& ipAddr);
static const char*			PrvGetErrorString (Err err);


#define PRINTF	if (!LogNetLib ()) ; else LogAppendMsg


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

/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibOpen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibOpen (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibOpen");

	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16*	netIFErrsP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_REF(UInt16, netIFErrsP, Marshal::kOutput);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Open (libRefNum, netIFErrsP);

		// Return any pass-by-reference values.
		netIFErrsP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibOpen (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16*	netIFErrsP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
	PARAM_REF(UInt16, netIFErrsP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*netIFErrsP = %s (0x%04X)", PrvGetErrorString (*netIFErrsP), (long) *netIFErrsP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibClose
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibClose (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibClose");

	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16	immediate;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(UInt16, immediate);

	// Examine the parameters
	PRINTF ("\timmediate = 0x%08X", (long) immediate);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Close (libRefNum, immediate);

		// Return any pass-by-reference values.

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibClose (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16	immediate;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(UInt16, immediate);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSleep
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSleep (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSleep");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Sleep (libRefNum);

		// Return any pass-by-reference values.

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}


void NetLibTailpatch::NetLibSleep (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibWake
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibWake (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibWake");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Wake (libRefNum);

		// Return any pass-by-reference values.

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibWake (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibAddrINToA
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibAddrINToA (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibAddrINToA");

	struct StackFrame
	{
		UInt16	libRefNum;
		// !!! TBD
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibAddrINToA (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_areg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibAddrAToIN
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibAddrAToIN (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibAddrAToIN");

	struct StackFrame
	{
		UInt16	libRefNum;
		// !!! TBD
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibAddrAToIN (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketOpen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketOpen (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketOpen");

	struct StackFrame
	{
		UInt16				libRefNum;
		UInt8 /*NetSocketAddrEnum*/	domain;
		UInt8				filler1;
		UInt8 /*NetSocketTypeEnum*/	type;
		UInt8				filler2;
		Int16				protocol;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(UInt8, domain);
	PARAM_VAL(UInt8, type);
	PARAM_VAL(Int16, protocol);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tdomain = 0x%08X", (long) domain);
	PRINTF ("\ttype = 0x%08X", (long) type);
	PRINTF ("\tprotocol = 0x%08X", (long) protocol);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketOpen (libRefNum,
			(NetSocketAddrEnum) (UInt8) domain, (NetSocketTypeEnum) (UInt8) type,
			protocol, timeout, errP);

		// Return any pass-by-reference values.
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketOpen (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		UInt8 /*NetSocketAddrEnum*/	domain;
		UInt8				filler1;
		UInt8 /*NetSocketTypeEnum*/	type;
		UInt8				filler2;
		Int16				protocol;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(UInt8, domain);
//	PARAM_VAL(UInt8, type);
//	PARAM_VAL(Int16, protocol);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketClose
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketClose (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketClose");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketClose (libRefNum,
			socket, timeout, errP);

		// Return any pass-by-reference values.
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketClose (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketOptionSet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketOptionSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketOptionSet");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		UInt16 /*NetSocketOptLevelEnum*/	level;
		UInt16 /*NetSocketOptEnum*/		option;
		MemPtr			optValueP;
		UInt16			optValueLen;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, level);
	PARAM_VAL(UInt16, option);
	PARAM_VAL(UInt16, optValueLen);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	PARAM_PTR(void, optValueP, optValueLen, Marshal::kInput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tlevel = %s (0x%08X)", PrvGetOptLevelString ((NetSocketOptLevelEnum) (UInt16) level), (long) (UInt16) level);
	PRINTF ("\toption = %s (0x%08X)", PrvGetOptString ((NetSocketOptLevelEnum) (UInt16) level, (NetSocketOptEnum) (UInt16) option), (long) (UInt16) option);
	PRINTF ("\toptValueP = 0x%08X", (long) (void*) optValueP);
	PRINTF ("\toptValueLen = 0x%08X", (long) optValueLen);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketOptionSet (libRefNum,
			socket,	(NetSocketOptLevelEnum) (UInt16) level,
			(NetSocketOptEnum) (UInt16) option, optValueP,
			optValueLen, timeout, errP);

		// Return any pass-by-reference values.
		optValueP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketOptionSet (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		UInt16 /*NetSocketOptLevelEnum*/	level;
		UInt16 /*NetSocketOptEnum*/		option;
		MemPtr			optValueP;
		UInt16			optValueLen;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_VAL(UInt16, level);
//	PARAM_VAL(UInt16, option);
	PARAM_VAL(UInt16, optValueLen);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	PARAM_PTR(void, optValueP, optValueLen, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketOptionGet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketOptionGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketOptionGet");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		UInt16 /*NetSocketOptLevelEnum*/	level;
		UInt16 /*NetSocketOptEnum*/		option;
		MemPtr			optValueP;
		UInt16*			optValueLenP;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, level);
	PARAM_VAL(UInt16, option);
	PARAM_REF(UInt16, optValueLenP, Marshal::kInOut);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	PARAM_PTR(void, optValueP, *optValueLenP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tlevel = 0x%08X", (long) level);
	PRINTF ("\toption = 0x%08X", (long) option);
	PRINTF ("\toptValueP = 0x%08X", (long) (void*) optValueP);
	PRINTF ("\toptValueLenP = 0x%08X", (long) *optValueLenP);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketOptionGet (libRefNum,
			socket, (NetSocketOptLevelEnum) (UInt16) level,
			(NetSocketOptEnum) (UInt16) option, optValueP,
			optValueLenP, timeout, errP);

		// Return any pass-by-reference values.
		optValueP.Put ();
		optValueLenP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketOptionGet (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		UInt16 /*NetSocketOptLevelEnum*/	level;
		UInt16 /*NetSocketOptEnum*/		option;
		MemPtr			optValueP;
		UInt16*			optValueLenP;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_VAL(UInt16, level);
//	PARAM_VAL(UInt16, option);
	PARAM_REF(UInt16, optValueLenP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	PARAM_PTR(void, optValueP, *optValueLenP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
	PRINTF ("\t*optValueLenP = 0x%08X", (long) *optValueLenP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketBind
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketBind (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketBind");

	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	sockAddrP;
		Int16				addrLen;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetSocketAddrType, sockAddrP, Marshal::kInput);
	PARAM_VAL(Int16, addrLen);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tsockAddr.family = 0x%08X", (long) (*sockAddrP).family);
	PRINTF ("\tsockAddr.port = %s", PrvGetPortString(*sockAddrP));
	PRINTF ("\tsockAddr.address = %s", PrvGetDottedIPString (*sockAddrP));
	PRINTF ("\taddrLen = 0x%08X", (long) addrLen);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketBind (libRefNum, socket,
			sockAddrP, addrLen, timeout, errP);

		// Return any pass-by-reference values.
		if (m68k_dreg (regs, 0) == 0)
		{
			sockAddrP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketBind (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	sockAddrP;
		Int16				addrLen;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_REF(NetSocketAddrType, sockAddrP, Marshal::kInput);
//	PARAM_VAL(Int16, addrLen);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketConnect
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketConnect (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketConnect");

	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	sockAddrP;
		Int16				addrLen;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetSocketAddrType, sockAddrP, Marshal::kInput);
	PARAM_VAL(Int16, addrLen);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tsockAddr.family = 0x%08X", (long) (*sockAddrP).family);
	PRINTF ("\tsockAddr.port = %s", PrvGetPortString(*sockAddrP));
	PRINTF ("\tsockAddr.address = %s", PrvGetDottedIPString(*sockAddrP));
	PRINTF ("\taddrLen = 0x%08X", (long) addrLen);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketConnect (libRefNum,
			socket, sockAddrP, addrLen, timeout, errP);

		// Return any pass-by-reference values.
		if (m68k_dreg (regs, 0) == 0)
		{
			sockAddrP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketConnect (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	sockAddrP;
		Int16				addrLen;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_REF(NetSocketAddrType, sockAddrP, Marshal::kInput);
//	PARAM_VAL(Int16, addrLen);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketListen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketListen (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketListen");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		UInt16			queueLen;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, queueLen);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tqueueLen = 0x%08X", (long) queueLen);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketListen (libRefNum,
			socket, queueLen, timeout, errP);

		// Return any pass-by-reference values.
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketListen (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		UInt16			queueLen;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_VAL(UInt16, queueLen);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketAccept
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketAccept (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketAccept");

	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	sockAddrP;
		Int16*				addrLenP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetSocketAddrType, sockAddrP, Marshal::kOutput);
	PARAM_REF(Int16, addrLenP, Marshal::kInOut);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\taddrLen = 0x%08X", (long) *addrLenP);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketAccept (libRefNum,
			socket, sockAddrP, addrLenP, timeout, errP);

		// Return any pass-by-reference values.
		if (m68k_dreg (regs, 0) >= 0)
		{
			sockAddrP.Put ();
			addrLenP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketAccept (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	sockAddrP;
		Int16*				addrLenP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetSocketAddrType, sockAddrP, Marshal::kInput);
	PARAM_REF(Int16, addrLenP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
	PRINTF ("\tsockAddr.family = 0x%08X", (long) (*sockAddrP).family);
	PRINTF ("\tsockAddr.port = %s", PrvGetPortString(*sockAddrP));
	PRINTF ("\tsockAddr.address = %s", PrvGetDottedIPString(*sockAddrP));
	PRINTF ("\taddrLen = 0x%08X", (long) *addrLenP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketShutdown
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketShutdown (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketShutdown");

	struct StackFrame
	{
		UInt16						libRefNum;
		NetSocketRef				socket;
		Int16 /*NetSocketDirEnum*/	direction;
		Int32						timeout;
		Err*						errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(Int16, direction);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tdirection = 0x%08X", (long) direction);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketShutdown (libRefNum,
			socket, direction, timeout, errP);

		// Return any pass-by-reference values.
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketShutdown (void)
{
	struct StackFrame
	{
		UInt16						libRefNum;
		NetSocketRef				socket;
		Int16 /*NetSocketDirEnum*/	direction;
		Int32						timeout;
		Err*						errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_VAL(Int16, direction);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSendPB
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSendPB (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSendPB");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		NetIOParamType*	pbP;
		UInt16			flags;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetIOParamType, pbP, Marshal::kInput);
	PARAM_VAL(UInt16, flags);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tflags = 0x%08X", (long) flags);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SendPB (libRefNum,
			socket, pbP, flags, timeout, errP);

		// Return any pass-by-reference values.
		pbP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSendPB (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		NetIOParamType*	pbP;
		UInt16			flags;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_REF(NetIOParamType, pbP, Marshal::kInput);
//	PARAM_VAL(UInt16, flags);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSend
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSend (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSend");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		const MemPtr	bufP;
		UInt16			bufLen;
		UInt16			flags;
		MemPtr			toAddrP;
		UInt16			toLen;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, bufLen);
	PARAM_VAL(UInt16, flags);
	PARAM_REF(NetSocketAddrType, toAddrP, Marshal::kInput);
	PARAM_VAL(UInt16, toLen);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	PARAM_PTR(void, bufP, bufLen, Marshal::kInput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
//	PRINTF ("\tbufP = 0x%08X", (long) bufP);
	PRINTF ("\tbufLen = 0x%08X", (long) bufLen);
	PRINTF ("\tflags = 0x%08X", (long) flags);
//	PRINTF ("\ttoAddrP = 0x%08X", (long) toAddrP);
	PRINTF ("\ttoLen = 0x%08X", (long) toLen);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Send (libRefNum, socket,
			bufP, bufLen, flags, toAddrP, toLen, timeout, errP);

		// Return any pass-by-reference values.
		bufP.Put ();
		toAddrP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSend (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		const MemPtr	bufP;
		UInt16			bufLen;
		UInt16			flags;
		MemPtr			toAddrP;
		UInt16			toLen;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, bufLen);
//	PARAM_VAL(UInt16, flags);
//	PARAM_REF(NetSocketAddrType, toAddrP, Marshal::kInput);
//	PARAM_VAL(UInt16, toLen);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	PARAM_PTR(void, bufP, bufLen, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibReceivePB
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibReceivePB (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibReceivePB");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		NetIOParamType*	pbP;
		UInt16			flags;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetIOParamType, pbP, Marshal::kInOut);
	PARAM_VAL(UInt16, flags);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
//	PRINTF ("\tpbP = 0x%08X", (long) pbP);
	PRINTF ("\tflags = 0x%08X", (long) flags);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::ReceivePB (libRefNum,
			socket, pbP, flags, timeout, errP);

		// Return any pass-by-reference values.
		pbP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibReceivePB (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		NetIOParamType*	pbP;
		UInt16			flags;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
//	PARAM_REF(NetIOParamType, pbP, Marshal::kInput);
//	PARAM_VAL(UInt16, flags);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibReceive
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibReceive (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibReceive");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		MemPtr			bufP;
		UInt16			bufLen;
		UInt16			flags;
		MemPtr			fromAddrP;
		UInt16*			fromLenP;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, bufLen);
	PARAM_VAL(UInt16, flags);
	PARAM_REF(NetSocketAddrType, fromAddrP, Marshal::kOutput);
	PARAM_REF(UInt16, fromLenP, Marshal::kInOut);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	PARAM_PTR(void, bufP, bufLen, Marshal::kInOut);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
//	PRINTF ("\tbufP = 0x%08X", (long) bufP);
	PRINTF ("\tbufLen = 0x%08X", (long) bufLen);
	PRINTF ("\tflags = 0x%08X", (long) flags);
//	PRINTF ("\tfromAddrP = 0x%08X", (long) fromAddrP);
	PRINTF ("\tfromLen = 0x%08X", (long) *fromLenP);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Receive (libRefNum,
			socket, bufP, bufLen, flags, fromAddrP, fromLenP, timeout, errP);

		// Return any pass-by-reference values.
		bufP.Put ();
		fromAddrP.Put ();
		fromLenP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibReceive (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socket;
		MemPtr			bufP;
		UInt16			bufLen;
		UInt16			flags;
		MemPtr			fromAddrP;
		UInt16*			fromLenP;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
	PARAM_VAL(UInt16, bufLen);
//	PARAM_VAL(UInt16, flags);
//	PARAM_REF(NetSocketAddrType, fromAddrP, Marshal::kInput);
//	PARAM_REF(UInt16, fromLenP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	PARAM_PTR(void, bufP, bufLen, Marshal::kInput);


	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);

	if (LogNetLibData () && (((long) *errP) == 0) && (((int32) m68k_dreg (regs, 0)) > 0))
	{
		LogAppendData (bufP, m68k_dreg (regs, 0), "Received Data");
	}
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibDmReceive
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibDmReceive (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibDmReceive");

	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socketRef;
		MemPtr			recordP;
		UInt32			recordOffset;
		UInt16			rcvLen;
		UInt16			flags;
		MemPtr			fromAddrP;
		UInt16*			fromLenP;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socketRef);
	PARAM_VAL(UInt32, recordOffset);
	PARAM_VAL(UInt16, rcvLen);
	PARAM_VAL(UInt16, flags);
	PARAM_REF(NetSocketAddrType, fromAddrP, Marshal::kOutput);
	PARAM_REF(UInt16, fromLenP, Marshal::kInOut);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	PARAM_PTR(void, recordP, rcvLen + recordOffset, Marshal::kInOut);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		CEnableFullAccess	munge;	// Remove blocks on memory access.

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::DmReceive (libRefNum,
			socketRef, recordP, recordOffset, rcvLen, flags,
			fromAddrP, fromLenP, timeout, errP);

		// Return any pass-by-reference values.
		recordP.Put ();
		fromAddrP.Put ();
		fromLenP.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibDmReceive (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		NetSocketRef	socketRef;
		MemPtr			recordP;
		UInt32			recordOffset;
		UInt16			rcvLen;
		UInt16			flags;
		MemPtr			fromAddrP;
		UInt16*			fromLenP;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socketRef);
	PARAM_VAL(UInt32, recordOffset);
	PARAM_VAL(UInt16, rcvLen);
//	PARAM_VAL(UInt16, flags);
//	PARAM_REF(NetSocketAddrType, fromAddrP, Marshal::kInput);
//	PARAM_REF(UInt16, fromLenP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	PARAM_PTR(void, recordP, recordOffset + rcvLen, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSelect
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSelect (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSelect");

	struct StackFrame
	{
		UInt16			libRefNum;
		UInt16			width;
		NetFDSetType*	readFDs;
		NetFDSetType*	writeFDs;
		NetFDSetType*	exceptFDs;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(UInt16, width);
	PARAM_REF(NetFDSetType, readFDs, Marshal::kInOut);
	PARAM_REF(NetFDSetType, writeFDs, Marshal::kInOut);
	PARAM_REF(NetFDSetType, exceptFDs, Marshal::kInOut);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\twidth = 0x%08X", (long) width);
	PRINTF ("\treadFDs = 0x%08X", (long) *readFDs);
	PRINTF ("\twriteFDs = 0x%08X", (long) *writeFDs);
	PRINTF ("\texceptFDs = 0x%08X", (long) *exceptFDs);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::Select (libRefNum,
			width, readFDs, writeFDs, exceptFDs, timeout, errP);

		// Return any pass-by-reference values.
		readFDs.Put ();
		writeFDs.Put ();
		exceptFDs.Put ();
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSelect (void)
{
	struct StackFrame
	{
		UInt16			libRefNum;
		UInt16			width;
		NetFDSetType*	readFDs;
		NetFDSetType*	writeFDs;
		NetFDSetType*	exceptFDs;
		Int32			timeout;
		Err*			errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(UInt16, width);
	PARAM_REF(NetFDSetType, readFDs, Marshal::kInput);
	PARAM_REF(NetFDSetType, writeFDs, Marshal::kInput);
	PARAM_REF(NetFDSetType, exceptFDs, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
	PRINTF ("\treadFDs = 0x%08X", (long) *readFDs);
	PRINTF ("\twriteFDs = 0x%08X", (long) *writeFDs);
	PRINTF ("\texceptFDs = 0x%08X", (long) *exceptFDs);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibMaster
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibMaster (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibMaster");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibMaster (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibGetHostByName
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibGetHostByName (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibGetHostByName");

	struct StackFrame
	{
		UInt16				libRefNum;
		Char*				nameP;
		NetHostInfoBufPtr	bufP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_STR(Char, nameP);
	PARAM_REF(NetHostInfoBufType, bufP, Marshal::kOutput);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		NetHostInfoPtr	result = Platform_NetLib::GetHostByName (libRefNum,
			nameP, bufP, timeout, errP);
		m68k_areg (regs, 0) = result ? (emuptr) bufP + offsetof (NetHostInfoBufType, hostInfo) : EmMemNULL;

		// Return any pass-by-reference values.
			// bufP is a complex type with internal pointers and such.  We
			// *can't* copy the contents back into emulated memory if they
			// are uninitialized.  Check for errors before attempting that.
		if (m68k_areg (regs, 0) != EmMemNULL)
		{
			nameP.Put ();
			bufP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibGetHostByName (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		Char*				nameP;
		NetHostInfoBufPtr	bufP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
	PARAM_STR(Char, nameP);
//	PARAM_REF(NetHostInfoBufType, bufP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_areg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSettingGet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSettingGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSettingGet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSettingGet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSettingSet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSettingSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSettingSet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSettingSet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFAttach
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFAttach (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFAttach");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFAttach (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFDetach
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFDetach (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFDetach");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFDetach (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFGet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFGet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFGet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFSettingGet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFSettingGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFSettingGet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFSettingGet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFSettingSet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFSettingSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFSettingSet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFSettingSet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFUp
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFUp (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFUp");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFUp (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibIFDown
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibIFDown (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibIFDown");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibIFDown (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibGetHostByAddr
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibGetHostByAddr (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibGetHostByAddr");

	struct StackFrame
	{
		UInt16				libRefNum;
		UInt8*				addrP;
		UInt16				len;
		UInt16				type;
		NetHostInfoBufPtr	bufP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(UInt16, len);
	PARAM_PTR(UInt8, addrP, len, Marshal::kInput);
	PARAM_VAL(UInt16, type);
	PARAM_REF(NetHostInfoBufType, bufP, Marshal::kOutput);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		NetHostInfoPtr	result	= Platform_NetLib::GetHostByAddr (libRefNum,
			(UInt8*) addrP, len, type, bufP, timeout, errP);
		m68k_areg (regs, 0) = result ? (emuptr) bufP + offsetof (NetHostInfoBufType, hostInfo) : EmMemNULL;

		// Return any pass-by-reference values.
			// bufP is a complex type with internal pointers and such.  We
			// *can't* copy the contents back into emulated memory if they
			// are uninitialized.  Check for errors before attempting that.
		if (m68k_areg (regs, 0) != EmMemNULL)
		{
			addrP.Put ();
			bufP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibGetHostByAddr (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		UInt8*				addrP;
		UInt16				len;
		UInt16				type;
		NetHostInfoBufPtr	bufP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_REF(NetSocketAddrType, addrP, Marshal::kInput);
//	PARAM_VAL(UInt16, len);
//	PARAM_VAL(UInt16, type);
//	PARAM_REF(NetHostInfoBufType, bufP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_areg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibGetServByName
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibGetServByName (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibGetServByName");

	struct StackFrame
	{
		UInt16				libRefNum;
		Char*				servNameP;
		Char*				protoNameP;
		NetServInfoBufPtr	bufP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_STR(Char, servNameP);
	PARAM_STR(Char, protoNameP);
	PARAM_REF(NetServInfoBufType, bufP, Marshal::kOutput);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		NetServInfoPtr	result	= Platform_NetLib::GetServByName (libRefNum,
			servNameP, protoNameP, bufP, timeout, errP);
		m68k_areg (regs, 0) = result ? (emuptr) bufP + offsetof (NetServInfoBufType, servInfo) : EmMemNULL;

		// Return any pass-by-reference values.
			// bufP is a complex type with internal pointers and such.  We
			// *can't* copy the contents back into emulated memory if they
			// are uninitialized.  Check for errors before attempting that.
		if (m68k_areg (regs, 0) != EmMemNULL)
		{
			servNameP.Put ();
			protoNameP.Put ();
			bufP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibGetServByName (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		Char*				servNameP;
		Char*				protoNameP;
		NetServInfoBufPtr	bufP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
	PARAM_STR(Char, servNameP);
	PARAM_STR(Char, protoNameP);
//	PARAM_REF(NetServInfoBufType, bufP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_areg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibSocketAddr
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibSocketAddr (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibSocketAddr");

	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	locAddrP;
		Int16*				locAddrLenP;
		NetSocketAddrType*	remAddrP;
		Int16*				remAddrLenP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetSocketAddrType, locAddrP, Marshal::kOutput);
	PARAM_REF(Int16, locAddrLenP, Marshal::kInOut);
	PARAM_REF(NetSocketAddrType, remAddrP, Marshal::kOutput);
	PARAM_REF(Int16, remAddrLenP, Marshal::kInOut);
	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tsocket = 0x%08X", (long) socket);
	PRINTF ("\tlocAddrLen = 0x%08X", (long) *locAddrLenP);
	PRINTF ("\tremAddrLen = 0x%08X", (long) *remAddrLenP);
	PRINTF ("\ttimeout = 0x%08X", (long) timeout);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::SocketAddr (libRefNum,
			socket, locAddrP, locAddrLenP, remAddrP, remAddrLenP, timeout, errP);

		// Return any pass-by-reference values.
		if (m68k_dreg (regs, 0) == 0)
		{
			locAddrP.Put ();
			locAddrLenP.Put ();
			remAddrP.Put ();
			remAddrLenP.Put ();
		}
		errP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibSocketAddr (void)
{
	struct StackFrame
	{
		UInt16				libRefNum;
		NetSocketRef		socket;
		NetSocketAddrType*	locAddrP;
		Int16*				locAddrLenP;
		NetSocketAddrType*	remAddrP;
		Int16*				remAddrLenP;
		Int32				timeout;
		Err*				errP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(NetSocketRef, socket);
	PARAM_REF(NetSocketAddrType, locAddrP, Marshal::kInput);
	PARAM_REF(Int16, locAddrLenP, Marshal::kInput);
	PARAM_REF(NetSocketAddrType, remAddrP, Marshal::kInput);
	PARAM_REF(Int16, remAddrLenP, Marshal::kInput);
//	PARAM_VAL(Int32, timeout);
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
	PRINTF ("\tlocAddr.family = 0x%08X", (long) (*locAddrP).family);
	PRINTF ("\tlocAddr.port = %s", PrvGetPortString(*locAddrP));
	PRINTF ("\tlocAddr.address = %s", PrvGetDottedIPString(*locAddrP));
	PRINTF ("\tlocAddrLen = 0x%08X", (long) *locAddrLenP);
	PRINTF ("\tremAddr.family = 0x%08X", (long) (*remAddrP).family);
	PRINTF ("\tremAddr.port = %s", PrvGetPortString(*remAddrP));
	PRINTF ("\tremAddr.address = %s", PrvGetDottedIPString(*remAddrP));
	PRINTF ("\tremAddrLen = 0x%08X", (long) *remAddrLenP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibFinishCloseWait
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibFinishCloseWait (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibFinishCloseWait");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::FinishCloseWait (libRefNum);

		// Return any pass-by-reference values.

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibFinishCloseWait (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibGetMailExchangeByName
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibGetMailExchangeByName (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibGetMailExchangeByName");

	struct StackFrame
	{
		UInt16	libRefNum;
		Char*	mailNameP;
		UInt16	maxEntries;
		Char**	hostNames;
		UInt16*	priorities;
		Int32	timeout;
		Err*	errP;
	};

	// Get the stack-based parameters.
//	UInt16			libRefNum	= GET_PARAMETER (libRefNum);
//	emuptr			mailNameP	= GET_PARAMETER (mailNameP);
//	UInt16			maxEntries	= GET_PARAMETER (maxEntries);
//	emuptr			hostNamesP	= GET_PARAMETER (hostNames);
//	emuptr			prioritiesP	= GET_PARAMETER (priorities);
//	Int32			timeout		= GET_PARAMETER (timeout);
//	emuptr			errP		= GET_PARAMETER (errP);

	// Get any pass-by-reference values.
//	Char*			mailName	= GetString (mailNameP);
//	Char*			hostNamesP	= GetString (hostNamesP);	// !!!Need to handle this!
//	UInt16			priorities	= EmMemGet16 (prioritiesP);
//	Err				err			= EmMemGet16 (errP);

#if 0
	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		Int16			result		= Platform_NetLib::GetMailExchangeByName (	libRefNum,
																				mailName,
																				maxEntries,
																				NULL,
																				&priorities,
																				timeout,
																				&err);

		// Return any pass-by-reference values.
		PutString (mailNameP, mailName);
//		PutString (hostNamesP, hostNames);	// !!!Need to handle this!
		EmMemPut16 (prioritiesP, priorities);
		EmMemPut16 (errP, err);

		// Return the host function result.
		m68k_dreg (regs, 0)			= (uint32) result;
		PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
		PRINTF ("\t*errP = 0x%08X", (long) err);

		return kSkipROM;
	}
#endif

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibGetMailExchangeByName (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
		Char*	mailNameP;
		UInt16	maxEntries;
		Char**	hostNames;
		UInt16*	priorities;
		Int32	timeout;
		Err*	errP;
	};

	// Get the stack-based parameters.
	PARAM_REF(Err, errP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_areg (regs, 0));
	PRINTF ("\t*errP = %s (0x%04X)", PrvGetErrorString (*errP), (long) *errP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibOpenCount
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibOpenCount (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibOpenCount");

	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16*	countP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_REF(UInt16, countP, Marshal::kOutput);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::OpenCount (libRefNum, countP);

		// Return any pass-by-reference values.
		countP.Put();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibOpenCount (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16*	countP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
	PARAM_REF(UInt16, countP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\tcount = 0x%08X", (long) *countP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibTracePrintF
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibTracePrintF (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibTracePrintF");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibTracePrintF (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibTracePutS
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibTracePutS (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibTracePutS");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibTracePutS (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibOpenIfCloseWait
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibOpenIfCloseWait (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibOpenIfCloseWait");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::OpenIfCloseWait (libRefNum);

		// Return any pass-by-reference values.

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibOpenIfCloseWait (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibHandlePowerOff
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibHandlePowerOff (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibHandlePowerOff");

	struct StackFrame
	{
		UInt16		libRefNum;
		EventPtr	eventP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_REF(EventType, eventP, Marshal::kInput);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::HandlePowerOff (libRefNum, eventP);

		// Return any pass-by-reference values.
		eventP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibHandlePowerOff (void)
{
	struct StackFrame
	{
		UInt16		libRefNum;
		EventPtr	eventP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_REF(EventType, eventP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConnectionRefresh
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConnectionRefresh (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConnectionRefresh");

	struct StackFrame
	{
		UInt16		libRefNum;
		Boolean		refresh;
		Boolean*	allInterfacesUpP;
		UInt16*		netIFErrP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(Boolean, refresh);
	PARAM_REF(Boolean, allInterfacesUpP, Marshal::kOutput);
	PARAM_REF(UInt16, netIFErrP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\trefresh = 0x%08X", (long) refresh);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::ConnectionRefresh (	libRefNum,
			refresh, allInterfacesUpP, netIFErrP);

		// Return any pass-by-reference values.
		allInterfacesUpP.Put ();
		netIFErrP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConnectionRefresh (void)
{
	struct StackFrame
	{
		UInt16		libRefNum;
		Boolean		refresh;
		Boolean*	allInterfacesUpP;
		UInt16*		netIFErrP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(Boolean, refresh);
	PARAM_REF(Boolean, allInterfacesUpP, Marshal::kInput);
	PARAM_REF(UInt16, netIFErrP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*netIFErrP = %s (0x%04X)", PrvGetErrorString (*netIFErrP), (long) *netIFErrP);
	PRINTF ("\t*allInterfacesUpP = 0x%08X", (long) *allInterfacesUpP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibOpenConfig
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibOpenConfig (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibOpenConfig");

	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16	configIndex;
		UInt32	openFlags;
		UInt16*	netIFErrP;
	};

	// Get the stack-based parameters.
	PARAM_VAL(UInt16, libRefNum);
	PARAM_VAL(UInt16, configIndex);
	PARAM_VAL(UInt16, openFlags);
	PARAM_REF(UInt16, netIFErrP, Marshal::kOutput);

	// Examine the parameters
	PRINTF ("\tconfigIndex = 0x%08X", (long) configIndex);
	PRINTF ("\topenFlags = 0x%08X", (long) openFlags);

	if (Platform_NetLib::Redirecting ())
	{
		PRINTF ("\t-->Executing host version");

		// Call the host function.
		m68k_dreg (regs, 0) = Platform_NetLib::OpenConfig (libRefNum,
			configIndex, openFlags, netIFErrP);

		// Return any pass-by-reference values.
		netIFErrP.Put ();

		return kSkipROM;
	}

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibOpenConfig (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16	configIndex;
		UInt32	openFlags;
		UInt16*	netIFErrP;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);
//	PARAM_VAL(UInt16, configIndex);
//	PARAM_VAL(UInt16, openFlags);
	PARAM_REF(UInt16, netIFErrP, Marshal::kInput);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
	PRINTF ("\t*netIFErrP = %s (0x%04X)", PrvGetErrorString (*netIFErrP), (long) *netIFErrP);
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigMakeActive
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigMakeActive (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigMakeActive");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigMakeActive (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigList
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigList (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigList");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigList (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigIndexFromName
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigIndexFromName (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigIndexFromName");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigIndexFromName (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigDelete
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigDelete (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigDelete");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigDelete (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigSaveAs
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigSaveAs (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigSaveAs");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigSaveAs (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigRename
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigRename (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigRename");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigRename (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigAliasSet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigAliasSet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigAliasSet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigAliasSet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	NetLibHeadpatch::NetLibConfigAliasGet
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

CallROMType NetLibHeadpatch::NetLibConfigAliasGet (void)
{
	PRINTF ("----------------------------------------");
	PRINTF ("NetLibConfigAliasGet");

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the parameters

	PRINTF ("\t-->Executing ROM version");
	return kExecuteROM;
}

void NetLibTailpatch::NetLibConfigAliasGet (void)
{
	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the stack-based parameters.
//	PARAM_VAL(UInt16, libRefNum);

	// Examine the results.
	PRINTF ("\tResult = 0x%08X", m68k_dreg (regs, 0));
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetOptLevelString
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

#define DOCASE(name) case name: return #name;

const char* PrvGetOptLevelString (NetSocketOptLevelEnum level)
{
	switch (level)
	{
		DOCASE (netSocketOptLevelIP)
		DOCASE (netSocketOptLevelTCP)
		DOCASE (netSocketOptLevelSocket)
	}

	return "Unknown NetSocketOptLevelEnum";
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetOptString
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

const char* PrvGetOptString (NetSocketOptLevelEnum level, NetSocketOptEnum opt)
{
	switch (level)
	{
		case netSocketOptLevelIP:
			switch (opt)
			{
				DOCASE (netSocketOptIPOptions)

//				case netSocketOptTCPNoDelay:		== 1 (netSocketOptIPOptions)
				case netSocketOptTCPMaxSeg:

//				case netSocketOptSockDebug:			== 1 (netSocketOptIPOptions)
//				case netSocketOptSockAcceptConn:	== 2 (netSocketOptTCPMaxSeg)
				case netSocketOptSockReuseAddr:
				case netSocketOptSockKeepAlive:
				case netSocketOptSockDontRoute:
				case netSocketOptSockBroadcast:
				case netSocketOptSockUseLoopback:
				case netSocketOptSockLinger:
				case netSocketOptSockOOBInLine:
				case netSocketOptSockSndBufSize:
				case netSocketOptSockRcvBufSize:
				case netSocketOptSockSndLowWater:
				case netSocketOptSockRcvLowWater:
				case netSocketOptSockSndTimeout:
				case netSocketOptSockRcvTimeout:
				case netSocketOptSockErrorStatus:
				case netSocketOptSockSocketType:
				case netSocketOptSockNonBlocking:
				case netSocketOptSockRequireErrClear:
				case netSocketOptSockMultiPktAddr:
					break;
			}
			break;

		case netSocketOptLevelTCP:
			switch (opt)
			{
//				case netSocketOptIPOptions:			== 1 (netSocketOptTCPNoDelay)
//					break;

				DOCASE (netSocketOptTCPNoDelay)
				DOCASE (netSocketOptTCPMaxSeg)

//				case netSocketOptSockDebug:			== 1 (netSocketOptTCPNoDelay)
//				case netSocketOptSockAcceptConn:	== 2 (netSocketOptTCPMaxSeg)
				case netSocketOptSockReuseAddr:
				case netSocketOptSockKeepAlive:
				case netSocketOptSockDontRoute:
				case netSocketOptSockBroadcast:
				case netSocketOptSockUseLoopback:
				case netSocketOptSockLinger:
				case netSocketOptSockOOBInLine:
				case netSocketOptSockSndBufSize:
				case netSocketOptSockRcvBufSize:
				case netSocketOptSockSndLowWater:
				case netSocketOptSockRcvLowWater:
				case netSocketOptSockSndTimeout:
				case netSocketOptSockRcvTimeout:
				case netSocketOptSockErrorStatus:
				case netSocketOptSockSocketType:
				case netSocketOptSockNonBlocking:
				case netSocketOptSockRequireErrClear:
				case netSocketOptSockMultiPktAddr:
					break;
			}
			break;

		case netSocketOptLevelSocket:
			switch (opt)
			{
//				case netSocketOptIPOptions:
//				case netSocketTCPNoDelay:
//				case netSocketTCPMaxSeg:
//					break;

				DOCASE (netSocketOptSockDebug)
				DOCASE (netSocketOptSockAcceptConn)
				DOCASE (netSocketOptSockReuseAddr)
				DOCASE (netSocketOptSockKeepAlive)
				DOCASE (netSocketOptSockDontRoute)
				DOCASE (netSocketOptSockBroadcast)
				DOCASE (netSocketOptSockUseLoopback)
				DOCASE (netSocketOptSockLinger)
				DOCASE (netSocketOptSockOOBInLine)

				DOCASE (netSocketOptSockSndBufSize)
				DOCASE (netSocketOptSockRcvBufSize)
				DOCASE (netSocketOptSockSndLowWater)
				DOCASE (netSocketOptSockRcvLowWater)
				DOCASE (netSocketOptSockSndTimeout)
				DOCASE (netSocketOptSockRcvTimeout)
				DOCASE (netSocketOptSockErrorStatus)
				DOCASE (netSocketOptSockSocketType)

				DOCASE (netSocketOptSockNonBlocking)
				DOCASE (netSocketOptSockRequireErrClear)
				DOCASE (netSocketOptSockMultiPktAddr)
			}
			break;
	}

	return "Unknown NetSocketOptEnum";
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetDottedIPString
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

const char* PrvGetDottedIPString (const NetSocketAddrType& addr)
{
	NetSocketAddrINType	ipAddr = (const NetSocketAddrINType&) addr;
	long				tempIP = ntohl (ipAddr.addr);

	static char			dottedIPStr[20];

	sprintf (dottedIPStr, "%ld.%ld.%ld.%ld",
		((tempIP >> 24) & 0xFF),
		((tempIP >> 16) & 0xFF),
		((tempIP >> 8) & 0xFF),
		((tempIP) & 0xFF));

	return dottedIPStr;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetPortString
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

const char* PrvGetPortString (const NetSocketAddrType& addr)
{
	NetSocketAddrINType	ipAddr = (const NetSocketAddrINType&) addr;
	UInt16				port = ntohs (ipAddr.port);

	static char			portStr[10];

	sprintf (portStr, "%lu", (long) port);

	return portStr;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetErrorString
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

const char* PrvGetErrorString (Err err)
{
	switch (err)
	{
		DOCASE (errNone)
		DOCASE (netErrAlreadyOpen)
		DOCASE (netErrNotOpen)
		DOCASE (netErrStillOpen)
		DOCASE (netErrParamErr)
		DOCASE (netErrNoMoreSockets)
		DOCASE (netErrOutOfResources)
		DOCASE (netErrOutOfMemory)
		DOCASE (netErrSocketNotOpen)
		DOCASE (netErrSocketBusy)
		DOCASE (netErrMessageTooBig)
		DOCASE (netErrSocketNotConnected)
		DOCASE (netErrNoInterfaces)
		DOCASE (netErrBufTooSmall)
		DOCASE (netErrUnimplemented)
		DOCASE (netErrPortInUse)
		DOCASE (netErrQuietTimeNotElapsed)
		DOCASE (netErrInternal)
		DOCASE (netErrTimeout)
		DOCASE (netErrSocketAlreadyConnected)
		DOCASE (netErrSocketClosedByRemote)
		DOCASE (netErrOutOfCmdBlocks)
		DOCASE (netErrWrongSocketType)
		DOCASE (netErrSocketNotListening)
		DOCASE (netErrUnknownSetting)
		DOCASE (netErrInvalidSettingSize)
		DOCASE (netErrPrefNotFound)
		DOCASE (netErrInvalidInterface)
		DOCASE (netErrInterfaceNotFound)
		DOCASE (netErrTooManyInterfaces)
		DOCASE (netErrBufWrongSize)
		DOCASE (netErrUserCancel)
		DOCASE (netErrBadScript)
		DOCASE (netErrNoSocket)
		DOCASE (netErrSocketRcvBufFull)
		DOCASE (netErrNoPendingConnect)
		DOCASE (netErrUnexpectedCmd)
		DOCASE (netErrNoTCB)
		DOCASE (netErrNilRemoteWindowSize)
		DOCASE (netErrNoTimerProc)
		DOCASE (netErrSocketInputShutdown)
		DOCASE (netErrCmdBlockNotCheckedOut)
		DOCASE (netErrCmdNotDone)
		DOCASE (netErrUnknownProtocol)
		DOCASE (netErrUnknownService)
		DOCASE (netErrUnreachableDest)
		DOCASE (netErrReadOnlySetting)
		DOCASE (netErrWouldBlock)
		DOCASE (netErrAlreadyInProgress)
		DOCASE (netErrPPPTimeout)
		DOCASE (netErrPPPBroughtDown)
		DOCASE (netErrAuthFailure)
		DOCASE (netErrPPPAddressRefused)
		DOCASE (netErrDNSNameTooLong)
		DOCASE (netErrDNSBadName)
		DOCASE (netErrDNSBadArgs)
		DOCASE (netErrDNSLabelTooLong)
		DOCASE (netErrDNSAllocationFailure)
		DOCASE (netErrDNSTimeout)
		DOCASE (netErrDNSUnreachable)
		DOCASE (netErrDNSFormat)
		DOCASE (netErrDNSServerFailure)
		DOCASE (netErrDNSNonexistantName)
		DOCASE (netErrDNSNIY)
		DOCASE (netErrDNSRefused)
		DOCASE (netErrDNSImpossible)
		DOCASE (netErrDNSNoRRS)
		DOCASE (netErrDNSAborted)
		DOCASE (netErrDNSBadProtocol)
		DOCASE (netErrDNSTruncated)
		DOCASE (netErrDNSNoRecursion)
		DOCASE (netErrDNSIrrelevant)
		DOCASE (netErrDNSNotInLocalCache)
		DOCASE (netErrDNSNoPort)
		DOCASE (netErrIPCantFragment)
		DOCASE (netErrIPNoRoute)
		DOCASE (netErrIPNoSrc)
		DOCASE (netErrIPNoDst)
		DOCASE (netErrIPktOverflow)
		DOCASE (netErrTooManyTCPConnections)
		DOCASE (netErrNoDNSServers)
		DOCASE (netErrInterfaceDown)
		DOCASE (netErrNoChannel)
		DOCASE (netErrDieState)
		DOCASE (netErrReturnedInMail)
		DOCASE (netErrReturnedNoTransfer)
		DOCASE (netErrReturnedIllegal)
		DOCASE (netErrReturnedCongest)
		DOCASE (netErrReturnedError)
		DOCASE (netErrReturnedBusy)
		DOCASE (netErrGMANState)
		DOCASE (netErrQuitOnTxFail)
		DOCASE (netErrFlexListFull)
		DOCASE (netErrSenderMAN)
		DOCASE (netErrIllegalType)
		DOCASE (netErrIllegalState)
		DOCASE (netErrIllegalFlags)
		DOCASE (netErrIllegalSendlist)
		DOCASE (netErrIllegalMPAKLength)
		DOCASE (netErrIllegalAddressee)
		DOCASE (netErrIllegalPacketClass)
		DOCASE (netErrBufferLength)
		DOCASE (netErrNiCdLowBattery)
		DOCASE (netErrRFinterfaceFatal)
		DOCASE (netErrIllegalLogout)
		DOCASE (netErrAAARadioLoad)
		DOCASE (netErrAntennaDown)
		DOCASE (netErrNiCdCharging)
		DOCASE (netErrAntennaWentDown)
		DOCASE (netErrNotActivated)
		DOCASE (netErrRadioTemp)
		DOCASE (netErrConfigNotFound)
		DOCASE (netErrConfigCantDelete)
		DOCASE (netErrConfigTooMany)
		DOCASE (netErrConfigBadName)
		DOCASE (netErrConfigNotAlias)
		DOCASE (netErrConfigCantPointToAlias)
		DOCASE (netErrConfigEmpty)
		DOCASE (netErrAlreadyOpenWithOtherConfig)
		DOCASE (netErrConfigAliasErr)
		DOCASE (netErrNoMultiPktAddr)
		DOCASE (netErrOutOfPackets)
		DOCASE (netErrMultiPktAddrReset)
		DOCASE (netErrStaleMultiPktAddr)
	}

	return "Unknown Error Code";
}


#include "PalmPackPop.h"
