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
#include "ROMStubs.h"

#include "ATraps.h"				// ATrap
#include "EmBankMapped.h"		// EmBankMapped::GetEmulatedAddress
#include "Byteswapping.h"		// Canonical
#include "Logging.h"
#include "Miscellaneous.h"		// StMemoryMapper
#include "UAE_Utils.h"			// uae_memmove

// (I actually tried using function templates here, but they ended up adding
//  more code, instead of saving code by coalescing identical functions like
//  I'd hoped.)

#define PushParm(p)		PushParameter (theTrap, p)

// Functions for pushing simple 1-, 2-, and 4- byte values onto the emulated stack.

inline void PushParameter (ATrap& theTrap, uint32 p)		{ theTrap.PushLong (p); }
inline void PushParameter (ATrap& theTrap, int32 p)			{ theTrap.PushLong (p); }
inline void PushParameter (ATrap& theTrap, uint16 p)		{ theTrap.PushWord (p); }
inline void PushParameter (ATrap& theTrap, int16 p)			{ theTrap.PushWord (p); }
inline void PushParameter (ATrap& theTrap, uint8 p)			{ theTrap.PushByte (p); }
inline void PushParameter (ATrap& theTrap, int8 p)			{ theTrap.PushByte (p); }


#define PUSH_VALUE(v)									\
	PushParm(v)

#define PUSH_PALM_PTR(p)								\
	PushParm((uint32) p)

#define PUSH_HOST_PTR(p)								\
	StMemoryMapper	mapper##p (p, sizeof (*(p)));		\
	PushParm(EmBankMapped::GetEmulatedAddress (p))

#define PUSH_HOST_PTR_LEN(p, len)						\
	StMemoryMapper	mapper##p (p, len);					\
	PushParm(EmBankMapped::GetEmulatedAddress (p))

#define PUSH_HOST_STRING(p)								\
	StMemoryMapper	mapper##p (p, p ? strlen (p) + 1 : 0);	\
	PushParm(EmBankMapped::GetEmulatedAddress (p))


#define CallROM(trapWord)				\
	theTrap.Call (trapWord)


#define CallIntl(selector)				\
	theTrap.SetNewDReg (2, selector);	\
	theTrap.Call (sysTrapIntlDispatch)


// ---------------------------------------------------------------------------
//		¥ Canonical_If_Not_Null
// ---------------------------------------------------------------------------
// Pointer parameters often need a little extra handling.  If they are not
// NULL, they generally point to either stack or heap buffers.  If they're on
// the stack, then on Windows they're in little-endian format and need to be
// converted into big-endian format.  Fortunately, we don't have to worry
// about the goofy word-swapping done for ROM and RAM memory, as the memory
// accessors that deal with data on the stack doesn't do any of that.
//
// For now, we don't actually check to see if data is on the stack or not.
// We just use this function on pointers that we _expect_ point to
// stack-resident data.  So far we haven't run into any problems with that
// approach.

template <class T>
inline void Canonical_If_Not_Null (T* p)
{
	if (p)
		Canonical (*p);
}


// ---------------------------------------------------------------------------
//		¥ Clipboard Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	trap patches that keep the Palm Clipboard in sync with the host clipboard.
// --------------------

void ClipboardAddItem (const ClipboardFormatType format, const void* ptr, UInt16 length)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE				(length);
	PUSH_HOST_PTR_LEN		(ptr, length);
	PUSH_VALUE				((uint8) format);

	CallROM (sysTrapClipboardAddItem);
}

// --------------------
// Called:
//
//	*	trap patches that keep the Palm Clipboard in sync with the host clipboard.
// --------------------

MemHandle ClipboardGetItem (const ClipboardFormatType format, UInt16* length)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR			(length);
	PUSH_VALUE				((uint8) format);

	CallROM (sysTrapClipboardGetItem);

	Canonical_If_Not_Null (length);

	return (MemHandle) theTrap.GetA0 ();
}

// ---------------------------------------------------------------------------
//		¥ Data Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	by SetHotSyncUserName when the user name needs to be set (during
//		boot-up, after a .psf file has been loaded, and after the preferences
//		have changed.
// --------------------

