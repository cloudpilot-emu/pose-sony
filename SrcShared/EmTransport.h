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

#ifndef EmTransport_h
#define EmTransport_h

#include "EmTypes.h"			// ErrCode

enum EmTransportType
{
	kTransportNone,
	kTransportSerial,
	kTransportSocket,
	kTransportUSB
};

class EmTransport
{
	public:
		struct Config
		{
									Config			(void) {};
			virtual					~Config			(void) {};

			virtual EmTransport*	NewTransport	(void) = 0;
			virtual EmTransport*	GetTransport	(void) = 0;
		};

	public:
								EmTransport			(void);
		virtual					~EmTransport		(void);

		virtual ErrCode			Open				(void);
		virtual ErrCode			Close				(void);

		virtual ErrCode			Read				(long&, void*);
		virtual ErrCode			Write				(long&, const void*);

		virtual Bool			CanRead				(void);
		virtual Bool			CanWrite			(void);

		virtual long			BytesInBuffer		(void);

		static void				CloseAllTransports	(void);
		static EmTransportType	GetTransportTypeFromPortName (const char* portName);
};

#endif /* EmTransport_h */
