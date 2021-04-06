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
#include "Emulator.h"

#include "Application.h"		// DoUIThread
#include "Document.h"			// QueryNewDocument, QueryOldDocument, etc.
#include "EmDlgWin.h"			// CenterWindow, CenterDlgProc
#include "EmFileRef.h"			// SetEmulatorRef
#include "EmMenus.h"			// MenuInitialize
#include "EmROMTransfer.h"		// EmROMTransfer::ROMTransfer
#include "EmTransportUSBWin.h"	// EmHostTransportUSB
#include "ErrorHandling.h"		// Errors::ReportIfError
#include "Hordes.h"				// gWarningHappened, gErrorHappened
#include "Miscellaneous.h"		// CommonStartup, CommonShutdown
#include "Platform.h"			// Platform::
#include "Preferences.h"		// EmulatorPreferences
#include "EmRPC.h"				// RPC::SignalWaiters
#include "SocketMessaging.h"	// CSocket::IdleAll
#include "StartMenu.h"			// AddStartMenuItem
#include "Startup.h"			// Startup::, etc.
#include "Strings.r.h"			// kStr_CmdNew, kStr_CmdOpen

#include "resource.h"


// ======================================================================
//	Globals and constants
// ======================================================================

HINSTANCE	gInstance;


/***********************************************************************
 *
 * FUNCTION:	PrvStartupDlgProc
 *
 * DESCRIPTION:	DlgProc for the startup dialog (the one with the New,
 *				Open, etc. buttons).  Centers the window, and ends
 *				the dialog once any of the buttons are clicked.
 *
 * PARAMETERS:	Standard DlgProc parameters
 *
 * RETURNED:	Standard DlgProc result
 *
 ***********************************************************************/

static BOOL CALLBACK PrvStartupDlgProc (HWND hwnd,
										UINT msg,
										WPARAM wparam,
										LPARAM lparam)
{
	UNUSED_PARAM (lparam);

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			::CenterWindow (hwnd);
			::SetTimer (hwnd, 1, 100, NULL);
			break;
		}

		case WM_DESTROY:
		{
			::KillTimer (hwnd, 1);
		}

		case WM_TIMER:
		{
			CSocket::IdleAll ();
			RPC::Idle ();

			// See if any event came in from an external client.
			if (!Startup::AskWhatToDo ())
			{
				::EndDialog (hwnd, IDOK);
			}
			break;
		}

		case WM_COMMAND:
		{
			switch (LOWORD (wparam))
			{
				case IDC_NEW:
				case IDC_OPEN:
				case IDC_DOWNLOAD:
				case IDC_EXIT:
				case IDCANCEL:
				{
					::EndDialog (hwnd, LOWORD (wparam));
					break;
				}
			}

			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}


/***********************************************************************
 *
 * FUNCTION:	PrvStartupScreen
 *
 * DESCRIPTION:	Presents the user with the New/Open/etc dialog.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Button the user clicked on.
 *
 ***********************************************************************/

static int PrvStartupScreen (void)
{
	EmAssert (!::IsBound ());

	int	choice = ::DialogBox (	gInstance,
								MAKEINTRESOURCE (IDD_STARTUP),
								NULL,	// hwndParent
								&::PrvStartupDlgProc);

	return choice;
}


/***********************************************************************
 *
 * FUNCTION:	PrvSetRegKey
 *
 * DESCRIPTION:	Set an entry in the Windows system registry.
 *
 * PARAMETERS:	lpszKey - key for the entry
 *
 *				lpszValue - value of the entry
 *
 *				lpszValueName - name for the value.  Could be NULL,
 *					in which case the value is assigned as the value
 *					of the key itself.
 *
 * RETURNED:	True if the operation was a success; false if the
 *				patient died.
 *
 ***********************************************************************/

