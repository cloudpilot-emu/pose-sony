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
#include "Document.h"

#include "Application.h"		// gOpenGCWWindow
#include "EmDlgWin.h"			// CenterWindow, CenterDlgProc
#include "EmSession.h"			// EmSession
#include "Emulator.h"			// gInstance
#include "ErrorHandling.h"		// Errors::SetParameter
#include "Platform.h"			// Platform::ReportError
#include "PreferenceMgr.h"		// Preference
#include "Strings.r.h"			// kStr_CmdSave values
#include "Hordes.h"				// Hordes::PostLoad()

#include "resource.h"


// ======================================================================
//	Globals and constants
// ======================================================================


// ======================================================================
//	Private functions
// ======================================================================


/***********************************************************************
 *
 * FUNCTION:	QueryNewDocument
 *
 * DESCRIPTION:	Ask the user for the parameters for a new session.
 *				This information includes: devide type, country
 *				specific graphics, ram size, and rom file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

Bool QueryNewDocument (HWND hwnd, Configuration& cfg)
{
	// If we have an embedded ROM, get the configuration settings
	// from the embedded configuration.

	if (::IsBound ())
	{
		cfg.fDevice		= Platform::GetBoundDevice ();
		cfg.fRAMSize	= Platform::GetBoundRAMSize ();

		return true;
	}

	EmAssert (!::IsBound ());

	EmDlgItemID	item = EmDlg::DoSessionNew (cfg);

	return (item == kDlgItemOK);
}


/***********************************************************************
 *
 * FUNCTION:	CreateNewDocument
 *
 * DESCRIPTION:	Create a new session based on the given configuration.
 *
 * PARAMETERS:	cfg - configuration containing the specifics of
 *					the new session.
 *
 * RETURNED:	Error code for any errors that may occur.
 *
 ***********************************************************************/

