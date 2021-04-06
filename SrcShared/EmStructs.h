/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef EmStructs_h
#define EmStructs_h

#include "EmFileRef.h"			// EmFileRef
#include "EmDevice.h"			// EmDevice

#include <deque>
#include <map>
#include <string>
#include <vector>

// ---------- Collections ----------

typedef vector<ScaleType>		ScaleList;
typedef vector<RAMSizeType>		RAMSizeList;

typedef vector<uint8>			ByteList;
typedef vector<string>			StringList;

typedef map<string, string>		StringStringMap;
typedef vector<CloseActionType>	CloseActionList;
typedef vector<LoggingType>		LoggingList;


// ---------- RGBType ----------

struct RGBType
{
	RGBType (void) {}
	RGBType (uint8 red, uint8 green, uint8 blue) :
		fRed (red),
		fGreen (green),
		fBlue (blue)
	{}

	bool operator== (const RGBType& rhs) const
	{
		return fRed == rhs.fRed && fGreen == rhs.fGreen && fBlue == rhs.fBlue;
	}

	bool operator!= (const RGBType& rhs) const
	{
		return !(*this == rhs);
	}

	uint8		fRed;
	uint8		fGreen;
	uint8		fBlue;
	uint8		fFiller;	// Pad this out to 4 bytes to make array lookups more efficient.
};
typedef vector<RGBType>	RGBList;


// ---------- SystemCallContext ----------

struct SystemCallContext
{
	emuptr		fPC;			// PC at which the system call was made.
	emuptr		fNextPC;		// PC of instruction after system call.
	emuptr		fDestPC1;		// Address of system function (in the trap table).
	emuptr		fDestPC2;		// Address of system function (if subdispatching is involved).
	uint16		fTrapWord;		// Trapword, even for SYSTRAP_FAST calls.
	uint16		fTrapIndex;		// Trapword with the high 4 bits removed.
	emuptr		fExtra;			// RefNum for library calls, D2 for "dispatch" calls.
	Bool		fViaTrap;		// True if called via TRAP $F.
	Bool		fViaJsrA1;		// True if called via SYSTRAP_FASTER.

	long		fError;			// If an error occurred getting the context, error is here
	long		fLibIndex;		// Resource base number for function name
	long		fMaxRefNum;		// If the refNum was too big, this is the max value it could be.
};
typedef vector<SystemCallContext>	SystemCallContextList;


// ---------- DatabaseInfo ----------

struct DatabaseInfo
{
	UInt32		creator;
	UInt32		type;
	UInt16		version;
	LocalID		dbID;
	UInt16 		cardNo;
	UInt32		modDate;
	UInt16		dbAttrs;

	char		name[dmDBNameLength];
	char		dbName[dmDBNameLength];

	Bool operator < (const DatabaseInfo& other) const
		{ if (creator < other.creator) return true;
		  if (creator > other.creator) return false;
		  if (type < other.type) return true;
		  return false; }

	Bool operator == (const DatabaseInfo& other) const
		{ return (creator == other.creator) &&
				 (type == other.type); }
};
typedef vector<DatabaseInfo>	DatabaseInfoList;


// ---------- SlotInfoType ----------

struct SlotInfoType
{
	long		fSlotNumber;
	Bool		fSlotOccupied;
	EmDirRef	fSlotRoot;
};
typedef vector<SlotInfoType>	SlotInfoList;


// ---------- Configuration ----------

#ifdef SONY_ROM

typedef int32	MSSizeType;			// MemoryStick size		// for Sony & MemoryStick 
#define	MSSIZE_DEFAULT	8			// Default MS size		// for Sony & MemoryStick 

struct Configuration
{
	Configuration () :
		fDevice (),
		fRAMSize (1024),
		fROMFile (),
		fMSSize (MSSIZE_DEFAULT)
	{
	}

	Configuration (const EmDevice& dt, long size, const EmFileRef& rom, long mssize) :
		fDevice (dt),
		fRAMSize (size),
		fROMFile (rom),
		fMSSize (mssize)
	{
	}

	EmDevice				fDevice;
	RAMSizeType				fRAMSize;
	EmFileRef				fROMFile;
	MSSizeType				fMSSize;	// for Sony & MemoryStick				
};

#else // SONY_ROM

struct Configuration
{
	Configuration () :
		fDevice (),
		fRAMSize (1024),
		fROMFile ()
	{
	}

	Configuration (const EmDevice& dt, long size, const EmFileRef& rom) :
		fDevice (dt),
		fRAMSize (size),
		fROMFile (rom)
	{
	}

	EmDevice				fDevice;
	RAMSizeType				fRAMSize;
	EmFileRef				fROMFile;
};

#endif	// SONY_ROM

typedef vector<Configuration>	ConfigurationList;


// ---------- GremlinInfo ----------

struct GremlinInfo
{
	GremlinInfo () :
		fNumber (0),
		fSteps (-1),
		fSaveFrequency (10000),
		fAppList ()
	{
	}

	long				fNumber;
	long				fSteps;
	long				fSaveFrequency;
	DatabaseInfoList	fAppList;
};
typedef vector<GremlinInfo>	GremlinInfoList;


// ---------- HordeInfo ----------

struct HordeInfo
{
	HordeInfo () :
		fStartNumber (0),
		fStopNumber (0),
		fDepthSwitch (30000),
		fDepthSave (10000),
		fDepthStop (1000000),
		fCanSwitch (false),
		fCanSave (false),
		fCanStop (false),
		fAppList ()

	{
		NewToOld ();
	}

	long				fStartNumber;
	long				fStopNumber;

	long				fDepthSwitch;
	long				fDepthSave;
	long				fDepthStop;

	Bool				fCanSwitch;
	Bool				fCanSave;
	Bool				fCanStop;

	DatabaseInfoList	fAppList;

	// Old fields that I want to get rid of, but
	// I need to update Gremlins and Hordes first.

	long				fSaveFrequency;
	long				fSwitchDepth;
	long				fMaxDepth;

	void NewToOld (void)
	{
		fSwitchDepth	= fCanSwitch ? fDepthSwitch : -1;
		fSaveFrequency	= fCanSave ? fDepthSave : 0;
		fMaxDepth		= fCanStop ? fDepthStop : -1;
	}

	void OldToNew (void)
	{
		fCanSwitch = fSwitchDepth > 0;
		fDepthSwitch = fCanSwitch ? fSwitchDepth : 30000;

		fCanSave = fSaveFrequency > 0;
		fDepthSave = fCanSave ? fSaveFrequency : 10000;

		fCanStop = fMaxDepth > 0;
		fDepthStop = fCanStop ? fMaxDepth : 1000000;
	}
};
typedef vector<HordeInfo>	HordeInfoList;


// ---------- SysLibTblEntryTypeV10 ----------

typedef struct SysLibTblEntryTypeV10 {
	MemPtr*			dispatchTblP;					// pointer to library dispatch table
	void*			globalsP;						// Library globals
	} SysLibTblEntryTypeV10;
typedef SysLibTblEntryTypeV10*	SysLibTblEntryV10Ptr;


// ---------- EmStackFrame ----------

struct EmStackFrame
{
	emuptr	fAddressInFunction;
	emuptr	fA6;
};

typedef vector<EmStackFrame>	EmStackFrameList;



#endif	// EmStructs_h
