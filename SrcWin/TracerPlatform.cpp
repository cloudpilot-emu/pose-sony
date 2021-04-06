/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "TracerPlatform.h"

#include "PreferenceMgr.h"
#include "EmCPU.h"

#include <stdarg.h>
#include <windows.h>

#define kTraceLibraryName		"PalmTrace.dll"
#define kRequiredLibraryVersion		0x01000000
#define kMaxSupportedLibraryVersion	0x01FFFFFF

unsigned long _stdcall	MoreConnectedProc	(void * unused);

HMODULE				hTracerLib;
HANDLE				hEvtConnectionRequested;
HANDLE				hEvtNoMoreTries;
HANDLE				hEvtExitMoreConnected;
HANDLE				hThreadMoreConnected;
CRITICAL_SECTION	sTracerControlCS;		// We need to serialize Inits and Closes because the auto-connect thread can call them at any time

Tracer				gTracer;

void (__stdcall *tracer_init_outputport)		(HINSTANCE, const char*, const char*);
void (__stdcall *tracer_close_outputport)		(void);

void (__stdcall *tracer_output_VT)				(WORD, const char*, const va_list*);
void (__stdcall *tracer_output_VTL)				(WORD, const char*, const va_list*);
void (__stdcall *tracer_output_B)				(WORD, const void*, size_t);

long (__stdcall *tracer_get_outputport_status)	(void);
void (__stdcall *tracer_get_caps)				(char*,size_t*);


// ---------------------------------------------------------------------------
//		¥ Tracer::Tracer
// ---------------------------------------------------------------------------

Tracer::Tracer(void)
{
}

// ---------------------------------------------------------------------------
//		¥ Tracer::~Tracer
// ---------------------------------------------------------------------------

Tracer::~Tracer(void)
{
}

// ---------------------------------------------------------------------------
//		¥ Tracer::Initialize
// ---------------------------------------------------------------------------

void Tracer::Initialize (void)
{
	DWORD threadId;
	unsigned long v;

	// Create events used to control the auto-connect thread
	hEvtConnectionRequested =	CreateEvent (0, 0, 0, 0);
	hEvtNoMoreTries			=	CreateEvent (0, 0, 0, 0);
	hEvtExitMoreConnected	=	CreateEvent (0, 0, 0, 0);

	// Load PalmTrace
	hTracerLib	= LoadLibrary (kTraceLibraryName);

	// If the library was found
	if (hTracerLib)
	{
		// If the version number looks OK
		v = GetLibraryVersionNumber ();
		if (v >= kRequiredLibraryVersion && v <= kMaxSupportedLibraryVersion)
		{
			// Request list of supported tracer Types
			tracer_get_caps			= (void (__stdcall *) (char *,size_t*)) GetProcAddress(hTracerLib, "tracer_get_capabilities");
		}
		else
		{
			FreeLibrary (hTracerLib);
			hTracerLib = 0;
		}
	}
		
	LoadTracerTypeList ();

	// Initialize the critical section used to serialize Init/Close requests
	InitializeCriticalSection (&sTracerControlCS);

	// Create auto-connection thread - it will start & block waiting for an event
	hThreadMoreConnected	=	CreateThread (0, 0, MoreConnectedProc, 0, 0, &threadId);


	// No trace user referenced at this time
	tracerRefCounter = 0;
}


// ---------------------------------------------------------------------------
//		¥ Tracer::Dispose
// ---------------------------------------------------------------------------

void Tracer::Dispose (void)
{
	// If not initialized, don't dispose.
	if (hEvtExitMoreConnected == NULL)
		return;

	// Terminate auto-connect thread
	SetEvent (hEvtExitMoreConnected);
	WaitForSingleObject (hThreadMoreConnected, INFINITE);

	// Close any pending connection
	StopTracer ();
		
	// Clean up our synchronization objects
	CloseHandle (hThreadMoreConnected);
	CloseHandle (hEvtConnectionRequested);
	CloseHandle (hEvtNoMoreTries);
	CloseHandle (hEvtExitMoreConnected);

	hThreadMoreConnected	=	0;
	hEvtConnectionRequested	=	0;
	hEvtNoMoreTries			=	0;
	hEvtExitMoreConnected	=	0;

	DeleteCriticalSection (&sTracerControlCS);

	// Unload PalmTrace
	FreeLibrary (hTracerLib);
	hTracerLib = 0;
	
	tracer_init_outputport		=	0;
	tracer_output_VT			=	0;
	tracer_output_VTL			=	0;
	tracer_output_B				=	0;
	tracer_close_outputport		=	0;
	tracer_get_outputport_status=	0;

	DisposeTracerTypeList();
}


// ---------------------------------------------------------------------------
//		¥ Tracer::InitOutputPort
// ---------------------------------------------------------------------------

