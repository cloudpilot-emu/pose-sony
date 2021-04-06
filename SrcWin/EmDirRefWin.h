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

#ifndef EmDirRefWin_h
#define EmDirRefWin_h

#include "EmDirRef.h"

string	PrvMaybeResolveLink		(const string& path);
BOOL	PrvIsExistingDirectory	(const string& path);
BOOL	PrvIsExistingFile		(const string& path);
string	PrvResolveLink			(const string& path);
HRESULT	ResolveIt				(LPCSTR lpszLinkFile, LPSTR lpszPath);

#endif	/* EmDirRefWin_h */