static Bool PrvSetRegKey (LPCTSTR lpszKey, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL)
{
	if (lpszValueName == NULL)
	{
		if (::RegSetValue ( HKEY_CLASSES_ROOT,
							lpszKey, REG_SZ,
							lpszValue,
							lstrlen(lpszValue) * sizeof(TCHAR)) != ERROR_SUCCESS)
		{
			return FALSE;
		}

		return TRUE;
	}

	HKEY hKey;

	if (::RegCreateKey (HKEY_CLASSES_ROOT, lpszKey, &hKey) == ERROR_SUCCESS)
	{
		LONG	lResult = ::RegSetValueEx ( hKey,
											lpszValueName,
											0,
											REG_SZ,
											(CONST BYTE*) lpszValue,
											(lstrlen(lpszValue) + 1) * sizeof(TCHAR));

		if (::RegCloseKey (hKey) == ERROR_SUCCESS && lResult == ERROR_SUCCESS)
		{
			return TRUE;
		}
	}

	return FALSE;

}


/***********************************************************************
 *
 * FUNCTION:	PrvRegisterTypes
 *
 * DESCRIPTION:	Register our file types and icons.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void PrvRegisterTypes (void)
{
	/*
		The following is from Eric Cloninger:

			Key 							Default Value
			HKEY_CLASSES_ROOT\\.psf 		"POSE_file"
			HKEY_CLASSES_ROOT\\POSE_file	"POSE File"    <<< Shows up in Explorer
			HKEY_CLASSES_ROOT\\POSE_file\\shell\open\command  "c:\pose\emulator.exe"
			HKEY_CLASSES_ROOT\\POSE_file\\DefaultIcon		  "c:\pose\emulator.exe,1"

		For more details, see MSDN Article ID: Q122787. (Also Article ID: Q142275)

		Also:

			Finding the path

			Rather than jam your application's files into Windows directories, you can
			now place them in a more logical location, and the system can still find
			them. For example, you register the path where you locate your executable
			(.EXE) files as well as dynamic-link library (DLL) files by using the
			AppPaths subkey under
			HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion. Add a subkey
			using your application executable filename and set its default value to
			the path for your .EXE file. Add an additional value named Path for this
			subkey, and set its value to the path of your DLLs. The system will
			automatically update the path for these entries if a user moves or renames
			the application's executable file using the standard system UI.
	*/

	char	moduleName[_MAX_PATH];
	::GetModuleFileName (gInstance, moduleName, _MAX_PATH);

	char	icon[_MAX_PATH];
	::sprintf (icon, "%s,-%d", moduleName, IDI_DOCUMENT);

	::PrvSetRegKey (".psf", "psffile");
	::PrvSetRegKey ("psffile", "Palm Emulator Session");

	::PrvSetRegKey (".rom", "romfile");
	::PrvSetRegKey ("romfile", "Palm Emulator ROM");

	::PrvSetRegKey ("psffile\\DefaultIcon", icon);
	::PrvSetRegKey ("romfile\\DefaultIcon", icon);

	char	open[_MAX_PATH];
	::sprintf (open, "\"%s\" -psf \"%%1\"", moduleName);
	::PrvSetRegKey ("psffile\\shell\\open\\command", open);

	::sprintf (open, "\"%s\" -rom \"%%1\"", moduleName);
	::PrvSetRegKey ("romfile\\shell\\open\\command", open);
}


/***********************************************************************
 *
 * FUNCTION:	ShouldAskAboutStartMenu
 *
 * DESCRIPTION:	Decides if the user should be prompted about the start
 *				menu
 *
 * PARAMETERS:	none
 *
 * RETURNED:	TRUE if Emulator should ask about the start menu
 *				FALSE if not
 *
 ***********************************************************************/

static bool ShouldAskAboutStartMenu(void)
{
	Preference<bool>		userWants (kPrefKeyAskAboutStartMenu);
	Preference<EmFileRef>	link (kPrefKeyStartMenuItem);
	bool linkExists = false;

	HANDLE shortcut =
		CreateFile((*link).GetFullPath ().c_str (),
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);

	if (shortcut != INVALID_HANDLE_VALUE)
	{
		linkExists = true;
		CloseHandle(shortcut);
	}
	
	// We should ask about the start menu if all of the following are true:
	// 1. The user wants us to ask about it.
	// 2. The link does not already exist.

	return (*userWants && !linkExists);
}


