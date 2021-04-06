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

#ifndef EmTransportUSBWin_h
#define EmTransportUSBWin_h

#include "EmTransportUSB.h"

#include <deque>

typedef deque<uint8>	ByteQueue;

#if(WINVER < 0x0500)
typedef  PVOID           HDEVNOTIFY;
#endif

class EmHostTransportUSB
{
	public:
								EmHostTransportUSB	(void);
								~EmHostTransportUSB	(void);

		Bool					ConnectionOpen		(void);
		void					BufferPendingData	(long minBytes);

		static void				Startup				(void);
		static void				Shutdown			(void);

	private:
		static LRESULT CALLBACK	WndProc				(HWND, UINT, WPARAM, LPARAM);
		static BOOL				IsOurDevice			(DWORD);
		static void				OpenPort			(void);
		static void				ClosePort			(void);

	public:
		Bool					fOpenLocally;
		ByteQueue				fBuffer;

		static HMODULE			fgUSBLib;
		static HWND				fgWnd;
		static HDEVNOTIFY		fgToken;
		static TCHAR			fgPortName[256];
		static HANDLE			fgPortHandle;
};

#endif /* EmTransportUSBWin_h */
