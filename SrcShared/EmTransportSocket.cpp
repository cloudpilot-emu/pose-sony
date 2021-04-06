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
#include "EmTransportSocket.h"

#include "EmErrCodes.h"			// kError_CommOpen, kError_CommNotOpen, kError_NoError
#include "Logging.h"			// LogSerial
#include "Platform.h"			// Platform::AllocateMemory

#if PLATFORM_MAC
#include <GUSIPOSIX.h>			// inet_addr
#endif

#if PLATFORM_UNIX
#include <errno.h>				// ENOENT, errno
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>			// timeval
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>				// close
#include <arpa/inet.h>			// inet_addr
#endif

#define PRINTF	if (!LogSerial ()) ; else LogAppendMsg

EmTransportSocket::OpenPortList	EmTransportSocket::fgOpenPorts;


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSocket::EmTransportSocket (void) :
	fReadMutex (),
	fReadBuffer (),
	fDataSocket (NULL),
	fConfig (),
	fCommEstablished (false)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	config - configuration information used when opening
 *					the TCP port.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSocket::EmTransportSocket (const ConfigSocket& config) :
	fReadMutex (),
	fReadBuffer (),
	fDataSocket (NULL),
	fConfig (config),
	fCommEstablished (false)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSocket::~EmTransportSocket (void)
{
	Close ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::Open
 *
 * DESCRIPTION:	Open the transport using the information provided
 *				either in the constructor or with SetConfig.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSocket::Open (void)
{
	PRINTF ("EmTransportSocket::Open...");

	// Exit if communications have already been established.

	if (fCommEstablished)
	{
		PRINTF ("EmTransportSocket::Open: TCP port already open...leaving...");
		return kError_CommOpen;
	}

	string registrationKey = "TCP:" + fConfig.fTargetHost + ":" + fConfig.fTargetHost;
	EmAssert (fgOpenPorts.find (registrationKey) == fgOpenPorts.end ());

	ErrCode err= OpenCommPort (fConfig);

	if (err)
	{
		err = CloseCommPort ();
	}
	else
	{
		fCommEstablished = true;
		fgOpenPorts[registrationKey] = this;
	}

	if (err)
		PRINTF ("EmTransportSocket::Open: err = %ld", err);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::Close
 *
 * DESCRIPTION:	Close the transport.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSocket::Close (void)
{
	PRINTF ("EmTransportSocket::Close...");

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSocket::Close: TCP port not open...leaving...");
		return kError_CommNotOpen;
	}

	fCommEstablished = false;

	string registrationKey = "TCP:" + fConfig.fTargetHost + ":" + fConfig.fTargetHost;
	
	fgOpenPorts.erase (registrationKey);

	ErrCode	err = CloseCommPort ();

	if (err)
		PRINTF ("EmTransportSocket::Close: err = %ld", err);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::Read
 *
 * DESCRIPTION:	Read up to the given number of bytes, storing them in
 *				the given buffer.
 *
 * PARAMETERS:	len - maximum number of bytes to read.
 *				data - buffer to receive the bytes.
 *
 * RETURNED:	0 if no error.  The number of bytes actually read is
 *				returned in len if there was no error.
 *
 ***********************************************************************/

ErrCode EmTransportSocket::Read (long& len, void* data)
{
	PRINTF ("EmTransportSocket::Read...");

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSocket::Read: port not open, leaving");
		return kError_CommNotOpen;
	}

	GetIncomingData (data, len);

	if (LogSerialData ())
		LogAppendData (data, len, "EmTransportSocket::Read: reading %ld bytes.", len);
	else
		PRINTF ("EmTransportSocket::Read: reading %ld bytes", len);

	return kError_NoError;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::Write
 *
 * DESCRIPTION:	Write up the the given number of bytes, using the data
 *				in the given buffer.
 *
 * PARAMETERS:	len - number of bytes in the buffer.
 *				data - buffer containing the bytes.
 *
 * RETURNED:	0 if no error.  The number of bytes actually written is
 *				returned in len if there was no error.
 *
 ***********************************************************************/

ErrCode EmTransportSocket::Write (long& len, const void* data)
{
	PRINTF ("EmTransportSocket::Write...");

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSocket::Write: port not open, leaving");
		return kError_CommNotOpen;
	}

	PutOutgoingData (data, len);

	if (LogSerialData ())
		LogAppendData (data, len, "EmTransportSocket::Write: writing %ld bytes.", len);
	else
		PRINTF ("EmTransportSocket::Write: writing %ld bytes", len);

	return kError_NoError;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::CanRead
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

Bool EmTransportSocket::CanRead (void)
{
	return fCommEstablished;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::CanWrite
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

Bool EmTransportSocket::CanWrite (void)
{
	return fCommEstablished;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::BytesInBuffer
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

long EmTransportSocket::BytesInBuffer (void)
{
	if (!fCommEstablished)
		return 0;

	return IncomingDataSize ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::GetTransport
 *
 * DESCRIPTION:	Return any transport object currently using the port
 *				specified in the given configuration.
 *
 * PARAMETERS:	config - The configuration object containing information
 *					on a port in which we're interested.  All or some
 *					of the information in this object is used when
 *					searching for a transport object already utilizing
 *					the port.
 *
 * RETURNED:	Any found transport object.  May be NULL.
 *
 ***********************************************************************/

EmTransportSocket* EmTransportSocket::GetTransport (const ConfigSocket& config)
{
	string registrationKey = "TCP:" + config.fTargetHost + ":" + config.fTargetHost;
	OpenPortList::iterator	iter = fgOpenPorts.find (registrationKey);

	if (iter == fgOpenPorts.end ())
		return NULL;

	return iter->second;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket:: GetPortNameList
 *
 * DESCRIPTION:	Return the list of TCP ports on this computer.  Used
 *				to prepare a menu of TCP port choices.
 *
 * PARAMETERS:	nameList - port names are added to this list.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSocket:: GetPortNameList (PortNameList& nameList)
{
	string portName = "TCP";
		
	nameList.push_back (portName);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::OpenCommPort
 *
 * DESCRIPTION:	Open the TCP port.
 *
 * PARAMETERS:	config - data block describing which port to use.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSocket::OpenCommPort (const EmTransportSocket::ConfigSocket& config)
{
	ErrCode err;
	
	EmAssert (fDataSocket == NULL);

	fDataSocket = new CTCPClientSocket (EmTransportSocket::EventCallBack, config.fTargetHost, config.fTargetPort, this);

	// Try establishing a connection to some peer already waiting on the target host, on the target port
	err = fDataSocket->Open ();

	// If no connection can be established
	if (err)
	{
		// Fall into server mode and start waiting
		err = fDataSocket->OpenInServerMode ();
	}

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::EventCallBack
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/
void EmTransportSocket::EventCallBack (CSocket* s, int event)
{
	switch (event)
	{
		case CSocket::kConnected:
			break;

		case CSocket::kDataReceived:
			while (s->HasUnreadData(1))
			{
				long len = 1;
				char buf;

				s->Read(&buf, len, &len);

				if (len > 0)
				{
					// Log the data.
					if (LogSerialData ())
						LogAppendData (&buf, len, "EmTransportSocket::CommRead: Received data:");
					else
						PRINTF ("EmTransportSocket::CommRead: Received %ld TCP bytes.", len);

					// Add the data to the EmTransportSocket object's buffer.
					((CTCPClientSocket*)s)->GetOwner()->PutIncomingData (&buf, len);
				}
			}
			break;

		case CSocket::kDisconnected:
			break;
	}
}

/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::CloseCommPort
 *
 * DESCRIPTION:	Close the comm port.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSocket::CloseCommPort (void)
{
	fDataSocket->Close ();
	fDataSocket->Delete ();
	fDataSocket = 0;

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::PutIncomingData
 *
 * DESCRIPTION:	Thread-safe method for adding data to the queue that
 *				holds data read from the TCP port.
 *
 * PARAMETERS:	data - pointer to the read data.
 *				len - number of bytes pointed to by "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSocket::PutIncomingData	(const void* data, long& len)
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
 * FUNCTION:	EmTransportSocket::GetIncomingData
 *
 * DESCRIPTION:	Thread-safe method for getting data from the queue
 *				holding data read from the TCP port.
 *
 * PARAMETERS:	data - pointer to buffer to receive data.
 *				len - on input, number of bytes available in "data".
 *					On exit, number of bytes written to "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSocket::GetIncomingData	(void* data, long& len)
{
	omni_mutex_lock lock (fReadMutex);

	if (len > (long) fReadBuffer.size ())
		len = (long) fReadBuffer.size ();

	char*	p = (char*) data;
	deque<char>::iterator	begin = fReadBuffer.begin ();
	deque<char>::iterator	end = begin + len;

	copy (begin, end, p);
	fReadBuffer.erase (begin, end);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::IncomingDataSize
 *
 * DESCRIPTION:	Thread-safe method returning the number of bytes in the
 *				read queue.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Number of bytes in the read queue.
 *
 ***********************************************************************/


long EmTransportSocket::IncomingDataSize (void)
{
	omni_mutex_lock lock (fReadMutex);

	return fReadBuffer.size ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::PutOutgoingData
 *
 * DESCRIPTION:	Thread-safe method for adding data to the queue that
 *				holds data to be written to the TCP port.
 *
 * PARAMETERS:	data - pointer to the read data.
 *				len - number of bytes pointed to by "data".
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSocket::PutOutgoingData	(const void* data, long& len)
{
	if (len == 0)
		return;

	fDataSocket->Write (data, len, &len);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::ConfigSocket c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSocket::ConfigSocket::ConfigSocket (void)
{
}
			

/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::ConfigSocket d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSocket::ConfigSocket::~ConfigSocket (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::ConfigSocket::NewTransport
 *
 * DESCRIPTION:	Create a new transport object based on the configuration
 *				information in this object.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The new transport object.
 *
 ***********************************************************************/

EmTransport* EmTransportSocket::ConfigSocket::NewTransport (void)
{
	return new EmTransportSocket (*this);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::ConfigSocket::GetTransport
 *
 * DESCRIPTION:	Return any transport object currently using the port
 *				specified in the given configuration.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Any found transport object.  May be NULL.
 *
 ***********************************************************************/

EmTransport* EmTransportSocket::ConfigSocket::GetTransport (void)
{
	return EmTransportSocket::GetTransport (*this);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSocket::ConfigSocket::operator==
 *
 * DESCRIPTION:	Compare two Config objects to each other
 *
 * PARAMETERS:	other - the object to compare "this" to.
 *
 * RETURNED:	True if the objects are equivalent.
 *
 ***********************************************************************/

bool EmTransportSocket::ConfigSocket::operator==(const ConfigSocket& other) const
{
	return
			fTargetHost	== other.fTargetHost	&&
			fTargetPort	== other.fTargetPort;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::CTCPClientSocket
 *
 * DESCRIPTION:CTPClientSocket c'tor
 *
 * PARAMETERS: 
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

enum { kSocketState_Unconnected, kSocketState_Listening, kSocketState_Connected };

CTCPClientSocket::CTCPClientSocket (EventCallback fn, string targetHost, int targetPort, EmTransportSocket* transport) :
	CTCPSocket (fn, targetPort)
{
		fTargetHost = targetHost;
		fTransport = transport;
}


/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::~CTCPClientSocket
 *
 * DESCRIPTION:
 *
 * PARAMETERS:
 *
 * RETURNED:
 *
 ***********************************************************************/

CTCPClientSocket::~CTCPClientSocket ()
{
}

/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::Open
 *
 * DESCRIPTION: Open a socket and try to establish a connection to a
 *					 TCP target. 
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	errNone if a connection has been established, or a
 *					nonzero error code otherwise
 *
 ***********************************************************************/

ErrCode CTCPClientSocket::Open (void)
{
	PRINTF ("CTCPClientSocket(0x%08X)::Open...", this);

	EmAssert (fSocketState == kSocketState_Unconnected);
	EmAssert (fConnectedSocket == INVALID_SOCKET);

	sockaddr	addr;
	int 		result;

	fConnectedSocket = this->NewSocket ();
	if (fConnectedSocket == INVALID_SOCKET)
	{
		PRINTF ("...NewSocket failed for connecting socket; result = %08X", this->GetError ());
		return this->ErrorOccurred ();
	}

	// Attempt to connect to that address (in case anyone is listening).
	result = connect (fConnectedSocket, this->FillAddress (&addr), sizeof(addr));
	if (result == 0)
	{
		PRINTF ("...connected!");

		fSocketState = kSocketState_Connected;
		return errNone;
	}

	closesocket (fConnectedSocket);
	fConnectedSocket = INVALID_SOCKET;

	return this->ErrorOccurred ();
}

/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::OpenInServerMode
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	errNone if a connection has been established, or a
 *					nonzero error code otherwise
 *
 ***********************************************************************/

ErrCode CTCPClientSocket::OpenInServerMode (void)
{
	int 		result;
	sockaddr	addr;
	
	fListeningSocket = this->NewSocket ();
	if (fListeningSocket == INVALID_SOCKET)
	{
		PRINTF ("...NewSocket failed for listening socket; result = %08X", this->GetError ());
		return this->ErrorOccurred ();
	}

	result = bind (fListeningSocket, this->FillLocalAddress (&addr), sizeof(addr));
	if (result != 0)
	{
		PRINTF ("...bind failed; result = %08X", this->GetError ());
		return this->ErrorOccurred ();
	}

	// Start listening for a connection.

	result = listen (fListeningSocket, 1);
	if (result != 0)
	{
		PRINTF ("...listen failed; result = %08X", this->GetError ());
		return this->ErrorOccurred ();
	}

	PRINTF ("...listening for connection");

	fSocketState = kSocketState_Listening;

	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::FillLocalAddress
 *
 * DESCRIPTION: Fill in a sockaddr data structure with values
 *					appropriate for connecting from the outside. This is an
 *					Internet address on the local machine.
 *
 * PARAMETERS:	addr - the sockaddr to fill in.
 *
 * RETURNED:	The same sockaddr passed in.
 *
 ***********************************************************************/

sockaddr* CTCPClientSocket::FillLocalAddress (sockaddr* addr)
{
	sockaddr_in*	addr_in = (sockaddr_in*) addr;

#ifdef HAVE_SIN_LEN
	addr_in->sin_len			= sizeof (*addr_in);
#endif
	addr_in->sin_family 		= AF_INET;
	addr_in->sin_port			= htons (fPort);
	addr_in->sin_addr.s_addr	= htonl (INADDR_ANY);

	return addr;
}

/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::GetOwner
 *
 * DESCRIPTION:This creates a link between a CTCPClientSocket object and
 *					a EmTransportSocket object. 
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Associated transport object, as passed to c'tor
 *
 ***********************************************************************/

EmTransportSocket* CTCPClientSocket::GetOwner (void)
{
	return fTransport;
}

/***********************************************************************
 *
 * FUNCTION:	CTCPClientSocket::FillAddress
 *
 * DESCRIPTION: Fill in a sockaddr data structure with values
 *					appropriate for connecting from the outside. This is an
 *					Internet address on the local machine. 
 *
 * PARAMETERS:	addr - the sockaddr to fill in.
 *
 * RETURNED:	The same sockaddr passed in.
 *
 ***********************************************************************/

sockaddr* CTCPClientSocket::FillAddress (sockaddr* addr)
{
	PRINTF ("CTCPSocket(0x%08X)::FillAddress...", this);

	sockaddr_in*	addr_in = (sockaddr_in*) addr;
	const char*		name = fTargetHost.c_str ();
	unsigned long ip;
	
	// Check for common "localhost" case in order to avoid a name lookup on the Mac
	if (!_stricmp(name,"localhost"))
	{
		ip = htonl(INADDR_LOOPBACK);
	}
	else
	{
		// Try decoding a dotted ip address string
		ip = inet_addr(name);
	
		if (ip == INADDR_NONE)
		{
			hostent* entry;
		
			// Perform a DNS lookup
			entry = gethostbyname(name);

			if (entry)
			{
				ip = *(unsigned long*) entry->h_addr;
			}
		}
	}

#ifdef HAVE_SIN_LEN
	addr_in->sin_len			= sizeof (*addr_in);
#endif
	addr_in->sin_family 		= AF_INET;
	addr_in->sin_port			= htons (fPort);
	addr_in->sin_addr.s_addr	= ip;

	return addr;
}
