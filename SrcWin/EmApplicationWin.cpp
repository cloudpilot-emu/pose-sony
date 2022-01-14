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
#include "EmApplicationWin.h"

#include "ChunkFile.h"			// Chunk
#include "EmDlgWin.h"			// GetCurrentModelessDialog
#include "EmDocument.h"			// gDocument
#include "EmMenus.h"			// MenuInitialize
#include "EmMenusWin.h"			// HostCreateAcceleratorTable
#include "EmRPC.h"				// RPC::Idle
#include "EmSession.h"			// gSession
#include "EmTransportUSBWin.h"	// EmHostTransportUSB::Startup
#include "Hordes.h"				// gErrorHappened, gWarningHappened
#include "Miscellaneous.h"		// CommonStartup
#include "MMFMessaging.h"		// MMFOpenFiles, MMFCloseFiles
#include "PreferenceMgr.h"		// EmulatorPreferences
#include "Sounds.h"				// gSounds
#include "StartMenu.h"			// AddStartMenuItem
#include "Startup.h"			// AskWhatToDo

#include "resource.h"			// IDC_NEW, IDC_OPEN, IDC_DOWNLOAD, IDC_EXIT

#include <crtdbg.h>


EmApplicationWin*	gHostApplication;
HINSTANCE			gInstance;
WORD				gWinSockVersion;

// Platform-specific structure used to communicate with the platform's
// Binder tool. Communicates a device configuration for bound ROMs.

typedef struct
{
	char				Device[20];
	RAMSizeType			RAMSize;
}
DeviceConfig;

const char	kResourceType[]	= "BIND";
const char*	kROMID			= MAKEINTRESOURCE (1);
const char*	kPSFID			= MAKEINTRESOURCE (2);
const char*	kConfigID		= MAKEINTRESOURCE (3);
const char*	kSkinfoID		= MAKEINTRESOURCE (4);
const char*	kSkin1xID		= MAKEINTRESOURCE (5);
const char*	kSkin2xID		= MAKEINTRESOURCE (6);


static Bool PrvResourcePresent	(const char* name);
static Bool PrvGetResource		(const char* name, Chunk& rom);
static Bool PrvBindROM			(HANDLE hExecutable);
static Bool PrvBindPSF			(HANDLE hExecutable);
static Bool PrvBindSkin			(HANDLE hExecutable);
static Bool PrvBindConfig		(HANDLE hExecutable);
static Bool PrvBindFile			(HANDLE hExecutable,
								 const EmFileRef&,
								 const char* resourceName);

/***********************************************************************
 *
 * FUNCTION:	WinMain
 *
 * DESCRIPTION:	Application entry point.  Creates the preferences and
 *				then the application object.  Uses the application
 *				object to initizalize, run, and shutdown the system.
 *				A top-level exception handler is also installed in
 *				order to catch any wayward exceptions and report them
 *				to the user with a Fatal Internal Error message.
 *
 * PARAMETERS:	Standard WinMain parameters
 *
 * RETURNED:	Zero by default.  If a non-fatal error occurred, returns
 *				1.  If a fatal error occurred while running a Gremlin,
 *				returns 2.  This is handy for Palm's automated testing.
 *
 ***********************************************************************/

