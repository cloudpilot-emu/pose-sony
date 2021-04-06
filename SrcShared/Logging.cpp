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
#include "Logging.h"

#include "Byteswapping.h"		// Canonical
#include "EmMemory.h"			// EmMemGet32, EmMemGet16, EmMemGet8
#include "EmStreamFile.h"		// EmStreamFile
#include "Hordes.h"				// Hordes::IsOn, Hordes::EventCounter
#include "Platform.h"			// GetMilliseconds
#include "PreferenceMgr.h"		// Preference<>
#include "ROMStubs.h"			// FrmGetTitle, WinGetFirstWindow
#include "Strings.r.h"			// kStr_LogFileSize
#include "UAE_Utils.h"			// uae_memcpy


LogStream*	gStdLog;
uint8		gLogCache[kCachedPrefKeyDummy];


LogStream*	LogGetStdLog (void)
{
	if (gStdLog == NULL)
	{
		gStdLog = new LogStream ("Log");
	}

	return gStdLog;
}


static void PrvPrefsChanged (PrefKeyType key, void*)
{
#define UPDATE_ONE_PREF(name)								\
	if (strcmp (key, kPrefKey##name) == 0)					\
	{														\
		Preference<uint8>	pref(kPrefKey##name, false);	\
		gLogCache[kCachedPrefKey##name] = *pref;			\
	}

	FOR_EACH_SCALAR_PREF (UPDATE_ONE_PREF)
}

void LogStartup (void)
{
#define REGISTER_ONE_PREF(name)	\
	gPrefs->AddNotification (PrvPrefsChanged, kPrefKey##name);	\
	PrvPrefsChanged (kPrefKey##name, NULL);

#define HARDCODE_ONE_PREF(name)	\
	gLogCache[kCachedPrefKey##name] = 0;

	if (::IsBound ())
	{
		FOR_EACH_SCALAR_PREF (HARDCODE_ONE_PREF)
	}
	else
	{
		FOR_EACH_SCALAR_PREF (REGISTER_ONE_PREF)
	}
}


// ---------------------------------------------------------------------------
//		¥ CLASS LogStream
// ---------------------------------------------------------------------------

const long		kDefaultBufferSize = 1 * 1024 * 1024L;
const long		kFindUniqueFile = -1;
const uint32	kInvalidTimestamp = (uint32) -1;
const int32		kInvalidGremlinCounter = -2;
const long		kEventTextMaxLen = 255;

/***********************************************************************
 *
 * FUNCTION:	LogStream::LogStream
 *
 * DESCRIPTION:	Constructor.
 *
 * PARAMETERS:	baseName - base name to use for the file the data gets
 *					written to.  This base name will be prepended with
 *					the path of the directory to write the file to, and
 *					appended with "%04d.txt", where %04d will be
 *					replaced with a number to make sure the file's
 *					name is unique.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

LogStream::LogStream (const char* baseName) :
	fMutex (),
	fInner (baseName)
{
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::~LogStream
 *
 * DESCRIPTION:	Destructor.  Writes any buffered text to the file and
 *				closes the file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

LogStream::~LogStream (void)
{
	omni_mutex_lock lock (fMutex);
	fInner.DumpToFile ();
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::Printf
 *
 * DESCRIPTION:	A printf-like function for adding text to the log file.
 *				The text is preceded by a timestamp, and is suffixed
 *				with a newline.
 *
 * PARAMETERS:	fmt - a printf-like string for formatting the output
 *				text.
 *
 *				... - additional printf-like parameters.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStream::Printf (const char* fmt, ...)
{
	int		n;
	va_list	arg;

	omni_mutex_lock	lock (fMutex);

	va_start (arg, fmt);

	n = fInner.VPrintf (fmt, arg);

	va_end (arg);

//	fInner.DumpToFile ();
	return n;
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::DataPrintf
 *
 * DESCRIPTION:	A printf-like function for adding text to the log file.
 *				The text is preceded by a timestamp, and is suffixed
 *				with a newline.
 *
 * PARAMETERS:	data - binary data to be included in the output
 *
 *				dataLen - length of binary data
 *
 *				fmt - a printf-like string for formatting the output
 *				text.
 *
 *				... - additional printf-like parameters.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStream::DataPrintf (const void* data, long dataLen, const char* fmt, ...)
{
	omni_mutex_lock	lock (fMutex);

	int		n;
	va_list	arg;

	va_start (arg, fmt);

	n = fInner.VPrintf (fmt, arg);

	// Dump the data nicely formatted

	n += fInner.DumpHex (data, dataLen);

	return n;
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::VPrintf
 *
 * DESCRIPTION:	A vprintf-like function for adding text to the log file.
 *
 * PARAMETERS:	fmt - a vprintf-like string for formatting the output
 *				text.
 *
 *				args - additional vprintf-like parameters.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStream::VPrintf (const char* fmt, va_list args)
{
	omni_mutex_lock	lock (fMutex);

	return fInner.VPrintf (fmt, args);
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::Write
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStream::Write (const void* buffer, long size)
{
	omni_mutex_lock	lock (fMutex);

	return fInner.Write (buffer, size);
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::Clear
 *
 * DESCRIPTION:	Clear any currently logged data
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStream::Clear (void)
{
	omni_mutex_lock	lock (fMutex);

	fInner.Clear ();
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::GetLogSize
 *
 * DESCRIPTION:	Returns the maximum amount of text to be written to
 *				the log file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The maximum size.
 *
 ***********************************************************************/

long LogStream::GetLogSize (void)
{
	omni_mutex_lock	lock (fMutex);

	return fInner.GetLogSize ();
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::SetLogSize
 *
 * DESCRIPTION:	Sets the maximum amount of text to be written to the
 *				log file.  Any currently logged data is lost.
 *
 * PARAMETERS:	size - the new maximum value.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStream::SetLogSize (long size)
{
	omni_mutex_lock	lock (fMutex);

	fInner.SetLogSize (size);
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::EnsureNewFile
 *
 * DESCRIPTION:	Ensure that the logged data is written to a new file the
 *				next time DumpToFile is called.  Otherwise, the data
 *				will be written to the same file it was written to the
 *				previous time DumpToFile was called.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStream::EnsureNewFile (void)
{
	omni_mutex_lock	lock (fMutex);

	fInner.EnsureNewFile ();
}


/***********************************************************************
 *
 * FUNCTION:	LogStream::DumpToFile
 *
 * DESCRIPTION:	Dumps any buffered text to the log file, prepending
 *				a message saying that only the last <mumble> bytes
 *				of text are buffered.
 *
 *				If no data has been logged (or has been discarded with
 *				a call to Clear), nothing is written out and no file is
 *				created.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStream::DumpToFile (void)
{
	omni_mutex_lock	lock (fMutex);

	fInner.DumpToFile ();
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::LogStreamInner
 *
 * DESCRIPTION:	Constructor.
 *
 * PARAMETERS:	baseName - base name to use for the file the data gets
 *					written to.  This base name will be prepended with
 *					the path of the directory to write the file to, and
 *					appended with "%04d.txt", where %04d will be
 *					replaced with a number to make sure the file's
 *					name is unique.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

LogStreamInner::LogStreamInner (const char* baseName) :
	fBaseName (baseName),
	fFileIndex (kFindUniqueFile),
	fBuffer (kDefaultBufferSize),
	fBufferSize (kDefaultBufferSize),
	fBufferOffset (0),
	fLastGremlinEventCounter (kInvalidGremlinCounter),
	fLastTimestampTime (kInvalidTimestamp),
	fBaseTimestampTime (kInvalidTimestamp)
{
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::~LogStreamInner
 *
 * DESCRIPTION:	Destructor.  Writes any buffered text to the file and
 *				closes the file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

LogStreamInner::~LogStreamInner (void)
{
	this->DumpToFile ();
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::DumpHex
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStreamInner::DumpHex (const void* data, long dataLen)
{
	int n = 0;
	const uint8*	dataP = (const uint8*) data;

	if (dataP && dataLen)
	{
		for (long ii = 0; ii < dataLen; ii += 16)
		{
			char	text[16 * 4 + 4];	// 16 bytes * (3 for hex + 1 for ASCII) + 2 tabs + 1 space + 1 NULL
			char*	p = text;

			*p++ = '\t';

			// Print up to 16 bytes of hex on the left
			long	jj;
			for (jj = ii; jj < ii + 16 ; ++jj)
			{
				if (jj < dataLen)
					p += sprintf (p, "%02X ", dataP[jj]);
				else
					p += sprintf (p, "   ");

				if (jj == ii + 7)
					p += sprintf (p, " ");

				EmAssert (p - text < (ptrdiff_t) sizeof (text));
			}

			// Print the ascii on the right
			*p++ = '\t';
			for (jj = ii; jj < ii + 16 && jj < dataLen; ++jj)
			{
				char	c = dataP[jj];
				if (!isprint(c))
					c = '.';
				*p++ = c;

				EmAssert (p - text < (ptrdiff_t) sizeof (text));
			}

			EmAssert (p - text <= (ptrdiff_t) sizeof (text));

			this->Write (text, p - text);
		}
	}	

	return n;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::VPrintf
 *
 * DESCRIPTION:	A vprintf-like function for adding text to the log file.
 *
 * PARAMETERS:	fmt - a vprintf-like string for formatting the output
 *				text.
 *
 *				args - additional vprintf-like parameters.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStreamInner::VPrintf (const char* fmt, va_list args)
{
	char	buffer[2000];

	int n = vsprintf (buffer, fmt, args);

	// debug check, watch for buffer overflows here
	if (n < 0 || n >= (int) sizeof (buffer))
	{
		Platform::Debugger();
	}

	this->Write (buffer, n);

	return n;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::Write
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

int LogStreamInner::Write (const void* buffer, long size)
{
	this->Timestamp ();
	this->Append ((const char*) buffer, size);
	this->NewLine ();

	return size;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::Clear
 *
 * DESCRIPTION:	Clear any currently logged data
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::Clear (void)
{
	fBufferOffset = 0;
	fBaseTimestampTime = kInvalidTimestamp;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::GetLogSize
 *
 * DESCRIPTION:	Returns the maximum amount of text to be written to
 *				the log file.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The maximum size.
 *
 ***********************************************************************/

long LogStreamInner::GetLogSize (void)
{
	return fBufferSize;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::SetLogSize
 *
 * DESCRIPTION:	Sets the maximum amount of text to be written to the
 *				log file.  Any currently logged data is lost.
 *
 * PARAMETERS:	size - the new maximum value.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::SetLogSize (long size)
{
	char*	newBuffer = (char*) Platform::AllocateMemory (size);

	if (newBuffer)
	{
		fBuffer.Adopt (newBuffer);

		fBufferOffset = 0;
		fBufferSize = size;
	}
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::EnsureNewFile
 *
 * DESCRIPTION:	Ensure that the logged data is written to a new file the
 *				next time DumpToFile is called.  Otherwise, the data
 *				will be written to the same file it was written to the
 *				previous time DumpToFile was called.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::EnsureNewFile (void)
{
	fFileIndex = kFindUniqueFile;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::DumpToFile
 *
 * DESCRIPTION:	Dumps any buffered text to the log file, prepending
 *				a message saying that only the last <mumble> bytes
 *				of text are buffered.
 *
 *				If no data has been logged (or has been discarded with
 *				a call to Clear), nothing is written out and no file is
 *				created.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::DumpToFile (void)
{
	if (fBufferOffset == 0)
		return;

	// Open the output stream.  No need to open it as a "text" file;
	// our Dump method does any host-specific conversion.

	EmFileRef		ref = this->CreateFileReference ();
	EmStreamFile	stream (ref, kCreateOrEraseForUpdate,
							kFileCreatorCodeWarrior, kFileTypeText);

	// If we didn't wrap, do a simple dump: no header text saying
	// that the text was truncated, and no attempt to dump the
	// text in two parts.

	if (fBufferOffset <= fBufferSize)
	{
		this->DumpToFile (stream, fBuffer, fBufferOffset);
	}
	else

	// If we wrapped, print a message saying that only the last portion
	// of the text is begin saved/dumped.

	{
		char	buffer[200];
		string	templ = Platform::GetString (kStr_LogFileSize);
		sprintf (buffer, templ.c_str (), fBufferSize / 1024L);
		this->DumpToFile (stream, buffer, strlen (buffer));

		// Dump the text.

		long	startingOffset = fBufferOffset % fBufferSize;
		long	firstPart = fBufferSize - startingOffset;
		long	secondPart = fBufferSize - firstPart;

		this->DumpToFile (stream, fBuffer + startingOffset, firstPart);
		this->DumpToFile (stream, fBuffer, secondPart);
	}
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::DumpToFile
 *
 * DESCRIPTION:	Dumps the given text to the log file, converting any
 *				EOL characters along the way.
 *
 * PARAMETERS:	f - open file to write the text to.
 *
 *				s - text to write.
 *
 *				size - number of characters to write (the input text
 *					is not necessarily NULL terminated).
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::DumpToFile (EmStreamFile& f, const char* s, long size)
{
	StMemory	converted;
	long		convertedLength;

	Platform::ToHostEOL (converted, convertedLength, s, size);

	f.PutBytes (converted.Get (), convertedLength);
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::CreateFileReference
 *
 * DESCRIPTION:	Creates a file reference based on the base file name
 *				passed in to the constructor and the current fFileIndex.
 *				If fFileIndex is kFindUniqueFile, this routine attempts
 *				to find a file index that results in a new file being
 *				created.  Otherwise, the current fFileIndex is used to
 *				either open an existing file or create a new one.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The desired EmFileRef.
 *
 ***********************************************************************/

EmFileRef LogStreamInner::CreateFileReference (void)
{
	EmFileRef	result;
	char		buffer[32];

	EmDirRef	poserDir = EmDirRef::GetEmulatorDirectory ();
	EmDirRef	gremlinDir = Hordes::GetGremlinDirectory ();

	if (fFileIndex == kFindUniqueFile)
	{
		// Look for an unused file name.

		fFileIndex = 0;

		do
		{
			++fFileIndex;

			sprintf (buffer, "%s_%04ld.txt", fBaseName, fFileIndex);

			if (Hordes::IsOn())
				result = EmFileRef (gremlinDir, buffer);
			else
				result = EmFileRef (poserDir, buffer);
		}
		while (result.IsSpecified () && result.Exists ());
	}
	else
	{
		sprintf (buffer, "%s_%04ld.txt", fBaseName, fFileIndex);

		if (Hordes::IsOn())
			result = EmFileRef (gremlinDir, buffer);
		else
			result = EmFileRef (poserDir, buffer);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::Timestamp
 *
 * DESCRIPTION:	Outputs a timestamp to the log stream.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::Timestamp (void)
{
	Bool	reformat = false;
	uint32	now = Platform::GetMilliseconds ();

	// This may be a case of pre-optimization, but we try to keep around
	// a formatted timestamp string for as long as possible.  If either
	// the time changes or the Gremlin event number changes, we force
	// the regeneration of the cached timestamp string.

	if (fLastTimestampTime != now)
		reformat = true;

	if (!reformat && Hordes::IsOn () && fLastGremlinEventCounter != Hordes::EventCounter ())
		reformat = true;

	if (reformat)
	{
		fLastTimestampTime = now;

		// We try to print out logged data with timestamps that are
		// relative to the first event recorded.

		if (fBaseTimestampTime == kInvalidTimestamp)
			fBaseTimestampTime = now;

		now -= fBaseTimestampTime;

		// If a Gremlin is running, use a formatting string that includes
		// the event number.  Otherwise, use a format string that omits it.

		if (Hordes::IsOn ())
		{
			fLastGremlinEventCounter = Hordes::EventCounter ();
			sprintf (fLastTimestampString, "%ld.%03ld (%ld):\t", now / 1000, now % 1000, fLastGremlinEventCounter);
		}
		else
		{
			sprintf (fLastTimestampString, "%ld.%03ld:\t", now / 1000, now % 1000);
		}
	}

	this->Append (fLastTimestampString, strlen (fLastTimestampString));
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::NewLine
 *
 * DESCRIPTION:	Outputs and EOL to the log stream.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::NewLine (void)
{
	this->Append ("\n", 1);
}


/***********************************************************************
 *
 * FUNCTION:	LogStreamInner::Append
 *
 * DESCRIPTION:	Generic function for adding text (actually, any kind of
 *				unformatted data) to the output stream.  If the amount
 *				of text in the buffer exceeds the maximum specified
 *				amount, any old text is deleted.  This function is
 *				the bottleneck for all such functions in this class.
 *
 * PARAMETERS:	buffer - pointer to the text to be added.
 *
 *				size - length of the text (in bytes) to be added.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void LogStreamInner::Append (const char* buffer, long size)
{
	// Nothing to do if nothing to copy.

	if (size == 0)
	{
	}
	
	// The amount we are copying in is larger than the size of our buffer;
	// fill in the entire buffer with the last part of the incoming data.

	else if (size >= fBufferSize)
	{
		memcpy (fBuffer, buffer + size - fBufferSize, fBufferSize);
		fBufferOffset = fBufferSize;
	}

	// The amount we are copying is smaller than the size of the buffer.
	// Copy as much of the new data into the buffer as possible before
	// we reach the end.  If we reach the end, wrap around.

	else
	{
		long	startingOffset = fBufferOffset % fBufferSize;
		long	amtToEndOfBuffer = fBufferSize - startingOffset;
		long	amtToCopy = size < amtToEndOfBuffer ? size : amtToEndOfBuffer;

		memcpy (fBuffer + startingOffset, buffer, amtToCopy);

		// See if we wrapped.  If so, copy the rest of the data.

		if (amtToCopy < size)
		{
			amtToCopy = size - amtToCopy;
			memcpy (fBuffer, buffer + amtToEndOfBuffer, amtToCopy);
		}

		fBufferOffset += size;

		// fBufferOffset is used to determine where to insert new characters.
		// Don't let it get too big in order to avoid overflow problems (yes,
		// we can overflow 32-bits when logging Gremlins events).
		//
		// The adjustment below is intended not only to preserve the starting
		// insertion point, but also to remember that we have wrapped (that's
		// what the "+ fBufferSize" is for).

		if (fBufferOffset > 2 * fBufferSize)
		{
			fBufferOffset = (fBufferOffset % fBufferSize) + fBufferSize;
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ StubEmFrmGetTitle
// ---------------------------------------------------------------------------
// Returns a pointer to the title string of a form.  Copied from Form.c.

static string StubEmFrmGetTitle (const FormPtr frm)
{
	const Char*	title = FrmGetTitle (frm);

	if (title)
	{
		char	buffer[256];
		uae_strcpy (buffer, (emuptr) title);
		return string (buffer);
	}

	return string ("Untitled");

}


// ---------------------------------------------------------------------------
//		¥ StubEmPrintFormID
// ---------------------------------------------------------------------------
// Displays the form resource id associated with the window passed.

static void StubEmPrintFormID (WinHandle winHandle, char* desc, char* eventText)
{
	emuptr winPtr;
	emuptr exitWinPtr;

	// Allow access to WindowType fields windowFlags and nextWindow.

	CEnableFullAccess	munge;
	
	if (winHandle)
		{
		exitWinPtr = (emuptr) WinGetWindowPointer (winHandle);
		
		// Check if the handle is still valid.  If the form has been deleted 
		// then we can't dereference the window pointer.
		
		// Search the window list for the pointer.
		winHandle = WinGetFirstWindow ();
		while (winHandle)
			{
			winPtr = (emuptr) WinGetWindowPointer (winHandle);
			if (winPtr == exitWinPtr)
				break;
			
			winHandle = (WinHandle) EmMemGet32 (winPtr + offsetof (WindowType, nextWindow));
			}
		
		
		if (winHandle && /*winPtr->windowFlags.dialog*/
			((EmMemGet16 (winPtr + offsetof (WindowType, windowFlags)) & 0x0200) != 0))
			{
			string	title = StubEmFrmGetTitle((FormPtr) winPtr);
			if (!title.empty())
				sprintf (&eventText[strlen(eventText)],"%s: \"%s\"", desc, title.c_str());
			else
				sprintf (&eventText[strlen(eventText)],"%s ID: %ld", desc, /*frm->formId*/
						EmMemGet16 (winPtr + offsetof (FormType, formId)));
			}
		}
}


// ---------------------------------------------------------------------------
//		¥ VirtualKeyDescriptions
// ---------------------------------------------------------------------------
// Return a key description

const char* kVirtualKeyDescriptions [] =
{
	" (vchrLowBattery)",				// 0x0101
	" (vchrEnterDebugger)",				// 0x0102
	" (vchrNextField)",					// 0x0103
	" (vchrStartConsole)",				// 0x0104
	" (vchrMenu)",						// 0x0105
	" (vchrCommand)",					// 0x0106
	" (vchrConfirm)",					// 0x0107
	" (vchrLaunch)",					// 0x0108
	" (vchrKeyboard)",					// 0x0109
	" (vchrFind)",						// 0x010A
	" (vchrCalc)",						// 0x010B
	" (vchrPrevField)",					// 0x010C
	" (vchrAlarm)",						// 0x010D
	" (vchrRonamatic)",					// 0x010E
	" (vchrGraffitiReference)",			// 0x010F
	" (vchrKeyboardAlpha)",				// 0x0110
	" (vchrKeyboardNumeric)",			// 0x0111
	" (vchrLock)",						// 0x0112
	" (vchrBacklight)",					// 0x0113
	" (vchrAutoOff)",					// 0x0114
	" (vchrExgTest)",					// 0x0115
	" (vchrSendData)",					// 0x0116
	" (vchrIrReceive)",					// 0x0117
	" (vchrTsm1)",						// 0x0118
	" (vchrTsm2)",						// 0x0119
	" (vchrTsm3)",						// 0x011A
	" (vchrTsm4)",						// 0x011B
	" (vchrRadioCoverageOK)",			// 0x011C
	" (vchrRadioCoverageFail)",			// 0x011D
	" (vchrPowerOff)"					// 0x011E
};



#define irGotDataChr 0x01FC		// to initiate NotifyReceive
#define irInitLibChr 0x01FD		// Used when intializing the library
#define irSerialChr 0x01FE		// Switches to Rs232 line driver


const char* kHardKeyDescriptions [] =
{
	" (irGotDataChr)",	// 0x01FC
	" (irInitLibChr)",	// 0x01FD
	" (irSerialChr)",	// 0x01FE
	"",					// 0x01FF
	"",					// 0x0200
	"",					// 0x0201
	"",					// 0x0202
	"",					// 0x0203
	" (vchrHard1)",		// 0x0204
	" (vchrHard2)",
	" (vchrHard3)",
	" (vchrHard4)",
	" (vchrHardPower)",
	" (vchrHardCradle)",
	" (vchrHardCradle2)",
	" (vchrHardContrast)",
	" (vchrHardAntenna)"
};

static const char* StubEmKeyDescription (Int16 key)
{
	COMPILE_TIME_ASSERT (countof (kVirtualKeyDescriptions) == vchrPowerOff - vchrLowBattery + 1);
	COMPILE_TIME_ASSERT (countof (kHardKeyDescriptions) == vchrHardAntenna - irGotDataChr + 1);

	if (key >= vchrLowBattery && key <= vchrPowerOff)
		return kVirtualKeyDescriptions [key - vchrLowBattery];

	if (key >= irGotDataChr && key <= vchrHardAntenna)
		return kHardKeyDescriptions [key - irGotDataChr];

	return "";
}


// ---------------------------------------------------------------------------
//		¥ PrvGetEventText
// ---------------------------------------------------------------------------
// Displays the passed event in the emulator's event tracewindow if it is
// active.

static Bool PrvGetEventText(EventPtr eventP, char* eventText)
{
	long curLen = strlen (eventText);
	eventText += curLen;

	switch (eventP->eType)
	{
		case nilEvent:
			return false;
			//sprintf(eventText,"nilEvent");
			//break;
		
		case penDownEvent:
			sprintf(eventText,"penDownEvent    X:%d   Y:%d", 
					eventP->screenX, eventP->screenY);
			break;

		case penUpEvent:
			strcpy(eventText,"penUpEvent");
			sprintf(eventText,"penUpEvent      X:%d   Y:%d", 
					eventP->screenX, eventP->screenY);
			break;

		case penMoveEvent:
			strcpy(eventText,"penMoveEvent");
			sprintf(eventText,"penMoveEvent    X:%d   Y:%d", 
					eventP->screenX, eventP->screenY);					
			break;

		case keyDownEvent:
			if ((eventP->data.keyDown.chr < 0x0100) && isprint (eventP->data.keyDown.chr))
			{
				sprintf(eventText,"keyDownEvent    Key:'%c' 0x%02X%s,  Modifiers: 0x%04X", 
						(char) eventP->data.keyDown.chr, eventP->data.keyDown.chr,  
						StubEmKeyDescription(eventP->data.keyDown.chr),
						eventP->data.keyDown.modifiers);
			}
			else
			{
				sprintf(eventText,"keyDownEvent    Key:0x%02X%s,  Modifiers: 0x%04X", 
						eventP->data.keyDown.chr,  
						StubEmKeyDescription(eventP->data.keyDown.chr),
						eventP->data.keyDown.modifiers);
			}
			break;

		case winEnterEvent:
			sprintf(eventText,"winEnterEvent   Enter: %p   Exit: %p", 
					eventP->data.winEnter.enterWindow, eventP->data.winEnter.exitWindow);		
			StubEmPrintFormID (eventP->data.winEnter.enterWindow, "  Enter Form", eventText);
			StubEmPrintFormID (eventP->data.winEnter.exitWindow, "  Exit Form", eventText);
			break;

		case winExitEvent:
			sprintf(eventText,"winExitEvent    Enter: %p   Exit: %p", 
					eventP->data.winExit.enterWindow, eventP->data.winExit.exitWindow);
			StubEmPrintFormID (eventP->data.winExit.enterWindow, "  Enter Form", eventText);
			StubEmPrintFormID (eventP->data.winExit.exitWindow, "  Exit Form", eventText);
			break;

		case ctlEnterEvent:
			sprintf(eventText,"ctlEnterEvent   ID: %u", 
					eventP->data.ctlEnter.controlID);
			break;

		case ctlSelectEvent:
			sprintf(eventText,"ctlSelectEvent  ID: %u   On: %u", 
					eventP->data.ctlSelect.controlID, eventP->data.ctlSelect.on);					
			break;

		case ctlRepeatEvent:
			sprintf(eventText,"ctlRepeatEvent  ID: %u   Time: %lu", 
					eventP->data.ctlRepeat.controlID, eventP->data.ctlRepeat.time);					
			break;

		case ctlExitEvent:
			sprintf(eventText,"ctlExitEvent");
			break;

		case lstEnterEvent:
			sprintf(eventText,"lstEnterEvent   ID: %u   Item: %u", 
					eventP->data.lstEnter.listID, eventP->data.lstEnter.selection);			
			break;

		case lstSelectEvent:
			sprintf(eventText,"lstSelectEvent  ID: %u   Item: %u", 
					eventP->data.lstSelect.listID, eventP->data.lstSelect.selection);
			break;

		case lstExitEvent:
			sprintf(eventText,"lstExitEvent    ID: %u", 
					eventP->data.lstExit.listID);
			break;

		case popSelectEvent:
			sprintf(eventText,"popSelectEvent  CtlID: %u  ListID: %u  Item: %u", 
					eventP->data.popSelect.controlID, eventP->data.popSelect.listID,
					eventP->data.popSelect.selection);
			break;

		case fldEnterEvent:
			sprintf(eventText,"fldEnterEvent   ID: %u", 
					eventP->data.fldEnter.fieldID);
			break;

		case fldHeightChangedEvent:
			sprintf(eventText,"fldHeightChangedEvent  ID: %u  Height: %u  Pos: %u", 
					eventP->data.fldHeightChanged.fieldID, 
					eventP->data.fldHeightChanged.newHeight,
					eventP->data.fldHeightChanged.currentPos);
			break;

		case fldChangedEvent:
			sprintf(eventText,"fldChangedEvent  ID: %u", 
					eventP->data.fldChanged.fieldID);
			break;

		case tblEnterEvent:
			sprintf(eventText,"tblEnterEvent   ID: %u   Row: %u  Col: %u",
					eventP->data.tblEnter.tableID,
					eventP->data.tblEnter.row,
					eventP->data.tblEnter.column);
			break;

		case tblSelectEvent:
			sprintf(eventText,"tblSelectEvent  ID: %u   Row: %u  Col: %u", 
					eventP->data.tblSelect.tableID, 
					eventP->data.tblSelect.row, 
					eventP->data.tblSelect.column);					
			break;

		case tblExitEvent:
			sprintf(eventText,"tblExitEvent  ID: %u   Row: %u  Col: %u", 
					eventP->data.tblExit.tableID, 
					eventP->data.tblExit.row, 
					eventP->data.tblExit.column);					
			break;
		
		case daySelectEvent:
			strcpy(eventText,"daySelectEvent");
			break;

		case menuEvent:
			sprintf(eventText,"menuEvent       ItemID: %u", 
					eventP->data.menu.itemID);
			break;

		case appStopEvent:
			strcpy(eventText,"appStopEvent");
			break;

		case frmLoadEvent:
			sprintf(eventText,"frmLoadEvent    ID: %u", 
					eventP->data.frmOpen.formID);
			break;

		case frmOpenEvent:
			sprintf(eventText,"frmOpenEvent    ID: %u", 
					eventP->data.frmOpen.formID);
			break;

		case frmGotoEvent:
			sprintf(eventText,"frmGotoEvent    ID: %u  Record: %u  Field: %u", 
					eventP->data.frmGoto.formID,
					eventP->data.frmGoto.recordNum,
					eventP->data.frmGoto.matchFieldNum);
			break;

		case frmUpdateEvent:
			sprintf(eventText,"frmUpdateEvent    ID: %u", 
					eventP->data.frmUpdate.formID);
			break;

		case frmSaveEvent:
			sprintf(eventText,"frmSaveEvent");
			break;

		case frmCloseEvent:
			sprintf(eventText,"frmCloseEvent   ID: %u", 
					eventP->data.frmClose.formID);
			break;

		case frmTitleEnterEvent:
			sprintf(eventText,"frmTitleEnterEvent   ID: %u", 
					eventP->data.frmTitleEnter.formID);
			break;

		case frmTitleSelectEvent:
			sprintf(eventText,"frmTitleSelectEvent   ID: %u", 
					eventP->data.frmTitleSelect.formID);
			break;

		case sclEnterEvent:
			sprintf(eventText,"sclEnterEvent   ID: %u", 
					eventP->data.sclEnter.scrollBarID);
			break;

		case sclRepeatEvent:
			sprintf(eventText,"sclRepeatEvent   ID: %u  Value: %u,  New value: %u", 
					eventP->data.sclRepeat.scrollBarID,
					eventP->data.sclRepeat.value,
					eventP->data.sclRepeat.newValue);
			break;

		case sclExitEvent:
			sprintf(eventText,"sclExitEvent   ID: %u", 
					eventP->data.sclExit.scrollBarID);
			break;

		case tsmConfirmEvent:
			curLen += sprintf(eventText,"tsmConfirmEvent   ID: %u  Text: ", 
					eventP->data.tsmConfirm.formID);
			uae_strncat(eventText, (emuptr)eventP->data.tsmConfirm.yomiText, kEventTextMaxLen - curLen);
			eventText[kEventTextMaxLen] = 0;	// Make sure we're terminated
			break;
			
		case tsmFepButtonEvent:
			sprintf(eventText,"tsmFepButtonEvent   ID: %u", 
					eventP->data.tsmFepButton.buttonID);
			break;
		
		case tsmFepModeEvent:
			sprintf(eventText,"tsmFepModeEvent   ID: %u", 
					eventP->data.tsmFepMode.mode);
			break;

		case menuCmdBarOpenEvent:
			sprintf(eventText,"menuCmdBarOpenEvent	preventFieldButtons: %u",
				eventP->data.menuCmdBarOpen.preventFieldButtons);
			break;
				
		case menuOpenEvent:
			sprintf(eventText,"menuOpenEvent   RscID:%u, cause:%u",
				eventP->data.menuOpen.menuRscID,
				eventP->data.menuOpen.cause);
			break;
				
		case menuCloseEvent:
			sprintf(eventText,"menuCloseEvent");
			break;

		case frmGadgetEnterEvent:
			sprintf(eventText,"frmGadgetEnterEvent   RscID:%u, gadget:0x%08lX",
					eventP->data.gadgetEnter.gadgetID,
					(unsigned long) eventP->data.gadgetEnter.gadgetP);
			break;

		case frmGadgetMiscEvent:
			sprintf(eventText,"frmGadgetMiscEvent   ID:%u, gadget:0x%08lX, selector:%u",
					eventP->data.gadgetMisc.gadgetID,
					(unsigned long) eventP->data.gadgetMisc.gadgetP,
					eventP->data.gadgetMisc.selector);
			break;

		default:
			if (eventP->eType >= firstINetLibEvent && eventP->eType < firstWebLibEvent)
			{
				sprintf(eventText, "NetLib event #%u", eventP->eType);
			}
			else if (eventP->eType >= firstWebLibEvent && eventP->eType < firstWebLibEvent + 0x0100)
			{
				sprintf(eventText, "WebLib event #%u", eventP->eType);
			}
			else if (eventP->eType >= firstUserEvent)
			{
				sprintf(eventText, "Application event #%u", eventP->eType);
			}
			else
			{
				sprintf(eventText,"Unknown Event!   Event->eType #: %u",
						eventP->eType);
			}
			break;
	}

	return true;
}

// ---------------------------------------------------------------------------
//		¥ LogEvtAddEventToQueue
// ---------------------------------------------------------------------------

void LogEvtAddEventToQueue (emuptr eventP)
{
	if (LogEnqueuedEvents ())
	{
		// Get a copy of the event record.  This will be in big-endian
		// format, so byteswap it if necessary..

		EventType	newEvent;
		uae_memcpy ((void*) &newEvent, eventP, sizeof (newEvent));
		Canonical (newEvent);

		// Get the text for this event.  If there is such text, log it.

		char	eventText[kEventTextMaxLen] = " -> EvtAddEventToQueue: ";
		if (PrvGetEventText (&newEvent, eventText))
		{
			LogAppendMsg (eventText);
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ LogEvtAddUniqueEventToQueue
// ---------------------------------------------------------------------------

void LogEvtAddUniqueEventToQueue (emuptr eventP, UInt32, Boolean)
{
	if (LogEnqueuedEvents ())
	{
		// Get a copy of the event record.  This will be in big-endian
		// format, so byteswap it if necessary..

		EventType	newEvent;
		uae_memcpy ((void*) &newEvent, eventP, sizeof (newEvent));
		Canonical (newEvent);

		// Get the text for this event.  If there is such text, log it.

		char	eventText[kEventTextMaxLen] = " -> EvtAddUniqueEventToQueue: ";
		if (PrvGetEventText (&newEvent, eventText))
		{
			LogAppendMsg (eventText);
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ LogEvtEnqueueKey
// ---------------------------------------------------------------------------

void LogEvtEnqueueKey (UInt16 ascii, UInt16 keycode, UInt16 modifiers)
{
	if (LogEnqueuedEvents ())
	{
		if ((ascii < 0x0100) && isprint (ascii))
		{
			LogAppendMsg (" -> EvtEnqueueKey: ascii = '%c' 0x%04X, keycode = 0x%04X, modifiers = 0x%04X.",
					(char) ascii, ascii, keycode, modifiers);
		}
		else
		{
			LogAppendMsg (" -> EvtEnqueueKey: ascii = 0x%04X, keycode = 0x%04X, modifiers = 0x%04X.",
					ascii, keycode, modifiers);
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ LogEvtEnqueuePenPoint
// ---------------------------------------------------------------------------

void LogEvtEnqueuePenPoint (emuptr ptP)
{
	if (LogEnqueuedEvents ())
	{
		Int16	penX = (Int16) EmMemGet16 (ptP + 0);
		Int16	penY = (Int16) EmMemGet16 (ptP + 2);

		LogAppendMsg (" -> EvtEnqueuePenPoint: pen->x=%d, pen->y=%d.", penX, penY);
	}
}


// ---------------------------------------------------------------------------
//		¥ LogEvtGetEvent
// ---------------------------------------------------------------------------

void LogEvtGetEvent (emuptr eventP, Int32 timeout)
{
	UNUSED_PARAM(timeout)

	if (LogDequeuedEvents ())
	{
		// Get a copy of the event record.  This will be in big-endian
		// format, so byteswap it if necessary..

		EventType	newEvent;
		uae_memcpy ((void*) &newEvent, eventP, sizeof (newEvent));
		Canonical (newEvent);

		// Get the text for this event.  If there is such text, log it.

		char	eventText[kEventTextMaxLen] = "<-  EvtGetEvent: ";
		if (PrvGetEventText (&newEvent, eventText))
		{
			LogAppendMsg (eventText);
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ LogEvtGetPen
// ---------------------------------------------------------------------------

void LogEvtGetPen (emuptr pScreenX, emuptr pScreenY, emuptr pPenDown)
{
	if (LogDequeuedEvents ())
	{
		Int16	screenX = (Int16) EmMemGet16 (pScreenX);
		Int16	screenY = (Int16) EmMemGet16 (pScreenY);
		Boolean	penDown = (Boolean) EmMemGet8 (pPenDown);

		static Int16	lastScreenX = -2;
		static Int16	lastScreenY = -2;
		static Boolean	lastPenDown = false;
		static long		numCollapsedEvents;

		if (screenX != lastScreenX ||
			screenY != lastScreenY ||
			penDown != lastPenDown)
		{
			lastScreenX = screenX;
			lastScreenY = screenY;
			lastPenDown = penDown;

			numCollapsedEvents = 0;

			LogAppendMsg ("<-  EvtGetPen: screenX=%d, screenY=%d, penDown=%d.",
					(int) screenX, (int) screenY, (int) penDown);
		}
		else
		{
			++numCollapsedEvents;
			if (numCollapsedEvents == 1)
				LogAppendMsg ("<-  EvtGetPen: <<<eliding identical events>>>.");
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ LogEvtGetSysEvent
// ---------------------------------------------------------------------------

void LogEvtGetSysEvent (emuptr eventP, Int32 timeout)
{
	UNUSED_PARAM(timeout)

	if (LogDequeuedEvents ())
	{
		// Get a copy of the event record.  This will be in big-endian
		// format, so byteswap it if necessary..

		EventType	newEvent;
		uae_memcpy ((void*) &newEvent, eventP, sizeof (newEvent));
		Canonical (newEvent);

		// Get the text for this event.  If there is such text, log it.

		char	eventText[kEventTextMaxLen] = "<-  EvtGetSysEvent: ";
		if (PrvGetEventText (&newEvent, eventText))
		{
			LogAppendMsg (eventText);
		}
	}
}