/***********************************************************************
 *
 * FUNCTION:	StartMenuDlgProc
 *
 * DESCRIPTION:	Start menu dialog processing
 *
 * PARAMETERS:	Standard Windows dialog function
 *
 * RETURNED:	Stardard Windows dialog function
 *
 ***********************************************************************/

BOOL CALLBACK StartMenuDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			Preference<bool>	userWantsPref (kPrefKeyAskAboutStartMenu);
			
			::CenterWindow(hWnd);

			if (*userWantsPref)
				::CheckDlgButton(hWnd, IDC_ASK_IN_FUTURE, BST_CHECKED);
			else
				::CheckDlgButton(hWnd, IDC_ASK_IN_FUTURE, BST_UNCHECKED);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDC_ADD_ITEM:
				{
					char appPath[_MAX_PATH];
					char linkPath[_MAX_PATH];

					::GetModuleFileName (gInstance, appPath, _MAX_PATH);
					if (::AddStartMenuItem(hWnd, gInstance, appPath, linkPath))
					{
						Preference<bool>	userWantsPref (kPrefKeyAskAboutStartMenu);
						userWantsPref = (BST_CHECKED ==
										::IsDlgButtonChecked(hWnd, IDC_ASK_IN_FUTURE));

						EmFileRef				link(linkPath);
						Preference<EmFileRef>	linkPref (kPrefKeyStartMenuItem);
						linkPref = link.ToPrefString();

						EndDialog(hWnd, 0);
					}
					break;
				}

				case IDNO:
				{
					Preference<bool>	userWantsPref (kPrefKeyAskAboutStartMenu);
					/*
					userWantsPref = (BST_CHECKED ==
										::IsDlgButtonChecked(hWnd, IDC_ASK_IN_FUTURE));
					*/
					userWantsPref = false;

					EmFileRef				link;
					Preference<EmFileRef>	linkPref (kPrefKeyStartMenuItem);
					linkPref = link;

					EndDialog(hWnd, 0);

					break;
				}
			}

			return (TRUE);
		}
	}

	return (FALSE);
}


/***********************************************************************
 *
 * FUNCTION:	PrvWinSockStartup
 *
 * DESCRIPTION:	DESCRIPTION
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

static void PrvWinSockStartup (void)
{
	/*
		Start up WinSock.

		FROM THE WINDOWS SOCKETS DOCUMENTATION:

			Upon entry to WSAStartup, the WS2_32.DLL examines the version requested
			by the application. If this version is equal to or higher than the lowest
			version supported by the DLL, the call succeeds and the DLL returns in
			wHighVersion the highest version it supports and in wVersion the minimum
			of its high version and wVersionRequested. The WS2_32.DLL then assumes
			that the application will use wVersion.

		WHAT THIS MEANS:

						requested_version
								|
				+---------------+---------------+
				| (a)			| (b)			| (c)
				V				V				V
				   +-------------------------+
				   | Supported version range |
				   +-------------------------+

			(a) We've requested a version older than that supported by the DLL. The docs
				don't say what happens here. I assume WSAStartup returns WSAVERNOTSUPPORTED.
				Outcome: we don't have a chance, so error out.

			(b) We've requested a version number in the range supported by the DLL.
				wHighVersion holds the right end of the range shown above.
				wVersion holds the requested version.
				Outcome: use the version in wVersion.

			(c) We've requested a version newer than that supported by the DLL.
				wHighVersion holds the right end of the range shown above.
				wVersion holds the right end of the range shown above.
				Outcome: use the version in wVersion
	*/

	WORD		versionRequested = MAKEWORD (2, 2);
	WSADATA 	wsaData;

	gWinSockVersion = 0;
	int result = ::WSAStartup (versionRequested, &wsaData);

	if (result == NO_ERROR)
	{
		gWinSockVersion = wsaData.wVersion;
	}
	else
	{
		::MessageBox (NULL, "Unable to open the Windows Sockets library.\n"
			"Networking services will be unavailable.", "Error", MB_OK);
	}

	// Don't make this check.  Windows '95 returns version 1.1 from WSAStartup.