ErrCode CreateNewDocument (const Configuration& cfg)
{
	// Try initializing the emulator with all this information.

//	EmAssert (!Platform::GetCPU ().Running ());

	ErrCode result = errNone;

	EmSession*	fSession = NULL;

	try
	{
		fSession = new EmSession;
		fSession->CreateNew (cfg);
		fSession->CreateThread (true);

		{
			// Save the newly created configuration for next time.

			Preference<Configuration>	pref (kPrefKeyLastConfiguration);
			pref = fSession->GetConfiguration ();
		}

		{
			// Zap the last-saved session file, so that we give preference
			// to a New operation at startup and not an Open operation.

			Preference<EmFileRef>	pref (kPrefKeyLastPSF);
			pref = EmFileRef();
		}
	}
	catch (ErrCode code)
	{
		delete fSession;
		fSession = NULL;

		result = code;
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	QueryOpenDocument
 *
 * DESCRIPTION:	Ask the user for an existing document to open.
 *
 * PARAMETERS:	hwnd - parent window.
 *
 *				fileRef - object to receive the user's choice.
 *
 * RETURNED:	True if the user made a choice, false otherwise.
 *
 ***********************************************************************/

Bool QueryOpenDocument (HWND hwnd, EmFileRef& fileRef)
{
	EmAssert (!::IsBoundFully ());

	// Ask the user to choose a .PSF file.

	EmFileRef		lastLoadedFile = gEmuPrefs->GetIndRAMMRU (0);

	string			prompt ("Choose session file to open");
	EmDirRef		defaultPath (lastLoadedFile.GetParent ());
	EmFileTypeList	filterList;

	filterList.push_back (kFileTypeSession);
	filterList.push_back (kFileTypeAll);

	EmDlgItemID		item = EmDlg::DoGetFile (	fileRef,
												prompt,
												defaultPath,
												filterList);

	return (item == kDlgItemOK);
}


/***********************************************************************
 *
 * FUNCTION:	OpenOldDocument
 *
 * DESCRIPTION:	Create a new session based on an existing .psf file.  If
 *				a file cannot be found at the given location, look in
 *				the emulator's directory for it.
 *
 * PARAMETERS:	ramFileRef - reference to the .psf file.
 *
 * RETURNED:	Error code for any errors that may occur.
 *
 ***********************************************************************/

ErrCode OpenOldDocument (const EmFileRef& ramFileRef)
{
//	EmAssert (!Platform::GetCPU ().Running ());

	EmFileRef	refCopy (ramFileRef);
	ErrCode		result = errNone;

	if (!refCopy.Exists ())
	{
		string		fileName = refCopy.GetName ();
		EmDirRef	poserDir = EmDirRef::GetEmulatorDirectory ();
		refCopy = EmFileRef (poserDir, fileName);
	}

	EmSession*	fSession = NULL;

	try
	{
		fSession = new EmSession;
		fSession->CreateOld (refCopy);
		fSession->CreateThread (true);

		Hordes::PostLoad();

		// If we loaded a session file with a Gremlin running, turn
		// it off, but let the user turn it back on.

		if (Hordes::IsOn ())
		{
//			Hordes::Stop ();
			gOpenGCWWindow = true;
		}

		{
			// Save the newly created configuration for next time.

			Preference<Configuration>	pref (kPrefKeyLastConfiguration);
			pref = fSession->GetConfiguration ();
		}

		{
			// Save the last-opened session file for next time.

			Preference<EmFileRef>	pref (kPrefKeyLastPSF);
			pref = refCopy;
		}

		gEmuPrefs->UpdateRAMMRU (refCopy);
	}
	catch (ErrCode code)
	{
		delete fSession;
		fSession = NULL;

		result = code;
	}

	return result;
}

/***********************************************************************
 *
 * FUNCTION:	OpenResourceDocument
 *
 * DESCRIPTION:	Opens a PSF file from the embedded resource.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	Error code for any errors that may occur.
 *
 ***********************************************************************/

ErrCode OpenResourceDocument(void)
{
	ErrCode result = errNone;

	EmAssert (::IsBoundFully ());

	EmSession*	fSession = NULL;

	try
	{
		fSession = new EmSession;
		fSession->CreateBound ();
		fSession->CreateThread (true);

		Hordes::PostLoad();
	}
	catch (ErrCode code)
	{
		delete fSession;
		fSession = NULL;

		result = code;
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	SaveRAMImage
 *
 * DESCRIPTION:	Save the current session to a .psf file.
 *
 * PARAMETERS:	hwnd - parent window (used when asking the user for
 *					a destination file name).
 *
 *				ramFileRef - reference to a currently existing file.
 *					Optional; may be unspecified if the current
 *					session has never been saved before.
 *
 *				forceQuery - True if this function should always ask the
 *					user for a destination filename.  Otherwise, the
 *					user is asked only if ramFileRef is unspecified.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

BOOL SaveRAMImage (HWND hwnd, const EmFileRef& ramFileRef, Bool forceQuery)
{
	EmFileRef	realFileRef (ramFileRef);

	if (forceQuery || !realFileRef.IsSpecified ())
	{
		Preference<EmFileRef>	pref (kPrefKeyLastPSF);
		EmFileRef				lastRAM = *pref;

		string			prompt (Platform::GetString (IDS_SAVE_RAM_PROMPT));
		EmDirRef		defaultPath (lastRAM.GetParent ());
		EmFileTypeList	filterList;
		string			defaultName (ramFileRef.GetName ());

		filterList.push_back (kFileTypeSession);
		filterList.push_back (kFileTypeAll);

		EmDlgItemID	item = EmDlg::DoPutFile (	realFileRef,
												prompt,
												defaultPath,
												filterList,
												defaultName);

		if (item != kDlgItemOK)
		{
			return false;
		}
	}

	try
	{
		gSession->Save (realFileRef, true);

		// Save the last-saved session file for next time.

		Preference<EmFileRef>	pref (kPrefKeyLastPSF);
		pref = realFileRef;

		// Update the MRU menu.

		gEmuPrefs->UpdateRAMMRU (realFileRef);
	}
	catch (ErrCode errCode)
	{
		Errors::SetParameter ("%filename", realFileRef.GetName ());
		Errors::ReportIfError (kStr_CmdSave, errCode);
		return false;
	}

	return true;
}