void Tracer::InitOutputPort	(void)
{
	EnterCriticalSection (&sTracerControlCS);
		
	tracerRefCounter++;

	if (tracerRefCounter == 1)
	{
		ResetEvent (hEvtNoMoreTries);

		if (tracer_init_outputport)
		{
			(*tracer_init_outputport) (GetModuleHandle(0), GetTracerTypeInfo(runningTracerType)->type, GetTracerTypeInfo(runningTracerType)->paramCurVal);
		}
	}

	CheckPeerStatus ();

	LeaveCriticalSection (&sTracerControlCS);
}


// ---------------------------------------------------------------------------
//		¥ Tracer::OutputVT
// ---------------------------------------------------------------------------

void Tracer::OutputVT (unsigned short aErrModule, const char* aFormatString, va_list args)
{
	if (tracerRefCounter && tracer_output_VT)
	{
		(*tracer_output_VT) (aErrModule, (char*) aFormatString, &args);
		CheckPeerStatus ();
	}
}


// ---------------------------------------------------------------------------
//		¥ Tracer::OutputVTL
// ---------------------------------------------------------------------------

void Tracer::OutputVTL (unsigned short aErrModule, const char* aFormatString, va_list args)
{
	if (tracerRefCounter && tracer_output_VTL)
	{
		(*tracer_output_VTL) (aErrModule, (char*) aFormatString, &args);
		CheckPeerStatus ();
	}
}


// ---------------------------------------------------------------------------
//		¥ Tracer::OutputB
// ---------------------------------------------------------------------------

void Tracer::OutputB (unsigned short aErrModule, const void* aBuffer, size_t aBufferLen)
{
	if (tracerRefCounter && tracer_output_B)
	{
		(*tracer_output_B) (aErrModule, aBuffer, aBufferLen);
		CheckPeerStatus ();
	}
}


// ---------------------------------------------------------------------------
//		¥ Tracer::CloseOutputPort
// ---------------------------------------------------------------------------

void Tracer::CloseOutputPort (void)
{
	EnterCriticalSection (&sTracerControlCS);

	if (tracerRefCounter == 1)
	{
		//	Stop trying to reconnect
		SetEvent (hEvtNoMoreTries);

		// We only close the output port (the current tracer instance keeps alive)
		if (tracer_close_outputport)
		{
			(*tracer_close_outputport) ();
		}
	}
	
	if (tracerRefCounter > 0)
	{
		tracerRefCounter--;
	}

	LeaveCriticalSection (&sTracerControlCS);
}


// ---------------------------------------------------------------------------
//		¥ Tracer::CheckPeerStatus
// ---------------------------------------------------------------------------

void Tracer::CheckPeerStatus (void)
{
	if (tracer_get_outputport_status && (*tracer_get_outputport_status) ())
	{
		SetEvent (hEvtConnectionRequested);
	}
}

// ---------------------------------------------------------------------------
//		¥ Tracer::GetConnectionStatus
// ---------------------------------------------------------------------------

long Tracer::GetConnectionStatus(void)
{
	if (tracer_get_outputport_status)
		return (*tracer_get_outputport_status) ();
	
	return 1; // 1 = Not connected
}

// ---------------------------------------------------------------------------
//		¥ MoreConnectedProc
// ---------------------------------------------------------------------------

