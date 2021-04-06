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
#include "EmTransport.h"

#include "EmTransportSerial.h"	// EmTransportSerial
#include "EmTransportSocket.h"	// EmTransportSocket
#include "EmTransportUSB.h"		// EmTransportUSB

#include <vector>				// vector
#include <algorithm>			// find()


typedef vector<EmTransport*>	EmTransportList;
EmTransportList					gTransports;


/***********************************************************************
 *
 * FUNCTION:	EmTransport c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransport::EmTransport (void)
{
	gTransports.push_back (this);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransport::~EmTransport (void)
{
	EmTransportList::iterator	iter = find (gTransports.begin (),
											 gTransports.end (),
											 this);

	if (iter != gTransports.end ())
	{
		gTransports.erase (iter);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::Open
 *
 * DESCRIPTION:	Open the transport using the information provided
 *				either in the constructor or with SetConfig.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransport::Open (void)
{
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::Close
 *
 * DESCRIPTION:	Close the transport.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransport::Close (void)
{
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::Read
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

ErrCode EmTransport::Read (long&, void*)
{
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::Write
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

ErrCode EmTransport::Write (long&, const void*)
{
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::CanRead
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

Bool EmTransport::CanRead (void)
{
	return false;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::CanWrite
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

Bool EmTransport::CanWrite (void)
{
	return false;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::BytesInBuffer
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

long EmTransport::BytesInBuffer (void)
{
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransport::CloseAllTransports
 *
 * DESCRIPTION:	Shutdown routine.  Close all existing transports.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransport::CloseAllTransports (void)
{
	EmTransportList::iterator	iter = gTransports.begin ();
	while (iter != gTransports.end ())
	{
		EmTransport*	transport = *iter;
		transport->Close ();

		++iter;
	}
}


/**********************************************************************
 *
 * FUNCTION:    EmTransport::GetTransportTypeFromPortName
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:  .
 *
 * RETURNED:    .
 *
 ***********************************************************************/

EmTransportType EmTransport::GetTransportTypeFromPortName (const char* portName)
{
#if PLATFORM_UNIX
	if (portName && strlen (portName) > 0)
	{
		if (portName[0] == '/')
		{
			return kTransportSerial;
		}

		return kTransportSocket;
	}
#else
	{
		EmTransportSerial::PortNameList	portNames;
		EmTransportSerial::GetPortNameList (portNames);
		EmTransportSerial::PortNameList::iterator	iter = portNames.begin ();

		while (iter != portNames.end ())
		{
			if (!strcmp (portName, iter->c_str ()))
			{
				return kTransportSerial;
			}

			++iter;
		}
	}

	{
		EmTransportSocket::PortNameList	portNames;
		EmTransportSocket::GetPortNameList (portNames);
		EmTransportSocket::PortNameList::iterator	iter = portNames.begin ();

		while (iter != portNames.end ())
		{
			if (!strcmp (portName, iter->c_str ()))
			{
				return kTransportSocket;
			}

			++iter;
		}
	}

	{
		EmTransportUSB::PortNameList	portNames;
		EmTransportUSB::GetPortNameList (portNames);
		EmTransportUSB::PortNameList::iterator	iter = portNames.begin ();

		while (iter != portNames.end ())
		{
			if (!strcmp (portName, iter->c_str ()))
			{
				return kTransportUSB;
			}

			++iter;
		}
	}
#endif

	return kTransportNone;
}

