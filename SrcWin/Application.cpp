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
#include "Application.h"

#include "About.h"				// DoAbout
#include "Document.h"			// QueryNewDocument, QueryOpenDocument, gCurrentRAMPath
#include "EmDlg.h"				// DoEditLoggingOptions
#include "EmFileImport.h"		// kMethodBest
#include "EmMenusWin.h"			// HostCreatePopupMenu
#include "EmPixMapWin.h"		// ConvertFromRGBList
#include "EmROMTransfer.h"		// EmROMTransfer::ROMTransfer
#include "EmRPC.h"				// RPC::Idle
#include "EmScreen.h"			// EmScreenUpdateInfo
#include "EmSession.h" 			// EmSessionStopper
#include "Emulator.h"			// gInstance
#include "EmWindow.h"			// EmWindow
#include "EmWindowWin.h"		// GetWindow
#include "ErrorHandling.h"		// Errors::ReportIfError
#include "Hordes.h"				// Hordes::New, etc.
#include "MMFMessaging.h"		// MMFOpenFiles, MMFCloseFiles
#include "Platform.h"			// Platform::CommonDialog
#include "PreferenceMgr.h"		// Preference
#include "Profiling.h"			// ProfileStart, etc.
#include "Skins.h"				// kElement_PowerButton, etc.
#include "SocketMessaging.h"	// CSocket::IdleAll
#include "Sounds.h"
#include "Startup.h"			// Startup::ScheduleCreateSession, ScheduleOpenSession
#include "Strings.r.h"			// kStr_ values

#include "resource.h"


// ======================================================================
//	Globals and constants
// ======================================================================

typedef void	(*MenuHandler) (HWND hWnd, WPARAM command);
typedef Bool	(*UpdateHandler) (HWND hWnd, WPARAM command);

WORD			gWinSockVersion;
Bool			gOpenGCWWindow;

EmPoint			gLastPen;


// ======================================================================
//	Private functions
// ======================================================================

typedef char* POSITION;

static POSITION 		GetStartPosition	(OPENFILENAME& ofn);
static string			GetNextPathName 	(OPENFILENAME& ofn, POSITION& pos);

