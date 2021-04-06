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

#include "EmStructs.h"			// Configuration
#include "EmTypes.h"			// ErrCode

class EmFileRef;

Bool	QueryNewDocument	(HWND hwnd, Configuration&);
ErrCode CreateNewDocument	(const Configuration&);

Bool	QueryOpenDocument	(HWND hwnd, EmFileRef& fileName);
ErrCode OpenResourceDocument(void);
ErrCode OpenOldDocument 	(const EmFileRef& ramFilePath);

#define kForceQuery		true

BOOL	SaveRAMImage		(HWND hwnd, const EmFileRef& ramFilePath, Bool forceQuery);
