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
#include "Document.h"			// QueryNewDocument
#include "EmCommands.h"			// kCommandNew
#include "EmDlgWin.h"			// CenterWindow, CenterDlgProc
#include "EmScreen.h"			// EmScreenUpdateInfo, EmScreen::GetBits
#include "EmSession.h"			// EmSessionStopper, gSession
#include "EmStreamFile.h"		// EmStreamFile
#include "Emulator.h"			// gInstance, gCPU
#include "EmWindow.h"			// EmWindow::GetWindow
#include "EmWindowWin.h"		// GetHostWindow
#include "ErrorHandling.h"		// Errors::kOKCancelMask
#include "Hordes.h"				// Hordes::CanStop, etc.
#include "Miscellaneous.h"		// RememberBlock, ForgetBlock
#include "MMFMessaging.h"		// CMMFSocket
#include "TrapPatches.h"		// Patches::SetSwitchApp
#include "SessionFile.h"		// SessionFile::kROMPathTag
#include "Sounds.h"
#include "Startup.h"			// Startup::ScheduleQuit
#include "Strings.r.h"			// kStr_ values

#include "resource.h"

#include <locale.h> 			// localconv, lconv

#if _DEBUG
#undef _CRTDBG_MAP_ALLOC
#define _MFC_OVERRIDES_NEW
#include <crtdbg.h>
#endif



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


// ======================================================================
//	Globals and constants
// ======================================================================

Bool				gEnableSounds;
unsigned long		gPseudoTickcount;


// ======================================================================
//	Private functions
// ======================================================================

static void PrvSetWindowText	(HWND hWndCtrl, const char* lpszNew);
static Bool PrvResourcePresent	(const char* name);
static Bool PrvGetResource		(const char* name, Chunk& rom);

static bool PrvOpenFile			(const EmFileRef& file, HANDLE& hFile, DWORD& iSize);
static bool PrvOpenFile			(const EmFileRef& file, HANDLE& hFile);
static bool PrvBindROM			(HANDLE hExecutable);
static bool PrvBindPSF			(HANDLE hExecutable);
static bool PrvBindSkin			(HANDLE hExecutable);
static bool PrvBindConfig		(HANDLE hExecutable);
static bool PrvBindFile			(HANDLE hExecutable,
								 const EmFileRef&,
								 const char* resourceName);

// Over in LCD.cpp
void ConvertFromRGBList (RGBQUAD* outColors, const RGBList& inColors);

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
	StopSound(); // kill any sounds in progress or pending
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
 * FUNCTION:	Platform::BindPoser
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

void Platform::BindPoser (Bool fullSave, const EmFileRef& dest)
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

	bool	succeeded = false;
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


Bool Platform::ROMResourcePresent (void)
{
	return ::PrvResourcePresent (kROMID);
}


Bool Platform::GetROMResource (Chunk& chunk)
{
	return ::PrvGetResource (kROMID, chunk);
}


Bool Platform::PSFResourcePresent (void)
{
	return ::PrvResourcePresent (kPSFID);
}


Bool Platform::GetPSFResource (Chunk& chunk)
{
	return ::PrvGetResource (kPSFID, chunk);
}


Bool Platform::ConfigResourcePresent (void)
{
	return ::PrvResourcePresent (kConfigID);
}


Bool Platform::GetConfigResource (Chunk& chunk)
{
	return ::PrvGetResource (kConfigID, chunk);
}


Bool Platform::SkinfoResourcePresent (void)
{
	return ::PrvResourcePresent (kSkinfoID);
}


Bool Platform::GetSkinfoResource (Chunk& chunk)
{
	return ::PrvGetResource (kSkinfoID, chunk);
}


Bool Platform::Skin1xResourcePresent (void)
{
	return ::PrvResourcePresent (kSkin1xID);
}


Bool Platform::GetSkin1xResource (Chunk& chunk)
{
	return ::PrvGetResource (kSkin1xID, chunk);
}


