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
#include "EmTransportSerialWin.h"

#include "Logging.h"			// LogSerial
#include "Platform.h"			// Platform::AllocateMemory


#define WINDOWS_95_SUCKS	1


static void PrvSearchForPorts (HKEY HLM, HKEY key, EmTransportSerial::PortNameList& results);

#define PRINTF	if (!LogSerial ()) ; else LogAppendMsg


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostConstruct
 *
 * DESCRIPTION:	Construct platform-specific objects/data.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostConstruct (void)
{
	fHost = new EmHostTransportSerial;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostDestruct
 *
 * DESCRIPTION:	Destroy platform-specific objects/data.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostDestruct (void)
{
	delete fHost;
	fHost = NULL;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostOpen
 *
 * DESCRIPTION:	Open the serial port in a platform-specific fashion.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::HostOpen (void)
{
	ErrCode	err = fHost->OpenCommPort (fConfig);

	if (!err)
		err = fHost->CreateCommThreads (fConfig);

	if (err)
		this->HostClose ();

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostClose
 *
 * DESCRIPTION:	Close the serial port in a platform-specific fashion.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::HostClose (void)
{
	ErrCode	err;

	err = fHost->DestroyCommThreads ();
	err = fHost->CloseCommPort ();

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostRead
 *
 * DESCRIPTION:	Read bytes from the port in a platform-specific fashion.
 *
 * PARAMETERS:	len - maximum number of bytes to read.
 *				data - buffer to receive the bytes.
 *
 * RETURNED:	0 if no error.  The number of bytes actually read is
 *				returned in len if there was no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::HostRead (long& len, void* data)
{
	fHost->GetIncomingData (data, len);

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostWrite
 *
 * DESCRIPTION:	Write bytes to the port in a platform-specific fashion.
 *
 * PARAMETERS:	len - number of bytes in the buffer.
 *				data - buffer containing the bytes.
 *
 * RETURNED:	0 if no error.  The number of bytes actually written is
 *				returned in len if there was no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::HostWrite (long& len, const void* data)
{
	fHost->PutOutgoingData (data, len);

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostBytesInBuffer
 *
 * DESCRIPTION:	Returns the number of bytes that can be read with the
 *				Read method.  Note that bytes may be received in
 *				between the time BytesInBuffer is called and the time
 *				Read is called, so calling the latter with the result
 *				of the former is not guaranteed to fetch all received
 *				and buffered bytes.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Number of bytes that can be read.
 *
 ***********************************************************************/

long EmTransportSerial::HostBytesInBuffer (void)
{
	return fHost->IncomingDataSize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostSetConfig
 *
 * DESCRIPTION:	Configure the serial port in a platform-specific
 *				fasion.  The port is assumed to be open.
 *
 * PARAMETERS:	config - configuration information.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::HostSetConfig (const ConfigSerial& config)
{
	PRINTF ("EmTransportSerial::HostSetConfig: Setting settings.");

	ErrCode	err = errNone;

	// Get the current state.

	DCB dcb;
	dcb.DCBlength		= sizeof (dcb);
	if (!::GetCommState (fHost->fCommHandle, &dcb))
	{
		err = (ErrCode) ::GetLastError ();
		return err;
	}

	// Set the state to the way we want it.
	//
	// Note that for fRtsControl, I used to have:
	//
	//	dcb.fRtsControl = config.fHwrHandshake ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;
	//
	// However, Olivier Naudan argues:
	//
	//	But I don't agree with: dcb.fRtsControl  = config.fHwrHandshake ?
	//		RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;
	//	As hardware overrun can't be emulated, because of Poser timing that
	//	can't be accurate, I believe that fRtsControl should always be set to
	//	RTS_CONTROL_HANDSHAKE. So bytes will always arrive succesfully to host,
	//	and only software overrun could happen if PalmOS does not control the
	//	emulated RTS.

	dcb.BaudRate		= config.fBaud;
	dcb.fBinary 		= TRUE;
	dcb.fParity 		= TRUE;
	dcb.fOutxCtsFlow	= config.fHwrHandshake;
	dcb.fOutxDsrFlow	= FALSE;
	dcb.fDtrControl 	= DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX			= FALSE;
	dcb.fInX			= FALSE;
	dcb.fErrorChar		= FALSE;
	dcb.fNull			= FALSE;
	dcb.fRtsControl 	= RTS_CONTROL_HANDSHAKE;
	dcb.fAbortOnError	= TRUE;
	dcb.ByteSize		= config.fDataBits;
	dcb.Parity			= config.fParity == EmTransportSerial::kNoParity ? NOPARITY :
						  config.fParity == EmTransportSerial::kOddParity ? ODDPARITY : EVENPARITY;
	dcb.StopBits		= config.fStopBits == 1 ? ONESTOPBIT : TWOSTOPBITS;

	PRINTF ("	dcb.DCBlength			= %ld", (long) dcb.DCBlength);
	PRINTF ("	dcb.BaudRate			= %ld", (long) dcb.BaudRate);
	PRINTF ("	dcb.fBinary				= %ld", (long) dcb.fBinary);
	PRINTF ("	dcb.fParity				= %ld", (long) dcb.fParity);
	PRINTF ("	dcb.fOutxCtsFlow		= %ld", (long) dcb.fOutxCtsFlow);
	PRINTF ("	dcb.fOutxDsrFlow		= %ld", (long) dcb.fOutxDsrFlow);
	PRINTF ("	dcb.fDtrControl			= %ld", (long) dcb.fDtrControl);
	PRINTF ("	dcb.fDsrSensitivity		= %ld", (long) dcb.fDsrSensitivity);
	PRINTF ("	dcb.fTXContinueOnXoff	= %ld", (long) dcb.fTXContinueOnXoff);
	PRINTF ("	dcb.fOutX				= %ld", (long) dcb.fOutX);
	PRINTF ("	dcb.fInX				= %ld", (long) dcb.fInX);
	PRINTF ("	dcb.fErrorChar			= %ld", (long) dcb.fErrorChar);
	PRINTF ("	dcb.fNull				= %ld", (long) dcb.fNull);
	PRINTF ("	dcb.fRtsControl			= %ld", (long) dcb.fRtsControl);
	PRINTF ("	dcb.fAbortOnError		= %ld", (long) dcb.fAbortOnError);
	PRINTF ("	dcb.fDummy2				= %ld", (long) dcb.fDummy2);
	PRINTF ("	dcb.wReserved			= %ld", (long) dcb.wReserved);
	PRINTF ("	dcb.XonLim				= %ld", (long) dcb.XonLim);
	PRINTF ("	dcb.XoffLim				= %ld", (long) dcb.XoffLim);
	PRINTF ("	dcb.ByteSize			= %ld", (long) dcb.ByteSize);
	PRINTF ("	dcb.Parity				= %ld", (long) dcb.Parity);
	PRINTF ("	dcb.StopBits			= %ld", (long) dcb.StopBits);
	PRINTF ("	dcb.XonChar				= %ld", (long) dcb.XonChar);
	PRINTF ("	dcb.XoffChar			= %ld", (long) dcb.XoffChar);
	PRINTF ("	dcb.ErrorChar			= %ld", (long) dcb.ErrorChar);
	PRINTF ("	dcb.EofChar				= %ld", (long) dcb.EofChar);
	PRINTF ("	dcb.EvtChar				= %ld", (long) dcb.EvtChar);
	PRINTF ("	dcb.wReserved1			= %ld", (long) dcb.wReserved1);

	// Install the new state.

	if (!::SetCommState (fHost->fCommHandle, &dcb))
	{
		err = (ErrCode) ::GetLastError ();
	}

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostGetSerialPortNameList
 *
 * DESCRIPTION:	Return the list of serial ports on this computer.  Used
 *				to prepare a menu of serial port choices.
 *
 * PARAMETERS:	nameList - port names are added to this list.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostGetSerialPortNameList (PortNameList& results)
{
	HKEY	curRootKey;

	if (::RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Enum", 0, KEY_READ, &curRootKey) == ERROR_SUCCESS)
	{
		// Windows 95
		//
		// Gotta look for them the hard way. The NT way is not reliable. For instance,
		// using the NT way under NT will turn up COM1, COM2, and COM3 on my PC, but
		// under NT will only turn up COM1 and COM2.

		::PrvSearchForPorts (HKEY_LOCAL_MACHINE, curRootKey, results);
	}
	else if (::RegOpenKeyEx (HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &curRootKey) == ERROR_SUCCESS)
	{
		// Windows NT
		//
		// This one's easy: just iterate over HARDWARE\DEVICEMAP\SERIALCOMM to get
		// the names of all the ports.

		DWORD	ii = 0;
		while (1)
		{
			char	name[256], value[256];
			DWORD	nameSize = sizeof(name);
			DWORD	valueSize = sizeof(value);
			DWORD	type;

			if (::RegEnumValue (curRootKey, ii, name, &nameSize, NULL, &type, (BYTE*) value, &valueSize) != ERROR_SUCCESS)
			{
				break;
			}

			results.push_back (string (value));
			++ii;
		}
	}

	::RegCloseKey (curRootKey);

	sort (results.begin (), results.end ());
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostGetSerialBaudList
 *
 * DESCRIPTION:	Return the list of baud rates support by this computer.
 *				Used to prepare a menu of baud rate choices.
 *
 * PARAMETERS:	baudList - baud rates are added to this list.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostGetSerialBaudList (BaudList& results)
{
	long	maxBaud = 115200;	// ::PrvGetMaxBaudRate ()? How to
								// determine that in Windows?.

	switch (maxBaud)
	{
		case 115200:	results.push_back (115200);
		case 57600:		results.push_back (57600);
		case 38400:		results.push_back (38400);
		case 19200:		results.push_back (19200);
		case 14400:		results.push_back (14400);
		case 9600:		results.push_back (9600);
	}
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmHostTransportSerial::EmHostTransportSerial (void) :
	fCommHandle (NULL),

	fCommReadThread (NULL),
	fCommReadQuitEvent (NULL),

	fCommWriteThread (NULL),
	fCommWriteQuitEvent (NULL),
	fCommWriteDataEvent (NULL),

	fReadMutex (),
	fReadBuffer (),

	fWriteMutex (),
	fWriteBuffer ()
{
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmHostTransportSerial::~EmHostTransportSerial (void)
{
	EmAssert (fCommHandle == NULL);

	EmAssert (fCommReadThread == NULL);
	EmAssert (fCommReadQuitEvent == NULL);

	EmAssert (fCommWriteThread == NULL);
	EmAssert (fCommWriteQuitEvent == NULL);
	EmAssert (fCommWriteDataEvent == NULL);
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::OpenCommPort
 *
 * DESCRIPTION:	Open the serial port.
 *
 * PARAMETERS:	config - data block describing which port to use.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmHostTransportSerial::OpenCommPort (const EmTransportSerial::ConfigSerial& config)
{
	ErrCode	err = errNone;

	EmTransportSerial::PortName	portName = config.fPort;

	PRINTF ("EmTransportSerial::HostOpen: attempting to open port \"%s\"",
			portName.c_str());

	if (!portName.empty ())
	{
		PRINTF ("EmTransportSerial::HostOpen: Opening serial port...");

		fCommHandle = ::CreateFile (portName.c_str (), GENERIC_READ | GENERIC_WRITE, 0,
								NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

		if (fCommHandle == INVALID_HANDLE_VALUE)
		{
			fCommHandle = NULL;

			err = (ErrCode) ::GetLastError ();
			return err;
		}

		// Configure the timeout values.
		//
		// These COMMTIMEOUTS seem to work pretty well for me.  With them,
		// I can download a Palm III ROM in 2.4 minutes.  With the settings
		// I used to use (50/0/100/0/0), I would download the ROM in 4 minutes.
		// With the settings HotSync uses (3000/25/2000/25/2500), downloading
		// would be so slow that I couldn't bear waiting for it to complete.

		COMMTIMEOUTS	ctmo;

		ctmo.ReadIntervalTimeout			= MAXDWORD;
		ctmo.ReadTotalTimeoutMultiplier 	= MAXDWORD;
		ctmo.ReadTotalTimeoutConstant		= MAXDWORD - 1;
		ctmo.WriteTotalTimeoutMultiplier	= 0;
		ctmo.WriteTotalTimeoutConstant		= 0;

		PRINTF ("	ctmo.ReadIntervalTimeout			= %ld", (long) ctmo.ReadIntervalTimeout);
		PRINTF ("	ctmo.ReadTotalTimeoutMultiplier		= %ld", (long) ctmo.ReadTotalTimeoutMultiplier);
		PRINTF ("	ctmo.ReadTotalTimeoutConstant		= %ld", (long) ctmo.ReadTotalTimeoutConstant);
		PRINTF ("	ctmo.WriteTotalTimeoutMultiplier	= %ld", (long) ctmo.WriteTotalTimeoutMultiplier);
		PRINTF ("	ctmo.WriteTotalTimeoutConstant		= %ld", (long) ctmo.WriteTotalTimeoutConstant);

		if (!::SetCommTimeouts (fCommHandle, &ctmo))
		{
			err = (ErrCode) ::GetLastError ();
			return err;
		}
	}
	else
	{
		PRINTF ("EmTransportSerial::HostOpen: No port selected in the Properties dialog box...");
		return -1;	// !!! better error number
	}

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::CreateCommThreads
 *
 * DESCRIPTION:	Create the threads that asynchronously read from and
 *				write to the serial port.
 *
 * PARAMETERS:	config - data block describing which port to use.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmHostTransportSerial::CreateCommThreads (const EmTransportSerial::ConfigSerial& /*config*/)
{
	PRINTF ("EmTransportSerial::HostOpen: Creating serial port handler threads...");

	// Create the events that the threads blocks on.

	fCommReadQuitEvent = ::CreateEvent (	NULL,		// pointer to security attributes
											TRUE,		// flag for manual-reset event
											FALSE,		// flag for initial state
											NULL);		// pointer to event-object name);

	fCommWriteQuitEvent = ::CreateEvent (	NULL,		// pointer to security attributes
											TRUE,		// flag for manual-reset event
											FALSE,		// flag for initial state
											NULL);		// pointer to event-object name);

	fCommWriteDataEvent = ::CreateEvent (	NULL,		// pointer to security attributes
											TRUE,		// flag for manual-reset event
											FALSE,		// flag for initial state
											NULL);		// pointer to event-object name);

	// Create the threads that will interact with the port.  Make
	// them high priority threads.

	DWORD tid;
	fCommReadThread = ::CreateThread (
							NULL,		// security descriptor
							0,			// stack size
							CommRead,	// thread function
							this,		// parameter to thread function
							0,			// creation flags
							&tid);		// address of thread ID

	fCommWriteThread = ::CreateThread (
							NULL,		// security descriptor
							0,			// stack size
							CommWrite,	// thread function
							this,		// parameter to thread function
							0,			// creation flags
							&tid);		// address of thread ID

	::SetThreadPriority (fCommReadThread, THREAD_PRIORITY_HIGHEST);
	::SetThreadPriority (fCommWriteThread, THREAD_PRIORITY_HIGHEST);

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::DestroyCommThreads
 *
 * DESCRIPTION:	Shutdown and destroy the comm threads.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmHostTransportSerial::DestroyCommThreads (void)
{
	if (fCommReadThread)
	{
		// Wake up the threads, signalling them to quit.

		::SetEvent (fCommReadQuitEvent);
		::SetEvent (fCommWriteQuitEvent);

		// Wait for them to quit.

		HANDLE	objects[] = { fCommReadThread, fCommWriteThread };
		::WaitForMultipleObjects (countof (objects), objects, TRUE, INFINITE);

		// Close the objects.

		BOOL	result;

		result = ::CloseHandle (fCommReadThread);
		result = ::CloseHandle (fCommReadQuitEvent);

		result = ::CloseHandle (fCommWriteThread);
		result = ::CloseHandle (fCommWriteQuitEvent);
		result = ::CloseHandle (fCommWriteDataEvent);

		// Remove our references.

		fCommReadThread = NULL;
		fCommReadQuitEvent = NULL;

		fCommWriteThread = NULL;
		fCommWriteQuitEvent = NULL;
		fCommWriteDataEvent = NULL;
	}

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::CloseCommPort
 *
 * DESCRIPTION:	Close the comm port.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmHostTransportSerial::CloseCommPort (void)
{
	BOOL	result;

	result = ::CloseHandle (fCommHandle);
	fCommHandle = NULL;

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::PutIncomingData
 *
 * DESCRIPTION:	Thread-safe method for adding data to the queue that
 *				holds data read from the serial port.
 *
 * PARAMETERS:	data - pointer to the read data.
 *				len - number of bytes pointed to by "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmHostTransportSerial::PutIncomingData	(const void* data, long& len)
{
	if (len == 0)
		return;

	omni_mutex_lock lock (fReadMutex);

	char*	begin = (char*) data;
	char*	end = begin + len;
	while (begin < end)
		fReadBuffer.push_back (*begin++);
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::GetIncomingData
 *
 * DESCRIPTION:	Thread-safe method for getting data from the queue
 *				holding data read from the serial port.
 *
 * PARAMETERS:	data - pointer to buffer to receive data.
 *				len - on input, number of bytes available in "data".
 *					On exit, number of bytes written to "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmHostTransportSerial::GetIncomingData	(void* data, long& len)
{
	omni_mutex_lock lock (fReadMutex);

	if (len > fReadBuffer.size ())
		len = fReadBuffer.size ();

	char*	p = (char*) data;
	deque<char>::iterator	begin = fReadBuffer.begin ();
	deque<char>::iterator	end = begin + len;

	copy (begin, end, p);
	fReadBuffer.erase (begin, end);
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::IncomingDataSize
 *
 * DESCRIPTION:	Thread-safe method returning the number of bytes in the
 *				read queue.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Number of bytes in the read queue.
 *
 ***********************************************************************/

long EmHostTransportSerial::IncomingDataSize (void)
{
	omni_mutex_lock lock (fReadMutex);

	return fReadBuffer.size ();
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::PutOutgoingData
 *
 * DESCRIPTION:	Thread-safe method for adding data to the queue that
 *				holds data to be written to the serial port.
 *
 * PARAMETERS:	data - pointer to the read data.
 *				len - number of bytes pointed to by "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmHostTransportSerial::PutOutgoingData	(const void* data, long& len)
{
	if (len == 0)
		return;

	omni_mutex_lock lock (fWriteMutex);

	char*	begin = (char*) data;
	char*	end = begin + len;
	while (begin < end)
		fWriteBuffer.push_back (*begin++);

	// Wake up CommWrite.
	::SetEvent (fCommWriteDataEvent);
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::GetOutgoingData
 *
 * DESCRIPTION:	Thread-safe method for getting data from the queue
 *				holding data to be written to the serial port.
 *
 * PARAMETERS:	data - pointer to buffer to receive data.
 *				len - on input, number of bytes available in "data".
 *					On exit, number of bytes written to "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmHostTransportSerial::GetOutgoingData	(void* data, long& len)
{
	omni_mutex_lock lock (fWriteMutex);

	if (len > fWriteBuffer.size ())
		len = fWriteBuffer.size ();

	char*	p = (char*) data;
	deque<char>::iterator	begin = fWriteBuffer.begin ();
	deque<char>::iterator	end = begin + len;

	copy (begin, end, p);
	fWriteBuffer.erase (begin, end);

	::ResetEvent (fCommWriteDataEvent);
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::OutgoingDataSize
 *
 * DESCRIPTION:	Thread-safe method returning the number of bytes in the
 *				write queue.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Number of bytes in the read queue.
 *
 ***********************************************************************/

long EmHostTransportSerial::OutgoingDataSize (void)
{
	omni_mutex_lock lock (fWriteMutex);

	return fWriteBuffer.size ();
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::CommRead
 *
 * DESCRIPTION:	This function sits in its own thread, waiting for data
 *				to show up in the serial port.	If data arrives, this
 *				function plucks it out and stores it in a thread-safe
 *				queue.  It quits when it detects that the comm handle
 *				has been deleted.
 *
 * PARAMETERS:	data - pointer to owning EmHostTransportSerial.
 *
 * RETURNED:	Thread status.
 *
 ***********************************************************************/

DWORD __stdcall EmHostTransportSerial::CommRead (void* data)
{
	EmHostTransportSerial*	This = (EmHostTransportSerial*) data;

	PRINTF ("CommRead starting.");

	HANDLE	hOverlappedEvent = ::CreateEvent (	NULL,		// pointer to security attributes
												TRUE,		// flag for manual-reset event
												FALSE,		// flag for initial state
												NULL);		// pointer to event-object name

	while (This->fCommHandle)
	{
		// Prepare to read some data.

		OVERLAPPED	ov;
		memset (&ov, 0, sizeof(ov));
		ov.hEvent = hOverlappedEvent;

		char	buf[1024];
#if WINDOWS_95_SUCKS
		DWORD	len = 1;
#else
		DWORD	len = 1024;
#endif

		// See if there's any data on the port.  If not, ReadFile will return
		// false and GetLastError will return ERROR_IO_PENDING.

		BOOL	readFinished = ::ReadFile (This->fCommHandle, buf, len, &len, &ov);

		if (!readFinished)
		{
			DWORD	err = ::GetLastError();
			if (err != ERROR_IO_PENDING)
			{
				::GetOverlappedResult (This->fCommHandle, &ov, &len, FALSE);
				break;	// !!! What else to do?
			}

			// No data is pending.  Block on the serial port, as well as the object
			// that gets signalled when we want the thread to quit.

			HANDLE	objects[] = { This->fCommReadQuitEvent, ov.hEvent };
			DWORD	object = ::WaitForMultipleObjects (countof (objects), objects, FALSE, INFINITE);

			// "Wake up! Time to die!"  Cancel the read operation and blow.

			if (object == WAIT_OBJECT_0)
			{
				break;
			}

			if (object != WAIT_OBJECT_0 + 1)
				continue;	// !!! What else to do?

			BOOL	ovRes = ::GetOverlappedResult (This->fCommHandle, &ov, &len, FALSE);
			if (ovRes == FALSE)
				continue;	// !!! What else to do?
		}

		if (len > 0)
		{
			// Log the data.
			if (LogSerialData ())
				LogAppendData (buf, len, "EmHostTransportSerial::CommRead: Received data:");
			else
				PRINTF ("EmHostTransportSerial::CommRead: Received %ld serial bytes.", len);

			// Add the data to the EmHostTransportSerial object's buffer.
			long	n = (long) len;
			This->PutIncomingData (buf, n);
		}
	}

	::CloseHandle (hOverlappedEvent);

	PRINTF ("CommRead exitting.");

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::CommWrite
 *
 * DESCRIPTION:	This function sits in its own thread, waiting for data
 *				to show up in the write queue.	If data arrives, this
 *				function plucks it out and sends it out to the port.  It
 *				quits when it detects that the comm handle has been
 *				deleted.
 *
 * PARAMETERS:	data - pointer to owning EmHostTransportSerial.
 *
 * RETURNED:	Thread status.
 *
 ***********************************************************************/

DWORD __stdcall EmHostTransportSerial::CommWrite (void* data)
{
	EmHostTransportSerial*	This = (EmHostTransportSerial*) data;

	PRINTF ("CommWrite starting.");

	HANDLE	hOverlappedEvent = ::CreateEvent (	NULL,		// pointer to security attributes
												TRUE,		// flag for manual-reset event
												FALSE,		// flag for initial state
												NULL);		// pointer to event-object name

	BOOL	timeToDie = false;
	while (!timeToDie)
	{
		// Wait for some data to show up.  This is an automatic reset
		// event, so we don't have to clear it.  Similarly, if more
		// data comes in while we're elsewhere in this loop, the
		// event will be set when we come back here, allowing us
		// to pick up that new data.

		HANDLE	objects[] = { This->fCommWriteQuitEvent, This->fCommWriteDataEvent };
		DWORD	object = ::WaitForMultipleObjects (countof (objects), objects, FALSE, INFINITE);

		if (object == WAIT_OBJECT_0)
		{
			timeToDie = true;	// Send any data, then die.
			PRINTF ("CommWrite: Time To Die.");
		}

		if (object != WAIT_OBJECT_0 && object != WAIT_OBJECT_0 + 1)
			continue;	// !!! What else to do?

		// Get the data to write.

		long	len = This->OutgoingDataSize ();

		// If there really wasn't any, go back to sleep.

		if (len == 0)
			continue;

		// Get the data.

		void*	buf = Platform::AllocateMemory (len);
		This->GetOutgoingData (buf, len);

		// Log the data.

		if (LogSerialData ())
			LogAppendData (buf, len, "EmHostTransportSerial::CommWrite: Transmitted data:");
		else
			PRINTF ("EmHostTransportSerial::CommWrite: Transmitted %ld serial bytes.", len);

		// Prepare to write the data.

		OVERLAPPED ov;
		memset (&ov, 0, sizeof (ov));
		ov.hEvent = hOverlappedEvent;

		DWORD	n = (DWORD) len;
		DWORD	err = 0;

		// Write the data.  If the operation doesn't complete, block until it does.

		if (!::WriteFile (This->fCommHandle, buf, n, &n, &ov) &&
			(err = ::GetLastError ()) == ERROR_IO_PENDING)
		{
			BOOL	result = ::GetOverlappedResult (This->fCommHandle, &ov, &n, TRUE);	// Wait for operation to end.
			if (!result)
			{
				err = ::GetLastError ();
				PRINTF ("CommWrite: GetOverlappedResult: err = %ld, n = %ld", err, n);
			}
		}
		else if (err != 0)
		{
			PRINTF ("CommWrite: WriteFile: err = %ld", err);
		}

		// Dispose of the data.

		Platform::DisposeMemory (buf);
	}

	::CloseHandle (hOverlappedEvent);

	PRINTF ("CommWrite exitting.");

	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	PrvSearchForPorts
 *
 * DESCRIPTION:	Recursive routine used on Windows NT to find the
 *				serial port devices.
 *
 * PARAMETERS:	HLM - HKEY for "HKEY_LOCAL_MACHINE".
 *				key - HKEY for specific part of the Registry we're
 *					searching in right now.
 *				results - container for found port names.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

//	Look for serial port information.  This search consists of two steps:
//	first finding the hardware keys, and then using information there
//	to look up the software keys in order to determine which of those keys
//	are for serial ports.
//
//	To find the hardware keys, we have to do a recursive search over
//	HKEY_LOCAL_MACHINE\Enum\.  This tree contains the hardware keys either
//	found by Plug and Play, built-in enumerators, or installation programs.
//	We're looking for keys with sub-keys of "Class" that contain the
//	string "Ports".  When we find one of those, we can get the "display
//	name" from either "PortName" ("COM1") or "FRIENDLYNAAME" ("Communications
//	Port (COM1)").
//
//	After we have that information, we still have to find out of the port
//	we're looking at is for a serial port.	To determine that, we have to
//	look up the software key.  Software keys are in HKEY_LOCAL_MACHINE\
//	System\CurrentControlSet\Services\Class\.  To determine which sub-key
//	to look up once we get there, we have to look back at the hardware key.
//	It contains a sub-key called "Driver" which contains the text of the
//	software sub-key to use.
//
//	Once we're at the right software sub-key, we have to look at the
//	"PortSubClass" sub-sub-key.  This key will contain 00 for parallel
//	ports, 01 for serial ports, and 02 for modem ports.
//
//	Whew!
//
//	By the way, a relatively "good" MSDN article talking about this is
//	"Installation and the Registry" in the "Windows 95 Device Driver Kit".
//	Just to nail things down, here are some sample registry entries for
//	you to look at:
//
//	Example Hardware Key:
//
//	[ HKEY_LOCAL_MACHINE\Enum\BIOS\*PNP0501\0C ]
//		Class			= "Ports"
//		PortName		= "COM1"
//		FRIENDLYNAME	= "Communications Port (COM1)"
//		Driver			= "Ports\0004"
//		...more...
//
//
//	Corresponding Software Key:
//
//	[ HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\Ports\0004 ]
//		PortSubClass	= 01
//		...more...

void PrvSearchForPorts (HKEY HLM, HKEY key, EmTransportSerial::PortNameList& results)
{
	char	buf[256];
	DWORD	n = sizeof(buf);
	DWORD	type;

	// See if we've got a "Class" key containing "Ports"

	if (::RegQueryValueEx (key, "Class", NULL, &type, (BYTE*) buf, &n) == ERROR_SUCCESS &&
		stricmp (buf, "Ports") == 0)
	{
		// We've got one. Now see if there's a "PortName" key. If there isn't
		// one, we assume we have a bogus hit. Otherwise, we use the value
		// as a comm port name.

		char	name[256];
		n = sizeof (name);

		if (::RegQueryValueEx (key, "PortName", NULL, &type, (BYTE*) name, &n) == ERROR_SUCCESS)
		{
			// We've got a "PortName" key/value. Now let's go for the "Driver"
			// key so that we can see if this port is a serial port. If we fail,
			// again bail out.

			n = sizeof (buf);

			if (::RegQueryValueEx (key, "Driver", NULL, &type, (BYTE*) buf, &n) == ERROR_SUCCESS)
			{
				// We now have the tag-end of the full software key. Catenate it
				// to the standard front-end, and go to the software key hierarchy.

				char kname[MAX_PATH];

				strcpy (kname, "System\\CurrentControlSet\\Services\\Class\\");
				strcat (kname, buf);

				HKEY softwareKey;

				if (::RegOpenKeyEx (HLM, kname, NULL, KEY_READ, &softwareKey) == ERROR_SUCCESS)
				{
					// Got this far...Now look up the "PortSubClass"

					n = sizeof(buf);
					if (::RegQueryValueEx (softwareKey, "PortSubClass", NULL, &type, (BYTE*) buf, &n) == ERROR_SUCCESS)
					{
						if (buf[0] == 1)
						{
							results.push_back (string (name));
						}
					}

					::RegCloseKey (softwareKey);
				}
			}
		}
	}

	// No "Class" key at this level of the Registry hierarchy.
	// Iterate over the sub-keys.

	else
	{
		DWORD	ii = 0;
		while (1)
		{
			char		keyName[256], keyClass[256];
			DWORD		keyLen = sizeof(keyName);
			DWORD		classLen = sizeof(keyClass);
			FILETIME	ft;

			// Get the next key. If there are no more, break out of this loop.

			if (::RegEnumKeyEx (key, ii, keyName, &keyLen, NULL, keyClass, &classLen, &ft) != ERROR_SUCCESS)
			{
				break;
			}

			// Iterate over the next level by recursing.

			HKEY	k;
			if (::RegOpenKeyEx (key, keyName, NULL, KEY_READ, &k) == ERROR_SUCCESS)
			{
				::PrvSearchForPorts (HLM, k, results);
				::RegCloseKey (k);
			}

			++ii;
		}
	}
}