int WINAPI WinMain (HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	gInstance = instance;

/*
	Bit field					Default	Description
	---------------------------	-------	-------------------------------------------------
	_CRTDBG_ALLOC_MEM_DF		ON		ON	Enable debug heap allocations and use of
											memory block type identifiers, such as
											_CLIENT_BLOCK.
										OFF	Add new allocations to heap’s linked list,
											but set block type to _IGNORE_BLOCK.
	_CRTDBG_CHECK_ALWAYS_DF		OFF		ON	Call _CrtCheckMemory at every allocation
											and deallocation request.
										OFF	_CrtCheckMemory must be called explicitly.
	_CRTDBG_CHECK_CRT_DF		OFF		ON	Include _CRT_BLOCK types in leak detection
											and memory state difference operations.
										OFF	Memory used internally by the run-time
											library is ignored by these operations.
	_CRTDBG_DELAY_FREE_MEM_DF	OFF		ON	Keep freed memory blocks in the heap’s
											linked list, assign them the _FREE_BLOCK
											type, and fill them with the byte value 0xDD.
										OFF	Do not keep freed blocks in the heap’s
											linked list.
	_CRTDBG_LEAK_CHECK_DF		OFF		ON	Perform automatic leak checking at program
											exit via a call to _CrtDumpMemoryLeaks and
											generate an error report if the application
											failed to free all the memory it allocated.
										OFF	Do not automatically perform leak checking
											at program exit.
*/

	// Set the flag that says to report leaks when the application shuts down.
	// We perform the leak dumping this way instead of calling _CrtDumpMemoryLeaks
	// at the end of WinMain, as the former method calls _CrtDumpMemoryLeaks after
	// all static/global objects have been destructed.

	_CrtSetDbgFlag (_CRTDBG_LEAK_CHECK_DF);

	EmulatorPreferences		prefs;
	EmApplicationWin		theApp;

	try
	{
		// !!! Calling GetCommandLine and CommandLineToArgvW may be
		// more compatible...

		if (theApp.Startup (__argc, __argv))
		{
			theApp.Run ();
		}
	}
	catch (...)
	{
		::MessageBox (NULL, "Palm OS Emulator: Fatal Internal Error",
			"Fatal Error", MB_OK);
	}

	theApp.Shutdown ();

	return
		gErrorHappened ? 2 :
		gWarningHappened ? 1 : 0;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::EmApplicationWin
 *
 * DESCRIPTION:	Constructor.  Sets the global host application variable
 *				to point to us.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmApplicationWin::EmApplicationWin (void) :
	EmApplication (),
	fAppWindow (NULL)
{
	EmAssert (gHostApplication == NULL);
	gHostApplication = this;
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::~EmApplicationWin
 *
 * DESCRIPTION:	Destructor.  Closes our window and sets the host
 *				application variable to NULL.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmApplicationWin::~EmApplicationWin (void)
{
	EmAssert (gHostApplication == this);
	gHostApplication = NULL;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::Startup
 *
 * DESCRIPTION:	Performs one-time startup initialization.
 *
 * PARAMETERS:	None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

Bool EmApplicationWin::Startup (int argc, char** argv)
{
	//------------------------------------------------------------------
	// Test code forcing us to run on a single processor.
	//------------------------------------------------------------------

#if 0
	HANDLE	hProcess = ::GetCurrentProcess ();
	DWORD	processAffinityMask;
	DWORD	systemAffinityMask;

	::GetProcessAffinityMask (hProcess,
		&processAffinityMask, &systemAffinityMask);

	processAffinityMask &= systemAffinityMask;
	systemAffinityMask = 0x80000000;

	while (systemAffinityMask)
	{
		if (systemAffinityMask & processAffinityMask)
			break;

		systemAffinityMask >>= 1;
	}

	if (systemAffinityMask)
	{
		::SetProcessAffinityMask (hProcess, systemAffinityMask);
	}
#endif

	//------------------------------------------------------------------
	// Establish a ref for the emulator application.
	//------------------------------------------------------------------

	char	path[_MAX_PATH];
	::GetModuleFileName (gInstance, path, _MAX_PATH);
	EmFileRef::SetEmulatorRef (EmFileRef (path));

	// When run under the VC++ debugger, the default directory appears
	// to be set to S:\emulator\windows\source, instead of the expected
	// \Debug or \Release sub-directories.	Jam the current directory
	// to the app directory so that we can find our files.

	::SetCurrentDirectory (EmDirRef::GetEmulatorDirectory ().GetFullPath ().c_str ());

	//------------------------------------------------------------------
	// Start up the sub-systems.
	//------------------------------------------------------------------

	// Startup some crucial Windows stuff.

	::InitCommonControls ();

	this->PrvWinSockStartup ();

	// Cross-platform initialization.

	if (!EmApplication::Startup (argc, argv))
		return false;

	// More platform-specific initialization.

	::MenuInitialize (false);

	EmHostTransportUSB::Startup ();

	// Create our window that receives application-level commands

	this->PrvCreateWindow ();

	// Register our files types, but only if we're not a bound version
	// of Poser (we don't want any bound Posers to establish themselves
	// over any non-bound versions).

	if (!this->IsBound ())
	{
		this->PrvRegisterTypes ();
	}

	// Check up on our start menu shortcut situation.

	this->PrvProcessStartMenu ();

	return true;
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::Run
 *
 * DESCRIPTION:	Generally run the application.  For our application,
 *				this mostly means switching back and forth between our
 *				document/session window, and a dialog asking us to
 *				create or open such a window.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::Run (void)
{
	// Open or create any documents as needed.

	this->HandleStartupActions ();

	// While it's not time to quit...

	while (!this->GetTimeToQuit ())
	{
		// If a document does not exist, ask the user to create
		// or open one.

		if (!gDocument)
		{
			this->HandleStartupDialog ();
		}

		// If a document does exist, go into a loop to handle
		// events for it.  HandleMainEventLoop exits when the
		// document closes.

		else
		{
			this->HandleMainEventLoop ();
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::Shutdown
 *
 * DESCRIPTION:	Performs one-time shutdown operations.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::Shutdown (void)
{
	// Reverse the steps we took in Startup.

	if (fAppWindow)
	{
		::DestroyWindow (fAppWindow);
		fAppWindow = NULL;
	}

	EmHostTransportUSB::Shutdown ();

	EmApplication::Shutdown ();

	::WSACleanup ();
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::HandleStartupDialog
 *
 * DESCRIPTION:	Present and handle the dialog asking the user to create
 *				a new session, open an old session, download a ROM, or
 *				quit the application altogether.
 *
 *				If the application is partially bound, we automatically
 *				try to create a new session from the bound configuration
 *				information.  If the application is fully bound, we
 *				automatically try to open the bound session.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::HandleStartupDialog (void)
{
	int command;

	if (this->IsBoundPartially ())
	{
		command = IDC_NEW;
	}
	else if (this->IsBoundFully ())
	{
		command = IDC_OPEN;
	}
	else
	{
		command = this->PrvStartupScreen ();
	}

	switch (command)
	{
		case IDC_NEW:
			this->HandleCommand (kCommandSessionNew);
			break;

		case IDC_OPEN:
			this->HandleCommand (kCommandSessionOpenOther);
			break;

		case IDC_DOWNLOAD:
			this->HandleCommand (kCommandDownloadROM);
			break;

		case IDC_EXIT:
		case IDCANCEL:
			this->HandleCommand (kCommandQuit);
			break;
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::HandleMainEventLoop
 *
 * DESCRIPTION:	Our main event loop.  Retrieve and handle events until
 *				the application quits.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::HandleMainEventLoop (void)
{
	// All of this stuff should probably be in some EmDocumentWin
	// Startup method.

	{
		// Open the connection to the external debugger.  We used to do
		// this from Platform::ConnectToDebugger, but that would get
		// called from a different thread.  Because the window used for
		// communications would belong to a different thread, the message
		// loop below would not process messages for it.  So we do it here.

		::MMFOpenFiles (gInstance, CMMFSocket::HandlePacketCB, 0,
						kFromEmulatorFileName, kToEmulatorFileName);

		// Create SoundMaker object.

		EmAssert (gSounds == NULL);
		gSounds = new WinSounds;
	}

	// Get a reference to our menu, and create an accelerator
	// table from it.  We use this accelerator table when calling
	// TranslateAccelerator.

	EmMenuItemList*	menu = ::MenuFindMenu (kMenuPopupMenuPreferred);
	EmAssert (menu);

	HACCEL	hAccel = ::HostCreateAcceleratorTable (*menu);

	// Our main event loop.  Retrieve and handle events until the application quits.

	MSG msg;
	while (::GetMessage (&msg, NULL, 0, 0))
	{
#if defined (_DEBUG) && defined (TRACE_MSG)
		_AfxTraceMsg ("HandleMainEventLoop", &msg);
#endif

		// Check for accelerator keystrokes.

		if (!::TranslateAccelerator (msg.hwnd, hAccel, &msg))
		{
			// Check for dialog events.
			//
			// Note: most Windows documentation and even an MSDN C++ Q&A
			// article says that IsDialogMessage must be called for all open
			// dialogs.  However, a Knowledgebase article (Q71450 -- "HOWTO:
			// Use One IsDialogMessage() Call for Many Modeless Dialogs")
			// shows how to avoid doing this.

			HWND	dlg = ::GetCurrentModelessDialog ();

			if (dlg == NULL || !::IsDialogMessage (dlg, &msg))
			{
				::TranslateMessage (&msg);
				::DispatchMessage (&msg);
			}
		}
	}

	::DestroyAcceleratorTable (hAccel);

	// All of this stuff should probably be in some EmDocumentWin
	// Shutdown method.

	{
		::MMFCloseFiles (gInstance);

		delete gSounds;
		gSounds = NULL;
	}
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::BindPoser
 *
 * DESCRIPTION:	Create a bound version of Poser.
 *
 * PARAMETERS:	fullSave - TRUE if the entire state should be bound
 *					with the new executable.  FALSE if only the
 *					configuration should be saved.
 *
 *				dest - spec for the bound Poser.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::BindPoser (Bool fullSave, const EmFileRef& dest)
{
	EmFileRef	poser = EmFileRef::GetEmulatorRef ();

	if (!::CopyFile (	poser.GetFullPath ().c_str (),
						dest.GetFullPath ().c_str (),
						false /* == don't fail if exists */))
	{
		char	buffer[200];
		sprintf (buffer, "Unable to copy %s to %s.",
			poser.GetFullPath ().c_str (), dest.GetFullPath ().c_str ());
		::MessageBox (NULL, buffer, "Error", MB_ICONERROR | MB_OK);
		return;
	}

	Bool	succeeded = false;
	HANDLE	hExecutable = ::BeginUpdateResource (dest.GetFullPath ().c_str (),
		false /* == don't delete existing resources */);

	if (hExecutable)
	{
		if (::PrvBindROM (hExecutable) &&
			::PrvBindConfig (hExecutable) &&
			::PrvBindSkin (hExecutable))
		{
			if (fullSave && ::PrvBindPSF (hExecutable))
			{
				succeeded = true;
			}
			else
			{
				succeeded = true;
			}
		}

		::EndUpdateResource (hExecutable, false /* == commit changes */);
	}

	if (!succeeded)
	{
		dest.Delete ();

		char	buffer[200];
		sprintf (buffer, "Unable to attach necessary resources to %s.",
			dest.GetFullPath ().c_str ());
		::MessageBox (NULL, buffer, "Error", MB_ICONERROR | MB_OK);
	}
}


Bool EmApplicationWin::ROMResourcePresent (void)
{
	return ::PrvResourcePresent (kROMID);
}


Bool EmApplicationWin::GetROMResource (Chunk& chunk)
{
	return ::PrvGetResource (kROMID, chunk);
}


Bool EmApplicationWin::PSFResourcePresent (void)
{
	return ::PrvResourcePresent (kPSFID);
}


Bool EmApplicationWin::GetPSFResource (Chunk& chunk)
{
	return ::PrvGetResource (kPSFID, chunk);
}


Bool EmApplicationWin::ConfigResourcePresent (void)
{
	return ::PrvResourcePresent (kConfigID);
}


Bool EmApplicationWin::GetConfigResource (Chunk& chunk)
{
	return ::PrvGetResource (kConfigID, chunk);
}


Bool EmApplicationWin::SkinfoResourcePresent (void)
{
	return ::PrvResourcePresent (kSkinfoID);
}


Bool EmApplicationWin::GetSkinfoResource (Chunk& chunk)
{
	return ::PrvGetResource (kSkinfoID, chunk);
}


Bool EmApplicationWin::Skin1xResourcePresent (void)
{
	return ::PrvResourcePresent (kSkin1xID);
}


Bool EmApplicationWin::GetSkin1xResource (Chunk& chunk)
{
	return ::PrvGetResource (kSkin1xID, chunk);
}


Bool EmApplicationWin::Skin2xResourcePresent (void)
{
	return ::PrvResourcePresent (kSkin2xID);
}


Bool EmApplicationWin::GetSkin2xResource (Chunk& chunk)
{
	return ::PrvGetResource (kSkin2xID, chunk);
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::GetBoundDevice
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The Device Type in the bound configuration resource
 *
 ***********************************************************************/

EmDevice EmApplicationWin::GetBoundDevice ()
{
	Chunk	data;

	if (!this->GetConfigResource (data))
		EmAssert (false);

	EmAssert (data.GetLength () >= sizeof (DeviceConfig));

	const char*	idName = ((DeviceConfig*) data.GetPointer ())->Device;
	return EmDevice (idName);
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::GetBoundRAMSize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The RAM size in the bound configuration resource
 *
 ***********************************************************************/

RAMSizeType EmApplicationWin::GetBoundRAMSize (void)
{
	Chunk	data;

	if (!this->GetConfigResource (data))
		EmAssert (false);

	EmAssert (data.GetLength () >= sizeof (DeviceConfig));

	RAMSizeType	ramSize = ((DeviceConfig*) data.GetPointer ())->RAMSize;
	return ramSize;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvCreateWindow
 *
 * DESCRIPTION:	Create the window used by the application to receive
 *				messages.  This is a messages-only window; we never
 *				display it to the user.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::PrvCreateWindow (void)
{
	DWORD		result;
	WNDCLASS	wc = {0};

	wc.lpfnWndProc		= &EmApplicationWin::PrvWndProc;
	wc.lpszClassName	= "Palm OS Emulator Application Window";
	wc.hInstance		= gInstance;

	if (::RegisterClass (&wc) == 0)
	{
		EmAssert (false);
		result = ::GetLastError ();
	}

	// Create the emulator window and show it.

	fAppWindow = ::CreateWindow (	"Palm OS Emulator Application Window",
									"Palm OS Emulator Application Window",
									0,			// style
									0, 0,		// position
									0, 0,		// size
									NULL,		// parent
									NULL,		// menu
									gInstance,
									NULL);		// window creation data

	if (fAppWindow == NULL)
	{
		EmAssert (false);
		result = ::GetLastError ();
	}

	// Set a timer so that we periodically handle idle-time events.

	::SetTimer (fAppWindow, 1, 100, NULL);
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvWndProc
 *
 * DESCRIPTION:	Dispatch the command to the right handler via the
 *				message cracker macros.
 *
 * PARAMETERS:	Standard Window DefProc parameters.
 *
 * RETURNED:	Result code specific to the command.
 *
 ***********************************************************************/

LRESULT CALLBACK EmApplicationWin::PrvWndProc (HWND hWnd,
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

	_AfxTraceMsg ("PrvWndProc", &_msg);
#endif

	// Redefine HANDLE_MSG to know about gHostApplication.

	#undef HANDLE_MSG
	#define HANDLE_MSG(hwnd, message, fn)	\
		case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (gHostApplication->fn))

	EmAssert (gHostApplication);

	switch (msg)
	{
		HANDLE_MSG (hWnd, WM_COMMAND,			PrvOnCommand);
		HANDLE_MSG (hWnd, WM_DROPFILES,			PrvOnDropFiles);
		HANDLE_MSG (hWnd, WM_TIMER,				PrvOnTimer);
	}

	return ::DefWindowProc (hWnd, msg, wParam, lParam);
}


/***********************************************************************
 *
 * FUNCTION:    EmApplicationWin::PrvOnCommand
 *
 * DESCRIPTION: WM_COMMAND handler.
 *
 *				The WM_COMMAND message is sent when the user selects a
 *				command item from a menu, when a control sends a
 *				notification message to its parent window, or when an
 *				accelerator keystroke is translated.
 *
 *				We handle this message by passing it to the cross-
 *				platform implementation.
 *
 * PARAMETERS:  Standard WM_COMMAND parameters:
 *
 *				hWnd - handle to window.
 *
 *				id - the identifier of the menu item, control, or
 *					accelerator.
 *
 *				hwndCtl - Handle to the control sending the message if
 *					the message is from a control. Otherwise, this
 *					parameter is NULL.
 *
 *				codeNotify - the notification code if the message is
 *					from a control. If the message is from an
 *					accelerator, this value is 1. If the message is from
 *					a menu, this value is zero.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmApplicationWin::PrvOnCommand (HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (hwndCtl);
	UNUSED_PARAM (codeNotify);

	this->HandleCommand ((EmCommandID) id);
}


/***********************************************************************
 *
 * FUNCTION:    EmApplicationWin::PrvOnDropFiles
 *
 * DESCRIPTION: WM_DROPFILES handler.
 *
 *				Sent when the user drops a file on the window of an
 *				application that has registered itself as a recipient
 *				of dropped files.
 *
 *				We handle this message by collecting the list of
 *				dropped files and passing them on to the cross-platform
 *				implementation.
 *
 * PARAMETERS:  Standard WM_DROPFILES parameters:
 *
 *				hWnd - handle to window.
 *
 *				hdrop - Handle to an internal structure describing the
 *					dropped files. Pass this handle DragFinish,
 *					DragQueryFile, or DragQueryPoint to retrieve
 *					information about the dropped files.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmApplicationWin::PrvOnDropFiles (HWND hWnd, HDROP hdrop)
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

	this->HandleFileList (fileList);
}


/***********************************************************************
 *
 * FUNCTION:    EmApplicationWin::PrvOnTimer
 *
 * DESCRIPTION: WM_TIMER handler.
 *
 *				The WM_TIMER message is posted to the installing
 *				thread's message queue when a timer expires. The message
 *				is posted by the GetMessage or PeekMessage function.
 *
 *				We handle this message by performing some idle time
 *				activities, and then forwarding the message to the
 *				cross-platform implementation.
 *
 * PARAMETERS:  Standard WM_TIMER parameters:
 *
 *				hWnd - handle to window.
 *
 *				id - the timer identifier.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmApplicationWin::PrvOnTimer (HWND hWnd, UINT id)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (id);

	this->HandleIdle ();
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvProcessStartMenu
 *
 * DESCRIPTION:	If we haven't already asked on this system, or if the
 *				shortcut has disappeared, ask if the user would like
 *				to add us to their start menu.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void EmApplicationWin::PrvProcessStartMenu (void)
{
	if (this->PrvAskAboutStartMenu ())
	{
		// Needed for "Select Directory" dialog box.

		::CoInitialize(NULL);

		::DialogBox (
			gInstance,
			MAKEINTRESOURCE (IDD_STARTMENU),
			NULL,								// Parent window
			&EmApplicationWin::PrvStartMenuDlgProc);
	
		::CoUninitialize();
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvAskAboutStartMenu
 *
 * DESCRIPTION:	Decides if the user should be prompted about the start
 *				menu
 *
 * PARAMETERS:	None
 *
 * RETURNED:	TRUE if Emulator should ask about the start menu
 *				FALSE if not
 *
 ***********************************************************************/

Bool EmApplicationWin::PrvAskAboutStartMenu (void)
{
	Preference<Bool>		userWants (kPrefKeyAskAboutStartMenu);
	Preference<EmFileRef>	link (kPrefKeyStartMenuItem);
	
	// We should ask about the start menu if all of the following are true:
	// 1. The user wants us to ask about it.
	// 2. The link does not already exist.

	return (*userWants && !(*link).Exists ());
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvStartMenuDlgProc
 *
 * DESCRIPTION:	Start menu dialog processing
 *
 * PARAMETERS:	Standard Windows dialog function
 *
 * RETURNED:	Stardard Windows dialog function
 *
 ***********************************************************************/

BOOL CALLBACK EmApplicationWin::PrvStartMenuDlgProc (HWND hWnd, UINT uMsg,
													 WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			Preference<Bool>	userWantsPref (kPrefKeyAskAboutStartMenu);
			
			::CenterWindow(hWnd);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (GET_WM_COMMAND_ID (wParam, lParam))
			{
				case IDYES:
				{
					char appPath[_MAX_PATH];
					char linkPath[_MAX_PATH];

					::GetModuleFileName (gInstance, appPath, _MAX_PATH);

					if (::AddStartMenuItem (hWnd, gInstance, appPath, linkPath))
					{
						Preference<Bool>	userWantsPref (kPrefKeyAskAboutStartMenu);
						userWantsPref = true;

						EmFileRef				link (linkPath);
						Preference<EmFileRef>	linkPref (kPrefKeyStartMenuItem);
						linkPref = link.ToPrefString ();

						::EndDialog (hWnd, 0);
					}
					break;
				}

				case IDNO:
				{
					Preference<Bool>	userWantsPref (kPrefKeyAskAboutStartMenu);
					userWantsPref = false;

					EmFileRef				link;
					Preference<EmFileRef>	linkPref (kPrefKeyStartMenuItem);
					linkPref = link;

					::EndDialog (hWnd, 0);

					break;
				}
			}

			return TRUE;
		}
	}

	return FALSE;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvStartupScreen
 *
 * DESCRIPTION:	Presents the user with the New/Open/etc dialog.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Button the user clicked on.
 *
 ***********************************************************************/

int EmApplicationWin::PrvStartupScreen (void)
{
	EmAssert (!this->IsBound ());

	int	choice = ::DialogBox (	gInstance,
								MAKEINTRESOURCE (IDD_STARTUP),
								NULL,	// hwndParent
								&EmApplicationWin::PrvStartupDlgProc);

	return choice;
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvStartupDlgProc
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

BOOL CALLBACK EmApplicationWin::PrvStartupDlgProc (HWND hwnd, UINT msg,
												   WPARAM wparam, LPARAM lparam)
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


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvRegisterTypes
 *
 * DESCRIPTION:	Register our file types and icons.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EmRegistryInfo
{
	const char*		fExtension;		// File's extension (e.g., ".foo").

	const char*		fRegistryRoot;	// File class registry Root (e.g.,
									// "MyApp.FooDoc").

	const char*		fFileType;		// Text for "Type" column in Explorer
									// (e.g., "MyApp Foo Document").

	int				fIconID;		// Icon for this file (specified as
									// resource ID, not resource index).

	const char*		fOpenCommand;	// Open command template (use %s to
									// hold .exe name, and be sure to
									// provide all correct quoting).  Can
									// be NULL if double-clicking on this
									// document does nothing.  !!! May want
									// to take advantage of "NoOpen" Registry
									// feature.
};

EmRegistryInfo	kRegistryInfo [] = 
{
	{
		".psf",
		"PalmEmulator.Session",
		"Palm Emulator Session",
		IDI_DOCUMENT,
		"\"%s\" -psf \"%%1\""
	},
	{
		".rom",
		"PalmEmulator.ROM",
		"Palm Emulator ROM",
		IDI_DOCUMENT,
		"\"%s\" -rom \"%%1\""
	},
	{
		".pev",
		"PalmEmulator.Events",
		"Palm Emulator Events",
		IDI_DOCUMENT,
		NULL
	}
};


void EmApplicationWin::PrvRegisterTypes (void)
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

	// Buffers to hold the registry path and the value to store at that path.

	char	key[256];
	char	value[256];

	// Get the current application's name, so that we can tell the registry
	// where to get icon information and what application to launch.

	char	moduleName[_MAX_PATH];
	::GetModuleFileName (gInstance, moduleName, _MAX_PATH);

	for (int ii = 0; ii < countof (kRegistryInfo); ++ii)
	{
		EmRegistryInfo*	info = &kRegistryInfo[ii];

		// Set up a registry entry to map from the file's extension to
		// the registry data about this file.

		this->PrvSetRegKey (info->fExtension, info->fRegistryRoot);

		// Set the text to show under the "Type" column in Windows Explorer
		// for files of this type.

		this->PrvSetRegKey (info->fRegistryRoot, info->fFileType);

		// If there is an icon for this file type, store that in the registry.

		if (info->fIconID)
		{
			strcpy (key, info->fRegistryRoot);
			strcat (key, "\\DefaultIcon");

			// Reference the icon with a negative number to indicate that
			// we are specifying it by resource ID and not index number.

			sprintf (value, "%s,-%d", moduleName, info->fIconID);

			this->PrvSetRegKey (key, value);
		}

		// If double-clicking on this file should execute a command, store
		// that in the registry.

		if (info->fOpenCommand)
		{
			strcpy (key, info->fRegistryRoot);
			strcat (key, "\\shell\\open\\command");

			sprintf (value, info->fOpenCommand, moduleName);

			this->PrvSetRegKey (key, value);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvSetRegKey
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

Bool EmApplicationWin::PrvSetRegKey (LPCTSTR lpszKey, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	if (lpszValueName == NULL)
	{
		if (::RegSetValue ( HKEY_CLASSES_ROOT,
							lpszKey, REG_SZ,
							lpszValue,
							lstrlen (lpszValue) * sizeof (TCHAR)) != ERROR_SUCCESS)
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
											(lstrlen (lpszValue) + 1) * sizeof (TCHAR));

		if (::RegCloseKey (hKey) == ERROR_SUCCESS && lResult == ERROR_SUCCESS)
		{
			return TRUE;
		}
	}

	return FALSE;

}


/***********************************************************************
 *
 * FUNCTION:	EmApplicationWin::PrvWinSockStartup
 *
 * DESCRIPTION:	DESCRIPTION
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmApplicationWin::PrvWinSockStartup (void)
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

	int result = ::WSAStartup (versionRequested, &wsaData);

	if (result == NO_ERROR)
	{
		gWinSockVersion = wsaData.wVersion;
	}
	else
	{
		gWinSockVersion = 0;
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


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	PrvResourcePresent
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:	
 *
 * RETURNED:	
 *
 ***********************************************************************/

Bool PrvResourcePresent (const char* name)
{
	HRSRC hRsrcLoc = ::FindResourceEx ( NULL, kResourceType, name,
										MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL));
	return (hRsrcLoc != NULL);
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetResource
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:	
 *
 * RETURNED:	
 *
 ***********************************************************************/

Bool PrvGetResource (const char* name, Chunk& rom)
{
	HRSRC hRsrcLoc = ::FindResourceEx ( NULL, kResourceType, name,
										MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL));

	HGLOBAL hResData = ::LoadResource (NULL, hRsrcLoc);

	if (hResData == NULL)
	{
		return false;
	}

	LPVOID	data = ::LockResource (hResData);
	DWORD	size = ::SizeofResource (NULL, hRsrcLoc);

	rom.SetLength (size);
	memcpy (rom.GetPointer (), data, size);
  
	return true;
}


/***********************************************************************
 *
 * FUNCTION:	PrvBindFile
 *
 * DESCRIPTION: Does the work of binding a file as a resource into an
 *				executable file.
 *
 * PARAMETERS:	hExecutable - Handle from BeginUpdateResource
 *				fileName - file name of file containing resource
 *					information
 *				resourceType - name of resource type
 *				resourceName - name of resource
 *
 * RETURNED:	TRUE if resource bound successfully
 *				FALSE otherwise
 *
 ***********************************************************************/

Bool PrvBindFile (HANDLE hExecutable, const EmFileRef& file,
					const char* resourceName)
{
	if (!file.Exists ())
		return false;

	Chunk	contents;
	::GetFileContents (file, contents);

	int32	iSize = contents.GetLength ();
	void*	pData = contents.GetPointer ();

	if (!pData || !::UpdateResource (hExecutable, kResourceType, resourceName,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pData, iSize))
	{
		return false;
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:	PrvBindROM
 *
 * DESCRIPTION: Does the work of binding a ROM file into an
 *				executable file.
 *
 * PARAMETERS:	hExecutable - Handle from BeginUpdateResource
 *
 * RETURNED:	TRUE if resource bound successfully
 *				FALSE otherwise
 *
 ***********************************************************************/

Bool PrvBindROM (HANDLE hExecutable)
{
	EmAssert (gSession);
	Configuration	cfg = gSession->GetConfiguration ();

	return ::PrvBindFile (hExecutable, cfg.fROMFile, kROMID);
}


/***********************************************************************
 *
 * FUNCTION:	PrvBindPSF
 *
 * DESCRIPTION: Does the work of binding a PSF file into an
 *				executable file.
 *
 * PARAMETERS:	hExecutable - Handle from BeginUpdateResource
 *
 * RETURNED:	TRUE if resource bound successfully
 *				FALSE otherwise
 *
 ***********************************************************************/

Bool PrvBindPSF (HANDLE hExecutable)
{
	// !!! Save the current session to a file so that we can read its
	// contents.  Later, we need a version of Emulator::Save that takes
	// a stream.  That way, we can call it with a RAM-base stream.

	EmFileRef	tempFile (tmpnam (NULL));

	// Suspend the session so that we can save its data.

	EmAssert (gSession);

	EmSessionStopper	stopper (gSession, kStopNow);

	gSession->Save (tempFile, false);

	Bool	result = ::PrvBindFile (hExecutable, tempFile, kPSFID);

	tempFile.Delete ();

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	PrvBindSkin
 *
 * DESCRIPTION: Does the work of binding a Skin file into an
 *				executable file.
 *
 * PARAMETERS:	hExecutable - Handle from BeginUpdateResource
 *
 * RETURNED:	TRUE if resource bound successfully
 *				FALSE otherwise
 *
 ***********************************************************************/

Bool PrvBindSkin (HANDLE hExecutable)
{
	EmFileRef	skinfoFile = ::SkinGetSkinfoFile ();
	EmFileRef	skin1xFile = ::SkinGetSkinFile (1);
	EmFileRef	skin2xFile = ::SkinGetSkinFile (2);

	if (skinfoFile.Exists () && skin1xFile.Exists () && skin2xFile.Exists ())
	{
		if (!::PrvBindFile (hExecutable, skinfoFile, kSkinfoID))
			return false;

		if (!::PrvBindFile (hExecutable, skin1xFile, kSkin1xID))
			return false;

		if (!::PrvBindFile (hExecutable, skin2xFile, kSkin2xID))
			return false;
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:	PrvBindConfig
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:	hExecutable - Handle from BeginUpdateResource
 *
 * RETURNED:	TRUE if resource bound successfully
 *				FALSE otherwise
 *
 ***********************************************************************/

Bool PrvBindConfig (HANDLE hExecutable)
{
	EmAssert (gSession);
	Configuration	cfg = gSession->GetConfiguration ();

	DeviceConfig	rc;
	string			idString (cfg.fDevice.GetIDString ());

	strcpy (rc.Device, idString.c_str ());
	rc.RAMSize = cfg.fRAMSize;

	return (::UpdateResource (hExecutable, kResourceType, kConfigID,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
				&rc, sizeof (DeviceConfig)) != 0);
}



/////////////////////////////////////////////////////////////////////////////
//	Pretty much the contents of AFXTRACE.CPP
/////////////////////////////////////////////////////////////////////////////
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#if defined (_DEBUG) && defined (TRACE_MSG)

#include "dde.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Build data tables by including data file three times

struct AFX_MAP_MESSAGE
{
	UINT    nMsg;
	LPCSTR  lpszMsg;
};

#define DEFINE_MESSAGE(wm)  { wm, #wm }

#define WM_MOUSEWHEEL                   0x020A
#define ASSERT	EmAssert
#define _countof countof
#define __T(x)      x
#define _T(x)       __T(x)
#define AfxTrace	LogAppendMsg

// The following trace macros are provided for backward compatiblity
//  (they also take a fixed number of parameters which provides
//   some amount of extra error checking)
#define TRACE0(sz)              AfxTrace(_T("%s"), _T(sz))
#define TRACE1(sz, p1)          AfxTrace(_T(sz), p1)
#define TRACE2(sz, p1, p2)      AfxTrace(_T(sz), p1, p2)
#define TRACE3(sz, p1, p2, p3)  AfxTrace(_T(sz), p1, p2, p3)


static const AFX_MAP_MESSAGE allMessages[] =
{
	DEFINE_MESSAGE(WM_CREATE),
	DEFINE_MESSAGE(WM_DESTROY),
	DEFINE_MESSAGE(WM_MOVE),
	DEFINE_MESSAGE(WM_SIZE),
	DEFINE_MESSAGE(WM_ACTIVATE),
	DEFINE_MESSAGE(WM_SETFOCUS),
	DEFINE_MESSAGE(WM_KILLFOCUS),
	DEFINE_MESSAGE(WM_ENABLE),
	DEFINE_MESSAGE(WM_SETREDRAW),
	DEFINE_MESSAGE(WM_SETTEXT),
	DEFINE_MESSAGE(WM_GETTEXT),
	DEFINE_MESSAGE(WM_GETTEXTLENGTH),
	DEFINE_MESSAGE(WM_PAINT),
	DEFINE_MESSAGE(WM_CLOSE),
	DEFINE_MESSAGE(WM_QUERYENDSESSION),
	DEFINE_MESSAGE(WM_QUIT),
	DEFINE_MESSAGE(WM_QUERYOPEN),
	DEFINE_MESSAGE(WM_ERASEBKGND),
	DEFINE_MESSAGE(WM_SYSCOLORCHANGE),
	DEFINE_MESSAGE(WM_ENDSESSION),
	DEFINE_MESSAGE(WM_SHOWWINDOW),
	DEFINE_MESSAGE(WM_CTLCOLORMSGBOX),
	DEFINE_MESSAGE(WM_CTLCOLOREDIT),
	DEFINE_MESSAGE(WM_CTLCOLORLISTBOX),
	DEFINE_MESSAGE(WM_CTLCOLORBTN),
	DEFINE_MESSAGE(WM_CTLCOLORDLG),
	DEFINE_MESSAGE(WM_CTLCOLORSCROLLBAR),
	DEFINE_MESSAGE(WM_CTLCOLORSTATIC),
	DEFINE_MESSAGE(WM_WININICHANGE),
	DEFINE_MESSAGE(WM_SETTINGCHANGE),
	DEFINE_MESSAGE(WM_DEVMODECHANGE),
	DEFINE_MESSAGE(WM_ACTIVATEAPP),
	DEFINE_MESSAGE(WM_FONTCHANGE),
	DEFINE_MESSAGE(WM_TIMECHANGE),
	DEFINE_MESSAGE(WM_CANCELMODE),
	DEFINE_MESSAGE(WM_SETCURSOR),
	DEFINE_MESSAGE(WM_MOUSEACTIVATE),
	DEFINE_MESSAGE(WM_CHILDACTIVATE),
	DEFINE_MESSAGE(WM_QUEUESYNC),
	DEFINE_MESSAGE(WM_GETMINMAXINFO),
	DEFINE_MESSAGE(WM_ICONERASEBKGND),
	DEFINE_MESSAGE(WM_NEXTDLGCTL),
	DEFINE_MESSAGE(WM_SPOOLERSTATUS),
	DEFINE_MESSAGE(WM_DRAWITEM),
	DEFINE_MESSAGE(WM_MEASUREITEM),
	DEFINE_MESSAGE(WM_DELETEITEM),
	DEFINE_MESSAGE(WM_VKEYTOITEM),
	DEFINE_MESSAGE(WM_CHARTOITEM),
	DEFINE_MESSAGE(WM_SETFONT),
	DEFINE_MESSAGE(WM_GETFONT),
	DEFINE_MESSAGE(WM_QUERYDRAGICON),
	DEFINE_MESSAGE(WM_COMPAREITEM),
	DEFINE_MESSAGE(WM_COMPACTING),
	DEFINE_MESSAGE(WM_NCCREATE),
	DEFINE_MESSAGE(WM_NCDESTROY),
	DEFINE_MESSAGE(WM_NCCALCSIZE),
	DEFINE_MESSAGE(WM_NCHITTEST),
	DEFINE_MESSAGE(WM_NCPAINT),
	DEFINE_MESSAGE(WM_NCACTIVATE),
	DEFINE_MESSAGE(WM_GETDLGCODE),
	DEFINE_MESSAGE(WM_NCMOUSEMOVE),
	DEFINE_MESSAGE(WM_NCLBUTTONDOWN),
	DEFINE_MESSAGE(WM_NCLBUTTONUP),
	DEFINE_MESSAGE(WM_NCLBUTTONDBLCLK),
	DEFINE_MESSAGE(WM_NCRBUTTONDOWN),
	DEFINE_MESSAGE(WM_NCRBUTTONUP),
	DEFINE_MESSAGE(WM_NCRBUTTONDBLCLK),
	DEFINE_MESSAGE(WM_NCMBUTTONDOWN),
	DEFINE_MESSAGE(WM_NCMBUTTONUP),
	DEFINE_MESSAGE(WM_NCMBUTTONDBLCLK),
	DEFINE_MESSAGE(WM_KEYDOWN),
	DEFINE_MESSAGE(WM_KEYUP),
	DEFINE_MESSAGE(WM_CHAR),
	DEFINE_MESSAGE(WM_DEADCHAR),
	DEFINE_MESSAGE(WM_SYSKEYDOWN),
	DEFINE_MESSAGE(WM_SYSKEYUP),
	DEFINE_MESSAGE(WM_SYSCHAR),
	DEFINE_MESSAGE(WM_SYSDEADCHAR),
	DEFINE_MESSAGE(WM_KEYLAST),
	DEFINE_MESSAGE(WM_INITDIALOG),
	DEFINE_MESSAGE(WM_COMMAND),
	DEFINE_MESSAGE(WM_SYSCOMMAND),
	DEFINE_MESSAGE(WM_TIMER),
	DEFINE_MESSAGE(WM_HSCROLL),
	DEFINE_MESSAGE(WM_VSCROLL),
	DEFINE_MESSAGE(WM_INITMENU),
	DEFINE_MESSAGE(WM_INITMENUPOPUP),
	DEFINE_MESSAGE(WM_MENUSELECT),
	DEFINE_MESSAGE(WM_MENUCHAR),
	DEFINE_MESSAGE(WM_ENTERIDLE),
	DEFINE_MESSAGE(WM_MOUSEWHEEL),
	DEFINE_MESSAGE(WM_MOUSEMOVE),
	DEFINE_MESSAGE(WM_LBUTTONDOWN),
	DEFINE_MESSAGE(WM_LBUTTONUP),
	DEFINE_MESSAGE(WM_LBUTTONDBLCLK),
	DEFINE_MESSAGE(WM_RBUTTONDOWN),
	DEFINE_MESSAGE(WM_RBUTTONUP),
	DEFINE_MESSAGE(WM_RBUTTONDBLCLK),
	DEFINE_MESSAGE(WM_MBUTTONDOWN),
	DEFINE_MESSAGE(WM_MBUTTONUP),
	DEFINE_MESSAGE(WM_MBUTTONDBLCLK),
	DEFINE_MESSAGE(WM_PARENTNOTIFY),
	DEFINE_MESSAGE(WM_MDICREATE),
	DEFINE_MESSAGE(WM_MDIDESTROY),
	DEFINE_MESSAGE(WM_MDIACTIVATE),
	DEFINE_MESSAGE(WM_MDIRESTORE),
	DEFINE_MESSAGE(WM_MDINEXT),
	DEFINE_MESSAGE(WM_MDIMAXIMIZE),
	DEFINE_MESSAGE(WM_MDITILE),
	DEFINE_MESSAGE(WM_MDICASCADE),
	DEFINE_MESSAGE(WM_MDIICONARRANGE),
	DEFINE_MESSAGE(WM_MDIGETACTIVE),
	DEFINE_MESSAGE(WM_MDISETMENU),
	DEFINE_MESSAGE(WM_CUT),
	DEFINE_MESSAGE(WM_COPYDATA),
	DEFINE_MESSAGE(WM_COPY),
	DEFINE_MESSAGE(WM_PASTE),
	DEFINE_MESSAGE(WM_CLEAR),
	DEFINE_MESSAGE(WM_UNDO),
	DEFINE_MESSAGE(WM_RENDERFORMAT),
	DEFINE_MESSAGE(WM_RENDERALLFORMATS),
	DEFINE_MESSAGE(WM_DESTROYCLIPBOARD),
	DEFINE_MESSAGE(WM_DRAWCLIPBOARD),
	DEFINE_MESSAGE(WM_PAINTCLIPBOARD),
	DEFINE_MESSAGE(WM_VSCROLLCLIPBOARD),
	DEFINE_MESSAGE(WM_SIZECLIPBOARD),
	DEFINE_MESSAGE(WM_ASKCBFORMATNAME),
	DEFINE_MESSAGE(WM_CHANGECBCHAIN),
	DEFINE_MESSAGE(WM_HSCROLLCLIPBOARD),
	DEFINE_MESSAGE(WM_QUERYNEWPALETTE),
	DEFINE_MESSAGE(WM_PALETTEISCHANGING),
	DEFINE_MESSAGE(WM_PALETTECHANGED),
	DEFINE_MESSAGE(WM_DDE_INITIATE),
	DEFINE_MESSAGE(WM_DDE_TERMINATE),
	DEFINE_MESSAGE(WM_DDE_ADVISE),
	DEFINE_MESSAGE(WM_DDE_UNADVISE),
	DEFINE_MESSAGE(WM_DDE_ACK),
	DEFINE_MESSAGE(WM_DDE_DATA),
	DEFINE_MESSAGE(WM_DDE_REQUEST),
	DEFINE_MESSAGE(WM_DDE_POKE),
	DEFINE_MESSAGE(WM_DDE_EXECUTE),
	DEFINE_MESSAGE(WM_DROPFILES),
	DEFINE_MESSAGE(WM_POWER),
	DEFINE_MESSAGE(WM_WINDOWPOSCHANGED),
	DEFINE_MESSAGE(WM_WINDOWPOSCHANGING),
#if 0
// MFC specific messages
	DEFINE_MESSAGE(WM_SIZEPARENT),
	DEFINE_MESSAGE(WM_SETMESSAGESTRING),
	DEFINE_MESSAGE(WM_IDLEUPDATECMDUI),
	DEFINE_MESSAGE(WM_INITIALUPDATE),
	DEFINE_MESSAGE(WM_COMMANDHELP),
	DEFINE_MESSAGE(WM_HELPHITTEST),
	DEFINE_MESSAGE(WM_EXITHELPMODE),
#endif
	DEFINE_MESSAGE(WM_HELP),
	DEFINE_MESSAGE(WM_NOTIFY),
	DEFINE_MESSAGE(WM_CONTEXTMENU),
	DEFINE_MESSAGE(WM_TCARD),
	DEFINE_MESSAGE(WM_MDIREFRESHMENU),
	DEFINE_MESSAGE(WM_MOVING),
	DEFINE_MESSAGE(WM_STYLECHANGED),
	DEFINE_MESSAGE(WM_STYLECHANGING),
	DEFINE_MESSAGE(WM_SIZING),
	DEFINE_MESSAGE(WM_SETHOTKEY),
	DEFINE_MESSAGE(WM_PRINT),
	DEFINE_MESSAGE(WM_PRINTCLIENT),
	DEFINE_MESSAGE(WM_POWERBROADCAST),
	DEFINE_MESSAGE(WM_HOTKEY),
	DEFINE_MESSAGE(WM_GETICON),
	DEFINE_MESSAGE(WM_EXITMENULOOP),
	DEFINE_MESSAGE(WM_ENTERMENULOOP),
	DEFINE_MESSAGE(WM_DISPLAYCHANGE),
	DEFINE_MESSAGE(WM_STYLECHANGED),
	DEFINE_MESSAGE(WM_STYLECHANGING),
	DEFINE_MESSAGE(WM_GETICON),
	DEFINE_MESSAGE(WM_SETICON),
	DEFINE_MESSAGE(WM_SIZING),
	DEFINE_MESSAGE(WM_MOVING),
	DEFINE_MESSAGE(WM_CAPTURECHANGED),
	DEFINE_MESSAGE(WM_DEVICECHANGE),
	DEFINE_MESSAGE(WM_PRINT),
	DEFINE_MESSAGE(WM_PRINTCLIENT),
#if 1	// Added in Poser
	DEFINE_MESSAGE(WM_CHANGEUISTATE),
	DEFINE_MESSAGE(WM_UPDATEUISTATE),
	DEFINE_MESSAGE(WM_QUERYUISTATE),
#endif

	{ 0, NULL, }    // end of message list
};

#undef DEFINE_MESSAGE

/////////////////////////////////////////////////////////////////////////////
// DDE special case

static void TraceDDE(LPCTSTR lpszPrefix, const MSG* pMsg)
{
	if (pMsg->message == WM_DDE_EXECUTE)
	{
		UINT nDummy;
		HGLOBAL hCommands;
		if (!UnpackDDElParam(WM_DDE_EXECUTE, pMsg->lParam,
			&nDummy, (PUINT)&hCommands))
		{
			TRACE1("Warning: Unable to unpack WM_DDE_EXECUTE lParam %08lX.",
				pMsg->lParam);
			return;
		}
		ASSERT(hCommands != NULL);

		LPCTSTR lpszCommands = (LPCTSTR)::GlobalLock(hCommands);
		ASSERT(lpszCommands != NULL);
		TRACE2("%s: Execute '%s'.", lpszPrefix, lpszCommands);
		::GlobalUnlock(hCommands);
	}
	else if (pMsg->message == WM_DDE_ADVISE)
	{
		ATOM aItem;
		HGLOBAL hAdvise;
		if (!UnpackDDElParam(WM_DDE_ADVISE, pMsg->lParam,
			(PUINT)&hAdvise, (PUINT)&aItem))
		{
			TRACE1("Warning: Unable to unpack WM_DDE_ADVISE lParam %08lX.",
				pMsg->lParam);
			return;
		}
		ASSERT(aItem != NULL);
		ASSERT(hAdvise != NULL);

		DDEADVISE* lpAdvise = (DDEADVISE*)::GlobalLock(hAdvise);
		ASSERT(lpAdvise != NULL);
		TCHAR szItem[80];
		szItem[0] = '\0';

		if (aItem != 0)
			::GlobalGetAtomName(aItem, szItem, _countof(szItem));

		TCHAR szFormat[80];
		szFormat[0] = '\0';
		if (((UINT)0xC000 <= (UINT)lpAdvise->cfFormat) &&
				((UINT)lpAdvise->cfFormat <= (UINT)0xFFFF))
		{
			::GetClipboardFormatName(lpAdvise->cfFormat,
				szFormat, _countof(szFormat));

			// User defined clipboard formats have a range of 0xC000->0xFFFF
			// System clipboard formats have other ranges, but no printable
			// format names.
		}

		AfxTrace(
			_T("%s: Advise item='%s', Format='%s', Ack=%d, Defer Update= %d"),
			 lpszPrefix, szItem, szFormat, lpAdvise->fAckReq,
			lpAdvise->fDeferUpd);
		::GlobalUnlock(hAdvise);
	}
}

/////////////////////////////////////////////////////////////////////////////

void _AfxTraceMsg(LPCTSTR lpszPrefix, const MSG* pMsg)
{
	ASSERT(lpszPrefix != NULL);
	ASSERT(pMsg != NULL);

	if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE ||
		pMsg->message == WM_NCHITTEST || pMsg->message == WM_SETCURSOR ||
		pMsg->message == WM_CTLCOLORBTN ||
		pMsg->message == WM_CTLCOLORDLG ||
		pMsg->message == WM_CTLCOLOREDIT ||
		pMsg->message == WM_CTLCOLORLISTBOX ||
		pMsg->message == WM_CTLCOLORMSGBOX ||
		pMsg->message == WM_CTLCOLORSCROLLBAR ||
		pMsg->message == WM_CTLCOLORSTATIC ||
		pMsg->message == WM_ENTERIDLE || pMsg->message == WM_CANCELMODE ||
		pMsg->message == 0x0118)    // WM_SYSTIMER (caret blink)
	{
		// don't report very frequently sent messages
		return;
	}

	LPCSTR lpszMsgName = NULL;
	char szBuf[80];

	// find message name
	if (pMsg->message >= 0xC000)
	{
		// Window message registered with 'RegisterWindowMessage'
		//  (actually a USER atom)
		if (::GetClipboardFormatNameA(pMsg->message, szBuf, _countof(szBuf)))
			lpszMsgName = szBuf;
	}
	else if (pMsg->message >= WM_USER)
	{
		// User message
		wsprintfA(szBuf, "WM_USER+0x%04X", pMsg->message - WM_USER);
		lpszMsgName = szBuf;
	}
	else
	{
		// a system windows message
		const AFX_MAP_MESSAGE* pMapMsg = allMessages;
		for (/*null*/; pMapMsg->lpszMsg != NULL; pMapMsg++)
		{
			if (pMapMsg->nMsg == pMsg->message)
			{
				lpszMsgName = pMapMsg->lpszMsg;
				break;
			}
		}
	}

	if (lpszMsgName != NULL)
	{
		AfxTrace(_T("%s: hwnd=0x%04X, msg = %hs (0x%04X, 0x%08lX)"),
			lpszPrefix, (UINT)pMsg->hwnd, lpszMsgName,
			pMsg->wParam, pMsg->lParam);
	}
	else
	{
		AfxTrace(_T("%s: hwnd=0x%04X, msg = 0x%04X (0x%04X, 0x%08lX)"),
			lpszPrefix, (UINT)pMsg->hwnd, pMsg->message,
			pMsg->wParam, pMsg->lParam);
	}

	if (pMsg->message >= WM_DDE_FIRST && pMsg->message <= WM_DDE_LAST)
		TraceDDE(lpszPrefix, pMsg);
}

/////////////////////////////////////////////////////////////////////////////

#endif // _DEBUG (entire file)