//	if (LOBYTE (wsaData.wVersion) != 2 || HIBYTE (wsaData.wVersion) != 2)
//	{
//		gSocketLog.Printf ("wsaData.wVersion = %08X\n", (long) wsaData.wVersion);
//		PrvErrorOccurred ();
//		return;
//	}
}


/***********************************************************************
 *
 * FUNCTION:	PrvSubsystemStartup
 *
 * DESCRIPTION:	DESCRIPTION
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

static void PrvSubsystemStartup (void)
{
	::InitCommonControls ();
	::PrvWinSockStartup ();

	EmHostTransportUSB::Startup ();

	::MenuInitialize (false);

	::CommonStartup ();
}


/***********************************************************************
 *
 * FUNCTION:	PrvSubsystemShutdown
 *
 * DESCRIPTION:	DESCRIPTION
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

static void PrvSubsystemShutdown (void)
{
	::CommonShutdown ();

	EmHostTransportUSB::Shutdown ();

	::WSACleanup ();
}


/***********************************************************************
 *
 * FUNCTION:	PrvProcessStartMenu
 *
 * DESCRIPTION:	If we haven't already asked on this system, or if the
 *				shortcut has disappeared, we ask if the user would like
 *				to add us to their start menu.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void PrvProcessStartMenu(void)
{
	if (::ShouldAskAboutStartMenu())
	{
		//this will be needed later
		CoInitialize(NULL);

		//create the start menu dialog box
		DialogBox(gInstance, MAKEINTRESOURCE(IDD_STARTMENU), NULL, StartMenuDlgProc);
	
		CoUninitialize();
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvAskWhatToDo
 *
 * DESCRIPTION: Present the New/Open/Download/Quit dialog.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void PrvAskWhatToDo (void)
{
	int command;
	
	if (::IsBoundPartially ())
	{
		command = IDC_NEW;
	}
	else if (::IsBoundFully ())
	{
		command = IDC_OPEN;
	}
	else
	{
		command = ::PrvStartupScreen ();
	}

	switch (command)
	{
		case IDC_NEW:
		{
			EmAssert (!::IsBoundFully ());

			// Get the last defaults in this area.

			Preference<Configuration>	pref (kPrefKeyLastConfiguration);
			Configuration				cfg = *pref;

			// Else, ask the user for new document settings.

			if (::QueryNewDocument (NULL, cfg))
			{
				Startup::ScheduleCreateSession (cfg);
			}

			break;
		}

		case IDC_OPEN:
		{
			EmFileRef	ref;

			if (::IsBoundFully ())
			{
				Startup::ScheduleOpenSession (ref);	// Parameter ignored when bound...
			}
			else if (::QueryOpenDocument (NULL, ref))
			{
				Startup::ScheduleOpenSession (ref);
			}
			break;
		}

//		case IDC_RESTART:
//		{
//			EmFileRef	ref;
//			Startup::ScheduleOpenSession(ref);	// Parameter ignored when bound...
//			break;
//		}

		case IDC_DOWNLOAD:
		{
			EmROMTransfer::ROMTransfer ();
			break;
		}

		case IDC_EXIT:
		case IDCANCEL:
		{
			Startup::ScheduleQuit();
			break;
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	WinMain
 *
 * DESCRIPTION:	Application entry point.  Attempts to restart a
 *				previous session, or asks the user if they want to
 *				start a new one.
 *
 * PARAMETERS:	Standard WinMain parameters
 *
 * RETURNED:	Standard WinMain result
 *
 ***********************************************************************/