Bool Platform::Skin2xResourcePresent (void)
{
	return ::PrvResourcePresent (kSkin2xID);
}


Bool Platform::GetSkin2xResource (Chunk& chunk)
{
	return ::PrvGetResource (kSkin2xID, chunk);
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetBoundDevice
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The Device Type in the bound configuration resource
 *
 ***********************************************************************/

EmDevice Platform::GetBoundDevice ()
{
	Chunk	data;

	if (!Platform::GetConfigResource (data))
		EmAssert (false);

	EmAssert (data.GetLength () >= sizeof (DeviceConfig));

	const char*	idName = ((DeviceConfig*) data.GetPointer ())->Device;
	return EmDevice (idName);
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GetBoundRAMSize
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The RAM size in the bound configuration resource
 *
 ***********************************************************************/

RAMSizeType Platform::GetBoundRAMSize (void)
{
	Chunk	data;

	if (!Platform::GetConfigResource (data))
		EmAssert (false);

	EmAssert (data.GetLength () >= sizeof (DeviceConfig));

	RAMSizeType	ramSize = ((DeviceConfig*) data.GetPointer ())->RAMSize;
	return ramSize;
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

void Platform::CopyToClipboard (const void* data, uint16 length)
{
	// Open the clipboard.

	if (::OpenClipboard (NULL))
	{
		// Empty the clipboard first.  This is a necessary first step,
		// or else we can't put our data there.

		::EmptyClipboard ();

		// If we have something to add, then add it.

		if (length != 0)
		{
			// Prepare the input text data for copying to the clipboard.
			// It needs to be NULL terminated, and the EOL sequence must
			// be the standard newline/linefeed sequence.

			EmAssert (data);

			string	origData ((const char*) data, length);
			string	convData (::ReplaceString (origData, "\x0A", "\x0D\x0A"));

			// Create a handle for passing to the Windows clipboard.

			HANDLE	clipHandle = ::GlobalAlloc (GMEM_MOVEABLE, convData.size () + 1);

			if (clipHandle)
			{
				// Get a pointer to the handle's contents.

				LPVOID	clipPtr = ::GlobalLock (clipHandle);
				if (clipPtr)
				{
					// Copy our converted text into it

					strcpy ((char*) clipPtr, convData.c_str ());

					::GlobalUnlock (clipHandle);

					// Hand over control of the handle to the Windows
					// clipboard.  If successful, Windows now owns the
					// handle.  Otherwise, we free it up ourself.

					if (::SetClipboardData (CF_TEXT, clipHandle) == NULL)
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

void Platform::CopyFromClipboard (Chunk& data)
{
	// Assume failure.

	data.SetLength (0);

	// Open the clipboard

	if (::OpenClipboard (NULL))
	{
		// Get the text data from the clipboard

		HANDLE	clipHandle =::GetClipboardData (CF_TEXT);

		if (clipHandle != NULL)
		{
			// If there was text data, get a pointer to it.

			LPVOID	clipPtr = ::GlobalLock (clipHandle);
			if (clipPtr != NULL)
			{
				// Copy the text data to a string object and convert
				// line endings.

				string	origData ((const char*) clipPtr);
				string	convData (::ReplaceString (origData, "\x0D\x0A", "\x0A"));

				// Store the converted string object into the Chunk
				// parameter passed to us.  Do not copy the terminating
				// NULL.

				data.SetLength (convData.size ());
				memcpy (data.GetPointer (), convData.c_str (), convData.size ());

				// Unlock the clipboard's handle.

				::GlobalUnlock (clipHandle);
			}
		}

		// Close up shop.

		::CloseClipboard ();
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

#define COMPILE_MULTIMON_STUBS
#include <MultiMon.h>

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
#if 1
	workBounds = r;
	HMONITOR	monitor = MonitorFromRect (&workBounds, MONITOR_DEFAULTTONEAREST);

	MONITORINFO	info;
	info.cbSize = sizeof (info);

	if (!GetMonitorInfo (monitor, &info))
		return false;	// Error, just leave.

	workBounds = info.rcWork;
#else
	if (!::SystemParametersInfo (SPI_GETWORKAREA, 0, &workBounds, 0))
		return false;	// Error, just leave.
#endif

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
 * FUNCTION:    Platform::QueryNewDocument
 *
 * DESCRIPTION: DESCRIPTION
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

Bool Platform::QueryNewDocument (Configuration& cfg)
{
	return ::QueryNewDocument (NULL, cfg);
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


// ---------------------------------------------------------------------------
//		¥ Platform::SaveScreen
// ---------------------------------------------------------------------------

void Platform::SaveScreen (const EmFileRef& fileRef)
{
	// This is the same as you'd find in LCD_ScreenShot, but with some
	// things take out.

//	EmFileRef		result;
//	string			prompt;
//	EmDirRef		defaultPath;
//	EmFileTypeList	filterList;
//	string			defaultName;
//
//	filterList.push_back (kFileTypePicture);
//	filterList.push_back (kFileTypeAll);
//
//	EmDlgItemID	item = EmDlg::DoPutFile (	result,
//											prompt,
//											defaultPath,
//											filterList,
//											defaultName);

//	if (item == kDlgItemOK)
	{
		HANDLE	file = ::CreateFile (fileRef.GetFullPath ().c_str (),
									GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL, NULL);

		if (file != INVALID_HANDLE_VALUE)
		{
			EmScreenUpdateInfo	info;
			{
//				EmSessionStopper	stopper (gSession, kStopNow);

//				if (stopper.Stopped ())
				{
					EmScreen::InvalidateAll ();
					EmScreen::GetBits (info);
					EmScreen::InvalidateAll ();
				}
			}

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

Bool Platform::CollectOptions (int argc, char** argv, int (*cb)(int, char**, int&))
{
	int	i = 1;	// Skip argv[0]
	while (i < argc)
	{
		if (cb (argc, argv, i) == 0)
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

void DoNew				(HWND hWnd, WPARAM command);
void DoOpenRecent		(HWND hWnd, WPARAM command);

ErrCode Platform::CreateSession (void)
{
	::DoNew (EmWindow::GetWindow()->GetHostData()->GetHostWindow(), kCommandSessionNew);

	return errNone;
}

ErrCode Platform::OpenSession (void)
{
	::DoOpenRecent (EmWindow::GetWindow()->GetHostData()->GetHostWindow(), kCommandSessionOpen0);

	return errNone;
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


// ---------------------------------------------------------------------------
//		¥ Gremlin Control Window
// ---------------------------------------------------------------------------

static HWND gGCW_Dlg;

// From Application.cpp

void DoGremlinsStep 	(HWND hWnd, WPARAM command);
void DoGremlinsResume	(HWND hWnd, WPARAM command);
void DoGremlinsStop 	(HWND hWnd, WPARAM command);


/***********************************************************************
 *
 * FUNCTION:	UpdateEventNumber
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void UpdateEventNumber (void)
{
	if (!gGCW_Dlg)
		return;

	// Update the "Event #: xxx of yyy" message.

	char	xxx[20];
	char	yyy[20];
	char	buffer[200];

	::FormatInteger (xxx, Hordes::EventCounter ());
	::FormatInteger (yyy, Hordes::EventLimit ());

	// If there is a max event number (indicated by a non -1 value
	// for gGCW_MaxEventNumber), display a string including that
	// value.  Otherwise, the number of events to post is unlimited.
	// In that case, we just show the current event number.

	if (Hordes::EventLimit () > 0)
	{
		static string	fmt (Platform::GetString (kStr_XofYEvents));
		sprintf(buffer, fmt.c_str (), xxx, yyy);
	}
	else
	{
		static string	fmt (Platform::GetString (kStr_XEvents));
		sprintf(buffer, fmt.c_str (), xxx);
	}

	HWND	dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_EVENT_NUMBER);
	::PrvSetWindowText (dlgItem, buffer);
}


/***********************************************************************
 *
 * FUNCTION:	UpdateElapsedTime
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void UpdateElapsedTime (void)
{
	if (!gGCW_Dlg)
		return;

	// Get hours, minutes, and seconds.

	const long	kMillisecondsPerSecond	= 1000;
	const long	kSecondsPerMinute		= 60;
	const long	kMinutesPerHour 		= 60;

	const long	kMillisecondsPerMinute	= kMillisecondsPerSecond * kSecondsPerMinute;
	const long	kMillisecondsPerHour	= kMillisecondsPerMinute * kMinutesPerHour;

	long	msecs = Hordes::ElapsedMilliseconds ();	//gGCW_ElapsedMilliseconds;
	long	hours = msecs / kMillisecondsPerHour;		msecs -= hours * kMillisecondsPerHour;
	long	minutes = msecs / kMillisecondsPerMinute;	msecs -= minutes * kMillisecondsPerMinute;
	long	seconds = msecs / kMillisecondsPerSecond;	msecs -= seconds * kMillisecondsPerSecond;

	// Format them into a string.

	char	formattedTime[20];
	sprintf (formattedTime, "%ld:%02ld:%02ld", hours, minutes, seconds);

	// Combine the formatted time with the rest of the string.

	char			msg[200];
	static string	fmt (Platform::GetString (kStr_ElapsedTime));
	sprintf (msg, fmt.c_str (), formattedTime);

	// Display the string.

	HWND	dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_ELAPSED_TIME);
	::PrvSetWindowText (dlgItem, msg);
}



/***********************************************************************
 *
 * FUNCTION:	PrvGCW_Close
 *
 * DESCRIPTION:	Closes the GCW window. Should only be called in same
 *				thread that created window.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static void PrvGCW_Close (void)
{
	if (!gGCW_Dlg)
		return;

	// Do not set gGCW_Dlg to NULL; this is done in the WM_DESTROY handler
	// of the GCW only if the window was successfully destroyed.

	::DestroyWindow (gGCW_Dlg);
}



/***********************************************************************
 *
 * FUNCTION:	GCW_DlgProc
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

static BOOL CALLBACK GCW_DlgProc (	HWND hWnd,
									UINT msg,
									WPARAM wParam,
									LPARAM lParam)
{
	UNUSED_PARAM (lParam);

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			Preference<PointType>	pref (kPrefKeyGCWLocation);

			if (!pref.Loaded ())
			{
				::CenterWindow (hWnd);
			}
			else
			{
				::SetWindowPos (hWnd, NULL, pref->x, pref->y,
							-1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}

			// Ensure the window is on the screen.

			RECT	wr;
			::GetWindowRect (hWnd, &wr);

			EmRect	r (wr);
			if (Platform::PinToScreen (r))
			{
				::SetWindowPos (hWnd, NULL, r.fLeft, r.fTop,
							-1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}

			// Set a timer so that we can periodically update the displayed values.

			::SetTimer (hWnd, (UINT) hWnd, 500, NULL);

			return TRUE;	// The dialog box procedure should return TRUE to direct
							// the system to set the keyboard focus to the control
							// given by hwndFocus. Otherwise, it should return FALSE
							// to prevent the system from setting the default keyboard focus.
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_STOP:
				{
					EmSessionStopper	stopper (gSession, kStopNow);
					if (stopper.Stopped())
						::DoGremlinsStop (EmWindow::GetWindow()->GetHostData()->GetHostWindow(), kCommandGremlinsStop);
					break;
				}

				case IDC_RESUME:
				{
					// Resuming attempts to make Palm OS calls (for instance,
					// to reset the pen calibration), so make sure we're stopped
					// at a good place.
					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped())
						::DoGremlinsResume (EmWindow::GetWindow()->GetHostData()->GetHostWindow(), kCommandGremlinsResume);
					break;
				}

				case IDC_STEP:
				{
					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped())
						::DoGremlinsStep (EmWindow::GetWindow()->GetHostData()->GetHostWindow(), kCommandGremlinsStep);
					break;
				}

				default:
					return 0;
			}

			return 1;
		}

		case WM_CLOSE:
		{
			PrvGCW_Close();
			return 1;
		}

		case WM_DESTROY:
		{
			RECT	wr;
			::GetWindowRect (hWnd, &wr);

			PointType	pt = { wr.left, wr.top };

			Preference<PointType>	pref (kPrefKeyGCWLocation);
			pref = pt;

			::KillTimer (hWnd, (UINT) hWnd);

			gGCW_Dlg = NULL;

			return 1;
		}

		case WM_TIMER:
		{
			// See comments in GCW_SetEventNumber.

//			if (gGCW_NeedUpdate)
			{
//				gGCW_NeedUpdate = false;

				::UpdateEventNumber ();
				::UpdateElapsedTime ();


				// Update the "Gremlin #xxx" message.

				char	buffer[200];
				string	fmt (Platform::GetString (kStr_GremlinNumber));
				sprintf(buffer, fmt.c_str (), (long) Hordes::GremlinNumber ());

				HWND	dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_GREMLIN_NUMBER);
				::PrvSetWindowText (dlgItem, buffer);

				// Update the buttons.

				dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_STOP);
				::EnableWindow (dlgItem, Hordes::CanStop ());

				dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_RESUME);
				::EnableWindow (dlgItem, Hordes::CanResume ());

				dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_STEP);
				::EnableWindow (dlgItem, Hordes::CanStep ());
			}

			return 1;
		}
	}

	return 0;	// Except in response to the WM_INITDIALOG message, the dialog box
				// procedure should return nonzero if it processes the message, and
				// zero if it does not.
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GCW_Open
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::GCW_Open (void)
{
	if (!gGCW_Dlg)
	{
		gGCW_Dlg = ::CreateDialogParam (	gInstance,
											MAKEINTRESOURCE (IDD_GREMLIN_CONTROL),
											::EmWindow::GetWindow()->GetHostData()->GetHostWindow(),
											GCW_DlgProc,
											0);
	}

	::ShowWindow (gGCW_Dlg, SW_SHOW);

	HWND	dlgItem;

	dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_EVENT_NUMBER);
	::PrvSetWindowText (dlgItem, "");

	dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_GREMLIN_NUMBER);
	::PrvSetWindowText (dlgItem, "");

	dlgItem = ::GetDlgItem (gGCW_Dlg, IDC_ELAPSED_TIME);
	::PrvSetWindowText (dlgItem, "");
}


/***********************************************************************
 *
 * FUNCTION:	Platform::GCW_Close
 *
 * DESCRIPTION:	Closes the GCW window. Thread-safe.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void Platform::GCW_Close (void)
{
	if (!gGCW_Dlg)
		return;

	::SendNotifyMessage(gGCW_Dlg, WM_CLOSE, NULL, NULL);
}




//sound functions


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

void Platform::StopSound()
{
	if (gSounds != NULL)
	{
		gSounds->StopSound ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	PrvSetWindowText
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void PrvSetWindowText(HWND hWndCtrl, const char* lpszNew)
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

bool PrvBindFile (HANDLE hExecutable, const EmFileRef& file,
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

bool PrvBindROM (HANDLE hExecutable)
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

bool PrvBindPSF (HANDLE hExecutable)
{
	// !!! Save the current session to a file so that we can read its
	// contents.  Later, we need a version of Emulator::Save that takes
	// a stream.  That way, we can call it with a RAM-base stream.

	EmFileRef	tempFile (tmpnam (NULL));

	EmAssert (gSession);
	gSession->Save (tempFile, false);

	bool	result = ::PrvBindFile (hExecutable, tempFile, kPSFID);

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

bool PrvBindSkin (HANDLE hExecutable)
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

bool PrvBindConfig (HANDLE hExecutable)
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
