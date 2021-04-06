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

#ifndef EmPixMapWin_h
#define EmPixMapWin_h

#include "EmPixMap.h"			// EmPixMap

void	ConvertPixMapToHost	(const EmPixMap& src, HDC destDC,
							 HDC&, HBITMAP&, HPALETTE&,
							 int firstLine, int lastLine, Bool scale);
void	CreatePixMapFromHost(const BITMAPINFO* src, EmPixMap& dest);

void 	ConvertFromRGBList	(RGBQUAD* outColors, const RGBList& inColors);
void 	ConvertFromRGBList	(LOGPALETTE* outColors, const RGBList& inColors);

void 	ConvertToRGBList	(RGBList& outColors, const RGBQUAD* inColors, int numColors);
void 	ConvertToRGBList	(RGBList& outColors, const LOGPALETTE* inColors);

BOOL	IsPaletteDevice		(HDC hDC);

#endif	// EmPixMapWin_h
