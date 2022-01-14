/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmTransportUSBWin.h"

#include "EmApplicationWin.h"	// gInstance
#include "Logging.h"			// LogSerial, LogAppendMsg
#include "Platform.h"			// Platform

#include <DBT.h>				// DBT_DEVICEARRIVAL, DBT_DEVICEREMOVECOMPLETE


#define PRINTF	if (!LogSerial ()) ; else LogAppendMsg

typedef struct USB_TIMEOUTS
{
	ULONG	readTimeout;		// milliseconds
	ULONG	writeTimeout;		// milliseconds
} USB_TIMEOUTS, *PUSB_TIMEOUTS;

// Error Codes
// ----------------------------------------------------------------------------
#define UsbNoError				0x00000000L		// No error
#define UsbErrUnknown			0x00000001L		// Unknown error
#define UsbErrSendTimedOut		0x00000002L		// Send timed out
#define UsbErrRecvTimedOut		0x00000003L		// Receive timed out
#define UsbErrPortNotOpen		0x00000004L		// USB port is not open
#define UsbErrIOError			0x00000005L		// I/O or line error
#define UsbErrPortBusy			0x00000006L		// USB port is already in use
#define UsbErrNotSupported		0x00000007L		// IOCTL code unsupported
#define UsbErrBufferTooSmall	0x00000008L		// Buffer size too small
#define UsbErrNoAttachedDevices	0x00000009L		// No devices currently attached
#define UsbErrDontMatchFilter	0x00000010L		// Creator ID provided doesn't
												// match the USB-active device
												// application creator ID

typedef ULONG					(*GetDeviceFriendlyNameProc)		(PTCHAR		pDeviceName,
																	 PTCHAR		pFriendlyName);

typedef ULONG					(*GetAttachedDevicesProc)			(ULONG*		pDeviceCount,
																	 PTCHAR		pBuffer,
																	 ULONG*		pBufferSize);
typedef HDEVNOTIFY				(*RegisterDeviceInterfaceProc)		(HWND);
typedef VOID					(*UnRegisterDeviceInterfaceProc)	(HDEVNOTIFY);
typedef BOOL					(*IsPalmOSDeviceNotificationProc)	(ULONG, ULONG, PTCHAR, GUID*);
typedef HANDLE					(*OpenPortProc)						(PTCHAR, ULONG);
typedef BOOL					(*ClosePortProc)					(HANDLE);
typedef ULONG					(*ReceiveBytesProc)					(HANDLE, LPVOID, ULONG, ULONG*);
typedef ULONG					(*SendBytesProc)					(HANDLE, LPCVOID, ULONG, ULONG*);
typedef ULONG					(*SetTimeoutsProc)					(HANDLE, USB_TIMEOUTS*);
typedef ULONG					(*GetTimeoutsProc)					(HANDLE, USB_TIMEOUTS*);

static GetDeviceFriendlyNameProc		gGetDeviceFriendlyName;
static GetAttachedDevicesProc			gGetAttachedDevices;
static RegisterDeviceInterfaceProc		gRegisterDeviceInterface;
static UnRegisterDeviceInterfaceProc	gUnRegisterDeviceInterface;
static IsPalmOSDeviceNotificationProc	gIsPalmOSDeviceNotification;
static OpenPortProc						gOpenPort;
static ClosePortProc					gClosePort;
static ReceiveBytesProc					gReceiveBytes;
static SendBytesProc					gSendBytes;
static SetTimeoutsProc					gSetTimeouts;
static GetTimeoutsProc					gGetTimeouts;

static const DWORD				kROMTransfer = 'ROMX';

static const char*				kUSBLibraryName = "USBPort.dll";

HMODULE							EmHostTransportUSB::fgUSBLib;
HWND							EmHostTransportUSB::fgWnd;
HDEVNOTIFY						EmHostTransportUSB::fgToken;
TCHAR							EmHostTransportUSB::fgPortName[256];
HANDLE							EmHostTransportUSB::fgPortHandle;


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostHasUSB
 *
 * DESCRIPTION:	Return whether or not USB facilities are available.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	True if the host has a USB port and we can use it.
 *				False otherwise.
 *
 ***********************************************************************/

