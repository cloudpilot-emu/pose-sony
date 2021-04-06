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
#include "MMFMessaging.h"

#include "EmException.h"		// EmExceptionReset
#include "EmSession.h"			// EmSessionStopper
#include "Logging.h"

#include "SLP.h"				// SLP
#include "SocketMessaging.h"	// CSocket

/*
	This file implements two-way messaging between two applications.
	The two applications can send up to 4K of data to each other.

	The data interchange is accomplished by the means of two memory-
	mapped files. When this module is initialized by calling
	MMFOpenFiles, it first looks for the existance of already-created
	memory-mapped files.  If it doesn't find them, it creates them.

	MMFOpenFiles also opens a hidden window for the purpose of receiving
	messages, and creates a unique message ID that is sent as the "you've
	got data" message.

	Once the module is initialized, data can be sent to the other end
	by calling MMFSendData.  The given data is written to the "outgoing"
	memory-mapped file, and a message is sent to the other application.
	The hidden window of the other application gets the message, and
	respondes to it by calling a user-defined callback function. The
	callback function is given a pointer to the data, the length of the
	data, and a user-defined callback value.  The callback is then free
	to do whatever it wants with the data, including sending back a response.

	When the application is done sending all messages, it closes down its
	end of the session by calling MMFCloseFiles.  This can usually be done
	when the application quits.
*/

// ======================================================================
//	Globals and constants
// ======================================================================

// Data structure overlaid on the memory-mapped files.
//
//		fDataSize: size of the data send to the receiving app.
//		fData: the buffer to hold the data being sent to the other app.

struct Buffer
{
	long	fDataSize;
	char	fData[1];
};

// A collection of information that's needed for a single direction
// of message flow.

struct FileBundle
{
	HANDLE	fFile;		// Handle of the memory-mapped file.
	Buffer* fView;		// Pointer to the mapping of that file.
	HANDLE	fEvent; 	// Event that gets signalled when there's new data.
};

// Size of the memory-mapped files we create. Way larger than we need,
// but the system will probably allocate in 4K chunks anyway, so what
// the heck. NOTE: we might want to parameterize this in the future.

const int				kBufferSize = 4 * 1024;

// Names of the memory-mapped files we create.

const char* 			kToEmulatorFileName 	= "Debugger to Emulator";
const char* 			kFromEmulatorFileName	= "Emulator to Debugger";

// Class and window names of the window we create.	We create a hidden
// window for the purpose of sending and receiving messages.

static const char*		kMMFClassName			= "MMFMessaging";
static const char*		kMMFWindowName			= "MMFMessaging";

// Handles of the hidden windows.

static HWND 			gOurWindow;
static HWND 			gHisWindow;

// Function to call when we receive a message saying that there's data
// for us, and a 32-bit parameter the user wants us to pass to the callback.

static PacketHandler	gCallback;
static LPARAM			gCallbackData;

// Bundles for the two (send and receive) memory-mapped files we create.

static FileBundle		gSendBundle;
static FileBundle		gReceiveBundle;

// The message ID we use in PostMessage when telling the other guy that
// there's new data waiting for him.

static UINT 			gMessageID;


// ======================================================================
//	Private functions
// ======================================================================

static DWORD			PrvMMFOpenABundle		(const char* name, FileBundle& bundle);
static void 			PrvMMFCloseABundle		(FileBundle& bundle);
static UINT 			PrvMMFRegisterMessage	(void);

