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
#include "Platform.h"

#include "ChunkFile.h"			// ReadString
#include "EmApplicationWin.h"	// gInstance
#include "EmSession.h"			// gSession
#include "ErrorHandling.h"		// Errors::ThrowIfNULL
//#include "Miscellaneous.h"		// RememberBlock, ForgetBlock
#include "MMFMessaging.h"		// CMMFSocket
#include "SessionFile.h"		// SessionFile::kROMPathTag
#include "Sounds.h"
#include "Strings.r.h"			// kStr_ values

#include "resource.h"

// Defining COMPILE_MULTIMON_STUBS causes MultiMon.h to create thunks
// that either call the functions in Windows, or emulate them if they
// don't exist.

#define COMPILE_MULTIMON_STUBS
#include <MultiMon.h>


#if _DEBUG
#undef _CRTDBG_MAP_ALLOC
#define _MFC_OVERRIDES_NEW
#include <crtdbg.h>
#endif



// ======================================================================
//	Globals and constants
// ======================================================================


// ======================================================================
//	Private functions
// ======================================================================

/***********************************************************************
 *
 * FUNCTION:	Platform::Initialize
 *
 * DESCRIPTION:	Initializes platform-dependent stuff
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::Initialize (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	Platform::Reset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::Reset (void)
{
	Platform::StopSound (); // Kill any sounds in progress or pending
}


// ---------------------------------------------------------------------------
//		¥ Platform::Save
// ---------------------------------------------------------------------------

void Platform::Save (SessionFile&)
{
}


// ---------------------------------------------------------------------------
//		¥ Platform::Load
// ---------------------------------------------------------------------------

void Platform::Load (SessionFile&)
{
}


// ---------------------------------------------------------------------------
//		¥ Platform::Dispose
// ---------------------------------------------------------------------------

void Platform::Dispose (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetString
 *
 * DESCRIPTION:	Return the resource-based string based on the given ID.
 *
 * PARAMETERS:	id - id of the string to return.  IDs are defined
 *					in Strings.r.h.
 *
 * RETURNED:	String object containing the requested text.  If the
 *				resource could not be locationed, the returned string
 *				object contains the error message "<missing string #>".
 *				Clients can test the result for this pattern to see if
 *				the request succeeded or not.  In general, its a
 *				programming error if the request fails.
 *
 ***********************************************************************/