Bool EmTransportUSB::HostHasUSB (void)
{
	return EmHostTransportUSB::fgUSBLib != NULL;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostConstruct
 *
 * DESCRIPTION:	Construct platform-specific objects/data.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The platform-specific serial object.
 *
 ***********************************************************************/

void EmTransportUSB::HostConstruct (void)
{
	fHost = new EmHostTransportUSB;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostDestruct
 *
 * DESCRIPTION:	Destroy platform-specific objects/data.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportUSB::HostDestruct (void)
{
	delete fHost;
	fHost = NULL;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostOpen
 *
 * DESCRIPTION:	Open the serial port in a platform-specific fashion.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportUSB::HostOpen (void)
{
	fHost->fOpenLocally = true;

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostClose
 *
 * DESCRIPTION:	Close the serial port in a platform-specific fashion.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportUSB::HostClose (void)
{
	fHost->fOpenLocally = false;

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostRead
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

ErrCode EmTransportUSB::HostRead (long& len, void* data)
{
	ErrCode	err = errNone;

	// Copy any data into our private buffer.

	fHost->BufferPendingData (len);

	// Return as much data as we can.

	if (len > fHost->fBuffer.size ())
	{
		err = serErrTimeOut;
		len = fHost->fBuffer.size ();
	}

	copy (
		fHost->fBuffer.begin (),
		fHost->fBuffer.begin () + len,
		(UInt8*) data);

	fHost->fBuffer.erase (
		fHost->fBuffer.begin (),
		fHost->fBuffer.begin () + len);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostWrite
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

ErrCode EmTransportUSB::HostWrite (long& len, const void* data)
{
	ErrCode	err = errNone;

	if (fHost->ConnectionOpen ())
	{
		ULONG	sent = 0;

		err = gSendBytes (fHost->fgPortHandle, (VOID*) data, len, &sent); 

		len = sent;
	}

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostCanRead
 *
 * DESCRIPTION:	Return whether or not the transport is available for
 *				a read operation (that is, it's connected to another
 *				entity).  Does NOT indicate whether or not there are
 *				actually any bytes available to be read.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	True if so.
 *
 ***********************************************************************/

Bool EmTransportUSB::HostCanRead (void)
{
	return fHost->ConnectionOpen ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostCanWrite
 *
 * DESCRIPTION:	Return whether or not the transport is available for
 *				a write operation (that is, it's connected to another
 *				entity).  Does NOT indicate whether or not there is
 *				actually any room in the transport's internal buffer
 *				for the data being written.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	True if so.
 *
 ***********************************************************************/

Bool EmTransportUSB::HostCanWrite (void)
{
	return fHost->ConnectionOpen ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostBytesInBuffer
 *
 * DESCRIPTION:	Returns the number of bytes that can be read with the
 *				Read method.  Note that bytes may be received in
 *				between the time BytesInBuffer is called and the time
 *				Read is called, so calling the latter with the result
 *				of the former is not guaranteed to fetch all received
 *				and buffered bytes.
 *
 * PARAMETERS:	minBytes - try to buffer at least this many bytes.
 *					Return when we have this many bytes buffered, or
 *					until some small timeout has occurred.
 *
 * RETURNED:	Number of bytes that can be read.
 *
 ***********************************************************************/

long EmTransportUSB::HostBytesInBuffer (long minBytes)
{
	// Copy any data into our private buffer.

	fHost->BufferPendingData (minBytes);

	// Return however many bytes we have buffered up.

	long	bytesRead = fHost->fBuffer.size ();

	return bytesRead;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportUSB::HostSetConfig
 *
 * DESCRIPTION:	Configure the serial port in a platform-specific
 *				fasion.  The port is assumed to be open.
 *
 * PARAMETERS:	config - configuration information.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportUSB::HostSetConfig (const ConfigUSB& /*config*/)
{
	ErrCode	result = errNone;

	return result;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmHostTransportUSB::EmHostTransportUSB (void) :
	fOpenLocally (false)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmHostTransportUSB::~EmHostTransportUSB (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::ConnectionOpen
 *
 * DESCRIPTION:	Return whether or not it's OK to use the USB port.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	True if HostOpen has been called, and the USB device is
 *				online.  False otherwise.
 *
 ***********************************************************************/

Bool EmHostTransportUSB::ConnectionOpen (void)
{
	return fOpenLocally && fgPortHandle;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::BufferPendingData
 *
 * DESCRIPTION:	Buffer up any data the USB driver may have waiting.
 *
 * PARAMETERS:	minBytes - return when we have at least this many
 *					bytes in our local buffer.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmHostTransportUSB::BufferPendingData (long minBytes)
{
	if (fgPortHandle)
	{
		while (minBytes == -1 || fBuffer.size () < minBytes)
		{
			// Read a chunk of data into a local buffer.  Read as much
			// as is requested, taking into account the bytes that we've
			// already read and buffered.

			UInt8	buffer[1024 + 64];
			ULONG	received	= 0;
			ULONG	needed		= minBytes - fBuffer.size ();

			UInt8*	dest		= buffer;
			ULONG	destSize	= countof (buffer);

			// Read only as many bytes as we have room for in our local,
			// stack-based buffer.

			if (needed > destSize || minBytes == -1)
				needed = destSize;

			// Read the bytes.

			ULONG	err = gReceiveBytes (fgPortHandle, dest, needed, &received);

			// Break if there was an error receiving the data.

			if (err && err != UsbErrRecvTimedOut)
				break;

			// Break if there is no more data

			if (received == 0)
				break;

			// Copy that data into our dynamic buffer.

			copy (&dest[0], &dest[received],
				back_insert_iterator<ByteQueue> (fBuffer));

			// Log the data.

			if (LogSerialData ())
				LogAppendData (dest, received, "EmHostTransportUSB::BufferPendingData: Received %ld of %ld bytes:", received, minBytes);
			else
				PRINTF ("EmHostTransportUSB::BufferPendingData: Received %ld of %ld bytes.", received, minBytes);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::Startup
 *
 * DESCRIPTION:	Perform sub-system initialization.  This function is
 *				called once at application startup.  In it, we open
 *				the USB-handler library, find the functions we want
 *				in it, create a hidden window to receive messages
 *				from the Windows device manager, and register for
 *				device notification.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmHostTransportUSB::Startup (void)
{
	fgUSBLib	= ::LoadLibrary (kUSBLibraryName);

	if (!fgUSBLib)
	{
		EmHostTransportUSB::Shutdown ();
		return;
	}

#define SNARF_FUNCTION(name)	\
	g##name	= (name##Proc) ::GetProcAddress (fgUSBLib, "PalmUsb" #name)

	SNARF_FUNCTION(GetDeviceFriendlyName);
	SNARF_FUNCTION(GetAttachedDevices);
	SNARF_FUNCTION(RegisterDeviceInterface);
	SNARF_FUNCTION(UnRegisterDeviceInterface);
	SNARF_FUNCTION(IsPalmOSDeviceNotification);
	SNARF_FUNCTION(OpenPort);
	SNARF_FUNCTION(ClosePort);
	SNARF_FUNCTION(ReceiveBytes);
	SNARF_FUNCTION(SendBytes);
	SNARF_FUNCTION(SetTimeouts);
	SNARF_FUNCTION(GetTimeouts);

	if (//!gGetDeviceFriendlyName ||	// We don't need this one, and it's not in
										// all versions of the .dll.
		//!gGetAttachedDevices ||		// This one's not in all versions of the .dll,
										// either, so check for it when used.
		!gRegisterDeviceInterface ||
		!gUnRegisterDeviceInterface ||
		!gIsPalmOSDeviceNotification ||
		!gOpenPort ||
		!gClosePort ||
		!gReceiveBytes ||
		!gSendBytes ||
		!gSetTimeouts ||
		!gGetTimeouts)
	{
		EmHostTransportUSB::Shutdown ();
		return;
	}

	const char*	kClassName = "PalmOSEmulatorUSB";

	WNDCLASS	wc;
	memset (&wc, 0, sizeof (wc));

	wc.lpfnWndProc		= &EmHostTransportUSB::WndProc;
	wc.hInstance		= gInstance;
	wc.lpszClassName	= kClassName;

	::RegisterClass (&wc);

	fgWnd = ::CreateWindow (kClassName, "Palm Emulator USB", 0,
							CW_USEDEFAULT, CW_USEDEFAULT,
							CW_USEDEFAULT, CW_USEDEFAULT,
							NULL, NULL,			// parent, menu
							gInstance, NULL);	// lpParam
	if (!fgWnd)
	{
		EmHostTransportUSB::Shutdown ();
		return;
	}

	fgToken = gRegisterDeviceInterface (fgWnd);
	if (!fgToken)
	{
		EmHostTransportUSB::Shutdown ();
		return;
	}

	// See if there are any currently-attached devices we need to
	// know about.

	if (gGetAttachedDevices)
	{
		ULONG	count;
		PTCHAR	buffer = NULL;
		ULONG	bufferSize = 0;

		ULONG	err = gGetAttachedDevices (&count, buffer, &bufferSize);

		if (err == UsbErrBufferTooSmall)
		{
			buffer = (PTCHAR) Platform::AllocateMemory (bufferSize * sizeof (TCHAR));
			err = gGetAttachedDevices (&count, buffer, &bufferSize);

			if (err == UsbNoError && count > 0)
			{
				// Get the name of the device to use.  If there's more than
				// one, we arbitrarily pick the first one.  We're not really
				// set up to work with more than one USB device.

				strcpy (fgPortName, buffer);

				// Now open the device.

				EmHostTransportUSB::OpenPort ();
			}

			Platform::FreeMemory (buffer);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::Shutdown
 *
 * DESCRIPTION:	Perform sub-system clean-up.  This function is called
 *				once at application shutdown.  In it, we unregister
 *				for device notification, close the hidden message-
 *				receiving window, drop our references to the USB
 *				library functions, and close the library.
 *				device notification.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void EmHostTransportUSB::Shutdown (void)
{
	if (fgToken)
	{
		gUnRegisterDeviceInterface (fgToken);
		fgToken = NULL;
	}

	if (fgWnd)
	{
		::CloseWindow (fgWnd);
		fgWnd = NULL;
	}

	gRegisterDeviceInterface = NULL;
	gUnRegisterDeviceInterface = NULL;
	gIsPalmOSDeviceNotification = NULL;
	gOpenPort = NULL;
	gClosePort = NULL;
	gReceiveBytes = NULL;
	gSendBytes = NULL;
	gSetTimeouts = NULL;
	gGetTimeouts = NULL;

	if (fgUSBLib)
	{
		::FreeLibrary (fgUSBLib);
		fgUSBLib = NULL;
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::WndProc
 *
 * DESCRIPTION:	Receive device notification messages from Windows.
 *
 * PARAMETERS:	hWnd - handle to our window.
 *
 *				msg - message from Windows (we're looking only for
 *					WM_DEVICECHANGE messages).
 *
 *				wParam - first parameter for the message.  We looking
 *					for DBT_DEVICEARRIVAL and DBT_DEVICEREMOVECOMPLETE
 *					messages).
 *
 *				lParam - second parameter for the message.  For the
 *					messages we're looking for, this parameter holds
 *					a pointer to a parameter block.
 *
 * RETURNED:	TRUE if we handled the message, FALSE otherwise.
 *
 ***********************************************************************/

LRESULT CALLBACK EmHostTransportUSB::WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DEVICECHANGE)
	{
		if (wParam == DBT_DEVICEARRIVAL && EmHostTransportUSB::IsOurDevice (lParam))
		{
			EmHostTransportUSB::OpenPort ();
			return TRUE;
		}

		if (wParam == DBT_DEVICEREMOVECOMPLETE && EmHostTransportUSB::IsOurDevice (lParam))
		{
			EmHostTransportUSB::ClosePort ();
			return TRUE;
		}
	}

	return ::DefWindowProc (hWnd, msg, wParam, lParam);
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::IsOurDevice
 *
 * DESCRIPTION:	Return whether or not the device notification message
 *				was for a USB device we handle.
 *
 * PARAMETERS:	dwEventData - lParam from the WndProc, which is a
 *					pointer to a parameter block.
 *
 * RETURNED:	TRUE if we manage the device, FALSE otherwise.
 *
 ***********************************************************************/

BOOL EmHostTransportUSB::IsOurDevice (DWORD dwEventData)
{
	BOOL	ok = gIsPalmOSDeviceNotification (
		dwEventData, kROMTransfer, fgPortName, NULL);

	return ok;
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::OpenPort
 *
 * DESCRIPTION:	Open a connection to the USB device.
 *
 * PARAMETERS:	None (we work from fgPortName).
 *
 * RETURNED:	Nothing (we set fgPortHandle).
 *
 ***********************************************************************/

void EmHostTransportUSB::OpenPort (void)
{
	if (fgPortHandle == NULL)
	{
		fgPortHandle = gOpenPort (fgPortName, kROMTransfer);

		if (fgPortHandle == INVALID_HANDLE_VALUE)
		{
			fgPortHandle = NULL;
		}

		EmAssert (fgPortHandle);

		if (fgPortHandle)
			PRINTF ("EmHostTransportUSB::OpenPort: opened port.");
		else
			PRINTF ("EmHostTransportUSB::OpenPort: failed to open port.");

		if (fgPortHandle)
		{
			USB_TIMEOUTS	timeouts;

			// Note: these timeout values are very specific.  In the
			// driver against which they were tested (1.2.0.0), lower
			// values would cause misreads.

			timeouts.readTimeout = 50;
			timeouts.writeTimeout = 50;

			gSetTimeouts (fgPortHandle, &timeouts);
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmHostTransportUSB::ClosePort
 *
 * DESCRIPTION:	Close the connection to the USB device.
 *
 * PARAMETERS:	None (we work from fgPortHandle).
 *
 * RETURNED:	Nothing (we clear fgPortHandle).
 *
 ***********************************************************************/

void EmHostTransportUSB::ClosePort (void)
{
	if (fgPortHandle)
	{
		gClosePort (fgPortHandle);
		fgPortHandle = NULL;

		PRINTF ("EmHostTransportUSB::ClosePort: closed port.");
	}
}
