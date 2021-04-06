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
#include "EmTransportSerial.h"

#include "EmErrCodes.h"			// kError_CommOpen
#include "Logging.h"			// LogSerial


EmTransportSerial::OpenPortList	EmTransportSerial::fgOpenPorts;

#define PRINTF	if (!LogSerial ()) ; else LogAppendMsg


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSerial::EmTransportSerial (void) :
	fHost (NULL),
	fConfig (),
	fCommEstablished (false)
{
	this->HostConstruct ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	config - configuration information used when opening
 *					the serial port.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSerial::EmTransportSerial (const ConfigSerial& config) :
	fHost (NULL),
	fConfig (),
	fCommEstablished (false)
{
	this->HostConstruct ();
	this->SetConfig (config);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSerial::~EmTransportSerial (void)
{
	this->Close ();

	this->HostDestruct ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::Open
 *
 * DESCRIPTION:	Open the transport using the information provided
 *				either in the constructor or with SetConfig.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::Open (void)
{
	PRINTF ("EmTransportSerial::Open...");

	// Exit if communications have already been established.

	if (fCommEstablished)
	{
		PRINTF ("EmTransportSerial::Open: Serial port already open...leaving...");
		return kError_CommOpen;
	}

	EmAssert (fgOpenPorts.find (fConfig.fPort) == fgOpenPorts.end ());

	ErrCode	err = this->HostOpen ();

	if (!err)
		err = this->HostSetConfig (fConfig);

	if (err)
	{
		this->HostClose ();
	}
	else
	{
		fCommEstablished = true;
		fgOpenPorts[fConfig.fPort] = this;
	}

	if (err)
		PRINTF ("EmTransportSerial::Open: err = %ld", err);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::Close
 *
 * DESCRIPTION:	Close the transport.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::Close (void)
{
	PRINTF ("EmTransportSerial::Close...");

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSerial::Close: Serial port not open...leaving...");
		return kError_CommNotOpen;
	}

	fCommEstablished = false;
	fgOpenPorts.erase (fConfig.fPort);

	ErrCode	err = this->HostClose ();

	if (err)
		PRINTF ("EmTransportSerial::Close: err = %ld", err);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::Read
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

ErrCode EmTransportSerial::Read (long& len, void* data)
{
	PRINTF ("EmTransportSerial::Read...");

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSerial::Read: port not open, leaving");
		return kError_CommNotOpen;
	}

	ErrCode	err = this->HostRead (len, data);

	if (err)
		PRINTF ("EmTransportSerial::Read: err = %ld", err);
	else
		if (LogSerialData ())
			LogAppendData (data, len, "EmTransportSerial::Read: reading %ld bytes.", len);
		else
			PRINTF ("EmTransportSerial::Read: reading %ld bytes", len);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::Write
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

ErrCode EmTransportSerial::Write (long& len, const void* data)
{
	PRINTF ("EmTransportSerial::Write...");

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSerial::Write: port not open, leaving");
		return kError_CommNotOpen;
	}

	ErrCode	err = this->HostWrite (len, data);

	if (err)
		PRINTF ("EmTransportSerial::Write: err = %ld", err);
	else
		if (LogSerialData ())
			LogAppendData (data, len, "EmTransportSerial::Write: writing %ld bytes.", len);
		else
			PRINTF ("EmTransportSerial::Write: writing %ld bytes", len);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::CanRead
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

Bool EmTransportSerial::CanRead (void)
{
	return fCommEstablished;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::CanWrite
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

Bool EmTransportSerial::CanWrite (void)
{
	return fCommEstablished;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::BytesInBuffer
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

long EmTransportSerial::BytesInBuffer (void)
{
	if (!fCommEstablished)
		return 0;

	return this->HostBytesInBuffer ();
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::SetConfig
 *
 * DESCRIPTION:	Set the configuration to be used when opening the port,
 *				or to reconfigure a currently open port.
 *
 * PARAMETERS:	config - the configuration to use.
 *
 * RETURNED:	0 if no error.
 *
 ***********************************************************************/

ErrCode EmTransportSerial::SetConfig (const ConfigSerial& config)
{
	PRINTF ("EmTransportSerial::SetConfig...");

	if (config == fConfig)
	{
		PRINTF ("EmTransportSerial::SetConfig: Config unchanged, so not setting settings...");
		return errNone;
	}

	fConfig = config;

	// Exit if communications have not been established.

	if (!fCommEstablished)
	{
		PRINTF ("EmTransportSerial::SetConfig: Serial port closed, so not setting settings...");
		return kError_CommNotOpen;
	}

	ErrCode	err = this->HostSetConfig (fConfig);

	if (err)
		PRINTF ("EmTransportSerial::SetConfig: err = %ld", err);

	return err;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::GetConfig
 *
 * DESCRIPTION:	Return the configuration specified in the constructor
 *				or in the last call to SetConfig.
 *
 * PARAMETERS:	config - config object to receive the settings.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial::GetConfig (ConfigSerial& config)
{
	config = fConfig;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::GetTransport
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

EmTransportSerial* EmTransportSerial::GetTransport (const ConfigSerial& config)
{
	OpenPortList::iterator	iter = fgOpenPorts.find (config.fPort);

	if (iter == fgOpenPorts.end ())
		return NULL;

	return iter->second;
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial:: GetPortNameList
 *
 * DESCRIPTION:	Return the list of serial ports on this computer.  Used
 *				to prepare a menu of serial port choices.
 *
 * PARAMETERS:	nameList - port names are added to this list.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial:: GetPortNameList (PortNameList& nameList)
{
	HostGetSerialPortNameList (nameList);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial:: GetSerialBaudList
 *
 * DESCRIPTION:	Return the list of baud rates support by this computer.
 *				Used to prepare a menu of baud rate choices.
 *
 * PARAMETERS:	baudList - baud rates are added to this list.
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmTransportSerial:: GetSerialBaudList (BaudList& baudList)
{
	HostGetSerialBaudList (baudList);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::ConfigSerial c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSerial::ConfigSerial::ConfigSerial (void) :
	fPort (),
	fBaud (57600),
	fParity (kNoParity),
	fStopBits (1),
	fDataBits (8),
	fHwrHandshake (true)
{
}
			

/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::ConfigSerial d'tor
 *
 * DESCRIPTION:	Destructor.  Delete our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmTransportSerial::ConfigSerial::~ConfigSerial (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::ConfigSerial::NewTransport
 *
 * DESCRIPTION:	Create a new transport object based on the configuration
 *				information in this object.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The new transport object.
 *
 ***********************************************************************/

EmTransport* EmTransportSerial::ConfigSerial::NewTransport (void)
{
	return new EmTransportSerial (*this);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::ConfigSerial::GetTransport
 *
 * DESCRIPTION:	Return any transport object currently using the port
 *				specified in the given configuration.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Any found transport object.  May be NULL.
 *
 ***********************************************************************/

EmTransport* EmTransportSerial::ConfigSerial::GetTransport (void)
{
	return EmTransportSerial::GetTransport (*this);
}


/***********************************************************************
 *
 * FUNCTION:	EmTransportSerial::ConfigSerial::operator==
 *
 * DESCRIPTION:	Compare two Config objects to each other
 *
 * PARAMETERS:	other - the object to compare "this" to.
 *
 * RETURNED:	True if the objects are equivalent.
 *
 ***********************************************************************/

bool EmTransportSerial::ConfigSerial::operator==(const ConfigSerial& other) const
{
	return
			fPort			== other.fPort			&&
			fBaud			== other.fBaud			&&
			fParity			== other.fParity		&&
			fStopBits		== other.fStopBits		&&
			fDataBits		== other.fDataBits		&&
			fHwrHandshake	== other.fHwrHandshake;
}
