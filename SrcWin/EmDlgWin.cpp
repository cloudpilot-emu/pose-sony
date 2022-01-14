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
#include "EmDlgWin.h"

#include "EmApplicationWin.h"	// gInstance
#include "EmFileRefWin.h"		// kExtension
#include "EmWindowWin.h"		// gHostWindow, GetHwnd
#include "ErrorHandling.h"		// Errors::SetParameter
#include "Miscellaneous.h"		// ReplaceString
#include "Platform.h"			// Platform::GetString
#include "Strings.r.h"			// kStr_AppName

#include <shlobj.h>
#include <tchar.h>				// _tcsinc, etc.

#include "resource.h"			// IDD_ROM_TRANSFER_QUERY, IDD_ROM_TRANSFER_PROGRESS, etc.

typedef char* POSITION;
typedef vector<char>	FilterList;

static UINT				PrvFromDlgID (EmDlgID id);
static EmDlgID			PrvToDlgID (UINT id);
static UINT				PrvFromDlgItemID (EmDlgItemID id);
static EmDlgItemID		PrvToDlgItemID (UINT id);

static FilterList		PrvConvertTypeList (const EmFileTypeList& typeList);
static void				PrvAddFilter (FilterList&, const char*, const char*);
static string			PrvGetExtension (EmFileType type);

#ifdef use_buffer_resizing_code
static LRESULT CALLBACK	PrvOpenMultiFileHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static UINT CALLBACK	PrvOpenMultiFileProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
static string			PrvGetNextPathName (OPENFILENAME& ofn, POSITION& pos);
static POSITION			PrvGetStartPosition (OPENFILENAME& ofn);

static Bool				PrvIsPushButton (HWND hWnd);
static Bool				PrvIsCheckBox (HWND hWnd);
static Bool				PrvIsRadioButton (HWND hWnd);
static Bool				PrvIsProgressBar (HWND hWnd);
static Bool				PrvIsButton (HWND hWnd);
static Bool				PrvIsComboBox (HWND hWnd);
static Bool				PrvIsListBox (HWND hWnd);
static Bool				PrvIsEdit (HWND hWnd);
static Bool				PrvIsStatic (HWND hWnd);
static Bool				PrvIsPopupMenuButton (HWND hWnd);
static Bool				PrvIsClass (HWND hWnd, const char* className);

static WNDPROC			gDlgProc;
HWND					gDlgCurrent;


// Array for mapping between MSDEV Resource Editor-assigned resource IDs
// and our own internal, cross-platform IDs.

#define ID__UNMAPPED		((UINT) -1)

struct
{
	EmDlgID		fMyID;
	UINT		fWinID;
} kDlgIDMap[] =
{
	{ kDlgAboutBox,					ID__UNMAPPED },
	{ kDlgSessionNew,				IDD_SESSION_NEW },
	{ kDlgSessionSave,				ID__UNMAPPED },	// Handled by MessageBox, for now.
	{ kDlgSessionInfo,				IDD_SESSION_INFO },
	{ kDlgHordeNew,					IDD_HORDE_NEW },
	{ kDlgDatabaseImport,			IDD_DATABASE_IMPORT },
	{ kDlgDatabaseExport,			IDD_DATABASE_EXPORT },
	{ kDlgROMTransferQuery,			IDD_ROM_TRANSFER_QUERY },
	{ kDlgROMTransferProgress,		IDD_ROM_TRANSFER_PROGRESS },
	{ kDlgGremlinControl,			IDD_GREMLIN_CONTROL },
	{ kDlgEditPreferences,			IDD_EDIT_PREFERENCES },
	{ kDlgEditLogging,				IDD_EDIT_LOGGING },
	{ kDlgEditDebugging,			IDD_EDIT_DEBUGGING },
	{ kDlgEditSkins,				IDD_EDIT_SKINS },
	{ kDlgCommonDialog,				IDD_COMMON_DIALOG },
	{ kDlgSaveBound,				IDD_SAVE_BOUND },
	{ kDlgEditHostFS,				IDD_EDIT_HOSTFS },
	{ kDlgEditBreakpoints,			IDD_EDIT_BREAKPOINTS },
	{ kDlgEditCodeBreakpoint,		IDD_EDIT_CODEBREAKPOINT },
	{ kDlgEditTracingOptions,		IDD_EDIT_TRACING },
	{ kDlgEditPreferencesFullyBound,IDD_EDIT_PREFERENCES_FULLY_BOUND },
	{ kDlgReset,					IDD_RESET },
	{ kDlgGetSocketAddress,			IDD_GET_SOCKET_ADDRESS },
	{ kDlgEditErrorHandling,		IDD_EDIT_ERROR_HANDLING },
	{ kDlgMinimizeProgress,			IDD_MINIMIZING },

	{ kDlgNone,						ID__UNMAPPED }
};