static LRESULT CALLBACK PrvMMFMessagingWndProc	(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static BOOL CALLBACK	PrvMMFWindowFinder		(HWND hwnd, LPARAM lParam);


/***********************************************************************
 *
 * FUNCTION:	MMFOpenFiles
 *
 * DESCRIPTION:	Open the memory-mapped-file messaging subsystem.  This
 *				function must be called before calls like MMFSendData
 *				can be called.	It eventually should be matched with a
 *				call to MMFCloseFiles.
 *
 * PARAMETERS:	hInstance - application instance handle.
 *
 *				fn - function to call when data's been received.
 *
 *				userData - parameter to pass to the user's callback
 *					function.
 *
 *				sendFileName - name of memory-mapped file to be used
 *					for outbound data.
 *
 *				receiveFileName - name of memory-mapped file to be used
 *					for inbound data.
 *
 * RETURNED:	Error code if an error occurred.  Zero otherwise.
 *
 ***********************************************************************/

DWORD MMFOpenFiles (HINSTANCE hInstance, PacketHandler fn, LPARAM userData,
					const char* sendFileName, const char* receiveFileName)
{
	DWORD		result;

	WNDCLASS	wc;
	::ZeroMemory (&wc, sizeof (wc));

	wc.lpfnWndProc		= &::PrvMMFMessagingWndProc;
	wc.lpszClassName	= kMMFClassName;
	wc.hInstance		= hInstance;

	if (::RegisterClass (&wc) == 0)
	{
		EmAssert (!"Unable to register window class.");
		result = ::GetLastError ();
		goto Error;
	}

	// Create the emulator window and show it.

	gOurWindow = ::CreateWindow (	kMMFClassName,
									kMMFWindowName, // (Might want to parameterize this)
									0,			// style
									0, 0,		// position
									0, 0,		// size
									NULL,		// parent
									NULL,		// menu
									hInstance,
									NULL);		// window creation data

	if (gOurWindow == NULL)
	{
		EmAssert (!"Unable to create window.");
		result = ::GetLastError ();
		goto Error;
	}

	gCallback = fn;
	gCallbackData = userData;

	result = ::PrvMMFOpenABundle (sendFileName, gSendBundle);
	if (result != 0)
		goto Error;

	result = ::PrvMMFOpenABundle (receiveFileName, gReceiveBundle);
	if (result != 0)
		goto Error;

	gMessageID = ::PrvMMFRegisterMessage ();
	if (gMessageID == 0)
		goto Error;

Error:
	if (result != 0)
	{
		::MMFCloseFiles (hInstance);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	MMFSendData
 *
 * DESCRIPTION:	Attempt to send the data to the requested "partner"
 *				application. Copy the given data into the "outgoing"
 *				memory-mapped file, and send a message to the "partner"
 *				application that there's data waiting for it.  If we
 *				can't send that message, it's either because we've never
 *				looked up the other guy's window handle, or the window
 *				handle we have is stale (either the other application
 *				has quit or has quit and restarted).  In any case, try
 *				to (re)find the window handle and send the message
 *				again.	If we fail again, then just give up.
 *
 * PARAMETERS:	data - pointer to data to send.
 *
 *				size - length of data to send in bytes.
 *
 * RETURNED:	Error code if an error occurred.  Zero otherwise.
 *
 ***********************************************************************/

DWORD MMFSendData (const void* data, long size)
{
	DWORD	error = ERROR_SUCCESS;

	if (LogLLDebugger ())
		LogAppendMsg ("Entered MMFSendData. Checking message semaphore.");

	// Make sure that there's not an unread message in the buffer.
	// In other words, make sure that the event is not signalled.

	if (::WaitForSingleObject (gSendBundle.fEvent, 0) == WAIT_OBJECT_0)
	{
		// The object is signalled, so it's "in use".

		if (LogLLDebugger ())
			LogAppendMsg ("Message semaphore busy; leaving.");

		EmAssert (!"Oops...sending data while previous data still in the buffer.");

		return ERROR_BUSY;
	}

	// Perform any logging.

	if (LogLLDebuggerData ())
		LogAppendData (data, size, "Copying user data to shared buffer:");
	else if (LogLLDebugger ())
		LogAppendMsg ("Copying user data to shared buffer.");

	// Copy the data to the file.

	memcpy (gSendBundle.fView->fData, data, size);
	gSendBundle.fView->fDataSize = size;

	// Signal that data is ready.

	if (LogLLDebugger ())
		LogAppendMsg ("Signalling event.");

	::SetEvent (gSendBundle.fEvent);

	// Send the message.

	if (LogLLDebugger ())
		LogAppendMsg ("Posting message.");

	if (!gHisWindow || !::PostMessage (gHisWindow, gMessageID, 0, 0))
	{
		// If we can't post the message, the reason could be because
		// either we haven't hooked up with the other window yet, or
		// the other window doesn't exist any more.  Try finding the
		// other window.

		gHisWindow = NULL;
		::EnumWindows (&::PrvMMFWindowFinder, (LPARAM) &gHisWindow);

		// If we found it, try posting the message again.

		if (gHisWindow)
		{
			if (!::PostMessage (gHisWindow, gMessageID, 0, 0))
			{
				// Complete and utter failure...

				error = GetLastError ();

				if (error == ERROR_SUCCESS)
				{
					// Make sure error is non-zero.

					error = ERROR_INVALID_WINDOW_HANDLE;
				}
			}
		}

		// Set the error in case we can't find the window.

		else
		{
			error = ERROR_INVALID_WINDOW_HANDLE;
		}
	}

	// Reset the event; no one is listening... If we don't
	// reset it, it will stay signalled, and the next time
	// we call this function, we'll think that there's pending
	// data in the outgoing buffer.

	if (error != ERROR_SUCCESS)
	{
		::ResetEvent (gSendBundle.fEvent);
	}

	if (LogLLDebugger ())
		LogAppendMsg ("Leaving MMFSendData.");

	return error;
}


/***********************************************************************
 *
 * FUNCTION:	MMFWaitForData
 *
 * DESCRIPTION:	Wait for the given amount of time (in milliseconds) for
 *				data to arrive.  If the data shows up, we handle it
 *				immediately by calling the user's callback function.  If
 *				the data doesn't show up, we return the result of
 *				WaitForSingleEvent, which will probably be WAIT_TIMEOUT.
 *				Otherwise, we return ERROR_SUCCESS.
 *
 * PARAMETERS:	timeout - timeout value in milliseconds.
 *
 * RETURNED:	Error code if an error occurred.  Zero otherwise.
 *
 ***********************************************************************/

DWORD MMFWaitForData (DWORD timeout)
{
	// Wait for the event to be signalled.

	if (LogLLDebugger ())
		LogAppendMsg ("Entered MMFWaitForData. Waiting for message semaphore.");

	DWORD	result = ::WaitForSingleObject (gReceiveBundle.fEvent, timeout);

	if (result == WAIT_OBJECT_0)
	{
		if (LogLLDebuggerData ())
			LogAppendData (gReceiveBundle.fView->fData,
						gReceiveBundle.fView->fDataSize,
						"MMFWaitForData: Got %ld bytes of data.",
						gReceiveBundle.fView->fDataSize);
		else if (LogLLDebugger ())
			LogAppendMsg ("MMFWaitForData: Got %ld bytes of data.",
						gReceiveBundle.fView->fDataSize);

		result = ERROR_SUCCESS;

		// Call the callback.

		gCallback ( gReceiveBundle.fView->fData,
					gReceiveBundle.fView->fDataSize,
					gCallbackData);

		if (LogLLDebugger ())
			LogAppendMsg ("Back from callback.");
	}
	else
	{
		if (LogLLDebugger ())
			LogAppendMsg ("Timed out waiting for message semaphore.");
	}

	if (LogLLDebugger ())
		LogAppendMsg ("Leaving MMFWaitForData.");

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	MMFCloseFiles
 *
 * DESCRIPTION:	Closes down the subsystem.	Should be called to counter
 *				the initialization performed by MMFOpenFiles.
 *
 * PARAMETERS:	hInstance - application instance handle.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void MMFCloseFiles (HINSTANCE hInstance)
{
	::DestroyWindow (gOurWindow);
	gOurWindow = NULL;

	::PrvMMFCloseABundle (gSendBundle);
	::PrvMMFCloseABundle (gReceiveBundle);

	gCallback = NULL;
	gCallbackData = 0;

	::UnregisterClass (kMMFClassName, hInstance);
}


/***********************************************************************
 *
 * FUNCTION:	PrvMMFOpenABundle
 *
 * DESCRIPTION:	Open a single "channel" (we create two channels for
 *				communications - one inbound and one outbound).
 *
 * PARAMETERS:	name - name of the memory-mapped file to use for
 *					exchanging data.
 *
 *				bundle - reference to structure holding information on
 *					the channel being opened.  This function initializes
 *					the structure.
 *
 * RETURNED:	Error code if an error occurred.  Zero otherwise.
 *
 ***********************************************************************/

DWORD PrvMMFOpenABundle (const char* name, FileBundle& bundle)
{
	DWORD	error = 0;
	BOOL	newFile = FALSE;

	// Assume glorious failure.

	bundle.fFile = NULL;
	bundle.fView = NULL;
	bundle.fEvent = NULL;

	// Try to open the mmf it it's already there

	bundle.fFile = ::OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, name);


	// If it's not there, create one.

	if (!bundle.fFile)
	{
		bundle.fFile = ::CreateFileMapping ((HANDLE) 0xFFFFFFFF,	// in-memory mmf
											NULL, PAGE_READWRITE, 0, kBufferSize, name);

		if (!bundle.fFile)
		{
			EmAssert (!"Unable to create file mapping");
			error = ::GetLastError ();
			return error;
		}

		newFile = TRUE;
	}

	// Map the file into our address space.

	bundle.fView = (Buffer*) ::MapViewOfFile (bundle.fFile, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	if (!bundle.fView)
	{
		EmAssert (!"Unable to map view of file");
		error = ::GetLastError ();
		return error;
	}

	if (newFile)
	{
		memset (bundle.fView, 0, kBufferSize);
	}

	// Create a new name for the event. We can base it on the name used for the
	// memory-mapped file (to ensure its uniqueness), but Win32 rules say that
	// event names must be different from those for semaphores, mutexes, and
	// memory mapped files.

	char	buffer[_MAX_PATH];

	strcpy (buffer, name);
	strcat (buffer, " event");

	bundle.fEvent = ::CreateEvent(	NULL,	// Security attributes
									FALSE,	// bManual reset = no
									FALSE,	// bInitialState = non-signalled
									buffer);

	if (!bundle.fEvent)
	{
		EmAssert (!"Unable to create event object");
		error = ::GetLastError ();
		return error;
	}
 
	return error;
}


/***********************************************************************
 *
 * FUNCTION:	PrvMMFCloseABundle
 *
 * DESCRIPTION:	Release all the resources acquired/created by
 *				PrvMMFOpenABundle.
 *
 * PARAMETERS:	bundle - structure containing information on the
 *					channel to be shut down.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void PrvMMFCloseABundle (FileBundle& bundle)
{
	DWORD	error = 0;

	if (bundle.fEvent)
	{
		if (!::CloseHandle (bundle.fEvent))
		{
			EmAssert (!"Error closing event");
			error = ::GetLastError ();
		}

		bundle.fEvent = NULL;
	}

	if (bundle.fView)
	{
		if (!::UnmapViewOfFile (bundle.fView))
		{
			EmAssert (!"Error unmapping file");
			error = ::GetLastError ();
		}

		bundle.fView = NULL;
	}

	if (bundle.fFile)
	{
		if (!::CloseHandle (bundle.fFile))
		{
			EmAssert (!"Error closing file");
			error = ::GetLastError ();
		}

		bundle.fFile = NULL;
	}
}


/***********************************************************************
 *
 * FUNCTION:	PrvMMFRegisterMessage
 *
 * DESCRIPTION:	A sending application signals the receiving application
 *				that data has been placed in the common buffer by
 *				sending a custom message value.  This function
 *				determines that value.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Custom message ID.
 *
 ***********************************************************************/

UINT PrvMMFRegisterMessage (void)
{
	DWORD	error = 0;
	UINT	msgID = ::RegisterWindowMessage ("Emulator/Debugger");

	if (msgID == 0)
	{
		EmAssert (!"Unable to register message ID");

		error = ::GetLastError ();
		EmAssert (error == 0);
	}

	return msgID;
}


/***********************************************************************
 *
 * FUNCTION:	PrvMMFMessagingWndProc
 *
 * DESCRIPTION:	The custom message is sent to a private/hidden window
 *				created for the sole purpose of receiving the message.
 *				This window proc is responsible for handling that
 *				message by calling the user-defined callback function.
 *
 * PARAMETERS:	Standard window procedure parameters
 *
 * RETURNED:	Standard window procedure result
 *
 ***********************************************************************/

LRESULT CALLBACK PrvMMFMessagingWndProc (	HWND hWnd,
											UINT msg,
											WPARAM wParam,
											LPARAM lParam)
{
	if (LogLLDebugger ())
	{
		LogAppendMsg ("----------------------------------------");
		LogAppendMsg ("PrvMMFMessagingWndProc called.");
	}

	if ((hWnd == gOurWindow) && (msg == gMessageID))
	{
		// Call MMFWaitForData with no timeout to process any incoming
		// data.  If the data's there, we handle it.  However, if the data
		// was already handled because the client waited for it by calling
		// MMFWaitForData himself, we don't want to try to handle it again.

		if (LogLLDebugger ())
			LogAppendMsg ("Handling debugger message.");

		::MMFWaitForData (0);

		if (LogLLDebugger ())
		{
			LogAppendMsg ("Handled message.");
			LogAppendMsg ("----------------------------------------");
		}

		return TRUE;
	}

	if (LogLLDebugger ())
	{
		LogAppendMsg ("Ignoring non-debugger message (0x%04X).", msg);
		LogAppendMsg ("----------------------------------------");
	}

	return ::DefWindowProc (hWnd, msg, wParam, lParam);
}


/***********************************************************************
 *
 * FUNCTION:	PrvMMFWindowFinder
 *
 * DESCRIPTION:	Callback function passed to EnumWindows when trying to
 *				find the peer application's receiver window.
 *
 * PARAMETERS:	Standard WNDENUMPROC parameters.
 *
 * RETURNED:	Standard WNDENUMPROC result.
 *
 ***********************************************************************/

BOOL CALLBACK PrvMMFWindowFinder(HWND hwnd, LPARAM lParam)
{
	// If it's the right kind, and it's not ours, then we'll talk with it.

	char	className[300];
	::GetClassName (hwnd, className, sizeof (className));

	if ((strcmp (className, kMMFClassName) == 0) && (hwnd != gOurWindow))
	{
		*(HWND*) lParam = hwnd;
		return FALSE;	// stop enumeration
	}

	return TRUE;
}


static CMMFSocket*	gSocket;

CMMFSocket::CMMFSocket (void) :
	fBufferedData ()
{
	EmAssert (gSocket == NULL || gSocket->Deleted ());
	gSocket = this;
}

CMMFSocket::~CMMFSocket (void)
{
	if (gSocket == this)
		gSocket = NULL;
}

ErrCode 		
CMMFSocket::Open (void)
{
	if (LogLLDebugger ())
		LogAppendMsg ("CMMFSocket::Open...");

	return errNone;
}

ErrCode 		
CMMFSocket::Close (void)
{
	if (LogLLDebugger ())
		LogAppendMsg ("CMMFSocket::Close...");

	return errNone;
}

ErrCode 		
CMMFSocket::Write (const void* buffer, long amountToWrite, long* amtWritten)
{
	if (amtWritten)
		*amtWritten = amountToWrite;

	return (ErrCode) ::MMFSendData (buffer, amountToWrite);
}

ErrCode 		
CMMFSocket::Read (void* buffer, long sizeOfBuffer, long* amtRead)
{
	if (sizeOfBuffer > fBufferedData.size ())
		sizeOfBuffer = fBufferedData.size ();

	if (amtRead)
		*amtRead = sizeOfBuffer;

	copy (fBufferedData.begin (), fBufferedData.begin () + sizeOfBuffer, (char*) buffer);
	fBufferedData.erase (fBufferedData.begin (), fBufferedData.begin () + sizeOfBuffer);

	return errNone;
}

Bool		
CMMFSocket::HasUnreadData (long timeout)
{
#pragma unused (timeout)

	return fBufferedData.size () > 0;
}

ErrCode 		
CMMFSocket::Idle (void)
{
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	HandlePacket
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void CMMFSocket::HandlePacketCB (const void* data, long size, LPARAM /*userData*/)
{
	if (gSocket && !gSocket->Deleted ())
	{
		gSocket->HandlePacket (data, size);
	}
}


void CMMFSocket::HandlePacket (const void* data, long amt)
{
	// Insert the received data into our long-term buffer.

	const unsigned char*	charData = (const unsigned char*) data;
	fBufferedData.insert (fBufferedData.end (), charData, charData + amt);

	// See if we have enough data to process.

	long	headerSize	= EmProxySlkPktHeaderType::GetSize ();
	long	bodySize	= EmProxySysPktBodyType::GetSize ();
	long	footerSize	= EmProxySlkPktFooterType::GetSize ();

	if (fBufferedData.size () > headerSize)
	{
		// Get the bodySize field to see if we have the entire body, too.

		EmProxySlkPktHeaderType	header;
		copy (fBufferedData.begin (), fBufferedData.begin () + headerSize, (unsigned char*) header.GetPtr ());
		bodySize = header.bodySize;
	}

	// Get the total size of the number of bytes that need to be buffered
	// if we are to believe that we have received a full packet.

	long	totalSize = headerSize + bodySize + footerSize;

	if (this->ShortPacketHack())
	{
		totalSize -= footerSize;
	}

	// If we have at least that many bytes, process them.

	if (fBufferedData.size () >= totalSize)
	{
		try
		{
			SLP::EventCallback (this, CSocket::kDataReceived);
		}
		catch (const EmExceptionReset& e)
		{
			EmSessionStopper	stopper (gSession, kStopNow);
			e.Display ();
		}
	}
}


Bool		
CMMFSocket::ShortPacketHack (void)
{
	return true;
}


Bool		
CMMFSocket::ByteswapHack (void)
{
	return true;
}
