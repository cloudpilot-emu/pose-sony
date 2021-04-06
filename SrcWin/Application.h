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

extern WORD			gWinSockVersion;
extern Bool			gOpenGCWWindow;

void				DoUIThread		(void);
LRESULT CALLBACK	DisplayWndProc	(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void				HandlePacket	(const void* data, long size, LPARAM userData);