Err DlkDispatchRequest (DlkServerSessionPtr sessP)
{
	// Patch up cmdP and map in the buffer it points to.

	UInt16	cmdLen	= sessP->cmdLen;
	void*	cmdP	= sessP->cmdP;

	Canonical (cmdLen);
	Canonical (cmdP);

	StMemoryMapper	mapper (cmdP, cmdLen);

	sessP->cmdP = (void*) EmBankMapped::GetEmulatedAddress (cmdP);
	Canonical (sessP->cmdP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(sessP);

	CallROM (sysTrapDlkDispatchRequest);

	// Put back cmdP and unmap it.

	sessP->cmdP = cmdP;
	Canonical (sessP->cmdP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called
//
//	*	in Patches::PostLoad to get the user name so that it
//		can be used to establish the preference setting.
// --------------------

Err DlkGetSyncInfo (UInt32* succSyncDateP, UInt32* lastSyncDateP,
			DlkSyncStateType* syncStateP, Char* nameBufP,
			Char* logBufP, Int32* logLenP)
{
	Int32	logLen = logLenP ? *logLenP : 0;

	Canonical_If_Not_Null (logLenP);

	Int8	tempSyncState;
	Int8*	tempSyncStateP = &tempSyncState;

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(logLenP);
	PUSH_HOST_PTR_LEN	(logBufP, logLen);
	PUSH_HOST_PTR_LEN	(nameBufP, dlkUserNameBufSize);
	PUSH_HOST_PTR		(tempSyncStateP);
	PUSH_HOST_PTR		(lastSyncDateP);
	PUSH_HOST_PTR		(succSyncDateP);

	CallROM (sysTrapDlkGetSyncInfo);

	Canonical_If_Not_Null (succSyncDateP);
	Canonical_If_Not_Null (lastSyncDateP);
//	Canonical_If_Not_Null (syncStateP);
	Canonical_If_Not_Null (logLenP);

	if (syncStateP)
		*syncStateP = (DlkSyncStateType) tempSyncState;

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Data Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	from My_DmCreateDatabaseFromImage after a database was successfully
//		installed, or after an error occurs (different code paths).  This
//		function is called any time a .prc, .pdb, or .pqa file is installed.
//
//	*	from My_ShlExportAsPilotFile (same comments)
//
//	*	from AppGetExtraInfo after opening a resource database to get its name.
//		This function is called any time a list of databases is needed (for
//		example, in the New Gremlin and Export Database dialogs).
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state.
// --------------------

Err DmCloseDatabase (DmOpenRef dbR)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(dbR);

	CallROM (sysTrapDmCloseDatabase);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called
//
//	*	from My_DmCreateDatabaseFromImage.  This function is called any
//		time a .prc, .pdb, or .pqa file is installed.
// --------------------

Err DmCreateDatabase (UInt16 cardNo, const Char * const nameP, 
					UInt32 creator, UInt32 type, Boolean resDB)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE			(resDB);
	PUSH_VALUE			(type);
	PUSH_VALUE			(creator);
	PUSH_HOST_STRING	(nameP);
	PUSH_VALUE			(cardNo);

	CallROM (sysTrapDmCreateDatabase);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	when exporting a database (first to generate the list of databases
//		that can be exported, then again to later determine if the
//		database is a resource database or not).
//
//	*	when we need to switch to another application.  Mostly called when
//		starting a Gremlin on a particular application.
//
//	*	after an application has been launched, in order to collect information
//		on that application for later error reporting.
// --------------------

Err DmDatabaseInfo (UInt16 cardNo, LocalID	dbID, Char* nameP,
					UInt16* attributesP, UInt16* versionP, UInt32* crDateP,
					UInt32* modDateP, UInt32* bckUpDateP,
					UInt32* modNumP, LocalID* appInfoIDP,
					LocalID* sortInfoIDP, UInt32* typeP,
					UInt32* creatorP)
{
	Canonical_If_Not_Null (attributesP);
	Canonical_If_Not_Null (versionP);
	Canonical_If_Not_Null (crDateP);
	Canonical_If_Not_Null (modDateP);
	Canonical_If_Not_Null (bckUpDateP);
	Canonical_If_Not_Null (modNumP);
	Canonical_If_Not_Null (appInfoIDP);
	Canonical_If_Not_Null (sortInfoIDP);
	Canonical_If_Not_Null (typeP);
	Canonical_If_Not_Null (creatorP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(creatorP);
	PUSH_HOST_PTR		(typeP);
	PUSH_HOST_PTR		(sortInfoIDP);
	PUSH_HOST_PTR		(appInfoIDP);
	PUSH_HOST_PTR		(modNumP);
	PUSH_HOST_PTR		(bckUpDateP);
	PUSH_HOST_PTR		(modDateP);
	PUSH_HOST_PTR		(crDateP);
	PUSH_HOST_PTR		(versionP);
	PUSH_HOST_PTR		(attributesP);
	PUSH_HOST_PTR_LEN	(nameP, dmDBNameLength);
	PUSH_VALUE			(dbID);
	PUSH_VALUE			(cardNo);

	CallROM (sysTrapDmDatabaseInfo);

	Canonical_If_Not_Null (attributesP);
	Canonical_If_Not_Null (versionP);
	Canonical_If_Not_Null (crDateP);
	Canonical_If_Not_Null (modDateP);
	Canonical_If_Not_Null (bckUpDateP);
	Canonical_If_Not_Null (modNumP);
	Canonical_If_Not_Null (appInfoIDP);
	Canonical_If_Not_Null (sortInfoIDP);
	Canonical_If_Not_Null (typeP);
	Canonical_If_Not_Null (creatorP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	from LoadPalmFile to delete a previous version of a database
//		that we're trying to install.
//
//	*	from My_DmCreateDatabaseFromImage if the attempt to install a
//		database fails.
// --------------------

Err DmDeleteDatabase (UInt16 cardNo, LocalID dbID)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(dbID);
	PUSH_VALUE	(cardNo);

	CallROM (sysTrapDmDeleteDatabase);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by the application install code (My_DmCreateDatabaseFromImage)
//		in order to get the dbID of the database it just created.
//
//	*	by the application install code (LoadPalmFile) to determine
//		if there's a previous instance of the application that needs
//		to be deleted.
//
//	*	by the application export code (My_ShlExportAsPilotFile) to find
//		the database to be exported.
//
//	*	by code that switches to an application installed from the
//		AutoRun directory.
// --------------------

LocalID DmFindDatabase (UInt16 cardNo, const Char* nameP)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_STRING	(nameP);
	PUSH_VALUE			(cardNo);

	CallROM (sysTrapDmFindDatabase);

	return (LocalID) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by AppGetExtraInfo to get the 'tAIN' resource.
//
//	*	by CollectCurrentAppInfo to get the 'tAIN' and 'tver' resources.
// --------------------

MemHandle DmGet1Resource (DmResType type, DmResID id)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(id);
	PUSH_VALUE	(type);

	CallROM (sysTrapDmGet1Resource);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by GetDatabases when iterating over all the databases in the system.
//		Called when generating a list of databases to display (e.g.,
//		in the New Gremlin and Export Database dialogs).
// --------------------

LocalID DmGetDatabase (UInt16 cardNo, UInt16 index)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(index);
	PUSH_VALUE	(cardNo);

	CallROM (sysTrapDmGetDatabase);

	return (LocalID) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by the application export code (My_ShlExportAsPilotFile) when failing
//		to find or open the requested database.
//
//	*	AppGetExtraInfo when failing to open an application database in
//		order to get its name.
// --------------------

Err DmGetLastErr (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapDmGetLastErr);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlins (Gremlins::New) when it needs to find the application
//		responsible for a "runnable" database.
//
//	*	same comments for Patches::SwitchToApp.
// --------------------

Err	DmGetNextDatabaseByTypeCreator (Boolean newSearch, DmSearchStatePtr stateInfoP,
			 		UInt32 type, UInt32 creator, Boolean onlyLatestVers, 
			 		UInt16* cardNoP, LocalID* dbIDP)
{
	Canonical_If_Not_Null (cardNoP);
	Canonical_If_Not_Null (dbIDP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(dbIDP);
	PUSH_HOST_PTR	(cardNoP);
	PUSH_VALUE		(onlyLatestVers);
	PUSH_VALUE		(creator);
	PUSH_VALUE		(type);
	PUSH_HOST_PTR	(stateInfoP);
	PUSH_VALUE		(newSearch);

	CallROM (sysTrapDmGetNextDatabaseByTypeCreator);

	Canonical_If_Not_Null (cardNoP);
	Canonical_If_Not_Null (dbIDP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state.
// --------------------

MemHandle DmGetResource (DmResType type, DmResID id)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(id);
	PUSH_VALUE	(type);

	CallROM (sysTrapDmGetResource);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by the application export code (My_ShlExportAsPilotFile)
//		when iterating over all the resources.
// --------------------

MemHandle DmGetResourceIndex (DmOpenRef dbP, UInt16 index)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(index);
	PUSH_PALM_PTR	(dbP);

	CallROM (sysTrapDmGetResourceIndex);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by the application install code (My_DmCreateDatabaseFromImage)
//		to create an app info block.
// --------------------

MemHandle DmNewHandle (DmOpenRef dbR, UInt32 size)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(size);
	PUSH_PALM_PTR	(dbR);

	CallROM (sysTrapDmNewHandle);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by the application install code (My_DmCreateDatabaseFromImage)
//		to create new records.
// --------------------

MemHandle DmNewRecord (DmOpenRef dbR, UInt16* atP, UInt32 size)
{
	Canonical_If_Not_Null (atP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(size);
	PUSH_HOST_PTR	(atP);
	PUSH_PALM_PTR	(dbR);

	CallROM (sysTrapDmNewRecord);

	Canonical_If_Not_Null (atP);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by the application install code (My_DmCreateDatabaseFromImage)
//		to create new resources.
//
//	*	while booting up (InstallCalibrationInfo)  as part of the process
//		of setting the calibration info to an identity state.
// --------------------

MemHandle DmNewResource (DmOpenRef dbR, DmResType resType, DmResID resID, UInt32 size)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(size);
	PUSH_VALUE		(resID);
	PUSH_VALUE		(resType);
	PUSH_PALM_PTR	(dbR);

	CallROM (sysTrapDmNewResource);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	when iterating over all databases in the system (GetDatabases).
//		Called when generating a list of databases to display (e.g.,
//		in the New Gremlin and Export Database dialogs).
// --------------------

UInt16 DmNumDatabases (UInt16 cardNo)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(cardNo);

	CallROM (sysTrapDmNumDatabases);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by the application export code (My_ShlExportAsPilotFile)
//		in order to loop over all the records/resources in a database.
// --------------------

UInt16 DmNumRecords (DmOpenRef dbP)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(dbP);

	CallROM (sysTrapDmNumRecords);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by the application install code (My_DmCreateDatabaseFromImage)
//		so that new records/resources can be added to the new database.
//
//	*	by the application export code (My_ShlExportAsPilotFile)
//		so that records/resources can be retrieved and written out.
//
//	*	by AppGetExtraInfo to get information out of an application's
//		'tAIN' resource.
// --------------------

DmOpenRef DmOpenDatabase (UInt16 cardNo, LocalID dbID, UInt16 mode)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(mode);
	PUSH_VALUE	(dbID);
	PUSH_VALUE	(cardNo);

	CallROM (sysTrapDmOpenDatabase);

	return (DmOpenRef) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	after an application has been launched (CollectCurrentAppInfo)
//		to get the app's dbID and card number.
// --------------------

Err DmOpenDatabaseInfo (DmOpenRef dbP, LocalID* dbIDP, 
					UInt16* openCountP, UInt16* modeP, UInt16* cardNoP,
					Boolean* resDBP)
{
	Canonical_If_Not_Null (dbIDP);
	Canonical_If_Not_Null (openCountP);
	Canonical_If_Not_Null (modeP);
	Canonical_If_Not_Null (cardNoP);
	Canonical_If_Not_Null (resDBP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(resDBP);
	PUSH_HOST_PTR	(cardNoP);
	PUSH_HOST_PTR	(modeP);
	PUSH_HOST_PTR	(openCountP);
	PUSH_HOST_PTR	(dbIDP);
	PUSH_PALM_PTR	(dbP);

	CallROM (sysTrapDmOpenDatabaseInfo);

	Canonical_If_Not_Null (dbIDP);
	Canonical_If_Not_Null (openCountP);
	Canonical_If_Not_Null (modeP);
	Canonical_If_Not_Null (cardNoP);
	Canonical_If_Not_Null (resDBP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by the application export code (My_ShlExportAsPilotFile) in order
//		to get information about exported records.
// --------------------

Err DmRecordInfo (DmOpenRef dbP, UInt16 index,
					UInt16* attrP, UInt32* uniqueIDP, LocalID* chunkIDP)
{
	Canonical_If_Not_Null (attrP);
	Canonical_If_Not_Null (uniqueIDP);
	Canonical_If_Not_Null (chunkIDP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(chunkIDP);
	PUSH_HOST_PTR	(uniqueIDP);
	PUSH_HOST_PTR	(attrP);
	PUSH_VALUE		(index);
	PUSH_PALM_PTR	(dbP);

	CallROM (sysTrapDmRecordInfo);

	Canonical_If_Not_Null (attrP);
	Canonical_If_Not_Null (uniqueIDP);
	Canonical_If_Not_Null (chunkIDP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by the application install code (My_DmCreateDatabaseFromImage) to release
//		records created by DmNewRecord.
// --------------------

Err DmReleaseRecord (DmOpenRef dbR, UInt16 index, Boolean dirty)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(dirty);
	PUSH_VALUE		(index);
	PUSH_PALM_PTR	(dbR);

	CallROM (sysTrapDmReleaseRecord);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by all code that calls DmNewResource, DmGet1Resource, DmGetResource,
//		DmGetResourceIndex, etc.
// --------------------

Err DmReleaseResource (MemHandle resourceH)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(resourceH);

	CallROM (sysTrapDmReleaseResource);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by application export code (My_ShlExportAsPilotFile) to get
//		information about the resources being written out.
// --------------------

Err DmResourceInfo (DmOpenRef dbP, UInt16 index,
					DmResType* resTypeP, DmResID* resIDP,  
					LocalID* chunkLocalIDP)
{
	Canonical_If_Not_Null (resTypeP);
	Canonical_If_Not_Null (resIDP);
	Canonical_If_Not_Null (chunkLocalIDP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(chunkLocalIDP);
	PUSH_HOST_PTR	(resIDP);
	PUSH_HOST_PTR	(resTypeP);
	PUSH_VALUE		(index);
	PUSH_PALM_PTR	(dbP);

	CallROM (sysTrapDmResourceInfo);

	Canonical_If_Not_Null (resTypeP);
	Canonical_If_Not_Null (resIDP);
	Canonical_If_Not_Null (chunkLocalIDP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by application export code (My_ShlExportAsPilotFile) to get
//		information about the records being written out.
// --------------------

MemHandle DmQueryRecord (DmOpenRef dbP, UInt16 index)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(index);
	PUSH_PALM_PTR	(dbP);

	CallROM (sysTrapDmQueryRecord);

	return (MemHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by application install code (My_DmCreateDatabaseFromImage) to
//		set the attributes of newly-created databases.
// --------------------

Err DmSetDatabaseInfo (UInt16 cardNo, LocalID dbID, const Char* nameP,
					UInt16* attributesP, UInt16* versionP, UInt32* crDateP,
					UInt32* modDateP, UInt32* bckUpDateP,
					UInt32* modNumP, LocalID* appInfoIDP,
					LocalID* sortInfoIDP, UInt32* typeP,
					UInt32* creatorP)
{
	Canonical_If_Not_Null (attributesP);
	Canonical_If_Not_Null (versionP);
	Canonical_If_Not_Null (crDateP);
	Canonical_If_Not_Null (modDateP);
	Canonical_If_Not_Null (bckUpDateP);
	Canonical_If_Not_Null (modNumP);
	Canonical_If_Not_Null (appInfoIDP);
	Canonical_If_Not_Null (sortInfoIDP);
	Canonical_If_Not_Null (typeP);
	Canonical_If_Not_Null (creatorP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(creatorP);
	PUSH_HOST_PTR	(typeP);
	PUSH_HOST_PTR	(sortInfoIDP);
	PUSH_HOST_PTR	(appInfoIDP);
	PUSH_HOST_PTR	(modNumP);
	PUSH_HOST_PTR	(bckUpDateP);
	PUSH_HOST_PTR	(modDateP);
	PUSH_HOST_PTR	(crDateP);
	PUSH_HOST_PTR	(versionP);
	PUSH_HOST_PTR	(attributesP);
	PUSH_HOST_STRING(nameP);
	PUSH_VALUE		(dbID);
	PUSH_VALUE		(cardNo);

	CallROM (sysTrapDmSetDatabaseInfo);

	Canonical_If_Not_Null (attributesP);
	Canonical_If_Not_Null (versionP);
	Canonical_If_Not_Null (crDateP);
	Canonical_If_Not_Null (modDateP);
	Canonical_If_Not_Null (bckUpDateP);
	Canonical_If_Not_Null (modNumP);
	Canonical_If_Not_Null (appInfoIDP);
	Canonical_If_Not_Null (sortInfoIDP);
	Canonical_If_Not_Null (typeP);
	Canonical_If_Not_Null (creatorP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by application install code (My_DmCreateDatabaseFromImage) to
//		set the attributes of newly-created records.
// --------------------

Err DmSetRecordInfo (DmOpenRef dbR, UInt16 index, UInt16* attrP, UInt32* uniqueIDP)
{
	Canonical_If_Not_Null (attrP);
	Canonical_If_Not_Null (uniqueIDP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(uniqueIDP);
	PUSH_HOST_PTR	(attrP);
	PUSH_VALUE		(index);
	PUSH_PALM_PTR	(dbR);

	CallROM (sysTrapDmSetRecordInfo);

	Canonical_If_Not_Null (attrP);
	Canonical_If_Not_Null (uniqueIDP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by application install code (My_DmCreateDatabaseFromImage) to
//		copy data into the newly-created records and resources.
//
//	*	while booting up (InstallCalibrationInfo) as part of the process
//		of setting the calibration info to an identity state.
// --------------------

Err DmWrite (MemPtr recordP, UInt32 offset, const void * const srcP, UInt32 bytes)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE			(bytes);
	PUSH_HOST_PTR_LEN	(srcP, bytes);
	PUSH_VALUE			(offset);
	PUSH_PALM_PTR		(recordP);

	CallROM (sysTrapDmWrite);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Event Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	by Gremlins and key event insertion code (StubAppEnqueueKey) to
//		insert a key event.
// --------------------

Err EvtEnqueueKey (UInt16 ascii, UInt16 keycode, UInt16 modifiers)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(modifiers);
	PUSH_VALUE	(keycode);
	PUSH_VALUE	(ascii);

	CallROM (sysTrapEvtEnqueueKey);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlins and pen event insertion code (StubEnqueuePt) to
//		insert a pen event.
// --------------------

Err EvtEnqueuePenPoint (PointType* ptP)
{
	// Make a copy of the point, as we may be munging it.

	PointType	pt = *ptP;
	PointType*	ptPtr = &pt;

	// Enqueue the new pen position. We must "reverse" correct it because the
	// Event Manager assumes that all points enqueued are raw digitizer points.

	if (pt.x != -1 || pt.y != -1)
	{
		(void) PenScreenToRaw(&pt);
	}

	// Byteswap it so that the ROM routines get it in the right format.
	// Byteswap it _after_ PenScreenToRaw, as PenScreenToRaw will do its
	// own byteswapping.

	Canonical (pt);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(ptPtr);

	CallROM (sysTrapEvtEnqueuePenPoint);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlins code (FakeEventXY) in order to target silkscreen buttons.
// --------------------

const PenBtnInfoType*	EvtGetPenBtnList(UInt16* numButtons)
{
	Canonical_If_Not_Null (numButtons);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(numButtons);

	CallROM (sysTrapEvtGetPenBtnList);

	Canonical_If_Not_Null (numButtons);

	// This one's tricky; it's returning a pointer to ROM data.  There are
	// no functions to access the fields in the struct, so the caller needs
	// to access those fields directly.  However, not only is the address
	// returned in "emulated space" and not native space, but there are
	// word- and byte-swapping issues involved.  To help out the caller,
	// we'll address those issues here, returning a pointer to a "pre-
	// digested" copy of the struct.

	static PenBtnInfoPtr	buttonListP;

	if (!buttonListP)
	{
		long	buttonListSize = *numButtons * sizeof (PenBtnInfoType);

		buttonListP = (PenBtnInfoPtr) malloc (buttonListSize);
		// !!! Should check for NULL here, but how to handle it?

		// Copy the data, sorting out address and wordswapping issues

		uae_memmove ((void*) buttonListP, theTrap.GetA0 (), buttonListSize);

		// Now sort out the byte-swapping issues.

		for (UInt16 ii = 0; ii < *numButtons; ++ii)
			Canonical (buttonListP[ii]);
	}

	return buttonListP;
}

// --------------------
// Called:
//
//	*	by Gremlins code (Gremlins::GetFakeEvent) to make sure the device
//		doesn't fall asleep, even though Gremlins is pumping in key events.
// --------------------

Err EvtResetAutoOffTimer (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapEvtResetAutoOffTimer);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (Gremlins::New, Gremlins::Resume, Hordes::Step) to
//		wake a device out of snooze mode or a call to EvtGetEvent with an
//		infinite timeout.
//
//	*	by pen and key event insertion code to wake a device out of snooze
//		mode or a call to EvtGetEvent with an infinite timeout.
//
//	*	by HostSignalWait if the caller is looking for an idle event.
// --------------------

Err EvtWakeup (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapEvtWakeup);

	return (Err) theTrap.GetD0 ();
}


// ---------------------------------------------------------------------------
//		¥ Exchange Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

typedef enum
{
	exgLibTrapHandleEvent = sysLibTrapCustom,
	exgLibTrapConnect,
	exgLibTrapAccept,
	exgLibTrapDisconnect,
	exgLibTrapPut,
	exgLibTrapGet,
	exgLibTrapSend,
	exgLibTrapReceive,
	exgLibTrapControl,	// <-- Needed for this
	exgLibReserved1,
	exgLibTrapLast
} ExgLibTrapNumberEnum;

Err 	ExgLibControl(UInt16 libRefNum, UInt16 op, void *valueP, UInt16 *valueLenP)
{
	ATrap	theTrap;
	PUSH_VALUE ((UInt32) valueLenP);
	PUSH_VALUE ((UInt32) valueP);
	PUSH_VALUE (op);
	PUSH_VALUE (libRefNum);

	CallROM (exgLibTrapControl);

	return (Err) theTrap.GetD0 ();
}


// ---------------------------------------------------------------------------
//		¥ Field Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	by Gremlin code (IsFocus, SpaceLeftInFocus) to see if a field
//		is editable.
// --------------------

void FldGetAttributes (const FieldType* fld, const FieldAttrPtr attrP)
{
	Canonical_If_Not_Null (attrP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(attrP);
	PUSH_PALM_PTR	(fld);

	CallROM (sysTrapFldGetAttributes);

	Canonical_If_Not_Null (attrP);

//	LogAppendMsg ("attr == 0x%04X", (int) *(short*) attrP);
}

// --------------------
// Called:
//
//	*	by Gremlin code (SpaceLeftInFocus) to determine if more characters
//		should be jammed into a field.
// --------------------

UInt16 FldGetMaxChars (const FieldType* fld)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(fld);

	CallROM (sysTrapFldGetMaxChars);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (SpaceLeftInFocus) to determine if more characters
//		should be jammed into a field.
// --------------------

UInt16 FldGetTextLength (const FieldType* fld)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(fld);

	CallROM (sysTrapFldGetTextLength);

	return (UInt16) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Font Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	by Gremlin code (FakeLocalMovement) to generate strokes based on
//		the font height.
// --------------------

Int16 FntLineHeight (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapFntLineHeight);

	return (Int16) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Form Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	by Gremlin code (GetFocusObject) to determine if the active form
//		is the active window.
//
//	*	by Gremlin code (FakeEventXY) to see if there's a form that should
//		be targeted.
// --------------------

FormType* FrmGetActiveForm (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapFrmGetActiveForm);

	return (FormType*) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (GetFocusObject) when trying to determine the
//		object that has the focus and that should get preferred attention.
// --------------------

UInt16 FrmGetFocus (const FormType* frm)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetFocus);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	NEVER
// --------------------

UInt16 FrmGetFormId (const FormType* frm)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetFormId);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (CollectOKObjects) in order to iterate over
//		all the objects in a form.
// --------------------

UInt16 FrmGetNumberOfObjects (const FormType* frm)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetNumberOfObjects);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (FakeEventXY) to generate a random point within
//		a form object.
//
//	*	by Gremlin code (CollectOKObjects) to get the bounds of an object
//		for validation.
// --------------------

void FrmGetObjectBounds (const FormType* frm, const UInt16 pObjIndex, const RectanglePtr r)
{
	Canonical_If_Not_Null (r);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(r);
	PUSH_VALUE		(pObjIndex);
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetObjectBounds);

	Canonical_If_Not_Null (r);
}

// --------------------
// Called:
//
//	*	by Gremlin code (CollectOKObjects) to get the ID of an object
//		for use in any error messages that might occur.
// --------------------

UInt16 FrmGetObjectId (const FormType* frm, const UInt16 objIndex)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(objIndex);
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetObjectId);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (GetFocusObject) to get a pointer to a table object
//		so that we can call TblGetCurrentField on it.
//
//	*	by Gremlin code (CollectOKObjects) to get a pointer to control
//		and list objects so that we can validate them.
// --------------------

MemPtr FrmGetObjectPtr (const FormType* frm, const UInt16 objIndex)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(objIndex);
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetObjectPtr);

	return (MemPtr) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (GetFocusObject) to see if the focussed object
//		is a table that might have an embedded field.
//
//	*	by Gremlin code (CollectOKObjects) to see if the object we're
//		iterating over is one that we'd like to emulate a tap on.
// --------------------

FormObjectKind FrmGetObjectType (const FormType* frm, const UInt16 objIndex)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(objIndex);
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetObjectType);

	// Cast to an 8-bit type first.  FormObjectKind is an 8-bit
	// value on 68K machines, but a 32-bit (int) value on other
	// platforms.  If we don't cast to an 8-bit value first, we'll
	// end up with unwanted garbage in the upper 24 bits.

	return (FormObjectKind) (uint8) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by logging code (StubEmFrmGetTitle) to get the form's title.
// --------------------

const Char* FrmGetTitle (const FormType* frm)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetTitle);

	return (const Char*) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by Gremlin code (GetFocusObject, FakeEventXY) to determine if the active
//		form is the active window.
// --------------------

WinHandle FrmGetWindowHandle (const FormType* frm)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(frm);

	CallROM (sysTrapFrmGetWindowHandle);

	return (WinHandle) theTrap.GetA0 ();
}

// ---------------------------------------------------------------------------
//		¥ File System functions
// ---------------------------------------------------------------------------

#pragma mark -

typedef enum {
	FSTrapLibAPIVersion = sysLibTrapCustom,
	FSTrapCustomControl,	// <-- Needed for this
	FSTrapFilesystemType,
	
	FSTrapFileCreate,
	FSTrapFileOpen,
	FSTrapFileClose,
	FSTrapFileRead,
	FSTrapFileWrite,
	FSTrapFileDelete,
	FSTrapFileRename,
	FSTrapFileSeek,
	FSTrapFileEOF,
	FSTrapFileTell,
	FSTrapFileResize,
	FSTrapFileAttributesGet,
	FSTrapFileAttributesSet,
	FSTrapFileDateGet,
	FSTrapFileDateSet,
	FSTrapFileSize,
	
	FSTrapDirCreate,
	FSTrapDirEntryEnumerate,
	
	FSTrapVolumeFormat,
	FSTrapVolumeMount,
	FSTrapVolumeUnmount,
	FSTrapVolumeInfo,
	FSTrapVolumeLabelGet,
	FSTrapVolumeLabelSet,
	FSTrapVolumeSize,
	
	FSMaxSelector = FSTrapVolumeSize,

	FSBigSelector = 0x7FFF	// Force FSLibSelector to be 16 bits.
} FSLibSelector;

Err FSCustomControl (UInt16 fsLibRefNum, UInt32 apiCreator, UInt16 apiSelector, 
					void *valueP, UInt16 *valueLenP)
{
	Canonical_If_Not_Null (valueLenP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(valueLenP);
	PUSH_VALUE			((UInt32) valueP);		// !!! Only works for our mount/unmount calls!
	PUSH_VALUE			(apiSelector);
	PUSH_VALUE			(apiCreator);
	PUSH_VALUE			(fsLibRefNum);

	CallROM (FSTrapCustomControl);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Feature Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	by Gremlin code (IntlMgrExists) to see if the Intenational Manager
//		exists before trying to call it.
//
//	*	by Gremlin code (Gremlins::GetFakeEvent) to see what language system
//		we're using (so we know whether to use the English or Japanese quotes).
//
//	*	during bootup (patch on FtrInit) to get the ROM version.
// --------------------

Err FtrGet (UInt32 creator, UInt16 featureNum, UInt32* valueP)
{
	Canonical_If_Not_Null (valueP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(valueP);
	PUSH_VALUE		(featureNum);
	PUSH_VALUE		(creator);

	CallROM (sysTrapFtrGet);

	Canonical_If_Not_Null (valueP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	to install the 'pose' feature after creating or loading a session.
//
//	*	by Poser's debugger handling code (Debug::EventCallback) after a
//		connection to an external debugger is established.
//
//	*	after creating a new session and we are currently connected to a debugger.
//
//	*	after reloading a .psf file and we are currently connected to a debugger.
// --------------------

Err FtrSet (UInt32 creator, UInt16 featureNum, UInt32 newValue)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(newValue);
	PUSH_VALUE		(featureNum);
	PUSH_VALUE		(creator);

	CallROM (sysTrapFtrSet);

	return (Err) theTrap.GetD0 ();
}


// --------------------
// Called:
//
//	*	by Poser's debugger handling code (Debug::EventCallback) when it
//		detects that the connection to an external debugger has been broken.
//
//	*	after reloading a .psf file and we are not currently connected to a debugger.
// --------------------

Err	FtrUnregister (UInt32 creator, UInt16 featureNum)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(featureNum);
	PUSH_VALUE		(creator);

	CallROM (sysTrapFtrUnregister);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Memory Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	in our patch to SysUIAppSwitch (called as MemPtrFree) to free up any
//		left over cmdPBP's.
// --------------------

Err MemChunkFree (MemPtr chunkDataP)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(chunkDataP);

	CallROM (sysTrapMemChunkFree);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	during the application install process (My_DmCreateDatabaseFromImage)
//		in order to lock a database handle before calling DmWrite.
//
//	*	by our SysAppStartup patch (CollectCurrentAppInfo) to get information
//		from the 'tAIN' or 'tver' resources.
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state.
// --------------------

MemPtr MemHandleLock (MemHandle h)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(h);

	CallROM (sysTrapMemHandleLock);

	return (MemPtr) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	during the application export process (My_ShlExportAsPilotFile)
//		all over the place
// --------------------

UInt32 MemHandleSize (MemHandle h)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(h);

	CallROM (sysTrapMemHandleSize);

	return (UInt32) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	during the application install process (My_DmCreateDatabaseFromImage)
//		to set the app info block field after the block's been created.
// --------------------

LocalID MemHandleToLocalID (MemHandle h)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(h);

	CallROM (sysTrapMemHandleToLocalID);

	return (LocalID) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	during the application install process (My_DmCreateDatabaseFromImage)
//		in order to unlock a database handle after copying the data
//		into it.
//
//	*	getting information about a database (AppGetExtraInfo) from its 'tAIN'
//		resource or app info block.
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state.
// --------------------

Err MemHandleUnlock (MemHandle h)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(h);

	CallROM (sysTrapMemHandleUnlock);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	from our patch to MemInitHeapTable when walking all current
//		heaps to locate them all.
// --------------------

UInt16 MemHeapID (UInt16 cardNo, UInt16 heapIndex)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(heapIndex);
	PUSH_VALUE		(cardNo);

	CallROM (sysTrapMemHeapID);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by EmPalmHeap::GetHeapHeaderInfo (heapID) in order to get a heap
//		pointer that can be passed to EmPalmHeap::GetHeapHeaderInfo (ptr).
//		This version of GetHeapHeaderInfo is called from the version of
//		the EmPalmHeap constructor that taks a heap ID.  This version of
//		the constructor is called from our patches on MemInitHeapTable
//		and MemHeapInit.
// --------------------

MemPtr MemHeapPtr (UInt16 heapID)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(heapID);

	CallROM (sysTrapMemHeapPtr);

	return (MemPtr) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	while getting an application's name (AppGetExtraInfo) from
//		the app info block -- we need to determine if the block
//		is a handle or not so that we know whether or not to lock it.
// --------------------

LocalIDKind MemLocalIDKind (LocalID local)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(local);

	CallROM (sysTrapMemLocalIDKind);

	return (LocalIDKind) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	when exporting a database (My_ShlExportAsPilotFile) to get the
//		app info and sort info handles so that we can get their sizes.
//
//	*	while getting an application's name (AppGetExtraInfo) from the
//		app info block.
// --------------------

MemPtr MemLocalIDToGlobal (LocalID local, UInt16 cardNo)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(cardNo);
	PUSH_VALUE		(local);

	CallROM (sysTrapMemLocalIDToGlobal);

	return (MemPtr) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	when iterating over all databases in the system (GetDatabases).
//		Called when generating a list of databases to display (e.g.,
//		in the New Gremlin and Export Database dialogs).
// --------------------

UInt16 MemNumCards (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapMemNumCards);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	while booting up on a patch to MemInitHeapTable to determine
//		how many heaps we need to track.
// --------------------

UInt16 MemNumHeaps (UInt16 cardNo)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(cardNo);

	CallROM (sysTrapMemNumHeaps);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	when booting up (PrvSetCurrentDate) to set the current date
//		stored in the non-volatile data section.
// --------------------

Err MemNVParams (Boolean set, SysNVParamsPtr paramsP)
{
	Canonical_If_Not_Null (paramsP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(paramsP);
	PUSH_VALUE		(set);

	CallROM (sysTrapMemNVParams);

	Canonical_If_Not_Null (paramsP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	by Gremlins (Gremlins::New) when it needs to create a cmdPBP  telling
//		 Clipper which PQA to launch.
//
//	*	same comments for Patches::SwitchToApp, called when a .pqa was in
//		AutoRun or AutoRunAndQuit.
// --------------------

MemPtr MemPtrNew (UInt32 size)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(size);

	CallROM (sysTrapMemPtrNew);

	return (MemPtr) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	by Gremlins (Gremlins::New) when it needs to create a cmdPBP  telling
//		 Clipper which PQA to launch.
//
//	*	same comments for Patches::SwitchToApp, called when a .pqa was in
//		AutoRun or AutoRunAndQuit.
// --------------------

Err MemPtrSetOwner (MemPtr p, UInt16 owner)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(owner);
	PUSH_PALM_PTR	(p);

	CallROM (sysTrapMemPtrSetOwner);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	while generate a list of databases and their names.  Some non-application
//		databases have special resources with this information; the resource
//		size needs to be checked to see if this information is included.
//
//	*	by our SysAppStartup patch (CollectCurrentAppInfo) to get the size of the stack.
// --------------------

UInt32 MemPtrSize (MemPtr p)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(p);

	CallROM (sysTrapMemPtrSize);

	return (UInt32) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	during the application export process (My_ShlExportAsPilotFile)
//		in order to unlock a database handle after copying the data
//		out of it.
// --------------------

Err MemPtrUnlock (MemPtr p)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(p);

	CallROM (sysTrapMemPtrUnlock);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Net Library functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	NetLib redirection code (Platform_NetLib::OpenConfig) to activate
//		the configuration passed into NetOpenConfig.  The process mirrors
//		that which goes on in NetLib itself.
// --------------------

Err NetLibConfigMakeActive (UInt16 refNum, UInt16 configIndex)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(configIndex);
	PUSH_VALUE	(refNum);

	CallROM (netLibConfigMakeActive);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Pen Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	while starting a Gremlin (ResetCalibrationInfo) to reset the Pen
//		calibration information *after* the Pen manager has read the
//		preference file (that is, merely changing the preference data
//		won't work, since PenOpen has already read it).
// --------------------

Err PenCalibrate (PointType* digTopLeftP, PointType* digBotRightP,
					PointType* scrTopLeftP, PointType* scrBotRightP)
{
	Canonical_If_Not_Null (digTopLeftP);
	Canonical_If_Not_Null (digBotRightP);
	Canonical_If_Not_Null (scrTopLeftP);
	Canonical_If_Not_Null (scrBotRightP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(scrBotRightP);
	PUSH_HOST_PTR	(scrTopLeftP);
	PUSH_HOST_PTR	(digBotRightP);
	PUSH_HOST_PTR	(digTopLeftP);

	CallROM (sysTrapPenCalibrate);

	Canonical_If_Not_Null (digTopLeftP);
	Canonical_If_Not_Null (digBotRightP);
	Canonical_If_Not_Null (scrTopLeftP);
	Canonical_If_Not_Null (scrBotRightP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state.
// --------------------

Err	 PenRawToScreen(PointType* penP)
{
	Canonical_If_Not_Null (penP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(penP);

	CallROM (sysTrapPenRawToScreen);

	Canonical_If_Not_Null (penP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	our EvtEnqueuePenPoint wrapper.
// --------------------

Err PenScreenToRaw (PointType* penP)
{
	Canonical_If_Not_Null (penP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(penP);

	CallROM (sysTrapPenScreenToRaw);

	Canonical_If_Not_Null (penP);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Peferences Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state (1.0 devices only).
// --------------------

DmOpenRef PrefOpenPreferenceDBV10 (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapPrefOpenPreferenceDBV10);

	return (DmOpenRef) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	while booting up (InstallCalibrationInfo) or starting a Gremlin
//		(ResetCalibrationInfo) as part of the process of setting the
//		calibration info to an identity state (2.0 devices and later).
// --------------------

DmOpenRef PrefOpenPreferenceDB (Boolean saved)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(saved);

	CallROM (sysTrapPrefOpenPreferenceDB);

	return (DmOpenRef) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	while booting up in a tailpatch to UIInitialize to turn off
//		any auto-off values.
// --------------------

void PrefSetPreference (SystemPreferencesChoice choice, UInt32 value)
{
	// Coerce "choice" to a 1-byte value to match the way the 68K
	// code-generator determines an enum's size.

	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(value);
	PUSH_VALUE	((uint8) choice);

	CallROM (sysTrapPrefSetPreference);
}

// ---------------------------------------------------------------------------
//		¥ Sound Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// ---------------------------------------------------------------------------
//		¥ System Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	in a headpatch to SysUIAppSwitch to see if the application we're
//		switching to is allowed (it may not be during Gremlins).
// --------------------

Err	SysCurAppDatabase (UInt16* cardNoP, LocalID* dbIDP)
{
	Canonical_If_Not_Null (cardNoP);
	Canonical_If_Not_Null (dbIDP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(dbIDP);
	PUSH_HOST_PTR	(cardNoP);

	CallROM (sysTrapSysCurAppDatabase);

	Canonical_If_Not_Null (cardNoP);
	Canonical_If_Not_Null (dbIDP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	in the memory access checking code (GWH_ExamineChunk) to iterate
//		the tasks.  This is part of the hueristic to see if a task
//		that's being terminated is still trying to access its stack after
//		it's been deleted.
// --------------------

Err SysKernelInfo (MemPtr p)
{
	SysKernelInfoPtr	infoP = (SysKernelInfoPtr) p;

	Canonical_If_Not_Null (infoP);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(infoP);

	CallROM (sysTrapSysKernelInfo);

	Canonical_If_Not_Null (infoP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	.
// --------------------

Err SysLibFind (const Char *nameP, UInt16 *refNumP)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(refNumP);
	PUSH_HOST_PTR_LEN	(nameP, strlen (nameP) + 1);

	CallROM (sysTrapSysLibFind);

	Canonical_If_Not_Null (refNumP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	.
// --------------------

Err SysLibLoad (UInt32 libType, UInt32 libCreator, UInt16 *refNumP)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR	(refNumP);
	PUSH_VALUE		(libCreator);
	PUSH_VALUE		(libType);

	CallROM (sysTrapSysLibLoad);

	Canonical_If_Not_Null (refNumP);

	return (Err) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	.
// --------------------

SysLibTblEntryPtr SysLibTblEntry (UInt16 refNum)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE		(refNum);

	CallROM (sysTrapSysLibTblEntry);

	return (SysLibTblEntryPtr) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	while booting up in a tailpatch to UIInitialize to turn off
//		any auto-off values.
// --------------------

UInt16 SysSetAutoOffTime (UInt16 seconds)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(seconds);

	CallROM (sysTrapSysSetAutoOffTime);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	By Gremlin code (Gremlins::New) to switch to an approved application
//		(either one on the approved list, or one that can "run" one on the
//		approved like, like a PQA).
//
//	*	By Patches::SwitchToApp to switch to a given application.  This
//		function is called after booting or loading a session and the user
//		has a file in the AutoRun or AutoRunAndQuit directories.
// --------------------

Err SysUIAppSwitch (UInt16 cardNo, LocalID dbID, UInt16 cmd, MemPtr cmdPBP)
{	
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(cmdPBP);
	PUSH_VALUE		(cmd);
	PUSH_VALUE		(dbID);
	PUSH_VALUE		(cardNo);

	CallROM (sysTrapSysUIAppSwitch);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Table Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	By Gremlin code (GetFocusObject) to determine if there's a sub-field
//		in a table that needs to be targeted.
// --------------------

FieldPtr TblGetCurrentField (const TableType* table)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR	(table);

	CallROM (sysTrapTblGetCurrentField);

	return (FieldPtr) theTrap.GetA0 ();
}

// ---------------------------------------------------------------------------
//		¥ Text Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	By Gremlin code (GetFakeEvent) when generating the percentage
//		that any character is randomly posted to the application.
// --------------------

UInt8 TxtByteAttr(UInt8 inByte)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_VALUE	(inByte);

	CallIntl (intlTxtByteAttr);

	return (UInt8) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	By Gremlin code (GetFakeEvent) when generating the percentage
//		that any character is randomly posted to the application.
// --------------------

UInt16 TxtCharBounds (const Char* inText, UInt32 inOffset, UInt32* outStart, UInt32* outEnd)
{
	Canonical_If_Not_Null (outStart);
	Canonical_If_Not_Null (outEnd);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(outEnd);
	PUSH_HOST_PTR		(outStart);
	PUSH_VALUE			(inOffset);
	PUSH_HOST_PTR_LEN	(inText, inOffset + 4);

	CallIntl (intlTxtCharBounds);

	Canonical_If_Not_Null (outStart);
	Canonical_If_Not_Null (outEnd);

	return (UInt16) theTrap.GetD0 ();
}

// --------------------
// Called:
//
//	*	By Gremlin code (SendCharsToType) when iterating over the hardcoded
//		text to be posted to the application.
// --------------------

UInt16 TxtGetNextChar (const Char* inText, UInt32 inOffset, WChar* outChar)
{
	Canonical_If_Not_Null (outChar);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(outChar);
	PUSH_VALUE			(inOffset);
	PUSH_HOST_PTR_LEN	(inText, inOffset + 4);

	CallIntl (intlTxtGetNextChar);

	Canonical_If_Not_Null (outChar);

	return (UInt16) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Window Manager functions
// ---------------------------------------------------------------------------

#pragma mark -

// --------------------
// Called:
//
//	*	By Gremlin code (GetFocusObject) to make sure that the active window
//		is also the active form.
//
//	*	By Gremlin code (RandomWindowXY) to determine if there's a window
//		within which a pen event should be generated.
//
//	*	By Gremlin code (RandomWindowXY) to set the draw window before calling
//		WinGetWindowBounds.
// --------------------

WinHandle WinGetActiveWindow (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapWinGetActiveWindow);

	return (WinHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	By Gremlin code (FakeLocalMovement) to clip random pen dragging
//		to the window's bounds.
//
//	*	By Gremlin code (RandomScreenXY) to generate a random pen event
//		within a window's bounds.
//
//	*	By Gremlin code (FakeEventXY) to clip a random pen event within
//		an object to the window's bounds.
//
//	*	By Gremlin code (CollectOKObjects) to check to see if all objects
//		are within the window's bounds.
// --------------------

void WinGetDisplayExtent (Int16* extentX, Int16* extentY)
{
	Canonical_If_Not_Null (extentX);
	Canonical_If_Not_Null (extentY);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(extentY);
	PUSH_HOST_PTR		(extentX);

	CallROM (sysTrapWinGetDisplayExtent);

	Canonical_If_Not_Null (extentY);
	Canonical_If_Not_Null (extentX);
}

// --------------------
// Called:
//
//	*	By logging code (StubEmPrintFormID) to walk the window list in
//		order to ensure that a WinHandle is valid before trying to get
//		its name/title.
// --------------------

WinHandle WinGetFirstWindow (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapWinGetFirstWindow);

	return (WinHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	By Gremlins code (RandomWindowXY) before calling WinGetWindowBounds
//		while determining a random location to tap in the window.
//
//	!!! This is renamed to WinGetDrawWindowBounds in Bellagio.
// --------------------

void WinGetWindowBounds (RectanglePtr r)
{
	Canonical_If_Not_Null (r);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(r);

	CallROM (sysTrapWinGetWindowBounds);

	Canonical_If_Not_Null (r);
}

// --------------------
// Called:
//
//	* In our tailpatch of TblHandleEvent to fix a Palm OS 3.5 bug.
// --------------------

void WinPopDrawState (void)
{
	// Call the ROM.

	ATrap	theTrap;

	CallROM (sysTrapWinPopDrawState);
}

// --------------------
// Called:
//
// --------------------

WinHandle WinSetDrawWindow (WinHandle winHandle)
{
	// Call the ROM.

	ATrap	theTrap;
	PUSH_PALM_PTR		(winHandle);

	CallROM (sysTrapWinSetDrawWindow);

	return (WinHandle) theTrap.GetA0 ();
}

// --------------------
// Called:
//
//	*	By Gremlins code (FakeEventXY) after picking a window object to click on.
// --------------------

void WinWindowToDisplayPt (Int16* extentX, Int16* extentY)
{
	Canonical_If_Not_Null (extentX);
	Canonical_If_Not_Null (extentY);

	// Call the ROM.

	ATrap	theTrap;
	PUSH_HOST_PTR		(extentY);
	PUSH_HOST_PTR		(extentX);

	CallROM (sysTrapWinWindowToDisplayPt);

	Canonical_If_Not_Null (extentX);
	Canonical_If_Not_Null (extentY);
}
