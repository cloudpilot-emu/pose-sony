/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#pragma once

#include "SocketMessaging.h"	// CSocket

extern const char*	kToEmulatorFileName;
extern const char*	kFromEmulatorFileName;

typedef void (*PacketHandler) (const void* data, long size, LPARAM userData);


DWORD	MMFOpenFiles		(HINSTANCE hInstance, PacketHandler, LPARAM userData,
							 const char* sendFileName, const char* receiveFileName);
DWORD	MMFSendData 		(const void* data, long size);
DWORD	MMFWaitForData		(DWORD timeout);
void	MMFCloseFiles		(HINSTANCE hInstance);

class CMMFSocket : public CSocket
{
	public:
								CMMFSocket			(void);
		virtual 				~CMMFSocket 		(void);

		virtual ErrCode 		Open				(void);
		virtual ErrCode 		Close				(void);
		virtual ErrCode 		Write				(const void* buffer, long amountToWrite, long*);
		virtual ErrCode 		Read				(void* buffer, long sizeOfBuffer, long*);
		virtual Bool			HasUnreadData		(long timeout);
		virtual ErrCode 		Idle				(void);

		virtual Bool			ShortPacketHack 	(void);
		virtual Bool			ByteswapHack		(void);

		static void				HandlePacketCB		(const void* data, long size, LPARAM userData);

	private:
		void					HandlePacket		(const void* data, long size);

	private:
		vector<unsigned char>	fBufferedData;
};