unsigned long _stdcall MoreConnectedProc (void * unused)
{
	DWORD	delay = INFINITE;
	HANDLE	events[3] = {hEvtExitMoreConnected, hEvtNoMoreTries, hEvtConnectionRequested};
	DWORD	eventCount;
	DWORD	reason;

	eventCount = 3;
	
	while (true)
	{
		reason = WaitForMultipleObjects (eventCount, events, FALSE, delay);

		switch (reason)
		{	
			case WAIT_OBJECT_0:		// Stop thread request
				return 0;
		
			case WAIT_OBJECT_0+1:	// Automatic retry explicitly canceled
				
				// Revert to normal wait state
				delay = INFINITE;
				eventCount = 3;
				
				// Clear any pending connection request
				ResetEvent (hEvtConnectionRequested);
				break;

			case WAIT_OBJECT_0+2:	// Explicit connection request
				EnterCriticalSection (&sTracerControlCS);
				
				if ((*tracer_get_outputport_status) ()) // Just in case we connected since the event was set, which could happen
				{
					// Attempt a connection to the tracing port
					// Comment: here is the reason why init and close are separated from create_instance & destroy_instance
					(*tracer_close_outputport) ();
					(*tracer_init_outputport) (GetModuleHandle(0), gTracer.GetCurrentTracerTypeInfo()->type, gTracer.GetCurrentTracerTypeInfo()->paramCurVal);
							
					// We don't change the tracerRefCount at this point, since it's
					// a client counter and has nothing to do with the avaibibility
					// or not of the actual tracing port

					// If the attempt failed
					if ((*tracer_get_outputport_status) ())
					{
						// Schedule an automatic retry
						delay = 1000;
						
						// Ignore the connection request event for now, as we're already
						// attempting to connect and this event can be set very frequently
						eventCount = 2;
					}
					else
					{
						// Successfuly connected
						delay = INFINITE;	

						// Clear any pending connection request
						ResetEvent (hEvtConnectionRequested);
					}
				}
				
				LeaveCriticalSection (&sTracerControlCS);
				break;

			case WAIT_TIMEOUT:		// Automatic retry
				EnterCriticalSection (&sTracerControlCS);
				
				(*tracer_close_outputport) ();
				(*tracer_init_outputport) (GetModuleHandle(0), gTracer.GetCurrentTracerTypeInfo()->type, gTracer.GetCurrentTracerTypeInfo()->paramCurVal);

				if (!(*tracer_get_outputport_status) ())
				{
					// Successfuly connected
					delay = INFINITE;	
					
					// Revert to normal handling of the explicit connection request
					eventCount = 3;

					// Clear any pending connection request
					ResetEvent (hEvtConnectionRequested);
				}

				LeaveCriticalSection (&sTracerControlCS);
				break;

			default:
				return 1; // Unexpected wake up reason: exit thread now
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ Tracer::SetCurrentTracerTypeIndex
// ---------------------------------------------------------------------------

void Tracer::SetCurrentTracerTypeIndex (unsigned short tracerType, Bool paramChange)
{
	if (runningTracerType)
	{
		previousTracerType = runningTracerType;
	}

	runningTracerType = tracerType;

	// Stop trying to reconnect
	SetEvent (hEvtNoMoreTries);

	if (tracerType)
	{
		// All tracer Types but the nil tracer Type must support init, close and output calls
		tracer_init_outputport	= (void (__stdcall *)(HINSTANCE, const char*,const char*)) GetProcAddress(hTracerLib, "tracer_init_outputport");
		tracer_close_outputport	= (void (__stdcall *)(void)) GetProcAddress(hTracerLib, "tracer_close_outputport");

		tracer_output_VT		= (void (__stdcall *)(unsigned short,const char *,const va_list * )) GetProcAddress(hTracerLib, "tracer_output_VT");
		tracer_output_VTL		= (void (__stdcall *)(unsigned short,const char *,const va_list * )) GetProcAddress(hTracerLib, "tracer_output_VTL");
		tracer_output_B			= (void (__stdcall *)(unsigned short,const void *,unsigned int))	GetProcAddress(hTracerLib, "tracer_output_B");

		// If the tracer supports auto-connect and that it's enabled, use it
		if (GetTracerTypeInfo(tracerType)->autoConnectCurState)
		{
			tracer_get_outputport_status	= (long (__stdcall *)(void)) GetProcAddress(hTracerLib, "tracer_get_outputport_status");
		}
		else
		{
			tracer_get_outputport_status	= 0;
		}
	}
	else
	{
		// The nil tracer doesn't support any service
		tracer_init_outputport		= 0;
		tracer_close_outputport		= 0;
		tracer_output_VT			= 0;
		tracer_output_VTL			= 0;
		tracer_output_B				= 0;
		tracer_get_outputport_status= 0;
	}

	// If there are trace subsystem users
	if (tracerRefCounter > 0)
	{
		// If we feel that we should close the current output port and open a new one
		if (tracerType != previousTracerType || paramChange)
		{
			if (tracer_init_outputport)
			{
				// Establish connection to the new tracer
				// As a side effect this closes any pending connection, whatever the previous tracer Type was
				(*tracer_init_outputport) (GetModuleHandle(0), GetTracerTypeInfo(runningTracerType)->type, GetTracerTypeInfo(runningTracerType)->paramCurVal);
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		¥ Tracer::GetTracerCapabilities
// ---------------------------------------------------------------------------

void Tracer::GetTracerCapabilities (char* buffer, size_t* buflen)
{
	// If buffer is null, the function will set buglen to the required len
	// otherwise will fill the buffer, or set it to \0\0 if it's too small
	if (tracer_get_caps)
	{	
		(*tracer_get_caps)(buffer, buflen);
	}
	else
	{
		// Library not found
		if (buffer)
		{
			buffer[0] = 0;
			buffer[1] = 0;
		}
		
		*buflen = 2;
	}
}


// ---------------------------------------------------------------------------
//		¥ Tracer::StopTracer
// ---------------------------------------------------------------------------

void Tracer::StopTracer (void)
{
	void (__stdcall *tracer_close_outputport) (void);

	EnterCriticalSection (&sTracerControlCS);

	// Stop trying to reconnect
	SetEvent (hEvtNoMoreTries);

	// Force a shutdown of any pending connection
	tracer_close_outputport	= (void (__stdcall *)(void)) GetProcAddress(hTracerLib, "tracer_close_outputport");

	// This works even if the user switched to nil tracer (the tracer is not shutdown
	// in this case because if the user reselect it, we want traces to be appended to the output)

	if (tracer_close_outputport)
	{
		(*tracer_close_outputport) ();
	}

	tracerRefCounter = 0;

	LeaveCriticalSection (&sTracerControlCS);
}


// ---------------------------------------------------------------------------
//		¥ Tracer::GetLibraryVersionString
// ---------------------------------------------------------------------------

void	Tracer::GetLibraryVersionString (char* buffer, size_t bufferLen)
{
	DWORD			dummy;
	char			imageName [MAX_PATH];
	DWORD			versionInfoSize;
	bool			descriptionRead = false;
	char *			versionInfoBuffer = 0;
	char *			pFileDescr;
	unsigned int	nFileDescrLen;

	if (hTracerLib)
	{
		// Get the exact path+name of the PalmTrace DLL that was loaded
		GetModuleFileName (hTracerLib, imageName, sizeof(imageName));
			
		// Extract its description from the VERSIONINFO embedded in the file		
		versionInfoSize = GetFileVersionInfoSize (imageName, &dummy);
		versionInfoBuffer = new char [versionInfoSize];
		descriptionRead = (GetFileVersionInfo (imageName, dummy, versionInfoSize, versionInfoBuffer) != 0);

		VerQueryValue(	versionInfoBuffer, 
						"\\StringFileInfo\\040904B0\\FileDescription", 
						(void**) &pFileDescr, 
						&nFileDescrLen); 
	}

	if (descriptionRead && nFileDescrLen)
	{
		_snprintf (buffer, bufferLen-1, "%s detected", pFileDescr);
	}
	else
	{
		if (GetLibraryVersionNumber())
		{
			_snprintf (buffer, bufferLen-1, "Found outdated %s", kTraceLibraryName);
		}
		else
		{
			_snprintf (buffer, bufferLen-1, "%s not found", kTraceLibraryName);
		}
	}

	delete versionInfoBuffer;
}


// ---------------------------------------------------------------------------
//		¥ Tracer::GetLibraryVersionNumber
// ---------------------------------------------------------------------------

unsigned long Tracer::GetLibraryVersionNumber (void)
{
	DWORD			dummy;
	char			imageName [MAX_PATH];
	DWORD			versionInfoSize;
	bool			descriptionRead = false;
	char *			versionInfoBuffer = 0;
	char *			pFileDescr;
	unsigned int	nFileDescrLen;
	unsigned char	nDigit1 = 0, nDigit2 = 0, nDigit3 = 0, nDigit4 = 0;
	char *			s;
	bool			loadedDLL = false;

	if (!hTracerLib)
	{
		hTracerLib = LoadLibrary (kTraceLibraryName);
		loadedDLL = true;
	}
		
	if (hTracerLib)
	{
		// Get the exact path+name of the PalmTrace DLL that was loaded
		GetModuleFileName (hTracerLib, imageName, sizeof(imageName));
			
		// Extract its description from the VERSIONINFO embedded in the file		
		versionInfoSize = GetFileVersionInfoSize (imageName, &dummy);
		versionInfoBuffer = new char [versionInfoSize];
		descriptionRead = (GetFileVersionInfo (imageName, dummy, versionInfoSize, versionInfoBuffer) != 0);

		VerQueryValue(	versionInfoBuffer, 
						"\\StringFileInfo\\040904B0\\ProductVersion", 
						(void**) &pFileDescr, 
						&nFileDescrLen);

		s= pFileDescr;
		
		if (s && *s)
		{
			nDigit1 = atoi(s);
			s = strchr(s+1, '.');
		}

		if (s && *s)
		{
			nDigit2 = atoi(s+1);
			s = strchr(s+1, '.');
		}

		if (s && *s)
		{
			nDigit3 = atoi(s+1);
			s = strchr(s+1, '.');
		}
		
		if (s && *s)
		{
			nDigit4 = atoi(s+1);
			s = strchr(s+1, '.');
		}
	}

	if (hTracerLib && loadedDLL)
	{
		FreeLibrary (hTracerLib);
		hTracerLib = 0;
	}

	return (nDigit1 << 24) + (nDigit2 << 16) + (nDigit3 << 8) + nDigit4;
}


// ---------------------------------------------------------------------------
//		¥ Tracer::IsLibraryLoaded
// ---------------------------------------------------------------------------

bool Tracer::IsLibraryLoaded (void)
{
	return (hTracerLib != 0);
}