string Platform::GetString (StrCode id)
{
	int 	amtRead;
	int 	size = 256; 	// Arbitrary starting size
	string	result;

	// Allocate a buffer; load the string; loop if the buffer wasn't
	// big enough.

	while (1)
	{
		result.resize (size);
		amtRead = ::LoadString(gInstance, id, &result[0], size);

		if (amtRead == 0)
		{
			char	buffer[20];
			sprintf (buffer, "%d", id);
			return string ("<missing string ") + buffer + ">";
		}

		if (amtRead < (size - 1))
			break;
		size += 256;	// Arbitrary amount by which to increment
	}

	result.resize (amtRead);

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetIDForError
 *
 * DESCRIPTION:	Map a platform-specific error number into an id for
 *				an error string.
 *
 * PARAMETERS:	error - platform-specific error code.
 *
 * RETURNED:	String ID for corresponding error message.	If a
 *				string specific to the given error cannot be found,
 *				kStr_GenericError is returned.
 *
 ***********************************************************************/

int Platform::GetIDForError (ErrCode error)
{
	switch (error)
	{
		case ERROR_FILE_NOT_FOUND:		return kStr_FileNotFound;		// The system cannot find the file specified
		case ERROR_PATH_NOT_FOUND:		return kStr_FileNotFound;		// The system cannot find the path specified
		case ERROR_TOO_MANY_OPEN_FILES: return kStr_TooManyFilesOpen;	// The system cannot open the file
		case ERROR_ACCESS_DENIED:		return kStr_SerialPortBusy; 	// Access is denied
		case ERROR_NOT_ENOUGH_MEMORY:	return kStr_MemFull;			// Not enough storage is available to process this command
		case ERROR_OUTOFMEMORY: 		return kStr_MemFull;			// Not enough storage is available to complete this operation
		case ERROR_INVALID_DRIVE:		return kStr_DiskMissing;		// The system cannot find the drive specified
		case ERROR_WRITE_PROTECT:		return kStr_DiskLocked; 		// The media is write protected
		case ERROR_NOT_READY:			return kStr_GenericError;		// The device is not ready
		case ERROR_SHARING_VIOLATION:	return kStr_FileBusy;			// The process cannot access the file because it is being used by another process
		case ERROR_LOCK_VIOLATION:		return kStr_GenericError;		// The process cannot access the file because another process has locked a portion of the file
		case ERROR_HANDLE_DISK_FULL:	return kStr_DiskFull;			// The disk is full
		case ERROR_DUP_NAME:			return kStr_GenericError;		// A duplicate name exists on the network
		case ERROR_BAD_NETPATH: 		return kStr_GenericError;		// The network path was not found
		case ERROR_NETWORK_BUSY:		return kStr_GenericError;		// The network is busy
		case ERROR_DEV_NOT_EXIST:		return kStr_DiskMissing;		// The specified network resource or device is no longer available
		case ERROR_FILE_EXISTS: 		return kStr_GenericError;		// The file exists
		case ERROR_CANNOT_MAKE: 		return kStr_GenericError;		// The directory or file cannot be created
		case ERROR_DRIVE_LOCKED:		return kStr_DiskLocked; 		// The disk is in use or locked by another process
		case ERROR_OPEN_FAILED: 		return kStr_GenericError;		// The system cannot open the device or file specified
		case ERROR_DISK_FULL:			return kStr_DiskFull;			// There is not enough space on the disk
		case ERROR_INVALID_NAME:		return kStr_BadFileName;		// The filename, directory name, or volume label syntax is incorrect
		case ERROR_BAD_PATHNAME:		return kStr_BadFileName;		// The specified path is invalid
		case ERROR_ALREADY_EXISTS:		return kStr_DuplicateFileName;	// Cannot create a file when that file already exists
		case ERROR_FILENAME_EXCED_RANGE:return kStr_BadFileName;		// The filename or extension is too long
		case ERROR_DIRECTORY:			return kStr_BadFileName;		// The directory name is invalid

		case ERROR_BADDB:				return kStr_GenericError;		// The configuration registry database is corrupt
		case ERROR_BADKEY:				return kStr_GenericError;		// The configuration registry key is invalid
		case ERROR_CANTOPEN:			return kStr_GenericError;		// The configuration registry key could not be opened
		case ERROR_CANTREAD:			return kStr_GenericError;		// The configuration registry key could not be read
		case ERROR_CANTWRITE:			return kStr_GenericError;		// The configuration registry key could not be written
		case ERROR_REGISTRY_CORRUPT:	return kStr_GenericError;		// The Registry is corrupt...
		case ERROR_REGISTRY_IO_FAILED:	return kStr_GenericError;		// An I/O operation initiated by the Registry failed...
		case ERROR_NO_LOG_SPACE:		return kStr_GenericError;		// System could not allocate the required space in a Registry log

		case ERROR_IO_DEVICE:			return kStr_GenericError;		// The request could not be performed because of an I/O device error.
		case ERROR_SERIAL_NO_DEVICE:	return kStr_GenericError;		// No serial device was successfully initialized.  The serial driver will unload.
		case ERROR_IRQ_BUSY:			return kStr_SerialPortBusy; 	// Unable to open a device that was sharing an interrupt request (IRQ) with other devices. At least one other device that uses that IRQ was already opened.

		case ERROR_TIMEOUT: 			return kStr_TimeoutError;		// This operation returned because the timeout period expired
	}

	return kStr_GenericError;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetIDForRecovery
 *
 * DESCRIPTION:	Map the given platform-specific error code into and ID
 *				for an error recovery string.
 *
 * PARAMETERS:	error - platform-specific error code.
 *
 * RETURNED:	String ID for corresponding recovery message.  If a
 *				string specific to the given error cannot be found,
 *				zero is returned.
 *
 ***********************************************************************/

int Platform::GetIDForRecovery (ErrCode error)
{
	UNUSED_PARAM (error);

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetShortVersionString
 *
 * DESCRIPTION:	Returns a short version string.  The format of the
 *				string is:
 *
 *					#.# (.#) ([dab]#)
 *
 *					# = one or more numeric digits
 *					. = literal '.'
 *					Patterns in parentheses are optional
 *					Patterns in brackets mean "one of these characters"
 *					Spaces are shown above for clarity; they do not
 *						appear in the string
 *
 *				Valid strings would be: 2.1d7, 2.1.1b14, 2.99, 2.1.1.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Formatted version string.
 *
 ***********************************************************************/

string Platform::GetShortVersionString (void)
{
	string	result;
	char	szFullPath[MAX_PATH];

	::GetModuleFileName (gInstance, szFullPath, sizeof(szFullPath));

	DWORD	dummy;
	DWORD	infoSize = ::GetFileVersionInfoSize (szFullPath, &dummy);

	if (infoSize > 0)
	{
		// Allocate memory to hold the verinfo block

		LPVOID	versionPtr = _alloca(infoSize);

		if (::GetFileVersionInfo (szFullPath, dummy, infoSize, versionPtr))
		{
			LPVOID	versString = NULL;
			UINT	versSize = 0;

			::VerQueryValue (	versionPtr, 
								"\\StringFileInfo\\040904b0\\FileVersion", 
								&versString, &versSize);

			if ((versString != NULL) && (versSize > 0))
			{
				result = (char*) versString;
			}
		}
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::CopyToClipboard
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

static UINT PrvPalmTextFormat (void)
{
	static UINT	format = 0;

	if (format == 0)
	{
		format = ::RegisterClipboardFormat ("POSE_PalmText");
	}

	return format;
}

static void PrvAddClipboardData (UINT format, ByteList& data)
{
	// NULL-terminate the input data.

	data.push_back (0);

	// Create a handle for passing to the Windows clipboard.

	HANDLE	clipHandle = ::GlobalAlloc (GMEM_MOVEABLE, data.size ());

	if (clipHandle)
	{
		// Get a pointer to the handle's contents.

		LPVOID	clipPtr = ::GlobalLock (clipHandle);

		if (clipPtr)
		{
			// Copy our host text into it

			strcpy ((char*) clipPtr, (char*) &data[0]);

			::GlobalUnlock (clipHandle);

			// Hand over control of the handle to the Windows
			// clipboard.  If successful, Windows now owns the
			// handle.  Otherwise, we free it up ourself.

			if (::SetClipboardData (format, clipHandle) == NULL)
			{
				::GlobalFree (clipHandle);
			}
		}
		else
		{
			// Couldn't lock the handle???  Free it up...

			::GlobalFree (clipHandle);
		}
	}
}

void Platform::CopyToClipboard (const ByteList& palmChars,
								const ByteList& hostChars)
{
	ByteList	palmChars2 (palmChars);
	ByteList	hostChars2 (hostChars);

	// See if any mapping needs to be done.

	if (hostChars2.size () > 0 && palmChars2.size () == 0)
	{
		Platform::RemapHostToPalmChars (hostChars2, palmChars2);
	}
	else if (palmChars2.size () > 0 && hostChars2.size () == 0)
	{
		Platform::RemapPalmToHostChars (palmChars2, hostChars2);
	}

	// Open the clipboard.

	if (::OpenClipboard (NULL))
	{
		// Empty the clipboard first.  This is a necessary first step,
		// or else we can't put our data there.

		::EmptyClipboard ();

		// If we have something to add, then add it.

		if (palmChars2.size () > 0)
		{
			::PrvAddClipboardData (::PrvPalmTextFormat (), palmChars2);
		}

		if (hostChars2.size () > 0)
		{
			::PrvAddClipboardData (CF_TEXT, hostChars2);
		}

		// Close up shop.

		::CloseClipboard ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Platform::CopyFromClipboard
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

static void PrvGetClipbardData (UINT format, ByteList& data)
{
	HANDLE	clipHandle =::GetClipboardData (format);

	if (clipHandle != NULL)
	{
		// If there was text data, get a pointer to it.

		LPVOID	clipPtr = ::GlobalLock (clipHandle);

		if (clipPtr != NULL)
		{
			// Copy the data.
			size_t	len = strlen ((const char*) clipPtr);

			data.resize (len);
			memcpy (&data[0], clipPtr, len);

			// Unlock the clipboard's handle.

			::GlobalUnlock (clipHandle);
		}
	}
}

void Platform::CopyFromClipboard (ByteList& palmChars,
								  ByteList& hostChars)
{
	// Assume failure.

	palmChars.clear ();
	hostChars.clear ();

	// Open the clipboard

	if (::OpenClipboard (NULL))
	{
		// Get the text data from the clipboard

		::PrvGetClipbardData (::PrvPalmTextFormat (), palmChars);
		::PrvGetClipbardData (CF_TEXT, hostChars);

		// Close up shop.

		::CloseClipboard ();
	}

	// See if any mapping needs to be done.

	if (hostChars.size () > 0 && palmChars.size () == 0)
	{
		Platform::RemapHostToPalmChars (hostChars, palmChars);
	}
	else if (palmChars.size () > 0 && hostChars.size () == 0)
	{
		Platform::RemapPalmToHostChars (palmChars, hostChars);
	}
}


/***********************************************************************
 *
 * FUNCTION:	Platform::RemapHostToPalmChars
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void Platform::RemapHostToPalmChars	(const ByteList& hostChars,
									 ByteList& palmChars)
{
	// Converting line endings is all we do for now.

	ByteList::const_iterator	iter = hostChars.begin ();
	while (iter != hostChars.end ())
	{
		uint8	ch = *iter++;

		if (ch == 0x0D && iter != hostChars.end () && *iter == 0x0A)
		{
			++iter;
			palmChars.push_back (chrLineFeed);
		}
		else
		{
			palmChars.push_back (ch);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	Platform::RemapHostToPalmChars
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void Platform::RemapPalmToHostChars	(const ByteList& palmChars,
									 ByteList& hostChars)
{
	// Converting line endings is all we do for now.

	ByteList::const_iterator	iter = palmChars.begin ();
	while (iter != palmChars.end ())
	{
		uint8	ch = *iter++;

		if (ch == chrLineFeed)
		{
			hostChars.push_back (0x0D);
			hostChars.push_back (0x0A);
		}
		else
		{
			hostChars.push_back (ch);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	Platform::PinToScreen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	True if the rectangle was changed.
 *
 ***********************************************************************/

Bool Platform::PinToScreen (EmRect& r)
{
	// Retrieves the size of the work area on the primary display monitor.
	// The work area is the portion of the screen not obscured by the
	// system taskbar or by application desktop toolbars. The pvParam
	// parameter must point to a RECT structure that receives the
	// coordinates of the work area, expressed in virtual screen coordinates.
	//
	// To get the work area of a monitor other than the primary display
	// monitor, call the GetMonitorInfo function.

	RECT		workBounds;

	workBounds = r;
	HMONITOR	monitor = ::MonitorFromRect (&workBounds, MONITOR_DEFAULTTONEAREST);

	MONITORINFO	info;
	info.cbSize = sizeof (info);

	if (!::GetMonitorInfo (monitor, &info))
		return false;	// Error, just leave.

	workBounds = info.rcWork;

	// If we're in it, just return now.  We're not changing
	// the rectangle, so return false.

	POINT	topLeft = { r.fLeft, r.fTop };
	POINT	topRight = { r.fRight - 1, r.fTop };
	POINT	bottomLeft = { r.fLeft, r.fBottom - 1 };
	POINT	bottomRight = { r.fRight - 1, r.fBottom - 1 };

	if (::PtInRect (&workBounds, topLeft) &&
		::PtInRect (&workBounds, topRight) &&
		::PtInRect (&workBounds, bottomLeft) &&
		::PtInRect (&workBounds, bottomRight))
	{
		return false;
	}

	// Pin the incoming rectangle to workBounds.

	EmRect r2 (workBounds);
	::PinRectInRect (r, r2);

	// We modified the original rect, so return true.

	return true;
}


/***********************************************************************
 *
 * FUNCTION:	ToHostEOL
 *
 * DESCRIPTION:	Converts a string of characters into another string
 *				where the EOL sequence is consistant for files on the
 *				current platform.
 *
 * PARAMETERS:	dest - receives the result.  The buffer is sized by
 *					this function, so the caller doesn't have to
 *					allocate any space itself.
 *
 *				destLen - receives the length of the resulting string.
 *
 *				src - pointer to input characters.
 *
 *				srcLen - number of characters pointed to by src.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Platform::ToHostEOL (	StMemory& dest, long& destLen,
							const char* src, long srcLen)
{
	char*	d = (char*) Platform::AllocateMemory (srcLen * 2);
	char*	p = d;
	Bool	previousWas0x0D = false;

	for (long ii = 0; ii < srcLen; ++ii)
	{
		char	ch = src[ii];

		// Convert 0x0D to 0x0D/0x0A.
		
		if (ch == 0x0D)
		{
			*p++ = 0x0D;
			*p++ = 0x0A;
		}

		// Convert 0x0A to 0x0D/0x0A, but not if it's
		// part of a 0x0D/0x0A sequence

		else if (ch == 0x0A)
		{
			if (previousWas0x0D)
			{
				*p++ = 0x0A;
			}
			else
			{
				*p++ = 0x0D;
				*p++ = 0x0A;
			}
		}

		// Copy all other characters straight through.

		else
		{
			*p++ = ch;
		}

		previousWas0x0D = ch == 0x0D;
	}

	destLen = p - d;
	d = (char*) Platform::ReallocMemory (d, destLen);
	dest.Adopt (d);
}


// ---------------------------------------------------------------------------
//		¥ Platform::ReadROMFileReference
// ---------------------------------------------------------------------------

Bool Platform::ReadROMFileReference (ChunkFile& docFile, EmFileRef& f)
{
	string	path;

	if (docFile.ReadString (SessionFile::kROMWindowsPathTag, path))
	{
		f = EmFileRef (path);
		if (f.Exists ())
			return true;
	}

	return false;
}


// ---------------------------------------------------------------------------
//		¥ Platform::WriteROMFileReference
// ---------------------------------------------------------------------------

void Platform::WriteROMFileReference (ChunkFile& docFile, const EmFileRef& f)
{
	// Save the ROM path.

	docFile.WriteString (SessionFile::kROMWindowsPathTag, f.GetFullPath ());
}


/***********************************************************************
 *
 * FUNCTION:	Platform::Delay
 *
 * DESCRIPTION:	Delay for a time period appropriate for a sleeping 68328.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::Delay (void)
{
	// Delay 10 msecs.	Delaying by this amount pauses us 1/100 of a
	// second, which is the rate at which the device's tickcount counter
	// increments.

	EmAssert (gSession);
	gSession->Sleep (10);
}


/***********************************************************************
 *
 * FUNCTION:	Platform::Cycle
 *
 * DESCRIPTION:	Perform platform-specific actions required after every
 *				opcode is executed.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::CycleSlowly (void)
{
	// Nothing to do in Windows.
}


/***********************************************************************
 *
 * FUNCTION:	Platform::RealAllocateMemory
 *
 * DESCRIPTION:	Allocate a buffer of the given size, optionally clearing
 *				the contents to zero.
 *
 * PARAMETERS:	size - size of the buffer to create.
 *
 *				clear - true if the buffer should be cleared.
 *
 * RETURNED:	Pointer to the allocated buffer.
 *
 ***********************************************************************/

void* Platform::RealAllocateMemory (size_t size, Bool clear, const char* file, int line)
{
	void*	result;

#if _DEBUG
	if (clear)
		result = _calloc_dbg (size, 1, _NORMAL_BLOCK, file, line);
	else
		result = _malloc_dbg (size, _NORMAL_BLOCK, file, line);
#else
	if (clear)
		result = calloc (size, 1);
	else
		result = malloc (size);
#endif

	Errors::ThrowIfNULL (result);

//	if (file)
//	{
//		::RememberBlock (p, file, line);
//	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::RealReallocMemory
 *
 * DESCRIPTION:	Resize the given buffer to the new given size.
 *
 * PARAMETERS:	p - Pointer to buffer to resize
 *
 *				size - desired new size of the buffer.
 *
 * RETURNED:	The buffer is allowed to move during the resize process,
 *				so this function returns the buffer's new location.
 *
 ***********************************************************************/

void* Platform::RealReallocMemory (void* p, size_t size, const char* file, int line)
{
	void*	result = NULL;

#if _DEBUG
	result = _realloc_dbg (p, size, _NORMAL_BLOCK, file, line);
#else
	result = realloc (p, size);
#endif

	Errors::ThrowIfNULL (result);

//	if (file)
//	{
//		::ForgetBlock (p);
//		::RememberBlock (result, file, line);
//	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::RealDisposeMemory
 *
 * DESCRIPTION:	Primitve function for releasing allocated memory.  This
 *				function is called by a templatized version of the
 *				function that deals with accepting pointers to any
 *				data type and casting it to "void*" so that this
 *				function can be called.
 *
 * PARAMETERS:	p - pointer to the chunk of memory to release.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::RealDisposeMemory (void* p)
{
	if (p)
	{
#if _DEBUG
		_free_dbg (p, _NORMAL_BLOCK);
#else
		free (p);
#endif

//		::ForgetBlock (p);
	}
}


/***********************************************************************
 *
 * FUNCTION:	Platform::ForceStartupScreen
 *
 * DESCRIPTION:	See if the user has requested that the Startup dialog
 *				be presented instead of attempting to use the latest
 *				session file or creation settings.
 *
 *				The current signal is to toggle the CAPSLOCK key.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	True if the user has signalled that the dialog should
 *				be presented.
 *
 ***********************************************************************/

Bool Platform::ForceStartupScreen (void)
{
	return ::GetKeyState (VK_CAPITAL) != 0;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::StopOnResetKeyDown
 *
 * DESCRIPTION:	Return whether or not the user wants to set a break-
 *				point at some early place in the ROM.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	True if an early breakpoint should be set, false otherwise.
 *
 ***********************************************************************/

Bool Platform::StopOnResetKeyDown (void)
{
	// Comment this out for now.  It seems that Windows doesn't always return
	// the expected result.  That is, this function often returns TRUE even
	// though the Control is not down.
	//
	// Since this functionality is really only required by Palm OS engineers,
	// and since they're working on Macs, we don't really need this feature
	// in the Windows version now anyway.

//	return ::GetAsyncKeyState (VK_CONTROL) != 0;
	return false;
}


// ---------------------------------------------------------------------------
//		¥ Platform::CollectOptions
// ---------------------------------------------------------------------------

Bool Platform::CollectOptions (int argc, char** argv, int& errorArg,
							   int (*cb)(int, char**, int&))
{
	errorArg = 1;	// Skip argv[0]
	while (errorArg < argc)
	{
		if (cb (argc, argv, errorArg) == 0)
			return false;
	}

	return true;
}


// ---------------------------------------------------------------------------
//		¥ Platform::PrintHelp
// ---------------------------------------------------------------------------

void Platform::PrintHelp (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetMilliseconds
 *
 * DESCRIPTION:	Return a current "millisecond count".  This value
 *				correspond to any current time; it's just used to
 *				measure elapsed time.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The current millisecond count.
 *
 ***********************************************************************/

uint32 Platform::GetMilliseconds (void)
{
	return (uint32) ::GetTickCount();
}


/***********************************************************************
 *
 * FUNCTION:	Platform::CreateDebuggerSocket
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

CSocket* Platform::CreateDebuggerSocket (void)
{
	return new CMMFSocket;
}


/***********************************************************************
 *
 * FUNCTION:	Platform::ExitDebugger
 *
 * DESCRIPTION:	Perform platform-specific operations when debug mode is
 *				exited.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::ExitDebugger (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	Platform::ViewDrawLine
 *
 * DESCRIPTION:	Draw a line between the given points on the LCD.
 *
 * PARAMETERS:	xStart, yStart, xEnd, yEnd - coordinates of line to draw.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::ViewDrawLine (int xStart, int yStart, int xEnd, int yEnd)
{
	UNUSED_PARAM (xStart);
	UNUSED_PARAM (yStart);
	UNUSED_PARAM (xEnd);
	UNUSED_PARAM (yEnd);

	// !!! Do nothing for now, as any drawing we do gets overwritten
	// by the next blit of the LCD buffer to the screen.
}


/***********************************************************************
 *
 * FUNCTION:	Platform::ViewDrawPixel
 *
 * DESCRIPTION:	Draw a pixel at the given location on the LCD.
 *
 * PARAMETERS:	xPos, yPos - location of pixel to plot.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::ViewDrawPixel (int xPos, int yPos)
{
	UNUSED_PARAM (xPos);
	UNUSED_PARAM (yPos);

	// !!! Do nothing for now, as any drawing we do gets overwritten
	// by the next blit of the LCD buffer to the screen.
}


static void PrvQueueNote (int frequency, int duration, int amplitude)
{
	// There are a couple of ways to play the note: with the Windows
	// Beep function, or with the "wave" API.  The former takes a
	// frequency and duration and sounds pretty good, but doesn't take
	// and amplitude, support asynchronous playback, or respect the
	// frequency and duration parameters on Windows 9x.  The latter
	// supports everything we want, but doesn't sound right.
	//
	// For now, use Beep on NT and "wave" on 9x.

	OSVERSIONINFO	version;
	version.dwOSVersionInfoSize = sizeof (version);
	GetVersionEx (&version);

	if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if (duration > 0 && amplitude > 0)
		{
			Beep (frequency, duration);
		}
	}
	else if (gSounds != NULL)
	{
		// Do it the hard way on '9x
		gSounds->QueueNote (frequency, duration, amplitude);
	}
}

CallROMType Platform::SndDoCmd (SndCommandType& cmd)
{
	switch (cmd.cmd)
	{
		case sndCmdFreqDurationAmp:
			PrvQueueNote (cmd.param1, cmd.param2, cmd.param3);
			break;

		case sndCmdNoteOn:
			return kExecuteROM;

		case sndCmdFrqOn:
			PrvQueueNote (cmd.param1, cmd.param2, cmd.param3);
			break;

		case sndCmdQuiet:
			return kExecuteROM;
	}

	return kSkipROM;
}

void Platform::StopSound (void)
{
	if (gSounds != NULL)
	{
		gSounds->StopSound ();
	}
}

void Platform::Beep (void)
{
	::MessageBeep (-1);
}