int WINAPI WinMain (HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	EmulatorPreferences    prefs;

//	_CrtSetBreakAlloc (1660);

	try
	{
		gInstance = instance;

		// Establish a ref for the emulator application.

		char	path[_MAX_PATH];
		::GetModuleFileName (gInstance, path, _MAX_PATH);
		EmFileRef::SetEmulatorRef (EmFileRef (path));

		// Load our preferences.

		prefs.Load ();

		// Start up the sub-systems.

		::PrvSubsystemStartup ();

#if 0
		{
			HANDLE	hProcess;				// handle to the process of interest
			DWORD	processAffinityMask;	// process affinity mask
			DWORD	systemAffinityMask; 	// system affinity mask

			GetProcessAffinityMask (hProcess = GetCurrentProcess(),
									&processAffinityMask,
									&systemAffinityMask);
			processAffinityMask &= systemAffinityMask;
			systemAffinityMask = 0x80000000;
			while (systemAffinityMask)
			{
				if (systemAffinityMask & processAffinityMask)
					break;
				systemAffinityMask >>= 1;
			}

			if (systemAffinityMask)
				SetProcessAffinityMask (hProcess, systemAffinityMask);
		}
#endif

		// When run under the VC++ debugger, the default directory appears
		// to be set to S:\emulator\windows\source, instead of the expected
		// \Debug or \Release sub-directories.	Jam the current directory
		// to the app directory so that we can find our files.

		::SetCurrentDirectory (EmDirRef::GetEmulatorDirectory ().GetFullPath ().c_str ());

		if (!::IsBound ())
			::PrvRegisterTypes ();

		// Check up on our start menu shortcut situation.

		::PrvProcessStartMenu();

		// Our main application loop merely loops between getting commands
		// and executing them.  Commands can come from one of two sources:
		// the user and the user.  Uh...by that I mean that...hmmm, let me
		// start again.
		//
		// The application normally starts up in a "non-running" state.
		// That is, it is not emulating a ROM.  In that state, it is
		// displaying a dialog asking the user what to do.	That dialog
		// allows the user to select from "New document", "Open document",
		// "Download ROM", and "Exit".
		//
		// After the user has started an emulation session, he can
		// choose to close that session or exit the emulator altogether
		// ("He can choose to close that session or exit the emulator").
		// He can also choose to create a new session or open a previously
		// saved one.

		Bool	result = Startup::DetermineStartupActions (__argc, __argv);
		if (!result)
			return 0;

		// Now loop, alternating between getting a command from the user
		// via the Startup dialog, and dispatching on that command.  Most
		// of the time, we just create the CPU and UI threads and let them
		// do the rest of the work.

		while (1)
		{
			ErrCode			error		= errNone;
			int 			operation	= 0;
			EmFileRef		psfRef;
			Configuration	cfg;

			// If we don't know what to do, ask the user.

			if (Startup::AskWhatToDo())
			{
				::PrvAskWhatToDo ();
			}

			// If it's time to quit, then quit.
			if (Startup::Quit())
			{
				break;
			}

			// If we need to create a new session, do it.
			else if (Startup::CreateSession(cfg))
			{
				error = ::CreateNewDocument (cfg);
				operation = kStr_CmdNew;
			}

			// If we need to load an old session from file, do it.
			else if (Startup::OpenSession(psfRef))
			{
				if (::IsBoundFully ())
					error = ::OpenResourceDocument ();
				else
					error = ::OpenOldDocument (psfRef);

				operation = kStr_CmdOpen;
			}
			else if (Startup::AskWhatToDo ())
			{
				continue;
			}
			else
			{
				EmAssert (false);
			}

			// If we need to fire up the UI thread, do it.

			if (!error)
			{
				::DoUIThread ();
			}

			// If there was some sort of error, report it.
			// The go back to asking the user what they want to do.

			if (error)
			{
				Errors::ReportIfError (operation, error, 0, false);
				Startup::ScheduleAskWhatToDo();
			}
		}
	}
	catch (...)
	{
		::MessageBox (NULL, "Unhandled exception caught in WinMain.\nDying like a log...",
			"Fatal Error", MB_OK);
	}

	RPC::SignalWaiters (hostSignalQuit);

	::PrvSubsystemShutdown ();

	prefs.Save ();

	return
		gErrorHappened ? 2 :
		gWarningHappened ? 1 : 0;
}
