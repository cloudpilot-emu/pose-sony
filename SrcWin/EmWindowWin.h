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

#ifndef EmWindowWin_h
#define EmWindowWin_h

#include "EmStructs.h"			// RGBList

class EmWindow;
class EmPoint;
class EmRect;
class EmScreenUpdateInfo;

class EmWindowHostData
{
	public:
								EmWindowHostData		(EmWindow*);
								~EmWindowHostData		(void);

		HWND					GetHostWindow			(void);

		void					CacheFlush				(void);
		void					GenerateSkinCache		(void);

		void					DrawDDB					(HDC, HBITMAP, HPALETTE,
														 const EmRect&,
														 const EmPoint&);

		void					GetSkin					(HDC&, HBITMAP&, HPALETTE&);
		HPALETTE				GetSystemPalette		(const EmScreenUpdateInfo&);

#ifdef SONY_ROM
		HDC						fDestDC;
		void					DrawStretchDDB (HDC, HBITMAP, HPALETTE, const EmRect&, const EmRect&);
#endif

	private:
		friend class EmWindow;

		EmWindow*				fWindow;

		HWND					fHostWindow;

		// Used when drawing/updating.
		PAINTSTRUCT				fPaintStruct;
#ifndef SONY_ROM
		HDC						fDestDC;
#endif
		HPALETTE				fDestPalette;
		HPALETTE				fOldDestPalette;

		HPALETTE				fCachedSystemPalette;

		HDC						fCachedSkinDC;
		HBITMAP 				fCachedSkinBitmap;
		HPALETTE				fCachedSkinPalette;
};

EmWindow* GetWindow (HWND hWnd);

#endif	// EmLCDWin_h
