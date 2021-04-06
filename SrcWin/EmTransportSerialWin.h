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

#ifndef EmTransportSerialWin_h
#define EmTransportSerialWin_h

#include "EmTransportSerial.h"

#include "omnithread.h"			// omni_mutex

class EmHostTransportSerial
{
	public:
								EmHostTransportSerial (void);
								~EmHostTransportSerial (void);

		ErrCode					OpenCommPort		(const EmTransportSerial::ConfigSerial&);
		ErrCode					CreateCommThreads	(const EmTransportSerial::ConfigSerial&);
		ErrCode					DestroyCommThreads	(void);
		ErrCode					CloseCommPort		(void);

		// Manage data coming in the host serial port.
		void					PutIncomingData	(const void*, long&);
		void					GetIncomingData	(void*, long&);
		long					IncomingDataSize (void);

		// Manage data going out the host serial port.
		void					PutOutgoingData	(const void*, long&);
		void					GetOutgoingData	(void*, long&);
		long					OutgoingDataSize (void);

		static DWORD __stdcall	CommRead (void*);
		static DWORD __stdcall	CommWrite (void*);

	public:
		HANDLE					fCommHandle;

		HANDLE					fCommReadThread;
		HANDLE					fCommReadQuitEvent;

		HANDLE					fCommWriteThread;
		HANDLE					fCommWriteQuitEvent;
		HANDLE					fCommWriteDataEvent;

		omni_mutex				fReadMutex;
		deque<char>				fReadBuffer;

		omni_mutex				fWriteMutex;
		deque<char>				fWriteBuffer;
};

#endif /* EmTransportSerialWin_h */