static UINT CALLBACK	OpenMultiFileProc	(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK OpenMultiFileHook	(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void	PrvLoadPalmFiles	(EmFileRefList& fileList);
static Bool PrvCloseSession		(HWND hWnd, const EmFileRef& saveFile, CloseActionType action);
static Bool PrvCloseSession		(HWND hWnd);

static void DoAbout					(HWND hWnd, WPARAM command);
	   void DoNew					(HWND hWnd, WPARAM command);
static void DoOpenOther				(HWND hWnd, WPARAM command);
	   void DoOpenRecent			(HWND hWnd, WPARAM command);
static void DoClose					(HWND hWnd, WPARAM command);
static void DoSave					(HWND hWnd, WPARAM command);
static void DoSaveAs				(HWND hWnd, WPARAM command);
static void DoSaveBound				(HWND hWnd, WPARAM command);
static void DoSaveScreen			(HWND hWnd, WPARAM command);
static void DoInstallOther			(HWND hWnd, WPARAM command);
static void DoInstallRecent			(HWND hWnd, WPARAM command);
static void DoExportDatabase		(HWND hWnd, WPARAM command);
static void DoHotSync				(HWND hWnd, WPARAM command);
static void DoReset					(HWND hWnd, WPARAM command);

#ifdef SONY_ROM
static void DoSoftReset				(HWND hWnd, WPARAM command);
#endif

static void DoTransferROM			(HWND hWnd, WPARAM command);
static void DoGremlinsNew			(HWND hWnd, WPARAM command);
	   void DoGremlinsStep			(HWND hWnd, WPARAM command);
	   void DoGremlinsResume		(HWND hWnd, WPARAM command);
	   void DoGremlinsStop			(HWND hWnd, WPARAM command);
static void DoProfileStart			(HWND hWnd, WPARAM command);
static void DoProfileStop			(HWND hWnd, WPARAM command);
static void DoProfileDump			(HWND hWnd, WPARAM command);
static void DoProperties			(HWND hWnd, WPARAM command);
static void DoLoggingOptions		(HWND hWnd, WPARAM command);
static void DoDebugOptions			(HWND hWnd, WPARAM command);
static void DoTracingOptions		(HWND hWnd, WPARAM command);
static void DoSkins					(HWND hWnd, WPARAM command);
static void DoBreakpoints			(HWND hWnd, WPARAM command);
static void DoCardOptions			(HWND hWnd, WPARAM command);
static void DoMinimize				(HWND hWnd, WPARAM command);
static void DoExit					(HWND hWnd, WPARAM command);


static const struct
{
	EmCommandID		fCommand;
	StrCode			fOperation;
	MenuHandler 	fHandler;
	UpdateHandler	fUpdater;
	EmStopMethod	fStopType;
}
kCommandHandlers[] =
{
	{ kCommandQuit,				kStr_CmdQuit,			DoExit, 			NULL,	kStopNone },

	{ kCommandSessionNew, 		kStr_CmdNew,			DoNew,				NULL,	kStopNone },
	{ kCommandSessionOpenOther,	kStr_CmdOpen,			DoOpenOther,		NULL,	kStopNone },
	{ kCommandSessionOpen0, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen1, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen2, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen3, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen4, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen5, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen6, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen7, 	kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen8,		kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionOpen9,		kStr_CmdOpen,			DoOpenRecent,		NULL,	kStopNone },
	{ kCommandSessionClose,		kStr_CmdClose,			DoClose,			NULL,	kStopNone },

	{ kCommandSessionSave,		kStr_CmdSave,			DoSave, 			NULL,	kStopNow },
	{ kCommandSessionSaveAs,	kStr_CmdSave,			DoSaveAs,			NULL,	kStopNow },
	{ kCommandSessionBound,		kStr_CmdSaveBound,		DoSaveBound,		NULL,	kStopNow },
	{ kCommandScreenSave,		kStr_CmdSaveScreen, 	DoSaveScreen,		NULL,	kStopNow },

	{ kCommandImportOther,		kStr_CmdInstall,		DoInstallOther, 	NULL,	kStopNone },
	{ kCommandImport0,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport1,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport2,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport3,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport4,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport5,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport6,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport7,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport8,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandImport9,			kStr_CmdInstall,		DoInstallRecent,	NULL,	kStopNone },
	{ kCommandExport, 			kStr_CmdExportDatabase,	DoExportDatabase,	NULL,	kStopOnSysCall },
	{ kCommandHotSync, 			kStr_CmdHotSync,		DoHotSync,			NULL,	kStopNow },
	{ kCommandReset,			kStr_CmdReset,			DoReset,			NULL,	kStopNow },
	{ kCommandDownloadROM, 		kStr_CmdTransferROM,	DoTransferROM,		NULL,	kStopNone },

	{ kCommandGremlinsNew,		kStr_CmdGremlinNew, 	DoGremlinsNew,		NULL,	kStopOnSysCall },
	{ kCommandGremlinsStep,		kStr_CmdGremlinStep,	DoGremlinsStep, 	NULL,	kStopOnSysCall },
	{ kCommandGremlinsResume, 	kStr_CmdGremlinResume,	DoGremlinsResume,	NULL,	kStopOnSysCall },
	{ kCommandGremlinsStop,		kStr_CmdGremlinStop,	DoGremlinsStop,		NULL,	kStopNow },

#if HAS_PROFILING
	{ kCommandProfileStart,		kStr_CmdProfileStart,	DoProfileStart, 	NULL,	kStopNow },
	{ kCommandProfileStop,		kStr_CmdProfileStop,	DoProfileStop,		NULL,	kStopNow },
	{ kCommandProfileDump,		kStr_CmdProfileDump,	DoProfileDump,		NULL,	kStopNow },
#endif

	{ kCommandPreferences,		kStr_CmdPreferences,	DoProperties,		NULL,	kStopOnSysCall },
	{ kCommandLogging,			kStr_CmdLoggingOptions, DoLoggingOptions,	NULL,	kStopNow },
	{ kCommandDebugging,		kStr_CmdDebugOptions,	DoDebugOptions, 	NULL,	kStopNow },
	{ kCommandTracing,			kStr_CmdTracingOptions,	DoTracingOptions, 	NULL,	kStopNow },
	{ kCommandSkins,			kStr_CmdSkins,			DoSkins,		 	NULL,	kStopNow },
	{ kCommandCards,			kStr_CmdCardOptions,	DoCardOptions,		NULL,	kStopOnIdle },
	{ kCommandBreakpoints, 		kStr_CmdBreakpoints,	DoBreakpoints,		NULL,	kStopNow },

//	{ ID_MINIMIZE,				kStr_CmdMinimize,		DoMinimize,			NULL,	kStopNone },
	{ kCommandAbout,			kStr_CmdAbout,			DoAbout,			NULL,	kStopNone },

#ifdef SONY_ROM
	{ kCommandSoftReset,		NULL,					DoSoftReset,		NULL,	kStopNow },
#endif

	{ kCommandNone,				kStr_GenericOperation,	NULL,				NULL,	kStopNone }
};


/***********************************************************************
 *
 * FUNCTION:	PrvLoadPalmFiles
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void PrvLoadPalmFiles (EmFileRefList& fileList)
{
	EmDlg::DoDatabaseImport (fileList, kMethodBest);
}



/***********************************************************************
 *
 * FUNCTION:	PrvButtonEvent
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	True if the session was closed, false if the user canceled.
 *
 ***********************************************************************/

static void PrvButtonEvent (Bool isDown, SkinElementType theButton)
{
	EmAssert (gSession);

	EmButtonEvent	event (theButton, isDown);
	gSession->PostButtonEvent (event);
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetModifiers
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	.
 *
 ***********************************************************************/

static void PrvGetModifiers (EmKeyEvent& event)
{
	if ((::GetKeyState (VK_SHIFT) & 0x80) != 0)
		event.fShiftDown = true;

	if ((::GetKeyState (VK_CAPITAL) & 0x01) != 0)
		event.fCapsLockDown = true;

	if ((::GetKeyState (VK_NUMLOCK) & 0x01) != 0)
		event.fNumLockDown = true;

//	if ((::GetKeyState (0) & 0x80) != 0)
//		event.fCommandDown = true;

//	if ((::GetKeyState (0) & 0x80) != 0)
//		event.fOptionDown = true;

	if ((::GetKeyState (VK_CONTROL) & 0x80) != 0)
		event.fControlDown = true;

	if ((::GetKeyState (VK_MENU) & 0x80) != 0)
		event.fAltDown = true;

	if ((::GetKeyState (VK_LWIN) & 0x80) != 0 ||
		(::GetKeyState (VK_RWIN) & 0x80) != 0)
		event.fWindowsDown = true;
}


/***********************************************************************
 *
 * FUNCTION:	PrvCloseSession
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	True if the session was closed, false if the user canceled.
 *
 ***********************************************************************/

Bool PrvCloseSession (HWND hWnd, const EmFileRef& saveFile, CloseActionType action)
{
	// If we're to save the session, then save it.

	if (action == kSaveAlways)
	{
		// Stop the CPU thread.  It will get stopped in our WM_DESTROY handler
		// anyway, but we need to stop it here in order to save the RAM image.

		EmSessionStopper	stopper (gSession, kStopNow);
		if (!stopper.Stopped ())
			return false;

		if (!::SaveRAMImage (hWnd, saveFile, !kForceQuery))
			return false;
	}

	// Close the LCD window.

	EmWindow*	window = ::GetWindow (hWnd);
	window->WindowDispose ();
	delete window;

	// Close the session.

	delete gSession;
	gSession = NULL;

	// Tell the message loop in DoUIThread to quit.

	::PostQuitMessage (0);

	return true;
}


Bool PrvCloseSession (HWND hWnd)
{
	// Get the user's preference as to what we're supposed to do.

	Preference<CloseActionType> pref1 (kPrefKeyCloseAction);
	CloseActionType 			action = *pref1;

	if (::IsBoundFully ())
	{
		action = kSaveNever;
	}

	// Get the name of the last-saved session file.

	Preference<EmFileRef>	pref2 (kPrefKeyLastPSF);
	EmFileRef				saveFile = *pref2;

	// If the user wants us to ask before quitting, then ask.

	if (action == kSaveAsk)
	{
		string		docName (saveFile.GetName ());
		EmDlgItemID	item = EmDlg::DoSessionSave (docName, false);

		if (item == kDlgItemOK)
			action = kSaveAlways;

		else if (item == kDlgItemDontSave)
			action = kSaveNever;

		else		// item == kDlgItemCancel
			return false;
	}

	return PrvCloseSession (hWnd, saveFile, action);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoAbout
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoAbout (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(command);

	::DoAbout (hWnd);
}


/***********************************************************************
 *
 * FUNCTION:	DoNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoNew (HWND hWnd, WPARAM command)
{
	Preference<Configuration>	pref (kPrefKeyLastConfiguration);
	Configuration	cfg = *pref;

	if (::QueryNewDocument (hWnd, cfg))
	{
		if (::PrvCloseSession (hWnd))
		{
			Startup::ScheduleCreateSession(cfg);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoOpenOther
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoOpenOther (HWND hWnd, WPARAM command)
{
	EmFileRef	ref;

	if (::QueryOpenDocument (hWnd, ref))
	{
		if (::PrvCloseSession (hWnd))
		{
			Startup::ScheduleOpenSession(ref);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoOpenRecent
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoOpenRecent (HWND hWnd, WPARAM command)
{
	if (command >= kCommandSessionOpen0 &&
		command < kCommandSessionOpen0 + MRU_COUNT)
	{
		if (::PrvCloseSession (hWnd))
		{
			EmFileRef	ref (gEmuPrefs->GetIndRAMMRU (command - kCommandSessionOpen0));
			Startup::ScheduleOpenSession (ref);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoClose
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoClose (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(command);

	if (::PrvCloseSession (hWnd))
	{
		Startup::ScheduleAskWhatToDo();
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoSave
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoSave (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(command);

	Preference<EmFileRef>	pref (kPrefKeyLastPSF);
	EmFileRef				currentRAMRef = *pref;
	::SaveRAMImage (hWnd, currentRAMRef, !kForceQuery);
}


/***********************************************************************
 *
 * FUNCTION:	DoSaveAs
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoSaveAs (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(command);

	Preference<EmFileRef>	pref (kPrefKeyLastPSF);
	EmFileRef				currentRAMRef = *pref;
	::SaveRAMImage (hWnd, currentRAMRef, kForceQuery);
}


/***********************************************************************
 *
 * FUNCTION:	DoSaveBound
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoSaveBound (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	OSVERSIONINFO	version;
	version.dwOSVersionInfoSize = sizeof (version);
	::GetVersionEx (&version);
	if (version.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		::MessageBox (NULL, 
			"This operation requires Windows NT or better.  Windows\n"
			"95, 98 and Millennium Edition don't provide the facilities\n"
			"for creating bound applications.", "Error",
			MB_ICONHAND | MB_OK);
		return;
	}

	EmDlg::DoSaveBound ();
}


/***********************************************************************
 *
 * FUNCTION:	DoSaveScreen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoSaveScreen (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmFileRef		result;
	string			prompt;
	EmDirRef		defaultPath;
	EmFileTypeList	filterList;
	string			defaultName;

	filterList.push_back (kFileTypePicture);
	filterList.push_back (kFileTypeAll);

	EmDlgItemID	item = EmDlg::DoPutFile (	result,
											prompt,
											defaultPath,
											filterList,
											defaultName);

	if (item == kDlgItemOK)
	{
		HANDLE	file = ::CreateFile (result.GetFullPath ().c_str (),
									GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL, NULL);

		if (file != INVALID_HANDLE_VALUE)
		{
			EmScreenUpdateInfo	info;
			::GetWindow (hWnd)->GetLCDContents (info);

			if (info.fImage.GetDepth () < 8)
			{
				info.fImage.ConvertToFormat (kPixMapFormat8);
			}
			else if (info.fImage.GetDepth () == 16)
			{
				info.fImage.ConvertToFormat (kPixMapFormat16RGB555);
			}
			else if (info.fImage.GetDepth () == 24)
			{
				info.fImage.ConvertToFormat (kPixMapFormat24BGR);
			}

			long	depth					= info.fImage.GetDepth ();
			long	numColors				= (depth <= 8) ? (1 << depth) : 0;
			EmPoint	size					= info.fImage.GetSize ();

			DWORD	fileHeaderSize			= sizeof (BITMAPFILEHEADER);
			DWORD	bitmapHeaderSize		= sizeof (BITMAPINFOHEADER);
			DWORD	colorTableSize			= numColors * sizeof (RGBQUAD);
			DWORD	bitmapOffset			= fileHeaderSize + bitmapHeaderSize + colorTableSize;
			DWORD	bitmapSize				= info.fImage.GetRowBytes () * size.fY;
			DWORD	fileSize				= bitmapOffset + bitmapSize;

			BITMAPFILEHEADER	fileHeader;

			fileHeader.bfType				= 'MB';
			fileHeader.bfSize				= fileSize;
			fileHeader.bfReserved1			= 0;
			fileHeader.bfReserved2			= 0;
			fileHeader.bfOffBits			= bitmapOffset;

			BITMAPINFOHEADER	bitmapHeader;

			bitmapHeader.biSize 			= sizeof (BITMAPINFOHEADER);
			bitmapHeader.biWidth			= size.fX;
			bitmapHeader.biHeight			= size.fY;
			bitmapHeader.biPlanes			= 1;
			bitmapHeader.biBitCount 		= depth;
			bitmapHeader.biCompression		= BI_RGB;
			bitmapHeader.biSizeImage		= 0;
			bitmapHeader.biXPelsPerMeter	= 0;
			bitmapHeader.biYPelsPerMeter	= 0;
			bitmapHeader.biClrUsed			= numColors;
			bitmapHeader.biClrImportant 	= 0;

			RGBQUAD*	colors = (RGBQUAD*) _alloca (colorTableSize);
			::ConvertFromRGBList (colors, info.fImage.GetColorTable ());

			info.fImage.FlipScanlines ();

			DWORD	written;

			::WriteFile (file, &fileHeader, fileHeaderSize, &written, NULL);
			::WriteFile (file, &bitmapHeader, bitmapHeaderSize, &written, NULL);
			::WriteFile (file, colors, colorTableSize, &written, NULL);
			::WriteFile (file, info.fImage.GetBits (), bitmapSize, &written, NULL);

			::CloseHandle (file);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoInstallOther
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoInstallOther (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(command);

	EmFileRef		lastLoadedFile = gEmuPrefs->GetIndPRCMRU (0);

	EmFileRefList	fileList;
	string			prompt;
	EmDirRef		defaultPath = lastLoadedFile.GetParent ();
	EmFileTypeList	filterList;

	filterList.push_back (kFileTypePalmAll);
	filterList.push_back (kFileTypePalmApp);
	filterList.push_back (kFileTypePalmDB);
	filterList.push_back (kFileTypePalmQA);
	filterList.push_back (kFileTypeAll);

	EmDlgItemID		item = EmDlg::DoGetFileList (	fileList,
													prompt,
													defaultPath,
													filterList);

	if (item == kDlgItemOK)
	{
		::PrvLoadPalmFiles (fileList);
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoInstallRecent
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoInstallRecent (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	if (command >= kCommandImport0 &&
		command < kCommandImport0 + MRU_COUNT)
	{
		EmFileRef	theRef = gEmuPrefs->GetIndPRCMRU (command - kCommandImport0);

		if (theRef.IsSpecified ())
		{
			EmFileRefList	fileList;
			fileList.push_back (theRef);
			::PrvLoadPalmFiles (fileList);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoExportDatabase
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoExportDatabase (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoDatabaseExport ();
}


/***********************************************************************
 *
 * FUNCTION:	DoHotSync
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoHotSync (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	::PrvButtonEvent (true, kElement_CradleButton);
	::PrvButtonEvent (false, kElement_CradleButton);
}


/***********************************************************************
 *
 * FUNCTION:	DoReset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoReset (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmResetType	type;
	if (EmDlg::DoReset (type) == kDlgItemOK)
	{
		gSession->Reset (type);
	}
}

#ifdef SONY_ROM
/***********************************************************************
 *
 * FUNCTION:	DoSoftReset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoSoftReset (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	gSession->Reset (kResetSoft);
}
#endif

/***********************************************************************
 *
 * FUNCTION:	DoTransferROM
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoTransferROM (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmROMTransfer::ROMTransfer ();
}


/***********************************************************************
 *
 * FUNCTION:	DoGremlinsNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoGremlinsNew (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoHordeNew ();
}


/***********************************************************************
 *
 * FUNCTION:	DoGremlinsStep
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoGremlinsStep (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	Hordes::Step ();
}


/***********************************************************************
 *
 * FUNCTION:	DoGremlinsResume
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoGremlinsResume (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	Hordes::Resume ();
}


/***********************************************************************
 *
 * FUNCTION:	DoGremlinsStop
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoGremlinsStop (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	Hordes::Stop ();
}


/***********************************************************************
 *
 * FUNCTION:	DoProfileStart
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoProfileStart (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	if (::ProfileCanStart ())
	{
		::ProfileStart ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoProfileStop
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoProfileStop (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	if (::ProfileCanStop ())
	{
		::ProfileStop ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoProfileDump
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoProfileDump (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	if (::ProfileCanDump ())
	{
		::ProfileDump (NULL);
	}
}


/***********************************************************************
 *
 * FUNCTION:	DoProperties
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoProperties (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditPreferences ();
}


/***********************************************************************
 *
 * FUNCTION:	DoLoggingOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoLoggingOptions (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditLoggingOptions (kNormalLogging);
}


/***********************************************************************
 *
 * FUNCTION:	DoDebugOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoDebugOptions (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditDebuggingOptions ();
}


/***********************************************************************
 *
 * FUNCTION:	DoTracingOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoTracingOptions (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditTracingOptions ();
}


/***********************************************************************
 *
 * FUNCTION:	DoSkins
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoSkins (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditSkins ();
}


/***********************************************************************
 *
 * FUNCTION:	DoBreakpoints
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoBreakpoints (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditBreakpoints ();
}


/***********************************************************************
 *
 * FUNCTION:	DoCardOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoCardOptions (HWND hWnd, WPARAM command)
{
	UNUSED_PARAM(hWnd);
	UNUSED_PARAM(command);

	EmDlg::DoEditCardOptions ();
}


/***********************************************************************
 *
 * FUNCTION:	DoMinimize
 *
 * DESCRIPTION:	Minimize the emulator window.
 *
 * PARAMETERS:	standard Windows interface parameters
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoMinimize (HWND hWnd, WPARAM command)
{
	ShowWindow(hWnd, SW_MINIMIZE);
}


/***********************************************************************
 *
 * FUNCTION:	DoExit
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoExit (HWND hWnd, WPARAM command)
{
	if (::PrvCloseSession (hWnd))
	{
		Startup::ScheduleQuit();
	}
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	PopupMenu
 *
 * DESCRIPTION:	Shows standard menu.
 *
 * PARAMETERS:	standard Windows interface parameters.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void PopupMenu (HWND hWnd, int x, int y)
{
	// Update the menu.

	EmMenuItemList*	menu = ::MenuFindMenu (kMenuPopupMenuPreferred);
	EmAssert (menu);

	::MenuUpdateMruMenus (*menu);
	::MenuUpdateMenuItemStatus (*menu);

	// Create and show the menu.

	HMENU	hostMenu = ::HostCreatePopupMenu (*menu);

	POINT	p = { x,  y };
	::ClientToScreen (hWnd, &p);

	::TrackPopupMenu (hostMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, 0, hWnd, NULL);

	::DestroyMenu (hostMenu);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:    FUNCTION_NAME
 *
 * DESCRIPTION: DESCRIPTION
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

static void PrvOnChar (HWND hWnd, TCHAR ch, int cRepeat)
{
	switch (ch)
	{
		case VK_RETURN: ch = chrLineFeed;		break;

			// These are handled in the key up/down handlers
			// below, so they shouldn't result in a key being
			// inserted into the event queue.
			//
			// Actually...pressing most of these keys doesn't
			// result in a WM_CHAR event.  We only need to check
			// for VK_ESCAPE.
//		case VK_UP: 	return;
//		case VK_DOWN:	return;
//		case VK_LEFT:	return;
//		case VK_RIGHT:	return;
		case VK_ESCAPE: return;
//		case VK_PRIOR:	return;
//		case VK_NEXT:	return;
//		case VK_F1: 	return;
//		case VK_F2: 	return;
//		case VK_F3: 	return;
//		case VK_F4: 	return;
	}

	EmKeyEvent	event ((unsigned char) ch);
	::PrvGetModifiers (event);
	gSession->PostKeyEvent (event);
}


static void PrvOnClose (HWND hWnd)
{
	::DoExit (hWnd, 0);
}


static void PrvOnCommand (HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
	// We need to check to see if the command was issued because of a match
	// with the accelerator table.  If so, the command may be disabled.

	if (codeNotify == 1)	// Indicates command from accelerator
	{
		// Get an up-to-date menu list.

		EmMenuItemList*	menu = ::MenuFindMenu (kMenuPopupMenuPreferred);
		EmAssert (menu);

		::MenuUpdateMruMenus (*menu);
		::MenuUpdateMenuItemStatus (*menu);

		// Get the menu item for this command.

		EmMenuItem*		selected = MenuFindMenuItemByCommandID (*menu, (EmCommandID) id, true);
		EmAssert (selected);

		// If we couldn't find it, leave.

		if (!selected)
			return;

		// If disabled, leave.

		if (!selected->GetIsActive ())
			return;

		// Otherwise, it should be OK to execute this command.
		// Fall through to the command handler.
	}

	int index = 0;
	while (kCommandHandlers[index].fCommand)
	{
		if (kCommandHandlers[index].fCommand == id)
		{
			try
			{
				EmStopMethod		how = kCommandHandlers[index].fStopType;
				EmSessionStopper	stopper (gSession, how);
				if (how == kStopNone || stopper.Stopped ())
				{
					kCommandHandlers[index].fHandler (hWnd, id);
				}
				else
				{
					// !!! Show an error?
				}
			}
			catch (ErrCode errCode)
			{
				Errors::ReportIfError (kCommandHandlers[index].fOperation, errCode, 0, false);
			}

			break;
		}

		index++;
	}
}


static void PrvOnContextMenu (HWND hWnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	UNUSED_PARAM (hwndContext);

	// When the user clicks with the right mouse button, popup our menu.

	if (xPos == 0x0000FFFF && xPos == 0x0000FFFF)
	{
		xPos = 0;
		yPos = 0;
	}
	else
	{
		POINT local = { xPos, yPos };
		::ScreenToClient (hWnd, &local);

		xPos = local.x;
		yPos = local.y;
	}

	::PopupMenu (hWnd, xPos, yPos);
}


static void PrvOnDisplayChange (HWND hWnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (bitsPerPixel);
	UNUSED_PARAM (cxScreen);
	UNUSED_PARAM (cyScreen);

	::GetWindow (hWnd)->HandleDisplayChange ();
}


static void PrvOnDropFiles (HWND hWnd, HDROP hdrop)
{
	// A file is being dropped.

	HDROP	hDropInfo = hdrop;

	// Get the number of files.

	int numFiles = ::DragQueryFile (hDropInfo, (DWORD)(-1), NULL, 0);

	// This would get the drop point, if we needed it.

//	POINT	pt;
//	::DragQueryPoint (hDropInfo, &pt);

	EmFileRefList	fileList;

	for (int ii = 0; ii < numFiles; ++ii)
	{
		char	filePath [MAX_PATH];
		::DragQueryFile (hDropInfo, ii, filePath, sizeof (filePath));

		EmFileRef	fileRef (filePath);
		fileList.push_back (fileRef);
	}

	// Signal that the drag-and-drop operation is over.

	::DragFinish (hDropInfo);

	::LoadAnyFiles (fileList);
}


static void PrvOnKeyDown (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	unsigned char		whichKey	= 0;
	SkinElementType 	whichButton = kElement_None;

	switch (vk)
	{
		case VK_UP: 	whichKey = chrUpArrow;		break;
		case VK_DOWN:	whichKey = chrDownArrow;	break;
		case VK_LEFT:	whichKey = chrLeftArrow;	break;
		case VK_RIGHT:	whichKey = chrRightArrow;	break;

		case VK_ESCAPE: whichButton = kElement_PowerButton; 	break;
		case VK_PRIOR:	whichButton = kElement_UpButton;		break;
		case VK_NEXT:	whichButton = kElement_DownButton;		break;
		case VK_F1: 	whichButton = kElement_App1Button;		break;
		case VK_F2: 	whichButton = kElement_App2Button;		break;
		case VK_F3: 	whichButton = kElement_App3Button;		break;
		case VK_F4: 	whichButton = kElement_App4Button;		break;
	}

	if (whichKey != 0)
	{
		EmKeyEvent	event ((unsigned char) whichKey);
		::PrvGetModifiers (event);
		gSession->PostKeyEvent (event);
	}

	if (whichButton != kElement_None)
	{
		::PrvButtonEvent (true, whichButton);
	}
}


static void PrvOnKeyUp (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	SkinElementType 	whichButton = kElement_None;

	switch (vk)
	{
		case VK_ESCAPE: whichButton = kElement_PowerButton; 	break;
		case VK_PRIOR:	whichButton = kElement_UpButton;		break;
		case VK_NEXT:	whichButton = kElement_DownButton;		break;
		case VK_F1: 	whichButton = kElement_App1Button;		break;
		case VK_F2: 	whichButton = kElement_App2Button;		break;
		case VK_F3: 	whichButton = kElement_App3Button;		break;
		case VK_F4: 	whichButton = kElement_App4Button;		break;
	}

	if (whichButton != kElement_None)
	{
		::PrvButtonEvent (false, whichButton);
	}
}


static void PrvOnLButtonDown (HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	gLastPen = EmPoint (x, y);
	::GetWindow (hWnd)->HandlePenEvent (gLastPen, true);
}


static void PrvOnLButtonUp (HWND hWnd, int x, int y, UINT keyFlags)
{
	gLastPen = EmPoint (x, y);
	::GetWindow (hWnd)->HandlePenEvent (gLastPen, false);
}


static void PrvOnMouseMove (HWND hWnd, int x, int y, UINT keyFlags)
{
	if (::GetCapture () == hWnd)
	{
		gLastPen = EmPoint (x, y);
	}
}


static UINT PrvOnNcHitTest (HWND hWnd, int x, int y)
{
	POINT	clientPt = { x, y };
	::ScreenToClient (hWnd, &clientPt);

	EmPoint			pt (clientPt.x, clientPt.y);
	SkinElementType what = ::SkinTestPoint (pt);

	if (what == kElement_Frame)
		return HTCAPTION;

	return FORWARD_WM_NCHITTEST (hWnd, x, y, DefWindowProc);
}


static void PrvOnNcRButtonUp (HWND hWnd, int x, int y, UINT codeHitTest)
{
	// When the user clicks with the right mouse button, popup our menu.
	// We need to post this message explicitly, because Windows won't
	// do it.  Because of our handling of WM_NCHITTEST, Windows thinks
	// that right-clicks in the skin frame are in the caption.  In that
	// case, Windows is supposed to show the "window" (or "system")
	// menu.  However, we don't have such a menu, so Windows actually
	// does nothing.  Therefore, we need to post the WM_CONTEXTMENU ourselves.

	::PostMessage (hWnd, WM_CONTEXTMENU, (WPARAM) hWnd, MAKELPARAM (x, y));
}


static void PrvOnPaint (HWND hWnd)
{
	::GetWindow (hWnd)->HandleUpdate ();
}


static void PrvOnPaletteChanged(HWND hWnd, HWND hwndPaletteChange)
{
	if (hwndPaletteChange != hWnd)
	{
		::GetWindow (hWnd)->HandlePalette ();
	}
}


static BOOL PrvOnQueryNewPalette(HWND hWnd)
{
	::GetWindow (hWnd)->HandlePalette ();
	return true;
}


static void PrvOnSysKeyDown (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
		// When the user presses Alt+F11, minimize our window.

		case VK_F11:
			if (flags & (1 << (29 - 16)))
				::DoMinimize (hWnd, 0);
			return;
	}

	FORWARD_WM_SYSKEYDOWN (hWnd, vk, cRepeat, flags, DefWindowProc);
}


static void PrvOnTimer (HWND hWnd, UINT id)
{
	// Don't let us recurse.  This could happen if, for example, we were
	// handling a debugger packet, an exception occurs, and Poser tries
	// to show an error message in a dialog.  That dialog would allow
	// re-entry into this function.

	static Bool	inOnTimer;

	if (inOnTimer)
		return;

	EmValueChanger<Bool>	oldInOnTimer (inOnTimer, true);

	// Perform periodic tasks.

	EmAssert (gSession);


	// Update our window with the current contents of the emulated LCD buffer.

	::GetWindow (hWnd)->HandleIdle (gLastPen);


	// Show any error messages from the CPU thread.

	{
		EmDlgFn	fn;
		void*	userData;
		EmDlgID	dlgID;

		if (gSession->BeginDialog (fn, userData, dlgID))
		{
			EmDlgItemID	result = EmDlg::RunDialog (fn, userData, dlgID);
			gSession->EndDialog (result);
		}
	}

	// Idle our sockets.

	CSocket::IdleAll ();

	// Idle any packets that are looking for signals.
	// That is, see if any have timed out by now.

	RPC::Idle ();

	// If we need to start a new Gremlin Horde, and the OS
	// is now booted enough to handle our mucking about as
	// we start it up, then start starting it up.

	if (Patches::EvtGetEventCalled ())
	{
		if (Startup::NewHorde (NULL))
		{
			EmSessionStopper	stopper (gSession, kStopOnSysCall);
			if (stopper.Stopped ())
			{
				HordeInfo	info;
				Startup::NewHorde (&info);
				Hordes::New (info);
			}
		}
	}

	if (gOpenGCWWindow)
	{
		gOpenGCWWindow = false;
		Platform::GCW_Open ();
	}


	// Bump this guy so that periodic tasks can be handled in
	// Hardware::Cycle.

	gPseudoTickcount++;

	if (Patches::TimeToQuit ())
	{
		::DoExit (hWnd, kCommandQuit);
	}

	EmFileRef	f;
	if (Startup::CloseSession (f))
	{
		CloseActionType	action = f.IsSpecified() ? kSaveAlways : kSaveNever;
		if (::PrvCloseSession (hWnd, f, action))
		{
			// Don't do this, in case the request to close was followed
			// by a request to quit.
//			Startup::ScheduleAskWhatToDo();
		}
	}
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DisplayWndProc
 *
 * DESCRIPTION:	Dispatch the command to the right handler via the
 *				message cracker macros.
 *
 * PARAMETERS:	Standard Window DefProc parameters.
 *
 * RETURNED:	Result code specific to the command.
 *
 ***********************************************************************/

LRESULT CALLBACK DisplayWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG (hWnd, WM_CHAR,				PrvOnChar);
		HANDLE_MSG (hWnd, WM_CLOSE,				PrvOnClose);
		HANDLE_MSG (hWnd, WM_COMMAND,			PrvOnCommand);
		HANDLE_MSG (hWnd, WM_CONTEXTMENU,		PrvOnContextMenu);
		HANDLE_MSG (hWnd, WM_DISPLAYCHANGE,		PrvOnDisplayChange);
		HANDLE_MSG (hWnd, WM_DROPFILES,			PrvOnDropFiles);
		HANDLE_MSG (hWnd, WM_KEYDOWN,			PrvOnKeyDown);
		HANDLE_MSG (hWnd, WM_KEYUP,				PrvOnKeyUp);
		HANDLE_MSG (hWnd, WM_LBUTTONDOWN,		PrvOnLButtonDown);
		HANDLE_MSG (hWnd, WM_LBUTTONUP,			PrvOnLButtonUp);
		HANDLE_MSG (hWnd, WM_MOUSEMOVE,			PrvOnMouseMove);
		HANDLE_MSG (hWnd, WM_NCHITTEST,			PrvOnNcHitTest);
		HANDLE_MSG (hWnd, WM_NCRBUTTONUP,		PrvOnNcRButtonUp);
		HANDLE_MSG (hWnd, WM_PAINT,				PrvOnPaint);
		HANDLE_MSG (hWnd, WM_PALETTECHANGED,	PrvOnPaletteChanged);
		HANDLE_MSG (hWnd, WM_QUERYNEWPALETTE,	PrvOnQueryNewPalette);
		HANDLE_MSG (hWnd, WM_SYSKEYDOWN,		PrvOnSysKeyDown);
		HANDLE_MSG (hWnd, WM_TIMER,				PrvOnTimer);

		case MM_WOM_DONE:
			gSounds->NextSound();
			return NULL;
	}

	return ::DefWindowProc (hWnd, msg, wParam, lParam);
}


/***********************************************************************
 *
 * FUNCTION:	DoUIThread
 *
 * DESCRIPTION:	Thread proc for the UI thread.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoUIThread (void)
{
	// Open the connection to the external debugger.  We used to do
	// this from Platform::ConnectToDebugger, but that would get
	// called from a different thread.  Because the window used for
	// communications would belong to a different thread, the message
	// loop below would not process messages for it.  So we do it here.

	::MMFOpenFiles (gInstance, CMMFSocket::HandlePacketCB, 0,
					kFromEmulatorFileName, kToEmulatorFileName);

	// Kickstart the CPU emulator (running in its own thread).  Do this
	// *before* creating the LCD window, in case any messages are sent
	// to the window that require the use of the CPU thread.

	gSession->ResumeThread ();

	// Create the LCD window.

	EmWindow*	window = new EmWindow;
	window->WindowCreate ();

	// Create SoundMaker object.

	gSounds = new WinSounds;

	EmMenuItemList*	menu = ::MenuFindMenu (kMenuPopupMenuPreferred);
	EmAssert (menu);

	HACCEL	hAccel = ::HostCreateAcceleratorTable (*menu);

	// Our main event loop.  Retrieve and handle events until the application quits.

	MSG msg;
	while (::GetMessage (&msg, NULL, 0, 0))
	{
	    // Check for accelerator keystrokes. 
 
		if (!::TranslateAccelerator (
            msg.hwnd,	// handle to receiving window
            hAccel,		// handle to active accelerator table
            &msg))		// message data
		{
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
		}
	}

	::DestroyAcceleratorTable (hAccel);

	::MMFCloseFiles (gInstance);

	delete gSounds;
	gSounds = NULL;
}