struct
{
	EmDlgItemID	fMyID;
	UINT		fWinID;
} kDlgItemIDMap[] =
{
	{ kDlgItemNone,					0 },

	{ kDlgItemOK,					IDOK },
	{ kDlgItemCancel,				IDCANCEL },

	{ kDlgItemAbtAppName,			ID__UNMAPPED },
	{ kDlgItemAbtURLPalmWeb,		ID__UNMAPPED },
	{ kDlgItemAbtURLPalmMail,		ID__UNMAPPED },
	{ kDlgItemAbtURLWindowsWeb,		ID__UNMAPPED },
	{ kDlgItemAbtURLWindowsMail,	ID__UNMAPPED },
	{ kDlgItemAbtURLMacWeb,			ID__UNMAPPED },
	{ kDlgItemAbtURLMacMail,		ID__UNMAPPED },
	{ kDlgItemAbtURLUAEWeb,			ID__UNMAPPED },
	{ kDlgItemAbtURLUAEMail,		ID__UNMAPPED },
	{ kDlgItemAbtURLQNXWeb,			ID__UNMAPPED },
	{ kDlgItemAbtURLQNXMail,		ID__UNMAPPED },

	{ kDlgItemNewDevice,			IDC_DEVICE },
	{ kDlgItemNewSkin,				IDC_SKIN },
	{ kDlgItemNewMemorySize,		IDC_RAMSIZE },
	{ kDlgItemNewROM,				IDC_ROMFILE },
#ifdef SONY_ROM
	{ kDlgItemNewMSSize,			IDC_MSSIZE },
#endif //SONY_ROM

	{ kDlgItemHrdAppList,			IDC_HORDE_APP_LIST },
	{ kDlgItemHrdStart,				IDC_HORDE_START_NUMBER },
	{ kDlgItemHrdStop,				IDC_HORDE_STOP_NUMBER },
	{ kDlgItemHrdCheckSwitch,		IDC_HORDE_SWITCH_CHECK },
	{ kDlgItemHrdCheckSave,			IDC_HORDE_SAVE_CHECK },
	{ kDlgItemHrdCheckStop,			IDC_HORDE_STOP_CHECK },
	{ kDlgItemHrdDepthSwitch,		IDC_HORDE_SWITCH_EVENTS },
	{ kDlgItemHrdDepthSave,			IDC_HORDE_SAVE_EVENTS },
	{ kDlgItemHrdDepthStop,			IDC_HORDE_STOP_EVENTS },
	{ kDlgItemHrdLogging,			IDC_HORDE_LOGGING },
	{ kDlgItemHrdFirstLaunchedApp,	IDC_FIRST_APP_LAUNCHED },
	{ kDlgItemHrdSelectAll,			IDC_SELECT_ALL },
	{ kDlgItemHrdSelectNone,		IDC_SELECT_NONE },

	{ kDlgItemImpNumFiles,			IDC_NUM_FILES },
	{ kDlgItemImpProgress,			IDC_PROGRESS_IMPORT },

	{ kDlgItemExpDbList,			IDC_DATABASE_LIST },

	{ kDlgItemDlqInstructions,		IDC_INSTRUCTIONS },
	{ kDlgItemDlqPortList,			IDC_PORT_LIST },
	{ kDlgItemDlqBaudList,			IDC_SPEED_LIST },

	{ kDlgItemDlpMessage,			IDC_TRANSFER_MESSAGE },
	{ kDlgItemDlpProgress,			IDC_PROGRESS_DOWNLOAD },

	{ kDlgItemGrmNumber,			IDC_GREMLIN_NUMBER },
	{ kDlgItemGrmEventNumber,		IDC_EVENT_NUMBER },
	{ kDlgItemGrmElapsedTime,		IDC_ELAPSED_TIME },
	{ kDlgItemGrmStop,				IDC_STOP },
	{ kDlgItemGrmResume,			IDC_RESUME },
	{ kDlgItemGrmStep,				IDC_STEP },

	{ kDlgItemPrfRedirectSerial,	IDC_PREF_REDIRECT_SERIAL },
	{ kDlgItemPrfRedirectIR,		IDC_PREF_REDIRECT_IR },
	{ kDlgItemPrfRedirectMystery,	IDC_PREF_REDIRECT_MYSTERY },
	{ kDlgItemPrfRedirectNetLib,	IDC_PREF_REDIRECT_NETLIB },
	{ kDlgItemPrfEnableSound,		IDC_PREF_ENABLE_SOUND },
	{ kDlgItemPrfSaveAlways,		IDC_PREF_SAVE_ALWAYS },
	{ kDlgItemPrfSaveAsk,			IDC_PREF_SAVE_ASK },
	{ kDlgItemPrfSaveNever,			IDC_PREF_SAVE_NEVER },
	{ kDlgItemPrfUserName,			IDC_PREF_USER_NAME },

	{ kDlgItemLogNormal,			IDC_LOG_NORMAL },
	{ kDlgItemLogGremlins,			IDC_LOG_GREMLINS },
#undef DEFINE_BUTTON_ID_MAPPING
#define DEFINE_BUTTON_ID_MAPPING(name)	\
	{ kDlgItemLog##name,			IDC_##name },
	FOR_EACH_LOG_PREF(DEFINE_BUTTON_ID_MAPPING)

	{ kDlgItemDbgDialogBeep,		IDC_DBG_DIALOG_BEEP },
#undef DEFINE_BUTTON_ID_MAPPING
#define DEFINE_BUTTON_ID_MAPPING(name)	\
	{ kDlgItemDbg##name,			IDC_##name },
	FOR_EACH_REPORT_PREF(DEFINE_BUTTON_ID_MAPPING)

	{ kDlgItemSknSkinList,			IDC_SKIN_SKIN_LIST },
	{ kDlgItemSknDoubleScale,		IDC_SKIN_DOUBLE_SCALE },
	{ kDlgItemSknWhiteBackground,	IDC_SKIN_WHITE_BACKGROUND },
	{ kDlgItemSknDim,				IDC_SKIN_DIM },
	{ kDlgItemSknRed,				IDC_SKIN_RED },
	{ kDlgItemSknGreen,				IDC_SKIN_GREEN },
	{ kDlgItemSknStayOnTop,			IDC_SKIN_STAY_ON_TOP },

	{ kDlgItemCmnText,				IDC_MESSAGE },
	{ kDlgItemCmnIconStop,			ID__UNMAPPED },
	{ kDlgItemCmnIconCaution,		ID__UNMAPPED },
	{ kDlgItemCmnIconNote,			ID__UNMAPPED },
	{ kDlgItemCmnButton1,			IDC_BUTTON1 },
	{ kDlgItemCmnButton2,			IDC_BUTTON2 },
	{ kDlgItemCmnButton3,			IDC_BUTTON3 },

	{ kDlgItemBndSaveROM,			IDC_BIND_ROM },
	{ kDlgItemBndSaveRAM,			IDC_BIND_RAM },

	{ kDlgItemHfsList,				IDC_HOSTFS_LIST },
	{ kDlgItemHfsPath,				IDC_HOSTFS_PATH },
	{ kDlgItemHfsMounted,			IDC_HOSTFS_MOUNTED },
	{ kDlgItemHfsBrowse,			IDC_HOSTFS_BROWSE },

	{ kDlgItemBrkList,				IDC_BREAKPOINT_LIST },
	{ kDlgItemBrkButtonEdit,		IDC_BREAKPOINT_EDIT },
	{ kDlgItemBrkButtonClear,		IDC_BREAKPOINT_CLEAR },
	{ kDlgItemBrkCheckEnabled,		IDC_BREAKPOINT_ENABLED },
	{ kDlgItemBrkStartAddress,		IDC_BREAKPOINT_STARTADDRESS },
	{ kDlgItemBrkNumberOfBytes,		IDC_BREAKPOINT_NUMBEROFBYTES },

	{ kDlgItemBrkAddress,			IDC_CODEBREAKPOINT_ADDRESS },
	{ kDlgItemBrkCondition,			IDC_CODEBREAKPOINT_CONDITION },

	{ kDlgItemTrcOutput,			IDC_TRACER_TYPE },
	{ kDlgItemTrcTargetText,		IDC_TRACER_PARAM_DESCR },
	{ kDlgItemTrcTargetValue,		IDC_TRACER_PARAM },
	{ kDlgItemTrcInfo,				IDC_TRACER_LIBSTATUS },
	{ kDlgItemTrcAutoConnect,		IDC_TRACER_AUTOCONNECT },

	{ kDlgItemRstSoft,				IDC_RESET_SOFT },
	{ kDlgItemRstHard,				IDC_RESET_HARD },
	{ kDlgItemRstDebug,				IDC_RESET_DEBUG },
	{ kDlgItemRstNoExt,				IDC_RESET_NO_EXTENSIONS },

	{ kDlgItemInfDeviceFld,			IDC_DEVICE_FIELD },
	{ kDlgItemInfRAMFld,			IDC_RAM_FIELD },
	{ kDlgItemInfROMFld,			IDC_ROM_FIELD },
	{ kDlgItemInfSessionFld,		IDC_SESSION_FIELD },

	{ kDlgItemSocketAddress,		IDC_SOCKET_ADDRESS },

	{ kDlgItemErrWarningOff,		IDC_ERROR_WARNING_OFF },
	{ kDlgItemErrErrorOff,			IDC_ERROR_ERROR_OFF },
	{ kDlgItemErrWarningOn,			IDC_ERROR_WARNING_ON },
	{ kDlgItemErrErrorOn,			IDC_ERROR_ERROR_ON },

	{ kDlgItemMinPassNumber,		IDC_MIN_PASS_NUMBER },
	{ kDlgItemMinEventNumber,		IDC_MIN_EVENT_NUMBER },
	{ kDlgItemMinElapsed,			IDC_MIN_ELAPSED_TIME },
	{ kDlgItemMinRange,				IDC_MIN_RANGE },
	{ kDlgItemMinDiscarded,			IDC_MIN_DISCARDED },
	{ kDlgItemMinProgress,			IDC_MIN_PROGRESS },

	{ kDlgItemLast,					ID__UNMAPPED }
};


// Control our modal state by hand.  We have to take control from
// Windows for several reasons:
//
//	*	Our window is non-standard.  In particular, it is not a
//		large frame window with lots of contents.
//
//	*	Windows seems to assume that the parent window of the
//		"Get File" and "Put File" dialogs are large frame windows,
//		and will resize those dialogs to fit within that parent
//		window.  For Poser, this often leads to those dialogs
//		being chopped off at the bottom.
//
//	*	Thus, our "Get File" and "Put File" dialogs are NOT
//		children of our main window.
//
//	->	Therefore, in order to disable our main window, we need
//		to handle that by hand.  We do this with EmModalState.
//
//	*	Additionally, our main window can also be an "always on top"
//		window, meaning that it floats over all other windows
//		that are not also "always on top" windows, a category
//		which includes our "Get File" and "Put File" dialogs.
//
//	->	Therefore, our dialogs need to be forced into the same window
//		layer as our main window.  This can be done by temporarily
//		forcing our main window into the normal application layer,
//		or by forcing our dialogs into the same layer as the
//		main window.  The documentation for SetWindowPos says that
//		we can achieve either of these regardless of the "topmost"
//		state of the main window by calling it to move the window
//		in front of the main window.  We do this in the same place
//		where we center our dialogs.
//
//	->	With these changes, I'm seeing something incorrect.  If
//		I am running a session, bring up the "New Session" dialog,
//		bring up the "pick a ROM" dialog, and then dismiss that
//		dialog, the "New Session" dialog remains in the background.
//		I have no idea if this is supposed to happen, since my PC
//		seems to be messed up with regards to keeping track of
//		the active window.  Tt has this problem with *all* applications,
//		so I have reason to believe that the problem is with Windows.
//		However, it's really annoying, so I'm adding a call to
//		SetForegroundWindow, which appears to address the problem.

#define DRIVE_BY_HAND 0

#if DRIVE_BY_HAND

	class EmModalState
	{
	public:
		EmModalState (void) :
		  fWnd (::GetActiveWindow ())
		{
			fOldState = ::IsWindowEnabled (fWnd);
			::EnableWindow (fWnd, false);
		}

		~EmModalState (void)
		{
			::EnableWindow (fWnd, fOldState);
			::SetForegroundWindow (fWnd);
		}

		HWND GetParent (void) const { return NULL; }

	private:
		HWND fWnd;
		BOOL fOldState;
	};

#else

	class EmModalState
	{
	public:
		EmModalState (void) :
		  fWnd (::GetActiveWindow ())
		{
		}

		~EmModalState (void)
		{
		}

		HWND GetParent (void) const { return fWnd; }

	private:
		HWND fWnd;
	};

#endif


static void PrvDrainEvents (void)
{
	MSG msg;
	while (::PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) 
	{
		HWND	dlg = ::GetCurrentModelessDialog ();

		if (dlg == NULL || !::IsDialogMessage (dlg, &msg))
		{
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::HostRunGetFile
 *
 * DESCRIPTION:	Platform-specific routine for getting the name of a
 *				file from the user.  This file name is used to load
 *				the file.
 *
 * PARAMETERS:	typeList - types of files user is allowed to select.
 *				ref - selected file is returned here. Valid only if
 *					function result is kDlgItemOK.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

EmDlgItemID EmDlg::HostRunGetFile (const void* parameters)
{
	EmModalState	modalState;

	EmAssert (parameters);
	DoGetFileParameters&	data = *(DoGetFileParameters*) parameters;

	FilterList		filters = ::PrvConvertTypeList (data.fFilterList);

	char			fileName[MAX_PATH] = {0};

	string			initialDir;

	if (data.fDefaultPath.IsSpecified ())
		initialDir = data.fDefaultPath.GetFullPath ();

	OPENFILENAME	reply;
	memset (&reply, 0, sizeof(reply));

	reply.lStructSize		= sizeof (OPENFILENAME);
	reply.hwndOwner 		= modalState.GetParent ();
	reply.hInstance 		= gInstance;
	reply.lpstrFilter		= &filters[0];
	reply.nFilterIndex		= 1;
	reply.lpstrFile 		= fileName;
	reply.nMaxFile			= MAX_PATH;
	reply.lpstrInitialDir	= initialDir.c_str ();
	reply.lpstrTitle		= data.fPrompt.empty() ? NULL : data.fPrompt.c_str ();
	reply.Flags 			= OFN_ENABLEHOOK | OFN_EXPLORER | OFN_FILEMUSTEXIST;
	reply.lpfnHook			= &::CenterDlgProc;

	BOOL	ok = ::GetOpenFileName (&reply);

	if (ok)
	{
		data.fResult = EmFileRef (fileName);
	}

	return ok ? kDlgItemOK : kDlgItemCancel;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::HostRunGetFileList
 *
 * DESCRIPTION:	Platform-specific routine for getting the name of a
 *				file from the user.  This file name is used to load
 *				the file.
 *
 * PARAMETERS:	typeList - types of files user is allowed to select.
 *				ref - selected file is returned here. Valid only if
 *					function result is kDlgItemOK.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

EmDlgItemID EmDlg::HostRunGetFileList (const void* parameters)
{
	EmModalState	modalState;

	EmAssert (parameters);
	DoGetFileListParameters&	data = *(DoGetFileListParameters*) parameters;

	FilterList		filters = ::PrvConvertTypeList (data.fFilterList);

	// When querying for multiple files, Windows stores the entire list
	// of files in the buffer pointed to by lpstrFile, where the size
	// of the buffer is stored in nMaxFile.  However, there's no guarantee
	// that this buffer will be large enough to hold any list of files that
	// the user may choose.  Therefore, MSDN includes a technique for
	// resizing the buffer (see KB: Q131462).  Sadly, this technique appears
	// to not work.  Calling CommDlg_OpenSave_GetFilePath to get the size
	// of the buffer needed to hold the entire return value fails if the
	// required size is greater than the value in nMaxFile (it returns -1).
	// Therefore, I'll just create a 10K buffer, and hope for the best.

	const int		kFileBufferSize = 10*1024L;

	string			initialDir;

	if (data.fDefaultPath.IsSpecified ())
		initialDir = data.fDefaultPath.GetFullPath ();

	OPENFILENAME	reply;
	memset (&reply, 0, sizeof(reply));

	reply.lStructSize		= sizeof (OPENFILENAME);
	reply.hwndOwner 		= modalState.GetParent ();
	reply.hInstance 		= gInstance;
	reply.lpstrFilter		= &filters[0];
	reply.nFilterIndex		= 1;
	reply.lpstrFile 		= (LPTSTR) malloc (kFileBufferSize);
	reply.nMaxFile			= kFileBufferSize;
	reply.lpstrInitialDir	= initialDir.c_str ();
	reply.lpstrTitle		= data.fPrompt.empty() ? NULL : data.fPrompt.c_str ();
	reply.Flags 			= OFN_ENABLEHOOK | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
#ifdef use_buffer_resizing_code
	reply.lpfnHook			= &::PrvOpenMultiFileProc;
#else
	reply.lpfnHook			= &::CenterDlgProc;
#endif

	reply.lpstrFile[0] = 0;

	BOOL	ok = ::GetOpenFileName (&reply);

	if (ok)
	{
		POSITION	pos = ::PrvGetStartPosition (reply);
		while (pos)
		{
			string	pathName = ::PrvGetNextPathName (reply, pos);
			data.fResults.push_back (EmFileRef (pathName));
		}
	}

	free (reply.lpstrFile);

	return ok ? kDlgItemOK : kDlgItemCancel;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::HostRunPutFile
 *
 * DESCRIPTION:	Platform-specific routine for getting the name of a
 *				file from the user.  This file name is used to save
 *				the file.
 *
 * PARAMETERS:	defName - default name for the file to be saved.
 *				ref - selected file is returned here. Valid only if
 *					function result is kDlgItemOK.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

EmDlgItemID EmDlg::HostRunPutFile (const void* parameters)
{
	EmModalState	modalState;

	EmAssert (parameters);
	DoPutFileParameters&	data = *(DoPutFileParameters*) parameters;

	// Create a Windows filter list, and make a guess at the default
	// extension by picking the first off the list.

	FilterList		filters = ::PrvConvertTypeList (data.fFilterList);
	string			defExt	= ::PrvGetExtension (data.fFilterList[0]);

	// Get the default file name.

	char			fileName[MAX_PATH] = {0};

	if (!data.fDefaultName.empty ())
		strcpy (fileName, data.fDefaultName.c_str ());

	// Get the starting directory.

	string			initialDir;

	if (data.fDefaultPath.IsSpecified ())
		initialDir = data.fDefaultPath.GetFullPath ();

	// Fill out the OPENFILENAME parameter block for windows and
	// call GetSaveFileName.

	OPENFILENAME	reply;
	memset (&reply, 0, sizeof(reply));

	reply.lStructSize		= sizeof (OPENFILENAME);
	reply.hwndOwner 		= modalState.GetParent ();
	reply.hInstance 		= gInstance;
	reply.lpstrFilter		= &filters[0];
	reply.nFilterIndex		= 1;
	reply.lpstrFile 		= fileName;
	reply.nMaxFile			= MAX_PATH;
	reply.lpstrInitialDir	= initialDir.c_str ();
	reply.lpstrTitle		= data.fPrompt.empty() ? NULL : data.fPrompt.c_str ();
	reply.Flags 			= OFN_ENABLEHOOK | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
	reply.lpstrDefExt		= defExt.size () > 0 ? defExt.c_str () : NULL;
	reply.lpfnHook			= &::CenterDlgProc;

	BOOL	ok = ::GetSaveFileName (&reply);

	// If the user pressed OK, initialize our return value.

	if (ok)
	{
		data.fResult = EmFileRef (fileName);
	}

	// Return the EmDlgID of the button that the user used to
	// dismiss the dialog.

	return ok ? kDlgItemOK : kDlgItemCancel;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::HostRunGetDirectory
 *
 * DESCRIPTION:	Platform-specific routine for getting the name of a
 *				directory from the user.
 *
 * PARAMETERS:	result - chosen directory is returned in this.
 *				prompt - prompt to display in the dialog box.
 *				defaultPath - starting position.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

static int CALLBACK BrowseCallbackProc(
	HWND hwnd, 
	UINT uMsg, 
	LPARAM lParam, 
	LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		EmDirRef*	defaultPath = (EmDirRef*) lpData;
		if (defaultPath->Exists ())
		{
			string		fullPath = defaultPath->GetFullPath ();
			fullPath.erase (fullPath.end() - 1);	// Strip off the trailing slash
													// or BFFM_SETSELECTION fails.
			SendMessage (hwnd, BFFM_SETSELECTION, (WPARAM) true, (LPARAM) fullPath.c_str());
		}
	}

	return 0;
}


EmDlgItemID EmDlg::HostRunGetDirectory (const void* parameters)
{
	EmModalState	modalState;

	EmAssert (parameters);
	DoGetDirectoryParameters&	data = *(DoGetDirectoryParameters*) parameters;

	EmDlgItemID	result = kDlgItemCancel;

	::CoInitialize (NULL);

	LPMALLOC	malloc;
	
	if (::SHGetMalloc (&malloc) == S_OK)
	{
		char			szTemp[MAX_PATH];
		BROWSEINFO		brInfo;

		brInfo.hwndOwner		= modalState.GetParent ();
		brInfo.pidlRoot			= NULL;
		brInfo.pszDisplayName	= szTemp;	// Receives display name
		brInfo.lpszTitle		= data.fPrompt.c_str ();
		brInfo.ulFlags			= BIF_RETURNONLYFSDIRS;
		brInfo.lpfn				= BrowseCallbackProc;
		brInfo.lParam			= (LPARAM) &data.fDefaultPath;

		// Use the shell's folder browser

		LPITEMIDLIST	pidlDestination = ::SHBrowseForFolder (&brInfo);
		
		if (pidlDestination != NULL)
		{
			// Get the path for the folder

			if (::SHGetPathFromIDList (pidlDestination, szTemp))
			{
				data.fResult = szTemp;
				result = kDlgItemOK;
			}

			malloc->Free (pidlDestination);
		}

		malloc->Release ();
	}

	::CoUninitialize ();

	return result;
}

/***********************************************************************
 *
 * FUNCTION:	HostRunAboutBox
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

static BOOL CALLBACK PrvAboutDlgProc (	HWND hwnd,
										UINT msg,
										WPARAM wparam,
										LPARAM lparam)
{
	// Except in response to the WM_INITDIALOG message, the dialog box procedure
	// should return nonzero if it processes the message, and zero if it does not.
	// In response to a WM_INITDIALOG message, the dialog box procedure should
	// return zero if it calls the SetFocus function to set the focus to one of
	// the controls in the dialog box. Otherwise, it should return nonzero, in
	// which case the system sets the focus to the first control in the dialog
	// box that can be given the focus.

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			HWND	hdlg = ::GetParent (hwnd);

			::CenterWindow (hdlg);

			Errors::SetParameter ("%application", Platform::GetString (kStr_AppName));
			Errors::SetParameter ("%version", Platform::GetShortVersionString ());

			string	appAndVersion = Errors::ReplaceParameters (kStr_AppAndVers);
			::SetDlgItemText (hwnd, IDC_ABOUT, appAndVersion.c_str ());

			// Hide the Cancel button and move the OK button over.

			HWND	cancelButton	= ::GetDlgItem (hdlg, IDCANCEL);
			HWND	okButton		= ::GetDlgItem (hdlg, IDOK);

			::ShowWindow (cancelButton, SW_HIDE);

			RECT	cancelRect, okRect;
			::GetWindowRect (cancelButton, &cancelRect);
			::GetWindowRect (okButton, &okRect);

			long	shift = cancelRect.right - okRect.right;

			::ScreenToClient (hdlg, (POINT*) &okRect.left);
			::ScreenToClient (hdlg, (POINT*) &okRect.right);

			::MoveWindow (okButton, okRect.left + shift, okRect.top,
						okRect.right - okRect.left, okRect.bottom - okRect.top, true);

			// Subclass the URLs so that they act like hyperlinks.

			new StaticHyperlink (::GetDlgItem (hwnd, IDC_HTTP));
			new StaticHyperlink (::GetDlgItem (hwnd, IDC_MAILTO));
			if (::GetDlgItem (hwnd, IDC_HTTP2) != NULL)
				new StaticHyperlink (::GetDlgItem (hwnd, IDC_HTTP2));

			return TRUE;
		}

		case WM_CTLCOLORSTATIC:
		{
			// Reflect this one back down to the child.

			LONG		wnd = ::GetWindowLong ((HWND) lparam, GWL_USERDATA);
			if (wnd)
				return (BOOL) ::SendMessage ((HWND) lparam, msg, wparam, lparam);

			return FALSE;
		}
	}

	return FALSE;
}


EmDlgItemID	EmDlg::HostRunAboutBox (const void* parameters)
{
	UNUSED_PARAM (parameters);

	PROPSHEETPAGE	psp[4];
	::ZeroMemory (psp, sizeof (psp));

	psp[0].dwSize		= sizeof (PROPSHEETPAGE);
	psp[0].dwFlags		= PSP_DEFAULT;
	psp[0].hInstance	= gInstance;
	psp[0].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_PALM);
	psp[0].pfnDlgProc	= PrvAboutDlgProc;

	psp[1].dwSize		= sizeof (PROPSHEETPAGE);
	psp[1].dwFlags		= PSP_DEFAULT;
	psp[1].hInstance	= gInstance;
	psp[1].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_WINDOWS);
	psp[1].pfnDlgProc	= PrvAboutDlgProc;

	psp[2].dwSize		= sizeof (PROPSHEETPAGE);
	psp[2].dwFlags		= PSP_DEFAULT;
	psp[2].hInstance	= gInstance;
	psp[2].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_MAC);
	psp[2].pfnDlgProc	= PrvAboutDlgProc;

	psp[3].dwSize		= sizeof (PROPSHEETPAGE);
	psp[3].dwFlags		= PSP_DEFAULT;
	psp[3].hInstance	= gInstance;
	psp[3].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_UAE);
	psp[3].pfnDlgProc	= PrvAboutDlgProc;

	char	aboutApp[200];
	string	about (Platform::GetString (IDS_ABOUT));
	string	app (Platform::GetString (kStr_AppName));
	sprintf (aboutApp, about.c_str (), app.c_str ());

	PROPSHEETHEADER psh;
	::ZeroMemory (&psh, sizeof (psh));

	psh.dwSize			= sizeof (psh);
	psh.dwFlags 		= PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent		= ::GetActiveWindow ();
	psh.hInstance		= gInstance;
	psh.pszCaption		= aboutApp;
	psh.nPages			= 4;
	psh.ppsp			= psp;

	::PropertySheet (&psh);

	return kDlgItemOK;
}


/***********************************************************************
 *
 * FUNCTION:	HostRunSessionSave
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgItemID	EmDlg::HostRunSessionSave (const void* parameters)
{
	EmModalState	modalState;

	EmAssert (parameters);
	DoSessionSaveParameters&	data = *(DoSessionSaveParameters*) parameters;

	char	buffer[300];
	string	saveChanges (Platform::GetString (IDS_SAVECHANGES));

	sprintf (buffer, saveChanges.c_str (), data.fDocName.c_str ());

	int result = ::MessageBox (modalState.GetParent (), buffer, data.fAppName.c_str (),
					MB_APPLMODAL | MB_ICONQUESTION | MB_YESNOCANCEL);

	if (result == IDYES)
		return kDlgItemYes;

	else if (result == IDNO)
		return kDlgItemNo;

	return kDlgItemCancel;
}


/***********************************************************************
 *
 * FUNCTION:	HostRunDialog
 *
 * DESCRIPTION:	Common routine that handles the creation of a dialog,
 *				initializes it (via the dialog handler), fetches events,
 *				handles events (via the dialog handler), and closes
 *				the dialog.
 *
 * PARAMETERS:	fn - the custom dialog handler
 *				userData - custom data passed back to the dialog handler.
 *				dlgID - ID of dialog to create.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

BOOL CALLBACK EmDlg::PrvHostModalProc (	HWND hWnd,
										UINT msg,
										WPARAM wParam,
										LPARAM lParam)
{
#if defined (_DEBUG) && defined (TRACE_MSG)
	MSG	_msg;

	_msg.hwnd = hWnd;
	_msg.lParam = lParam;
	_msg.message = msg;
	_msg.wParam = wParam;

	_AfxTraceMsg ("PrvHostModalProc", &_msg);
#endif

	// Except in response to the WM_INITDIALOG message, the dialog box procedure
	// should return nonzero if it processes the message, and zero if it does not.
	// In response to a WM_INITDIALOG message, the dialog box procedure should
	// return zero if it calls the SetFocus function to set the focus to one of
	// the controls in the dialog box. Otherwise, it should return nonzero, in
	// which case the system sets the focus to the first control in the dialog
	// box that can be given the focus.

	// Remember our custom data for later.

	if (msg == WM_INITDIALOG)
	{
		::SetWindowLong (hWnd, GWL_USERDATA, lParam);
	}

	EmDlgContext&	context = *(EmDlgContext*) ::GetWindowLong (hWnd, GWL_USERDATA);

	try
	{
		switch (msg)
		{
			case WM_INITDIALOG:
			{
				context.fDlg = (EmDlgRef) hWnd;

				// Center the window.

				::CenterWindow (hWnd);
				::SetWindowPos (hWnd, ::GetActiveWindow (), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

				// Perform any host-specific initialization.

				EmDlg::HostDialogInit (context);

				// Call the user's Init function.

				if (context.Init () == kDlgResultClose)
				{
					::EndDialog (hWnd, kDlgItemNone);
					return TRUE;
				}

				// The dialog box procedure should return TRUE to direct the
				// system to set the keyboard focus to the control specified
				// by wParam. Otherwise, it should return FALSE to prevent
				// the system from setting the default keyboard focus.

				return TRUE;
			}

			case WM_COMMAND:
			{
				UINT		wItemID	= LOWORD (wParam);
				EmDlgItemID	itemID	= ::PrvToDlgItemID (wItemID);

				if (wItemID == IDOK && context.fDefaultItem != kDlgItemNone)
				{
					itemID = context.fDefaultItem;
				}
				else if (wItemID == IDCANCEL && context.fCancelItem != kDlgItemNone)
				{
					itemID = context.fCancelItem;
				}

				if (context.Event (itemID) == kDlgResultClose)
				{
					::EndDialog (hWnd, itemID);
				}

				return TRUE;
			}

			case WM_CHAR:
			{
				if (wParam == 3)	// Ctrl-C
				{
					EmDlgItemID	itemID	= kDlgItemCmnButtonCopy;

					if (context.Event (itemID) == kDlgResultClose)
					{
						::EndDialog (hWnd, itemID);
					}

					return TRUE;
				}

				break;
			}

			case WM_TIMER:
			{
				if (context.Idle () == kDlgResultClose)
				{
					::EndDialog (hWnd, kDlgItemOK);
				}

				return TRUE;
			}

			case WM_DESTROY:
			{
				context.Destroy ();
				EmDlg::HostStopIdling (context);
				return FALSE;
			}
		}
	}
	catch (...)
	{
		EmAssert (false);
		::EndDialog (hWnd, IDCANCEL);
		return TRUE;
	}

	// Typically, the dialog box procedure should return TRUE if it
	// processed the message, and FALSE if it did not. If the dialog
	// box procedure returns FALSE, the dialog manager performs the
	// default dialog operation in response to the message.

	return FALSE;
}


EmDlgItemID EmDlg::HostRunDialog (const void* parameters)
{
	EmModalState	modalState;

	EmAssert (parameters);
	RunDialogParameters&	data = *(RunDialogParameters*) parameters;

	EmDlgContext			context;

	context.fFn				= data.fFn;
	context.fDlgID			= data.fDlgID;
	context.fUserData		= data.fUserData;

	UINT	windowsDlgID	= ::PrvFromDlgID (context.fDlgID);
	EmAssert (windowsDlgID != ID__UNMAPPED);

	// Insert a small loop here to drain any pending messages.  There
	// appears to be a circumstance where this is required.  If Poser
	// closes its main window, creates a new one, and then tries to
	// show a dialog (this situation can occur when opening a new
	// event file for Minimization and there's no error record in the
	// event stream), then the dialog destroys itself just before it's
	// shown.  Examining the messages sent to windows during successful
	// and failed window and dialog creation situations seemed to
	// indicate that the problem had to do with the newly created main
	// window not being redrawn.  Or something.  I really don't know.
	// But inserting this loop to handle any pending events fixes the
	// problem, and the dialog successfully shows up.

	::PrvDrainEvents ();

	int choice = ::DialogBoxParam (
		gInstance,
		MAKEINTRESOURCE (windowsDlgID),
		modalState.GetParent (),
		&PrvHostModalProc,
		(LPARAM) &context);

	return (EmDlgItemID) choice;
}


/***********************************************************************
 *
 * FUNCTION:	HostDialogOpen
 *
 * DESCRIPTION:	Common routine that handles the creation of a dialog,
 *				initializes it (via the dialog handler), fetches events,
 *				handles events (via the dialog handler), and closes
 *				the dialog.
 *
 * PARAMETERS:	fn - the custom dialog handler
 *				userData - custom data passed back to the dialog handler.
 *				dlgID - ID of dialog to create.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

BOOL CALLBACK EmDlg::PrvHostModelessProc (	HWND hWnd,
											UINT msg,
											WPARAM wParam,
											LPARAM lParam)
{
	// Except in response to the WM_INITDIALOG message, the dialog box procedure
	// should return nonzero if it processes the message, and zero if it does not.
	// In response to a WM_INITDIALOG message, the dialog box procedure should
	// return zero if it calls the SetFocus function to set the focus to one of
	// the controls in the dialog box. Otherwise, it should return nonzero, in
	// which case the system sets the focus to the first control in the dialog
	// box that can be given the focus.

	// Remember our custom data for later.

	if (msg == WM_INITDIALOG)
	{
		::SetWindowLong (hWnd, GWL_USERDATA, lParam);
	}

	EmDlgContext&	context = *(EmDlgContext*) ::GetWindowLong (hWnd, GWL_USERDATA);

	try
	{
		switch (msg)
		{
			case WM_INITDIALOG:
			{
				context.fDlg = (EmDlgRef) hWnd;

				// Center the window.

				::CenterWindow (hWnd);
				::SetWindowPos (hWnd, ::GetActiveWindow (), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

				// Perform any host-specific initialization.

				EmDlg::HostDialogInit (context);

				// Call the user's Init function.

				if (context.Init () == kDlgResultClose)
				{
					::DestroyWindow (hWnd);
					return TRUE;
				}

				// Show the window after it's been set up.

				::ShowWindow (hWnd, SW_SHOW);

				// The dialog box procedure should return TRUE to direct the
				// system to set the keyboard focus to the control specified
				// by wParam. Otherwise, it should return FALSE to prevent
				// the system from setting the default keyboard focus.

				return TRUE;
			}

			case WM_COMMAND:
			{
				UINT		wItemID	= LOWORD (wParam);
				EmDlgItemID	itemID	= ::PrvToDlgItemID (wItemID);

				if (wItemID == IDOK && context.fDefaultItem != kDlgItemNone)
				{
					itemID = context.fDefaultItem;
				}
				else if (wItemID == IDCANCEL && context.fCancelItem != kDlgItemNone)
				{
					itemID = context.fCancelItem;
				}

				if (context.Event (itemID) == kDlgResultClose)
				{
					::DestroyWindow (hWnd);
				}

				return TRUE;
			}

			case WM_TIMER:
			{
				if (context.Idle () == kDlgResultClose)
				{
					::DestroyWindow (hWnd);
				}

				return TRUE;
			}

			case WM_DESTROY:
			{
				context.Destroy ();

				EmDlg::HostStopIdling (context);

				// Clean up the context.

				EmDlgContext*	ctxPtr = &context;
				delete ctxPtr;

				::SetWindowLong (hWnd, GWL_USERDATA, 0);

				return FALSE;
			}

			case WM_ACTIVATE:
			{
				// Knowledgebase article Q71450 -- "HOWTO: Use One
				// IsDialogMessage() Call for Many Modeless Dialogs"
				// -- suggests keeping track of the active modeless
				// dialog in order to avoid having to call
				// IsDialogMessage for *every* open modeless dialog.

				if (0 == wParam)				// becoming inactive
					gDlgCurrent = NULL;
				else							// becoming active
					gDlgCurrent = hWnd;

				return FALSE;
			}
		}
	}
	catch (...)
	{
		EmAssert (false);
		::DestroyWindow (hWnd);
		return TRUE;
	}

	// Typically, the dialog box procedure should return TRUE if it
	// processed the message, and FALSE if it did not. If the dialog
	// box procedure returns FALSE, the dialog manager performs the
	// default dialog operation in response to the message.

	return FALSE;
}


/***********************************************************************
 *
 * FUNCTION:	HostDialogOpen
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	.
 *
 ***********************************************************************/

EmDlgRef EmDlg::HostDialogOpen (EmDlgFn fn, void* data, EmDlgID dlgID)
{
	EmDlgContext*			context = new EmDlgContext;

	if (!context)
		return NULL;

	context->fFn			= fn;
	context->fDlgID			= dlgID;
	context->fUserData		= data;

	EmAssert (gHostWindow);

	HWND	parent			= gHostWindow ? gHostWindow->GetHwnd () : NULL;
	UINT	windowsDlgID	= ::PrvFromDlgID (dlgID);
	EmAssert (windowsDlgID != ID__UNMAPPED);

	// Insert a small loop here to drain any pending messages.  There
	// appears to be a circumstance where this is required.  If Poser
	// closes its main window, creates a new one, and then tries to
	// show a dialog (this situation can occur when opening a new
	// event file for Minimization and there's no error record in the
	// event stream), then the dialog destroys itself just before it's
	// shown.  Examining the messages sent to windows during successful
	// and failed window and dialog creation situations seemed to
	// indicate that the problem had to do with the newly created main
	// window not being redrawn.  Or something.  I really don't know.
	// But inserting this loop to handle any pending events fixes the
	// problem, and the dialog successfully shows up.

	::PrvDrainEvents ();

	HWND	hDlg = ::CreateDialogParam (gInstance, MAKEINTRESOURCE (windowsDlgID),
										parent, &PrvHostModelessProc, (LPARAM) context);

	return (EmDlgRef) hDlg;
}


/***********************************************************************
 *
 * FUNCTION:	HostDialogClose
 *
 * DESCRIPTION:	Common routine that handles the creation of a dialog,
 *				initializes it (via the dialog handler), fetches events,
 *				handles events (via the dialog handler), and closes
 *				the dialog.
 *
 * PARAMETERS:	fn - the custom dialog handler
 *				userData - custom data passed back to the dialog handler.
 *				dlgID - ID of dialog to create.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/

void EmDlg::HostDialogClose (EmDlgRef dlg)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	// Post a close message so that closing a window works from any thread.
	// DestroyWindow will fail if we call it from a thread other than the
	// on that created the window.

//	::DestroyWindow (hDlg);
	::PostMessage (hDlg, WM_CLOSE, 0, 0);
}


/***********************************************************************
 *
 * FUNCTION:	HostDialogInit
 *
 * DESCRIPTION:	Perform any platform-specific initialization.
 *
 * PARAMETERS:	context - dialog context data.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmDlg::HostDialogInit (EmDlgContext& context)
{
	HWND	hWnd = (HWND) context.fDlg;

	// If this is the "New Session" window, subclass some of the buttons.

	if (context.fDlgID == kDlgSessionNew)
	{
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_DEVICE));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_SKIN));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_RAMSIZE));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_ROMFILE));
#ifdef SONY_ROM
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_MSSIZE));
#endif //SONY_ROM
	}

	// If this is the "ROM Transfer" window, subclass some of the buttons.

	else if (context.fDlgID == kDlgROMTransferQuery)
	{
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_PORT_LIST));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_SPEED_LIST));
	}

	// If this is the "Properties" window, subclass some of the buttons.

	else if (context.fDlgID == kDlgEditPreferences ||
		context.fDlgID == kDlgEditPreferencesFullyBound)
	{
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_PREF_REDIRECT_SERIAL));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_PREF_REDIRECT_IR));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_PREF_REDIRECT_MYSTERY));
	}

	// If this is the "New Horde" window, subclass some of the buttons.
	
	else if (context.fDlgID == kDlgHordeNew)
	{
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_FIRST_APP_LAUNCHED));
	}

	// If this is the Common Dialog, subclass some of the buttons

	else if (context.fDlgID == kDlgCommonDialog)
	{
		new ControlCButton (::GetDlgItem (hWnd, IDC_BUTTON1));
		new ControlCButton (::GetDlgItem (hWnd, IDC_BUTTON2));
		new ControlCButton (::GetDlgItem (hWnd, IDC_BUTTON3));
	}

	// If this is the "Error Handling" window, subclass some of the buttons.

	else if (context.fDlgID == kDlgEditErrorHandling)
	{
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_ERROR_WARNING_OFF));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_ERROR_ERROR_OFF));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_ERROR_WARNING_ON));
		new PopupMenuButton (::GetDlgItem (hWnd, IDC_ERROR_ERROR_ON));
	}
}


/***********************************************************************
 *
 * FUNCTION:	HostStartIdling
 *
 * DESCRIPTION:	Enable the dialog to receive idle time.
 *
 * PARAMETERS:	context - dialog context data.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmDlg::HostStartIdling (EmDlgContext& context)
{
	// Start idling if we need it.

	if (context.fNeedsIdle)
	{
		::SetTimer ((HWND) context.fDlg, 0, 10, NULL);
	}
}


/***********************************************************************
 *
 * FUNCTION:	HostStopIdling
 *
 * DESCRIPTION:	Disable the dialog from receiving idle time.
 *
 * PARAMETERS:	context - dialog context data.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmDlg::HostStopIdling (EmDlgContext& context)
{
	::KillTimer ((HWND) context.fDlg, 0);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetDlgBounds
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetDlgBounds (EmDlgRef dlg, const EmRect& bounds)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	EmRect	oldBounds = EmDlg::GetDlgBounds (dlg);

	if (oldBounds != bounds)
	{
		::MoveWindow (hDlg, bounds.fLeft, bounds.fTop,
			bounds.Width (), bounds.Height (), true);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetDlgBounds
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmRect EmDlg::GetDlgBounds (EmDlgRef dlg)
{
	EmRect	result (0, 0, 0, 0);

	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return result;

	RECT	r;
	::GetWindowRect (hDlg, &r);  

	result = r;

	return result;
}


void EmDlg::CenterDlg (EmDlgRef dlg)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	::CenterWindow (hDlg);
}


void EmDlg::SetDlgDefaultButton (EmDlgContext& context, EmDlgItemID item)
{
	context.fDefaultItem = item;

	EmDlgRef	dlg = context.fDlg;
	if (!dlg)
		return;

	HWND		hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	UINT	dlgItemID = ::PrvFromDlgItemID (item);
	if (dlgItemID == ID__UNMAPPED)
		return;

	// Update the default push button's control ID.
	::SendMessage (hDlg, DM_SETDEFID, dlgItemID, 0);

	// Set the new style.
	::SendDlgItemMessage (hDlg, dlgItemID, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
}

void EmDlg::SetDlgCancelButton (EmDlgContext& context, EmDlgItemID item)
{
	context.fCancelItem = item;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemMin
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemMin (EmDlgRef dlg, EmDlgItemID item, long minValue)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsProgressBar (hWnd))
	{
#if 0
		// Can't do this unless _WIN32_IE >= 0x0300.  I'm not sure
		// if that's a condition I want to make.
		PBRANGE	range;
		::SendMessage (hWnd, PBM_GETRANGE, true, (LPARAM) &range);
		range.iLow = minValue;
		::SendMessage (hWnd, PBM_SETRANGE, 0, MAKELPARAM (range.iLow, range.iHigh));
#endif
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemValue
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemValue (EmDlgRef dlg, EmDlgItemID item, long value)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsProgressBar (hWnd))
	{
		::SendMessage (hWnd, PBM_SETPOS, value, 0);
	}
	else if (::PrvIsPopupMenuButton (hWnd))
	{
		::SendMessage (hWnd, CB_SETCURSEL, (WPARAM) value, 0);
	}
	else if (::PrvIsButton (hWnd))
	{
		::SendMessage (hWnd, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
	}
	else if (::PrvIsComboBox (hWnd))
	{
		::SendMessage (hWnd, CB_SETCURSEL, value, 0);
	}
	else if (::PrvIsListBox (hWnd))
	{
		::SendMessage (hWnd, LB_SETCURSEL, value, 0);
	}
	else if (::PrvIsEdit (hWnd))
	{
		char	buffer[20];
		sprintf (buffer, "%ld", value);
		::SetWindowText (hWnd, buffer);
	}
	else if (::PrvIsStatic (hWnd))
	{
		char	buffer[20];
		::FormatInteger (buffer, value);
		::SetWindowText (hWnd, buffer);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemMax
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemMax (EmDlgRef dlg, EmDlgItemID item, long maxValue)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsProgressBar (hWnd))
	{
#if 0
		// Can't do this unless _WIN32_IE >= 0x0300.  I'm not sure
		// if that's a condition I want to make.
		PBRANGE	range;
		::SendMessage (hWnd, PBM_GETRANGE, true, (LPARAM) &range);
#else
		EmAssert (maxValue <= 0x7FFF);
#endif
		::SendMessage (hWnd, PBM_SETRANGE, 0, MAKELPARAM (0, maxValue));
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemBounds
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemBounds (EmDlgRef dlg, EmDlgItemID item, const EmRect& bounds)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	::MoveWindow (hWnd, bounds.fLeft, bounds.fTop,
		bounds.Width (), bounds.Height (), true);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemText
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemText (EmDlgRef dlg, EmDlgItemID item, string str)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	// Convert returns to the EOL sequence that Windows likes
	// in a text field.

	string	str2 = ::ReplaceString (str, "\x0A", "\x0D\x0A");

	::EmSetWindowText (hWnd, str2.c_str ());
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetItemValue
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

long EmDlg::GetItemValue (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return 0;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return 0;

	if (::PrvIsProgressBar (hWnd))
	{
#if 0
		// Can't do this unless _WIN32_IE >= 0x0300.  I'm not sure
		// if that's a condition I want to make.
		return (long) ::SendMessage (hWnd, PBM_GETPOS, 0, 0);
#endif
		return 0;
	}
	else if (::PrvIsButton (hWnd))
	{
		return ::SendMessage (hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
	}
	else if (::PrvIsComboBox (hWnd) || ::PrvIsPopupMenuButton (hWnd))
	{
		return (long) ::SendMessage (hWnd, CB_GETCURSEL, 0, 0);
	}
	else if (::PrvIsListBox (hWnd))
	{
		return (long) ::SendMessage (hWnd, LB_GETCURSEL, 0, 0);
	}
	else if (::PrvIsEdit (hWnd))
	{
		char	buffer[20];
		::GetWindowText (hWnd, buffer, 20);

		long	num;
		if (EmDlg::StringToLong (buffer, &num))
			return num;
	}

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetItemBounds
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmRect EmDlg::GetItemBounds (EmDlgRef dlg, EmDlgItemID item)
{
	EmRect	result (0, 0, 0, 0);

	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return result;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return result;

	RECT	r;
	::GetWindowRect (hWnd, &r);  
	::MapWindowPoints (NULL, ::GetParent (hWnd), (LPPOINT) &r, 2);

	result = r;

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetItemText
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

string EmDlg::GetItemText (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return string ();

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return string ();

	string	result;
	int		textLength = ::GetWindowTextLength (hWnd);

	if (textLength > 0)
	{
		result.resize (textLength);
		::GetWindowText (hWnd, &result[0], textLength + 1);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::EnableItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::EnableItem (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	::EnableWindow (hWnd, true);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::DisableItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::DisableItem (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	::EnableWindow (hWnd, false);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::ShowItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::ShowItem (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	::ShowWindow (hWnd, SW_SHOW);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::HideItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::HideItem (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	::ShowWindow (hWnd, SW_HIDE);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::ClearMenu
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::ClearMenu (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsComboBox (hWnd) || ::PrvIsPopupMenuButton (hWnd))
	{
		::SendMessage (hWnd, CB_RESETCONTENT, 0, 0);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::AppendToMenu
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::AppendToMenu (EmDlgRef dlg, EmDlgItemID item, const StringList& strList)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsComboBox (hWnd) || ::PrvIsPopupMenuButton (hWnd))
	{
		StringList::const_iterator	iter = strList.begin();
		while (iter != strList.end ())
		{
			::SendMessage (hWnd, CB_ADDSTRING, 0, (LPARAM) iter->c_str ());
			++iter;
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::EnableMenuItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::EnableMenuItem (EmDlgRef dlg, EmDlgItemID item, long menuItem)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsPopupMenuButton (hWnd))
	{
		::SendMessage (hWnd, CB_SETITEMDATA, (WPARAM) menuItem, 1);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::DisableMenuItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::DisableMenuItem (EmDlgRef dlg, EmDlgItemID item, long menuItem)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	if (::PrvIsPopupMenuButton (hWnd))
	{
		::SendMessage (hWnd, CB_SETITEMDATA, (WPARAM) menuItem, 0);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::ClearList
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::ClearList (EmDlgRef dlg, EmDlgItemID item)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	EmAssert (::PrvIsListBox (hWnd));

	ListBox_ResetContent (hWnd);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::AppendToList
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::AppendToList (EmDlgRef dlg, EmDlgItemID item, const StringList& strList)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	EmAssert (::PrvIsListBox (hWnd));

	StringList::const_iterator	iter = strList.begin();
	while (iter != strList.end ())
	{
		ListBox_AddString (hWnd, iter->c_str ());
		++iter;
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SelectListItems
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SelectListItems (EmDlgRef dlg, EmDlgItemID item, const EmDlgListIndexList& itemList)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	EmAssert (::PrvIsListBox (hWnd));

	UINT	style = ::GetWindowLong (hWnd, GWL_STYLE);

	int	err;
	
	if ((style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0)
	{
		EmDlgListIndexList::const_iterator	iter = itemList.begin();
		while (iter != itemList.end())
		{
			err = ListBox_SetSel (hWnd, true, *iter);

			++iter;
		}
	}
	else
	{
		err = ListBox_SetCurSel (hWnd, itemList[0]);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::UnselectListItems
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::UnselectListItems (EmDlgRef dlg, EmDlgItemID item, const EmDlgListIndexList& itemList)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	EmAssert (::PrvIsListBox (hWnd));

	UINT	style = ::GetWindowLong (hWnd, GWL_STYLE);

	int	err;

	if ((style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0)
	{
		EmDlgListIndexList::const_iterator	iter = itemList.begin();
		while (iter != itemList.end())
		{
			err = ListBox_SetSel (hWnd, false, *iter);

			++iter;
		}
	}
	else
	{
		err = ListBox_SetCurSel (hWnd, itemList[0]);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetSelectedItems
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::GetSelectedItems (EmDlgRef dlg, EmDlgItemID item, EmDlgListIndexList& itemList)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	EmAssert (::PrvIsListBox (hWnd));

	UINT	style = ::GetWindowLong (hWnd, GWL_STYLE);

	if ((style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0)
	{
		int	numSelItems = ListBox_GetSelCount (hWnd);

		if (numSelItems > 0)
		{
			int*	selectedItems = new int[numSelItems];
			if (ListBox_GetSelItems (hWnd, numSelItems, selectedItems) != LB_ERR)
			{
				for (int ii = 0; ii < numSelItems; ++ii)
				{
					itemList.push_back (selectedItems[ii]);
				}
			}

			delete [] selectedItems;
		}
	}
	else
	{
		int	sel = ListBox_GetCurSel (hWnd);
		if (sel != LB_ERR)
		{
			itemList.push_back (sel);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetTextHeight
 *
 * DESCRIPTION:	Determine the height the text would be if fitted into a
 *				rectangle with the given width.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	New text height
 *
 ***********************************************************************/

int EmDlg::GetTextHeight (EmDlgRef dlg, EmDlgItemID item, const string& s)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return 0;

	EmRect	bounds	= EmDlg::GetItemBounds (dlg, item);
	EmCoord	width	= bounds.Width ();

	HDC 	dc		= ::GetDC (hDlg);
	HGDIOBJ oldFont	= ::SelectObject (dc, ::GetStockObject (DEFAULT_GUI_FONT));

	RECT	newRect = { 0, 0, width, 0 };
	::DrawText (dc, s.c_str (), -1, &newRect, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);

	::SelectObject (dc, oldFont);
	::ReleaseDC (hDlg, dc);

	return newRect.bottom;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	CenterWindow
 *
 * DESCRIPTION:	Centers the given window the desktop window.
 *
 *				(This routine doesn't need to be in this file, but
 *				 there's no better place for it.)
 *
 * PARAMETERS:	hWnd - window to center.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

void CenterWindow (HWND hWnd)
{
	RECT	workArea;
	::SystemParametersInfo (SPI_GETWORKAREA, NULL, &workArea, NULL);

	RECT	dRect;
	::GetWindowRect (hWnd, &dRect);

	int xLeft	= (workArea.left + workArea.right) / 2 - (dRect.right - dRect.left) / 2;
	int yTop	= (workArea.top + workArea.bottom) / 2 - (dRect.bottom - dRect.top) / 2;

	::SetWindowPos (hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


/***********************************************************************
 *
 * FUNCTION:	CenterDlgProc
 *
 * DESCRIPTION:	DlgProc for centering dialogs.
 *
 *				(This routine doesn't need to be in this file, but
 *				 there's no better place for it.)
 *
 * PARAMETERS:	Standard DlgProc parameters.
 *
 * RETURNED:	Standard DlgProc result.
 *
 ***********************************************************************/

UINT CALLBACK CenterDlgProc (	HWND hWnd,
								UINT msg,
								WPARAM wParam,
								LPARAM lParam)
{
	UNUSED_PARAM (wParam);
	UNUSED_PARAM (lParam);

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			::CenterWindow (::GetParent (hWnd));
			::SetWindowPos (::GetParent (hWnd), ::GetActiveWindow (), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}


/***********************************************************************
 *
 * FUNCTION:	GetCurrentModelessDialog
 *
 * DESCRIPTION:	Returns the handle to the currently-active modeless
 *				dialog, or NULL if there is none.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Window handle or NULL.
 *
 ***********************************************************************/

HWND GetCurrentModelessDialog (void)
{
	return gDlgCurrent;
}


/***********************************************************************
 *
 * FUNCTION:	EmSetWindowText
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmSetWindowText (HWND hWndCtrl, const char* lpszNew)
{
	int 	nNewLen = strlen (lpszNew);
	char	szOld[256];

	// fast check to see if text really changes (reduces flash in controls)

	if (nNewLen > countof (szOld) ||
		::GetWindowText (hWndCtrl, szOld, countof(szOld)) != nNewLen ||
		strcmp (szOld, lpszNew) != 0)
	{
		// change it
		::SetWindowText (hWndCtrl, lpszNew);
	}
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	PrvFromDlgID
 *
 * DESCRIPTION:	Convert an EmDlgID to a Windows dialog ID.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

UINT PrvFromDlgID (EmDlgID id)
{
	for (int ii = 0; ii < countof (kDlgIDMap); ++ii)
	{
		if (kDlgIDMap[ii].fMyID == id)
		{
			return kDlgIDMap[ii].fWinID;
		}
	}

	return ID__UNMAPPED;
}


/***********************************************************************
 *
 * FUNCTION:	PrvToDlgID
 *
 * DESCRIPTION:	Convert a Windows dialog ID to an EmDlgID.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

EmDlgID PrvToDlgID (UINT id)
{
	for (int ii = 0; ii < countof (kDlgIDMap); ++ii)
	{
		if (kDlgIDMap[ii].fWinID == id)
		{
			return kDlgIDMap[ii].fMyID;
		}
	}

	return kDlgNone;
}


/***********************************************************************
 *
 * FUNCTION:	PrvFromDlgItemID
 *
 * DESCRIPTION:	Convert an EmDlgItemID to a Windows dialog ID.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

UINT PrvFromDlgItemID (EmDlgItemID id)
{
#if !defined (NDEBUG)
	// Make sure there aren't any duplicate control IDs in the list.
	// Such a duplicate could mess up our mapping.
	for (int iii = 0; iii < countof (kDlgItemIDMap); ++iii)
	{
		for (int jjj = iii + 1; jjj < countof (kDlgItemIDMap); ++jjj)
		{
			EmAssert (kDlgItemIDMap[iii].fMyID != kDlgItemIDMap[jjj].fMyID);
		}
	}
#endif

	for (int ii = 0; ii < countof (kDlgItemIDMap); ++ii)
	{
		if (kDlgItemIDMap[ii].fMyID == id)
		{
			return kDlgItemIDMap[ii].fWinID;
		}
	}

	return ID__UNMAPPED;
}


/***********************************************************************
 *
 * FUNCTION:	PrvToDlgItemID
 *
 * DESCRIPTION:	Convert a Windows dialog ID to an EmDlgItemID.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

EmDlgItemID PrvToDlgItemID (UINT id)
{
#if !defined (NDEBUG)
	// Make sure there aren't any duplicate control IDs in the list.
	// Such a duplicate could mess up our mapping.
	for (int iii = 0; iii < countof (kDlgItemIDMap); ++iii)
	{
		for (int jjj = iii + 1; jjj < countof (kDlgItemIDMap); ++jjj)
		{
			if (kDlgItemIDMap[iii].fWinID != ID__UNMAPPED)
			{
				EmAssert (kDlgItemIDMap[iii].fWinID != kDlgItemIDMap[jjj].fWinID);
			}
		}
	}
#endif

	for (int ii = 0; ii < countof (kDlgItemIDMap); ++ii)
	{
		if (kDlgItemIDMap[ii].fWinID == id)
		{
			return kDlgItemIDMap[ii].fMyID;
		}
	}

	return kDlgItemNone;
}


/***********************************************************************
 *
 * FUNCTION:	PrvConvertTypeList
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

FilterList PrvConvertTypeList (const EmFileTypeList& typeList)
{
	FilterList	filter;

	EmAssert (typeList.size () > 0);

	EmFileTypeList::const_iterator	iter = typeList.begin ();

	while (iter != typeList.end ())
	{
		switch (*iter)
		{
			case kFileTypeApplication:
				::PrvAddFilter (filter, "Emulator Executable Files (*.exe)", "*.exe");
				break;

			case kFileTypeROM:
				::PrvAddFilter (filter, "Palm OS ROM Files (*.rom)", "*.rom;* rom;rom.*;rom*;*.widebin");
				break;

			case kFileTypeSession:
				::PrvAddFilter (filter, "Emulator Session Files (*.psf)", "*.psf");
				break;

			case kFileTypeEvents:
				::PrvAddFilter (filter, "Emulator Event Files (*.pev)", "*.pev");
				break;

			case kFileTypePreference:
				::PrvAddFilter (filter, "Preference File (*.ini)", "*.ini");
				break;

			case kFileTypePalmApp:
				::PrvAddFilter (filter, "Palm OS Applications (*.prc)", "*.prc");
				break;

			case kFileTypePalmDB:
				::PrvAddFilter (filter, "Palm OS Databases (*.pdb)", "*.pdb");
				break;

			case kFileTypePalmQA:
				::PrvAddFilter (filter, "Palm OS Clippings (*.pqa)", "*.pqa");
				break;

			case kFileTypeText:
				::PrvAddFilter (filter, "Text (*.txt)", "*.txt");
				break;

			case kFileTypePicture:
				::PrvAddFilter (filter, "Bitmap (*.bmp)", "*.bmp");
				break;

			case kFileTypeSkin:
				::PrvAddFilter (filter, "Skin (*.skin)", "*.skin");
				break;

			case kFileTypeProfile:
				::PrvAddFilter (filter, "Profile (*.mwp)", "*.mwp");
				break;

			case kFileTypePalmAll:
				::PrvAddFilter (filter, "Palm OS Files (*.prc, *.pdb, *.pqa)", "*.prc;*.pdb;*.pqa");
				break;

			case kFileTypeAll:
				::PrvAddFilter (filter, "All Files (*.*)", "*.*");
				break;
		}

		++iter;
	}

	const char	emptyString[] = "";
	filter.insert (filter.end (), emptyString, emptyString + 1);

	return filter;
}


/***********************************************************************
 *
 * FUNCTION:	PrvAddFilter
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void PrvAddFilter (FilterList& filter, const char* desc, const char* pattern)
{
	filter.insert (filter.end (), desc, desc + strlen (desc) + 1);
	filter.insert (filter.end (), pattern, pattern + strlen (pattern) + 1);
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetExtension
 *
 * DESCRIPTION:	Return an extension appropriate for the given file
 *				type.  If there is no extension for the type. return
 *				an empty string.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

string PrvGetExtension (EmFileType type)
{
	string		result;
	const char*	extension = kExtension[type];

	if (extension)
	{
		result = extension;

		// If we got an extension and it starts with a '.'.  remove
		// the '.'.

		if (result.size () > 0 && result[0] == '.')
		{
			result.erase (result.begin ());
		}
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetStartPosition
 *
 * DESCRIPTION:	Iterator-style function returning the starting
 *				position for the list of files returned from
 *				GetOpenFileName.  Call this function first; then call
 *				GetNextPathName for as long as POSITION is non-zero.
 *
 *				Function unabashedly stolen from MFC.
 *
 * PARAMETERS:	ofn - the OPENFILENAME passed to GetOpenFileName.
 *
 * RETURNED:	An opaque iterator type containing a value indicating
 *				the first item in the list.
 *
 ***********************************************************************/

POSITION PrvGetStartPosition (OPENFILENAME& ofn)
{
	return (POSITION) ofn.lpstrFile;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetNextPathName
 *
 * DESCRIPTION:	Get the pathname indicated by pos, and then increment
 *				pos to refer to the next file in the list.
 *
 *				Function unabashedly stolen from MFC.
 *
 * PARAMETERS:	ofn - the OPENFILENAME passed to GetOpenFileName.
 *
 *				pos - POSITION originally obtained by calling
 *					GetStartPosition.  Subsequent values are returned
 *					by updating pos in-place.
 *
 * RETURNED:	The next pathname indicated by pos.
 *
 ***********************************************************************/

string PrvGetNextPathName (OPENFILENAME& ofn, POSITION& pos)
{
	BOOL	bExplorer	= (ofn.Flags & OFN_EXPLORER) != 0;
	TCHAR	chDelimiter = bExplorer ? '\0' : ' ';
	LPTSTR	lpsz		= (LPTSTR) pos;

	if (lpsz == ofn.lpstrFile) // first time
	{
		if ((ofn.Flags & OFN_ALLOWMULTISELECT) == 0)
		{
			pos = NULL;
			return ofn.lpstrFile;
		}

		// find char pos after first Delimiter
		while (*lpsz != chDelimiter && *lpsz != '\0')
		{
			lpsz = _tcsinc (lpsz);
		}

		lpsz = _tcsinc (lpsz);

		// if single selection then return only selection
		if (*lpsz == 0)
		{
			pos = NULL;
			return ofn.lpstrFile;
		}
	}

	string strPath = ofn.lpstrFile;
	if (!bExplorer)
	{
		LPTSTR	lpszPath = ofn.lpstrFile;

		while (*lpszPath != chDelimiter)
		{
			lpszPath = _tcsinc (lpszPath);
		}

		strPath = strPath.substr (0, lpszPath - ofn.lpstrFile);
	}

	LPTSTR	lpszFileName = lpsz;
	string	strFileName = lpsz;

	// find char pos at next Delimiter
	while (*lpsz != chDelimiter && *lpsz != '\0')
	{
		lpsz = _tcsinc (lpsz);
	}

	if (!bExplorer && *lpsz == '\0')
	{
		pos = NULL;
	}
	else
	{
		if (!bExplorer)
		{
			strFileName = strFileName.substr (0, lpsz - lpszFileName);
		}

		lpsz = _tcsinc (lpsz);

		if (*lpsz == '\0') // if double terminated then done
			pos = NULL;
		else
			pos = (POSITION) lpsz;
	}

	// only add '\\' if it is needed
	if (!strPath.empty ())
	{
		// check for last back-slash or forward slash (handles DBCS)
		LPCTSTR lpsz = _tcsrchr (strPath.c_str (), '\\');
		if (lpsz == NULL)
			lpsz = _tcsrchr (strPath.c_str (), '/');

		// if it is also the last character, then we don't need an extra
		if (lpsz != NULL &&
			(lpsz - strPath.c_str ()) == strPath.size () - 1)
		{
			EmAssert (*lpsz == '\\' || *lpsz == '/');
			return strPath + strFileName;
		}
	}

	return strPath + '\\' + strFileName;
}


#ifdef use_buffer_resizing_code
/***********************************************************************
 *
 * FUNCTION:	PrvOpenMultiFileProc
 *
 * DESCRIPTION:	Hook the OpenFileName dialog so that we can look for
 *				presses on the OK button.  We look for those so that
 *				we can ensure that the buffer receiving the selection
 *				is large enough.  From MSDN:
 *
 *				"With the introduction of the new common dialogs for
 *				Windows 95, a new way of handling the FNERR_BUFFERTOOSMALL
 *				error was developed. It is still necessary to watch for
 *				the Open button to be pressed and reallocate the buffer
 *				if needed, but the way to watch for the OK is much different. 
 *
 *				"When you install a hook on the Open File common dialog
 *				in Windows 95 using the OPENFILENAME.lpfnHook member,
 *				the dialog you are hooking is a child of the main Open
 *				File dialog. Therefore, to intercept the OK button, you
 *				need to subclass the parent dialog. To do this, you can
 *				install the hook procedure and watch for the CDN_INITDONE
 *				notification. The Open File dialog will send this as part
 *				of a WM_NOTIFY message when the initialization for the
 *				dialog is complete."
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

UINT CALLBACK PrvOpenMultiFileProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNUSED_PARAM(wParam);

	static HWND	hwndParentDialog;
	LPOFNOTIFY	lpofn;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			// You need to use a copy of the OPENFILENAME struct used to
			// create this dialog. You can store a pointer to the
			// OPENFILENAME struct in the ofn.lCustData so you can retrieve
			// it here in the lParam. Once you have it, you need to hang on
			// to it. Using window properties provides a good thread safe
			// solution to using a global variable.

			::SetProp (hWnd, "OFN", (HANDLE) lParam);
			::CenterWindow (::GetParent (hWnd));
			::SetWindowPos (::GetParent (hWnd), ::GetActiveWindow (), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			return (0);

		case WM_NOTIFY:
			// The OFNOTIFY struct is passed in the lParam of this message.

			lpofn = (LPOFNOTIFY) lParam;

			switch (lpofn->hdr.code)
			{
				case CDN_INITDONE:
					// Subclass the parent dialog to watch for the OK
					// button.

					hwndParentDialog = ::GetParent (hWnd);
					gDlgProc = (WNDPROC) ::SetWindowLong (hwndParentDialog,
											DWL_DLGPROC, (LONG) &::PrvOpenMultiFileHook);
					break;

			}
			return 0;

		case WM_DESTROY:
			// Need to clean up the subclassing we did on the dialog.
			::SetWindowLong (hwndParentDialog, DWL_DLGPROC, (LONG) gDlgProc);

			// Also need to free the property with the OPENFILENAME struct
			::RemoveProp (hWnd, "OFN");
			return 0;
	}

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	PrvOpenMultiFileHook
 *
 * DESCRIPTION:	Part 2 of hooking the Open File dialog so that we can
 *				look for clicks on OK.	See OpenMultiFileProc for the
 *				discussion of the first part.  Continuing the MSDN
 *				description:
 *
 *				"Once the parent dialog is subclassed, the program can
 *				watch for the actual Open button. When the program gets
 *				the Open button command, it needs to check to see if the
 *				buffer originally allocated is large enough to handle
 *				all the files selected. The CommDlg_OpenSave_GetFilePath()
 *				API will return the length needed."
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

LRESULT CALLBACK PrvOpenMultiFileHook (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPTSTR			lpsz;
	int 			cbLength;
	LPOPENFILENAME	lpofn;

	switch (uMsg)
	{
		case WM_COMMAND:
			switch (LOWORD (wParam))
			{
				case IDOK:
					// Need to verify the original buffer size is large
					// enough to handle the files selected. The
					// CommDlg_OpenSave_GetFilePath() API will return the
					// length needed for this buffer.

					cbLength = CommDlg_OpenSave_GetFilePath (hWnd, NULL, 0);

					// MAX_PATH is the size of the buffer originally
					// used in the OPENFILENAME.lpstrFile member.

					if (MAX_PATH < cbLength)
					{
						// The buffer is too small, so allocate a
						// new buffer.
						lpsz = (LPTSTR) malloc (cbLength);
						if (lpsz)
						{
							// The OFN struct is stored in a property of
							// the dialog window.

							lpofn = (LPOPENFILENAME) ::GetProp(hWnd, "OFN");

							free (lpofn->lpstrFile);

							lpofn->lpstrFile = lpsz;
							lpofn->nMaxFile  = cbLength;
						}
					}

					// Now let the dialog handle the message normally.
					break;
			}
			break;
	}

	return ::CallWindowProc (gDlgProc, hWnd, uMsg, wParam, lParam);
}
#endif	// use_buffer_resizing_code


#pragma mark -

/*
	List of Common Control classes in case we ever need to test for them.

	ANIMATE_ CLASS		Creates animation controls. These controls silently
						display an AVI clip. 
	DATETIMEPICK_CLASS  Creates date and time picker controls. These controls
						provide a simple and intuitive interface to exchange
						date and time information with a user.  
	HOTKEY_CLASS		Creates hotkey controls. These controls make it easy
						for the user to define hotkeys.  
	MONTHCAL_CLASS		Creates month calendar controls. These controls provide
						a simple and intuitive way for a user to select a date
						from a familiar interface.  
	PROGRESS_CLASS		Creates progress bars. These controls indicate the
						progress of a lengthy operation.  
	REBARCLASSNAME		Creates rebar controls. These controls act as a container
						for child windows.  
	STATUSCLASSNAME		Creates status windows. These controls display status
						information in a horizontal window.  
	TOOLBARCLASSNAME	Creates toolbars. These controls contain buttons that
						carry out menu commands.  
	TOOLTIPS_CLASS		Creates tooltip controls. These controls display a small
						pop-up window containing a line of text that describes
						the purpose of a tool in an application.  
	TRACKBAR_CLASS		Creates trackbars. These controls let the user select
						from a range of values by moving a slider.  
	UPDOWN_CLASS		Creates up-down controls. These controls combine a pair
						of arrows with an edit control. Clicking the arrows
						increments or decrements the value in the edit control.  
	WC_COMBOBOXEX		Creates ComboBoxEx controls. These controls provide an
						extension of the combobox control that provides native
						support for item images.  
	WC_HEADER			Creates header controls. These controls display headings
						at the top of columns of information and let the user
						sort the information by clicking the headings.  
	WC_IPADDRESS		Creates IP address controls. These controls are similar
						to an edit control, but allow you to enter a numerical
						address in IP format.  
	WC_LISTVIEW			Creates list view controls. These controls display a
						collection of items, each consisting of an icon and a
						label, and provide several ways to arrange the items.  
	WC_PAGESCROLLER		Creates pager controls. These controls are used to
						contain and scroll another window.  
	WC_TABCONTROL		Creates tab controls. These controls define multiple
						pages for the same area of a window or dialog box. Each
						page consists of a set of information or a group of
						controls that an application displays when the user
						selects the corresponding tab.  
	WC_TREEVIEW			Creates tree view controls. These controls display a
						hierarchical list of items. Each item consists of a
						label and an optional bitmap.  
*/

/*
	List of standard control classes in case we ever need to test for them.

		Listbox
		ComboBox
		ScrollBar
		Button
		Static
		Edit

	and some less familiar controls:

		The class for menus.
		The class for the desktop window.
		The class for dialog boxes.
		The class for the task switch window.
		The class for icon titles.
		ComboLBox: The class for the drop-down list box contained in a combo box.
		MDIClient: The class for multiple-document interface (MDI) client windows.

	also:
		RichEdit		RichEdit 1.0
		RICHEDIT_CLASS	RichEdit 2.0
*/

const char	kButtonClassName[]		= "Button";
const char	kComboBoxClassName[]	= "ComboBox";
const char	kListBoxClassName[]		= "ListBox";
const char	kEditClassName[]		= "Edit";
const char	kStaticClassName[]		= "Static";
const char	kScrollBarClassName[]	= "ScrollBar";

const UINT	kButtonStyleMask = 0xFF;		// This is what MFC uses in CButton::GetButtonStyle


/***********************************************************************
 *
 * FUNCTION:	PrvIsPushButton
 * FUNCTION:	PrvIsCheckBox
 * FUNCTION:	PrvIsRadioButton
 * FUNCTION:	PrvIsProgressBar
 * FUNCTION:	PrvIsButton
 * FUNCTION:	PrvIsClass
 *
 * DESCRIPTION:	Predicates to test HWNDs to see what kinds of controls
 *				they represent.
 *
 * PARAMETERS:	hWnd - HWND to test
 *
 * RETURNED:	True if the HWND is of the type being tested.
 *
 ***********************************************************************/

Bool PrvIsPushButton (HWND hWnd)
{
	if (!::PrvIsButton (hWnd))
		return false;

	UINT	style = ::GetWindowLong (hWnd, GWL_STYLE) & kButtonStyleMask;
	return	style == BS_PUSHBUTTON ||		// Creates a pushbutton that posts a WM_COMMAND message to the owner window when the user selects the button.
			style == BS_DEFPUSHBUTTON;		// Creates a button that has a heavy black border. The user can select this button by pressing the ENTER key. This style enables the user to quickly select the most likely option (the default option).
}

Bool PrvIsCheckBox (HWND hWnd)
{
	if (!::PrvIsButton (hWnd))
		return false;

	UINT	style = ::GetWindowLong (hWnd, GWL_STYLE) & kButtonStyleMask;
	return	style == BS_AUTOCHECKBOX ||		// Same as a check box, except that a check mark appears in the check box when the user selects the box; the check mark disappears the next time the user selects the box.
			style == BS_AUTO3STATE ||		// Same as a three-state check box, except that the box changes its state when the user selects it.
			style == BS_CHECKBOX ||			// Creates a small square that has text displayed to its right (unless this style is combined with the BS_LEFTTEXT style).
			style == BS_3STATE;				// Same as a check box, except that the box can be dimmed as well as checked. The dimmed state typically is used to show that a check box has been disabled.
}

Bool PrvIsRadioButton (HWND hWnd)
{
	if (!::PrvIsButton (hWnd))
		return false;

	UINT	style = ::GetWindowLong (hWnd, GWL_STYLE) & kButtonStyleMask;
	return	style == BS_AUTORADIOBUTTON ||	// Same as a radio button, except that when the user selects it, the button automatically highlights itself and removes the selection from any other radio buttons with the same style in the same group.
			style == BS_RADIOBUTTON;		// Creates a small circle that has text displayed to its right (unless this style is combined with the BS_LEFTTEXT style). Radio buttons are usually used in groups of related but mutually exclusive choices.
}

Bool PrvIsProgressBar (HWND hWnd)
{
	return ::PrvIsClass (hWnd, PROGRESS_CLASS);
}

Bool PrvIsButton (HWND hWnd)
{
	return ::PrvIsClass (hWnd, kButtonClassName) && ::GetWindowLong (hWnd, GWL_USERDATA) == 0;
}

Bool PrvIsComboBox (HWND hWnd)
{
	return ::PrvIsClass (hWnd, kComboBoxClassName);
}

Bool PrvIsListBox (HWND hWnd)
{
	return ::PrvIsClass (hWnd, kListBoxClassName);
}

Bool PrvIsEdit (HWND hWnd)
{
	return ::PrvIsClass (hWnd, kEditClassName);
}

Bool PrvIsStatic (HWND hWnd)
{
	return ::PrvIsClass (hWnd, kStaticClassName);
}

Bool PrvIsPopupMenuButton (HWND hWnd)
{
	return ::PrvIsClass (hWnd, kButtonClassName) && ::GetWindowLong (hWnd, GWL_USERDATA) != 0;
}

Bool PrvIsClass (HWND hWnd, const char* className)
{
	char	buffer[256];
	if (::GetClassName (hWnd, buffer, countof (buffer)))
	{
		return _stricmp (className, buffer) == 0;	// MFC's _AfxIsComboBoxControl uses an unsigned compare, too.
	}

	return false;
}



#pragma mark -

// ===========================================================================
//		¥ SubclassedWindow
// ===========================================================================

/***********************************************************************
 *
 * FUNCTION:	SubclassedWindow constructor
 *
 * DESCRIPTION:	Remember the original window procedure and install
 *				ours in its place.
 *
 * PARAMETERS:	hWnd - window to subclass
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

SubclassedWindow::SubclassedWindow (HWND hWnd) :
	fWnd (hWnd)
{
	EmAssert (fWnd);

	fOldWndProc = (WNDPROC) ::GetWindowLong (fWnd, GWL_WNDPROC);
	::SetWindowLong (fWnd, GWL_WNDPROC, (LONG) &StaticWndProc);
	::SetWindowLong (fWnd, GWL_USERDATA, (LONG) this);
}


/***********************************************************************
 *
 * FUNCTION:	SubclassedWindow destructor
 *
 * DESCRIPTION:	Restore the original window procedure.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

SubclassedWindow::~SubclassedWindow (void)
{
	::SetWindowLong (fWnd, GWL_WNDPROC, (LONG) fOldWndProc);
}


/***********************************************************************
 *
 * FUNCTION:	SubclassedWindow::StaticWndProc
 *
 * DESCRIPTION:	Installed window procedure.  Retrieves the pointer
 *				to the "SubclassedWindow" object associated with this
 *				window and calls its (virtual) WndProc method.
 *
 * PARAMETERS:	Standard window procedure parameters
 *
 * RETURNED:	Standard window procedure result
 *
 ***********************************************************************/

LRESULT CALLBACK SubclassedWindow::StaticWndProc (HWND hWnd, UINT msg,
												  WPARAM wParam, LPARAM lParam)
{
	SubclassedWindow*	This = reinterpret_cast<SubclassedWindow*> (
										::GetWindowLong (hWnd, GWL_USERDATA));

	EmAssert (hWnd == This->fWnd);

	LRESULT 			r = This->WndProc (hWnd, msg, wParam, lParam);

	if (msg == WM_NCDESTROY)		// last message this window will get
	{
		::SetWindowLong (This->fWnd, GWL_WNDPROC, (LONG) This->fOldWndProc);
		delete This;
	}

	return r;
}


#pragma mark -

// ===========================================================================
//		¥ StaticHyperlink
// ===========================================================================

/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink constructor
 *
 * DESCRIPTION:	Initialize our member data.
 *
 * PARAMETERS:	hWnd - window to subclass.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

StaticHyperlink::StaticHyperlink (HWND hWnd) :
	SubclassedWindow(hWnd),
	fFont (NULL),
	fFinger (::LoadCursor (gInstance, MAKEINTRESOURCE (IDC_FINGER))),
	fColor (RGB (0, 0, 255))
{
}


/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink destructor
 *
 * DESCRIPTION:	Release our resources.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

StaticHyperlink::~StaticHyperlink (void)
{
	::DeleteObject (fFont);
}


/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink::OnCtlColorStatic
 *
 * DESCRIPTION:	Handle WM_CTLCOLORSTATIC messages sent to our window.
 *				For Hyperlinks, set the color appropriate for selected
 *				and unselected links.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

HBRUSH StaticHyperlink::OnCtlColorStatic (HWND /*parent*/, HDC dc, HWND /*control*/, int /*type*/)
{
	HBRUSH	hbr = NULL;

	if ((GetWindowLong (fWnd, GWL_STYLE) & 0xFF) <= SS_RIGHT)
	{
		if (fFont == NULL)
		{
			LOGFONT lf;

			::GetObject ((HGDIOBJ) ::SendMessage (fWnd, WM_GETFONT, 0, 0), sizeof (lf), &lf);

			lf.lfUnderline = TRUE;
			fFont = ::CreateFontIndirect (&lf);
		}

		::SelectObject (dc, fFont);
		::SetTextColor (dc, fColor);
		::SetBkMode (dc, TRANSPARENT);
		hbr = (HBRUSH) ::GetStockObject (HOLLOW_BRUSH);
	}

	return hbr;
}


/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink::OnLButtonDown
 *
 * DESCRIPTION:	Handle WM_LBUTTONDOWN messages sent to our window.
 *				For Hyperlinks, launch the associated URL by calling
 *				ShellExecute.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void StaticHyperlink::OnLButtonDown (HWND hWnd, BOOL doubleclick, int x, int y, UINT flags)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (doubleclick);
	UNUSED_PARAM (x);
	UNUSED_PARAM (y);
	UNUSED_PARAM (flags);

	// Get the URL.

	char	buf[256];
	::GetWindowText (hWnd, buf, sizeof (buf));

	// Strip off the leading and trailing elbow-brackets.

	size_t	len = strlen (buf);
	memcpy (buf, &buf[1], len - 2);
	buf[len - 2] = 0;

	HCURSOR cur = ::GetCursor ();
	::SetCursor (::LoadCursor (NULL, IDC_WAIT));

	HINSTANCE	r = ::ShellExecute (NULL, "open", buf, NULL, NULL, SW_SHOWNORMAL);

	::SetCursor (cur);

	if ((UINT) r > 32)
	{
		fColor = RGB (128, 0, 128);
		::InvalidateRect (hWnd, NULL, FALSE);
	}
}


/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink::OnNcHitTest
 *
 * DESCRIPTION:	Handle WM_NCHITTEST messages sent to our window.
 *				For some reason, always returning HTCLIENT is good.
 *				I guess static text items always return some sort of
 *				"nothing hit" code otherwise.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

UINT StaticHyperlink::OnNcHitTest (HWND hWnd, int x, int y)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (x);
	UNUSED_PARAM (y);

	return HTCLIENT;
}


/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink::OnSetCursor
 *
 * DESCRIPTION:	Handle WM_SETCURSOR messages sent to our window.
 *				For Hyperlinks, set the cursor to the de facto
 *				standard that browsers use when the cursor is over
 *				an active hyperlink.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

BOOL StaticHyperlink::OnSetCursor (HWND hWnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (hwndCursor);
	UNUSED_PARAM (codeHitTest);
	UNUSED_PARAM (msg);

	::SetCursor (fFinger);

	return TRUE;
}


/***********************************************************************
 *
 * FUNCTION:	StaticHyperlink::WndProc
 *
 * DESCRIPTION:	Respond to the Windows messages appropriate for turning
 *				a static text dialog item into an active URL Hyperlink.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

LRESULT StaticHyperlink::WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG (hWnd, WM_CTLCOLORSTATIC, OnCtlColorStatic);
		HANDLE_MSG (hWnd, WM_LBUTTONDOWN, OnLButtonDown);
		HANDLE_MSG (hWnd, WM_NCHITTEST, OnNcHitTest);
		HANDLE_MSG (hWnd, WM_SETCURSOR, OnSetCursor);
	}

	return ::CallWindowProc (fOldWndProc, hWnd, msg, wParam, lParam);
}


#pragma mark -

// ===========================================================================
//		¥ PopupMenuButton
// ===========================================================================

/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton constructor
 *
 * DESCRIPTION:	Initialize our member data.
 *
 * PARAMETERS:	hWnd - window to subclass.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

PopupMenuButton::PopupMenuButton (HWND hWnd) :
	SubclassedWindow(hWnd),
	fMenu (::CreatePopupMenu ()),
	fFont (NULL),
	fMenuItem (-1),
	fSelected (false)
{
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton destructor
 *
 * DESCRIPTION:	Release our resources.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

PopupMenuButton::~PopupMenuButton (void)
{
	::DestroyMenu (fMenu);
	::DeleteObject (fFont);
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnPaint
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static BOOL CALLBACK DrawStateProc (HDC hDC,
									LPARAM lData, WPARAM wData,
									int cx, int cy)
{
	RECT	bounds;

	// Draw the text.

	bounds.left		= 10;
	bounds.top		= 0;
	bounds.right	= cx - 10;
	bounds.bottom	= cy;

	::DrawText (hDC, (LPCTSTR) lData, -1, &bounds, DT_SINGLELINE | DT_VCENTER);

	// Draw the triangle.

	HBRUSH	brush = (HBRUSH) ::GetStockObject (BLACK_BRUSH);

	bounds.left		= cx - 11;
	bounds.top		= cy / 2;
	bounds.right	= cx - 10;
	bounds.bottom	= cy / 2 + 1;

	for (int yy = 0; yy < 4; ++yy)
	{
		::FillRect (hDC, &bounds, brush);

		bounds.left--;		// Move out...
		bounds.right++;
		bounds.top--;		// ...and up.
		bounds.bottom--;
	}

	return TRUE;
}

void PopupMenuButton::OnPaint (HWND hWnd)
{
	// Start the painting operation.

	PAINTSTRUCT ps;
	HDC 		hDC = ::BeginPaint (hWnd, &ps);

	// Get the bounds of the button.

	RECT	buttonBounds;
	::GetClientRect (hWnd, &buttonBounds);

	RECT	interiorBounds = buttonBounds;

	// Get the styles to use for our calls to DrawFrameControl
	// and DrawState.

	UINT	dfStyle	= DFCS_BUTTONPUSH;
	UINT	dsStyle	= DST_COMPLEX;

	// If drawing selected, add the pushed style to DrawFrameControl,
	// and adjust the location of the graphic items down and to the right.

	if (this->fSelected)
	{
		dfStyle |= DFCS_PUSHED;

		interiorBounds.left += 1;
		interiorBounds.top += 2;
	}

	// If drawing disabled, add the inactive style to DrawFrameControl,
	// and add the disabled style to DrawState.

	if (!::IsWindowEnabled (hWnd))
	{
		dfStyle |= DFCS_INACTIVE;
		dsStyle |= DSS_DISABLED;
	}

	// Draw the button frame.

	::DrawFrameControl (hDC, &buttonBounds, DFC_BUTTON, dfStyle);
	
	// Get the button's text.

	char	strText[256];
	::GetWindowText (hWnd, strText, 256);

	// Get the font for the text.

	if (fFont == NULL)
	{
		LOGFONT lf;

		::GetObject ((HGDIOBJ) ::SendMessage (fWnd, WM_GETFONT, 0, 0), sizeof (lf), &lf);

		fFont = ::CreateFontIndirect (&lf);
	}

	// Get and install the right colors and font.

	DWORD		textColor		= ::GetSysColor (COLOR_BTNTEXT);
	DWORD		backColor		= ::GetSysColor (COLOR_BTNFACE);

	COLORREF	oldForeColor	= ::SetTextColor (hDC, textColor);
	COLORREF	oldBackColor	= ::SetBkColor (hDC, backColor);

	HFONT		oldTextFont		= SelectFont (hDC, fFont);

	// Draw the button contents.
	//
	// (May want to use "DitherBlt" if DrawState is not drawing the
	//  way you want it to.)
	//
	//	<http://www.vckbase.com/english/code/bitmap/dither_blt.shtml.htm>
	//

	::DrawState (
		hDC,							// handle to DC
		NULL,							// handle to the brush
		DrawStateProc,					// callback function
		(LPARAM) strText,				// application-defined data
		0,								// number of characters
		interiorBounds.left,			// horizontal position
		interiorBounds.top,				// vertical position
		interiorBounds.right - interiorBounds.left,		// width
		interiorBounds.bottom - interiorBounds.top,		// height
		dsStyle);						// image type and state

	// Restore the drawing environment.

	SelectFont (hDC, oldTextFont);

	::SetTextColor (hDC, oldForeColor);
	::SetBkColor (hDC, oldBackColor);

	// Finish the painting operation.

	::EndPaint (hWnd, &ps);
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnLButtonDown
 *
 * DESCRIPTION:	Handle WM_LBUTTONDOWN messages sent to our window.
 *				For Hyperlinks, launch the associated URL by calling
 *				ShellExecute.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void PopupMenuButton::OnLButtonDown (HWND hWnd, BOOL doubleclick, int x, int y, UINT flags)
{
	UNUSED_PARAM (doubleclick);
	UNUSED_PARAM (x);
	UNUSED_PARAM (y);
	UNUSED_PARAM (flags);

	RECT	buttonBounds;
	::GetWindowRect (hWnd, &buttonBounds);

	fSelected = true;
	::InvalidateRect (hWnd, NULL, false);

	UINT	result = ::TrackPopupMenu (fMenu, TPM_NONOTIFY | TPM_RETURNCMD,
									buttonBounds.left, buttonBounds.bottom, 0, hWnd, NULL);

	fSelected = false;
	::InvalidateRect (hWnd, NULL, false);

	if (result > 0)
	{
		this->OnCBSetCurSel (hWnd, result - 1);

		::SendMessage (::GetParent (hWnd), WM_COMMAND, ::GetDlgCtrlID (hWnd), (LPARAM) hWnd);
	}
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnCBAddString
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

INT PopupMenuButton::OnCBAddString (HWND hWnd, LPCSTR str)
{
	INT	count = ::GetMenuItemCount (fMenu);

	if (strlen(str) == 0) {
		if (::AppendMenu (fMenu, MF_SEPARATOR, count + 1, NULL))
			return count;
	} else {
		if (::AppendMenu (fMenu, MF_STRING, count + 1, str))
			return count;
	}

	return CB_ERR;
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnCBResetContent
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void PopupMenuButton::OnCBResetContent (HWND hWnd)
{
	::DestroyMenu (fMenu);
	fMenu = ::CreatePopupMenu ();
	fMenuItem = -1;
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnCBGetCurSel
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

INT PopupMenuButton::OnCBGetCurSel (HWND hWnd)
{
	return fMenuItem;
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnCBSetCurSel
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

INT PopupMenuButton::OnCBSetCurSel (HWND hWnd, INT item)
{
	if (fMenuItem >= 0)
		::CheckMenuItem (fMenu, fMenuItem, MF_BYPOSITION | MF_UNCHECKED);

	int	numItems = ::GetMenuItemCount (fMenu);

	if (item >= 0 && item < numItems)
		fMenuItem = item;
	else
		fMenuItem = -1;

	if (fMenuItem >= 0)
		::CheckMenuItem (fMenu, fMenuItem, MF_BYPOSITION | MF_CHECKED);

	char	text[256] = {0};
	if (fMenuItem >= 0)
		::GetMenuString (fMenu, fMenuItem, text, sizeof (text), MF_BYPOSITION);
	::SetWindowText (hWnd, text);

	::InvalidateRect (hWnd, NULL, false);

	return fMenuItem >= 0 ? fMenuItem : CB_ERR;
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::OnCBSetCurSel
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

INT PopupMenuButton::OnCBSetItemData (HWND hWnd, INT item, LONG data)
{
	MENUITEMINFO	info;
	
	memset(&info, '\0', sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = MIIM_STATE;

	::GetMenuItemInfo (fMenu, item, TRUE, &info);

	if (data)
		info.fState |= MFS_ENABLED;
	else
		info.fState |= MFS_GRAYED;

	::SetMenuItemInfo (fMenu, item, TRUE, &info);

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	PopupMenuButton::WndProc
 *
 * DESCRIPTION:	Respond to the Windows messages appropriate for turning
 *				a static text dialog item into an active URL Hyperlink.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

#if 0
++ = implemented
+  = to be implemented

	#define CB_GETEDITSEL               0x0140
	#define CB_LIMITTEXT                0x0141
	#define CB_SETEDITSEL               0x0142
++	#define CB_ADDSTRING                0x0143
+	#define CB_DELETESTRING             0x0144
	#define CB_DIR                      0x0145
+	#define CB_GETCOUNT                 0x0146
++	#define CB_GETCURSEL                0x0147
	#define CB_GETLBTEXT                0x0148
	#define CB_GETLBTEXTLEN             0x0149
+	#define CB_INSERTSTRING             0x014A
++	#define CB_RESETCONTENT             0x014B
+	#define CB_FINDSTRING               0x014C
+	#define CB_SELECTSTRING             0x014D
++	#define CB_SETCURSEL                0x014E
	#define CB_SHOWDROPDOWN             0x014F
	#define CB_GETITEMDATA              0x0150
	#define CB_SETITEMDATA              0x0151
	#define CB_GETDROPPEDCONTROLRECT    0x0152
	#define CB_SETITEMHEIGHT            0x0153
	#define CB_GETITEMHEIGHT            0x0154
	#define CB_SETEXTENDEDUI            0x0155
	#define CB_GETEXTENDEDUI            0x0156
	#define CB_GETDROPPEDSTATE          0x0157
	#define CB_FINDSTRINGEXACT          0x0158
	#define CB_SETLOCALE                0x0159
	#define CB_GETLOCALE                0x015A
	#define CB_GETTOPINDEX              0x015b
	#define CB_SETTOPINDEX              0x015c
	#define CB_GETHORIZONTALEXTENT      0x015d
	#define CB_SETHORIZONTALEXTENT      0x015e
	#define CB_GETDROPPEDWIDTH          0x015f
	#define CB_SETDROPPEDWIDTH          0x0160
	#define CB_INITSTORAGE              0x0161

	#define CBN_ERRSPACE        (-1)
	#define CBN_SELCHANGE       1
	#define CBN_DBLCLK          2
	#define CBN_SETFOCUS        3
	#define CBN_KILLFOCUS       4
	#define CBN_EDITCHANGE      5
	#define CBN_EDITUPDATE      6
	#define CBN_DROPDOWN        7
	#define CBN_CLOSEUP         8
	#define CBN_SELENDOK        9
	#define CBN_SELENDCANCEL    10
#endif

LRESULT PopupMenuButton::WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG (hWnd, WM_PAINT, OnPaint);
		HANDLE_MSG (hWnd, WM_LBUTTONDOWN, OnLButtonDown);

		case CB_ADDSTRING:
			return this->OnCBAddString (hWnd, (LPCSTR) lParam);

		case CB_RESETCONTENT:
			return this->OnCBResetContent (hWnd), CB_OKAY;

		case CB_GETCURSEL:
			return this->OnCBGetCurSel (hWnd);

		case CB_SETCURSEL:
			return this->OnCBSetCurSel (hWnd, wParam);

		case CB_SETITEMDATA:
			return this->OnCBSetItemData (hWnd, wParam, lParam);
	}

	return ::CallWindowProc (fOldWndProc, hWnd, msg, wParam, lParam);
}


#pragma mark -

// ===========================================================================
//		¥ ControlCButton
// ===========================================================================

/***********************************************************************
 *
 * FUNCTION:	ControlCButton constructor
 *
 * DESCRIPTION:	Initialize our member data.
 *
 * PARAMETERS:	hWnd - window to subclass.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

ControlCButton::ControlCButton (HWND hWnd) :
	SubclassedWindow(hWnd)
{
}


/***********************************************************************
 *
 * FUNCTION:	ControlCButton destructor
 *
 * DESCRIPTION:	Release our resources.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

ControlCButton::~ControlCButton (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	ControlCButton::WndProc
 *
 * DESCRIPTION:	Look for Control-C messages.  If we find one, reflect
 *				it up to our parent.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

LRESULT ControlCButton::WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_GETDLGCODE:
			return DLGC_WANTCHARS;
			break;

		case WM_CHAR:
		{
			if (wParam == 3)	// Ctrl-C
			{
				return ::SendMessage (::GetParent (hWnd), msg, wParam, lParam);
			}
		}
	}

	return ::CallWindowProc (fOldWndProc, hWnd, msg, wParam, lParam);
}

#ifdef SONY_ROM
void EmDlg::ShowItem (EmDlgRef dlg, EmDlgItemID item, BOOL show)
{
	HWND	hDlg = (HWND) dlg;
	if (!hDlg)
		return;

	HWND	hWnd = ::GetDlgItem (hDlg, ::PrvFromDlgItemID (item));
	if (!hWnd)
		return;

	::ShowWindow (hWnd, (show) ? SW_SHOW : SW_HIDE);
}
#endif // SONY_ROM
