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


static void PrvSearchForPorts (HKEY HLM, HKEY key, EmTransportSerial::PortNameList& results);


#define PRINTF	if (!LogSerial ()) ; else LogAppendMsg
#define DPRINTF	if (1) ; else LogAppendMsg


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

long EmTransportSerial::HostBytesInBuffer (long /*minBytes*/)
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

	DCB dcb = { sizeof (DCB) };

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
	dcb.fDtrControl 	= DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX			= FALSE;
	dcb.fInX			= FALSE;
	dcb.fErrorChar		= FALSE;
	dcb.fNull			= FALSE;
	dcb.fRtsControl 	= RTS_CONTROL_HANDSHAKE;
	dcb.fAbortOnError	= FALSE;
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
 * FUNCTION:	EmTransportSerial::HostSetRTS
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostSetRTS (RTSControl state)
{
	// I implemented this stuff, but don't have a way to test it, so
	// I'm not turning it on for now...

#if 0
	DWORD	err;

	// Get the desired setting for fRtsControl.

	DWORD	fRtsControl =	state == kRTSAuto ? RTS_CONTROL_HANDSHAKE :
							state == kRTSOff ? RTS_CONTROL_DISABLE :
							RTS_CONTROL_ENABLE;

	// Get the actual state of fRtsControl.

	DCB		dcb = { sizeof (DCB) };

	if (!::GetCommState (fHost->fCommHandle, &dcb))
	{
		err = ::GetLastError ();
		EmAssert (false);
	}

	// If fRtsControl needs to be changed, then do it.

	if (dcb.fRtsControl != fRtsControl)
	{
		dcb.fRtsControl = fRtsControl;

		if (!::SetCommState (fHost->fCommHandle, &dcb))
		{
			err = ::GetLastError ();
			EmAssert (false);
		}
	}

	// Given the call to SetCommState, I'm not sure if the following is needed.

#if 0
	// If we are manually controlling the RTS bit, then do that now.

	if (state != kRTSAuto)
	{
		if (!::EscapeCommFunction (fHost->fCommHandle, state == kRTSOn ? SETRTS : CLRRTS))
		{
			err = ::GetLastError ();
			EmAssert (false);
		}
	}
#endif
#endif
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostSetDTR
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostSetDTR (Bool state)
{
	if (!::EscapeCommFunction (fHost->fCommHandle, state ? SETDTR : CLRDTR))
	{
		DWORD	err = ::GetLastError ();
		EmAssert (false);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostSetBreak
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostSetBreak (Bool state)
{
	if (!::EscapeCommFunction (fHost->fCommHandle, state ? SETBREAK : CLRBREAK))
	{
		DWORD	err = ::GetLastError ();
		EmAssert (false);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostGetCTS
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

Bool EmTransportSerial::HostGetCTS (void)
{
	omni_mutex_lock lock (fHost->fFlagsMutex);

	return fHost->fCtsOn;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostGetDSR
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

Bool EmTransportSerial::HostGetDSR (void)
{
	omni_mutex_lock lock (fHost->fFlagsMutex);

	DPRINTF ("EmTransportSerial::HostGetDSR: returning %d", (int) fHost->fDsrOn);

	return fHost->fDsrOn;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::HostGetPortNameList
 *
 * DESCRIPTION:	Return the list of serial ports on this computer.  Used
 *				to prepare a menu of serial port choices.
 *
 * PARAMETERS:	nameList - port names are added to this list.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::HostGetPortNameList (PortNameList& results)
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
	fWriteBuffer (),

	fFlagsMutex (),
	fCtsOn (false),
	fDsrOn (false)
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
		// If an application sets ReadIntervalTimeout and ReadTotalTimeoutMultiplier
		// to MAXDWORD and sets ReadTotalTimeoutConstant to a value greater than zero
		// and less than MAXDWORD, one of the following occurs when the ReadFile
		// function is called:
		//
		//	*	If there are any characters in the input buffer, ReadFile returns
		//		immediately with the characters in the buffer.
		//
		//	*	If there are no characters in the input buffer, ReadFile waits until
		//		a character arrives and then returns immediately.
		//
		//	*	If no character arrives within the time specified by
		//		ReadTotalTimeoutConstant, ReadFile times out. 

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

		DWORD	commMask = EV_CTS | EV_DSR /*| EV_ERR | EV_BREAK | EV_RXCHAR */;	// EV_RING | EV_RLSD | fRXFLAG | fTXEMPTY
		if (!::SetCommMask (fCommHandle, commMask))
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

		fCommReadThread		= NULL;
		fCommReadQuitEvent	= NULL;

		fCommWriteThread	= NULL;
		fCommWriteQuitEvent	= NULL;
		fCommWriteDataEvent	= NULL;
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

	//
	// Wake up CommWrite.
	//

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


void EmHostTransportSerial::PrvCheckModemStatus (BOOL forceUpdate)
{
	DWORD	dwModemStatus;

	if (::GetCommModemStatus (this->fCommHandle, &dwModemStatus))
	{
		Bool	ctsOn = (dwModemStatus & MS_CTS_ON) != 0;
		Bool	dsrOn = (dwModemStatus & MS_DSR_ON) != 0;

		Bool	changedFlags = forceUpdate ||
			(ctsOn != this->fCtsOn) ||
			(dsrOn != this->fDsrOn);

		if (changedFlags)
		{
			omni_mutex_lock lock (fFlagsMutex);

			if (dsrOn != this->fDsrOn)
			{
				DPRINTF ("EmHostTransportSerial::PrvCheckModemStatus: DSR changed to %d", (int) dsrOn);
			}

			this->fCtsOn = ctsOn;
			this->fDsrOn = dsrOn;
		}
	}
}


void EmHostTransportSerial::PrvCheckComStat (BOOL forceUpdate)
{
	COMSTAT	comStat;
	DWORD	dwErrors;

	if (::ClearCommError (this->fCommHandle, &dwErrors, &comStat))
	{
	}
}


void EmHostTransportSerial::PrvHandleCommEvent (DWORD dwStatus)
{
	if ((dwStatus & (EV_CTS | EV_DSR)) != 0)
	{
		EmHostTransportSerial::PrvCheckModemStatus (false);
	}
}


void EmHostTransportSerial::PrvBufferData (const void* data, DWORD len)
{
	if (len > 0)
	{
		//
		// Log the data.
		//

		if (LogSerialData ())
			LogAppendData (data, len, "CommRead: Received data:");
		else
			PRINTF ("CommRead: Received %ld bytes.", len);

		//
		// Add the data to the EmHostTransportSerial object's buffer.
		//

		long n = (long) len;
		this->PutIncomingData (data, n);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportSerial::CommRead
 *
 * DESCRIPTION:	This function sits in its own thread, waiting for data
 *				to show up in the serial port.	If data arrives, this
 *				function plucks it out and stores it in a thread-safe
 *				queue.
 *
 *				This function is largely modelled after ReaderAndStatusProc
 *				from Allen Denver's "Serial Communications in Win32"
 *				article in MSL.
 *
 * PARAMETERS:	data - pointer to owning EmHostTransportSerial.
 *
 * RETURNED:	Thread status.
 *
 ***********************************************************************/

DWORD __stdcall EmHostTransportSerial::CommRead (void* data)
{
	EmHostTransportSerial*	This = (EmHostTransportSerial*) data;

	return This->CommRead ();
}


DWORD EmHostTransportSerial::CommRead (void)
{
	PRINTF ("CommRead starting.");

#define AMOUNT_TO_READ          512

	OVERLAPPED	osReader		= {0};		// Overlapped structure for read operations.
	OVERLAPPED	osStatus		= {0};		// Overlapped structure for status operations.
	DWORD		dwCommEvent;				// Result from WaitCommEvent.
	DWORD		dwOvRes;					// Result from GetOverlappedResult.
	DWORD 		dwRead;						// Bytes actually read.
	DWORD		dwRes;						// Result from WaitForMultipleObjects.
	DWORD		err;
	BOOL		waitingOnRead	= FALSE;
	BOOL		waitingOnStat	= FALSE;
	BOOL		threadDone		= FALSE;
	char		lpBuf[AMOUNT_TO_READ];

	//
	// Create two overlapped structures, one for read events
	// and another for status events.
	//

	osReader.hEvent = ::CreateEvent (NULL, TRUE, FALSE, NULL);
	osStatus.hEvent = ::CreateEvent (NULL, TRUE, FALSE, NULL);

	//
	// We want to detect the following events:
	//	Thread exit events (from our shutdown functions)
	//	Read events (from ReadFile)
	//	Status events (from WaitCommEvent)
	//

	HANDLE		hArray[] =
	{
		this->fCommReadQuitEvent,
		osReader.hEvent,
		osStatus.hEvent
	};

	//
	// Initial check, forces updates.
	//

	this->PrvCheckModemStatus (TRUE);
	this->PrvCheckComStat(TRUE);

	while (!threadDone)
	{
		//
		// If no read is outstanding, then issue another one.
		//

		if (!waitingOnRead)
		{
			DPRINTF ("CommRead: Queuing up a ReadFile.");

			if (::ReadFile (this->fCommHandle, lpBuf, AMOUNT_TO_READ, &dwRead, &osReader))
			{
				//
				// Read completed immediately.
				//

				this->PrvBufferData (lpBuf, dwRead);

				DPRINTF ("CommRead: ReadFile: completed (immediate).");
			}
			else if ((err = ::GetLastError ()) == ERROR_IO_PENDING)
			{
				waitingOnRead = TRUE;

				DPRINTF ("CommRead: ReadFile: deferred.");
			}
			else
			{
				DPRINTF ("CommRead: ReadFile: error %ld.", err);
			}
		}
		else
		{
			DPRINTF ("CommRead: ReadFile already queued.");
		}

		//
		// If no status check is outstanding, then issue another one.
		//

		if (!waitingOnStat)
		{
			DPRINTF ("CommRead: Queuing up a WaitCommEvent.");

			if (::WaitCommEvent (this->fCommHandle, &dwCommEvent, &osStatus))
			{
				//
				// WaitCommEvent returned immediately.
				//

				this->PrvHandleCommEvent (dwCommEvent);

				DPRINTF ("CommRead: WaitCommEvent: completed (immediate).");
			}
			else if ((err = ::GetLastError ()) == ERROR_IO_PENDING)
			{
				waitingOnStat = TRUE;

				DPRINTF ("CommRead: WaitCommEvent: deferred.");
			}
			else
			{
				DPRINTF ("CommRead: WaitCommEvent: error %ld.", err);
			}
		}
		else
		{
			DPRINTF ("CommRead: WaitCommEvent already queued.");
		}

		//
		// Wait for pending operations to complete.
		//

		if (waitingOnStat && waitingOnRead)
		{
			DPRINTF ("CommRead: Waiting for events.");

			dwRes = ::WaitForMultipleObjects (countof (hArray), hArray, FALSE, INFINITE);

			DPRINTF ("CommRead: WaitForMultipleObjects: dwRes = %d.", dwRes - WAIT_OBJECT_0);

			switch (dwRes)
			{
				//
				// Thread exit event.
				//

				case WAIT_OBJECT_0 + 0:
					threadDone = TRUE;
					DPRINTF ("CommRead: time to die.");
					break;

				//
				// Read completed.
				//

				case WAIT_OBJECT_0 + 1:
					//
					// See how the read operation went.
					//

					if (::GetOverlappedResult (this->fCommHandle, &osReader, &dwRead, FALSE))
					{
						//
						// Read completed successfully.
						//

						this->PrvBufferData (lpBuf, dwRead);

						DPRINTF ("CommRead: ReadFile: completed (deferred).");
					}
					else
					{
						err = ::GetLastError ();
						DPRINTF ("CommRead: ReadFile: GetOverlappedResult error %ld.", err);
					}

					waitingOnRead = FALSE;
					break;

				//
				// Status completed.
				//

				case WAIT_OBJECT_0 + 2: 
					//
					// See how the status operation went.
					//

					if (::GetOverlappedResult (this->fCommHandle, &osStatus, &dwOvRes, FALSE))
					{
						//
						// Status check completed successfully.
						//

						this->PrvHandleCommEvent (dwCommEvent);

						DPRINTF ("CommRead: WaitCommEvent: completed (deferred).");
					}
					else
					{
						err = ::GetLastError ();
						DPRINTF ("CommRead: WaitCommEvent: GetOverlappedResult error %ld.", err);
					}

					waitingOnStat = FALSE;
					break;
			}
		}
	}

	//
	// Close event handles.
	//

	::CloseHandle (osReader.hEvent);
	::CloseHandle (osStatus.hEvent);

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

	return This->CommWrite ();
}


DWORD EmHostTransportSerial::CommWrite (void)
{
	PRINTF ("CommWrite starting.");

	OVERLAPPED	osWriter		= {0};		// Overlapped structure for write operations.
	DWORD		dwRes;						// Result from WaitForMultipleObjects.
	BOOL		waitingOnWrite	= FALSE;
	BOOL		threadDone		= FALSE;
	ByteList	buffer;
	DWORD		err;

	//
	// Create an overlapped structure for write events.
	//

	osWriter.hEvent = ::CreateEvent (NULL, TRUE, FALSE, NULL);

	//
	// We want to detect the following events:
	//
	//	Thread exit events (from our shutdown functions).
	//	Requests to write data.
	//	Write events (from WriteFile).

	HANDLE	hArray[] =
	{
		this->fCommWriteQuitEvent,
		this->fCommWriteDataEvent,
		osWriter.hEvent
	};

	while (!threadDone)
	{
		//
		// Determine how many events we're looking for.  We do NOT want
		// to look for the event that says a pending write operation has
		// completed if there is, in fact, no pending write operation.
		// If we were to do that, we'd get pelted with those events.

		int	count = waitingOnWrite ? countof (hArray) : countof (hArray) - 1;

		DPRINTF ("CommWrite: Waiting for %d events.", count);

		dwRes = ::WaitForMultipleObjects (count, hArray, FALSE, INFINITE);

		DPRINTF ("CommWrite: WaitForMultipleObjects: dwRes = %d.", dwRes - WAIT_OBJECT_0);

		switch (dwRes)
		{
			//
			// Thread exit event.
			//

			case WAIT_OBJECT_0 + 0:
				threadDone = TRUE;
				DPRINTF ("CommWrite: time to die.");
				break;

			//
			// Write request.
			//

			case WAIT_OBJECT_0 + 1:
			{
				//
				// If we're already in the middle of a write, then reset
				// this event so that we don't keep getting interrupted
				// before it's done.  We'll set the event again after the
				// write has completed.
				//

				if (waitingOnWrite)
				{
					DPRINTF ("CommWrite: clearing 'have data' event.");
					::ResetEvent (fCommWriteDataEvent);
				}
				else
				{
					//
					// See if there's any data to write.
					//

					long	len = this->OutgoingDataSize ();

					if (len > 0)
					{
						//
						// Get the data.
						//

						buffer.resize (len);

						void*	bufPtr = &buffer[0];
						this->GetOutgoingData (bufPtr, len);

						//
						// Log the data.
						//

						if (LogSerialData ())
							LogAppendData (bufPtr, len, "CommWrite: Transmitted data:");
						else
							DPRINTF ("CommWrite: Transmitted %ld bytes.", len);

						//
						// Write the data.  If the operation doesn't complete, block until it does.
						//

						DWORD	n = (DWORD) len;

						if (::WriteFile (this->fCommHandle, bufPtr, len, &n, &osWriter))
						{
							//
							// Write completed immediately.
							//
							DPRINTF ("CommWrite: WriteFile: completed (immediate).");
						}
						else if ((err = ::GetLastError ()) == ERROR_IO_PENDING)
						{
							//
							// Write is delayed.
							//

							waitingOnWrite = TRUE;

							DPRINTF ("CommWrite: WriteFile: deferred.");
						}
						else
						{
							//
							// Some other write error occurred...what to do?
							//
							DPRINTF ("CommWrite: WriteFile: error %ld.", err);
						}
					}
				}

				break;
			}

			//
			// Write completed.
			//

			case WAIT_OBJECT_0 + 2:
			{
				//
				// See how the write operation went.
				//

				DWORD	n;
				if (::GetOverlappedResult (this->fCommHandle, &osWriter, &n, TRUE))
				{
					DPRINTF ("CommWrite: WriteFile: completed (deferred).");
				}
				else
				{
					err = ::GetLastError ();
					DPRINTF ("CommWrite: WriteFile: GetOverlappedResult error %ld.", err);
				}

				waitingOnWrite = FALSE;

				//
				// If any data has come in while we were away, signal our
				// event object so that WaitForMultipleEvents will indicate
				// that data is ready.

				if (this->OutgoingDataSize ())
				{
					DPRINTF ("CommWrite: signalling 'have data' event.");
					::SetEvent (fCommWriteDataEvent);
				}

				break;
			}
		}
	}

	//
	// Close event handle.
	//

	::CloseHandle (osWriter.hEvent);

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
