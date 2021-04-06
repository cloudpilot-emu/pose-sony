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

#ifndef EmWindow_h
#define EmWindow_h

#include "EmPixMap.h"			// EmPixMap
#include "EmRegion.h"			// EmRegion
#include "EmStructs.h"			// RGBList
#include "PreferenceMgr.h"		// PrefKeyType

class EmWindowHostData;
class EmPoint;
class EmScreenUpdateInfo;

class EmWindow
{
	public:
								EmWindow			(void);
								~EmWindow			(void);

		static EmWindow*		GetWindow			(void);

		void					WindowCreate		(void);
		void					WindowDispose		(void);

		void					WindowReset			(void);

		void					HandlePenEvent		(const EmPoint&, Bool down);
		void					HandleUpdate		(void);
		void 					HandlePalette		(void);
		void					HandleDisplayChange	(void);
		void					HandleIdle			(const EmPoint&);

		void					GetLCDContents		(EmScreenUpdateInfo& info);

	private:
		void					PaintScreen			(Bool drawCase, Bool always);
		void					PaintCase			(const EmScreenUpdateInfo& info);
		void					PaintLCDFrame		(const EmScreenUpdateInfo& info);
		void					PaintLCD			(const EmScreenUpdateInfo& info);
		void					PaintLED			(uint16 ledState);
		void					PaintButtonFrames	(void);

		static void				PrefsChangedCB		(PrefKeyType key, void* data);
		void					PrefsChanged		(PrefKeyType key);

		EmRect					GetLCDBounds		(void);
		EmRect					GetLEDBounds		(void);

		Bool					GetSkin				(EmPixMap&);
		void					GetDefaultSkin		(EmPixMap&);

	private:
		void					HostConstruct		(void);
		void					HostDestruct		(void);

		void					HostWindowCreate	(void);
		void					HostWindowDispose	(void);

		void					HostWindowReset		(void);

		void					HostMouseCapture	(void);
		void					HostMouseRelease	(void);

		void					HostUpdateBegin		(void);
		void					HostUpdateEnd		(void);

		void					HostDrawingBegin	(void);
		void					HostDrawingEnd		(void);

		void					HostPaletteSet		(const EmScreenUpdateInfo&);
		void					HostPaletteRestore	(void);

		void					HostWindowMoveBy	(const EmPoint&);
		void					HostWindowMoveTo	(const EmPoint&);
		EmRect					HostWindowBoundsGet	(void);
		void					HostWindowCenter	(void);
		void					HostWindowShow		(void);
		void					HostWindowDrag		(void);

		void					HostRectFrame		(const EmRect&, const EmPoint&, const RGBType&);
		void					HostOvalPaint		(const EmRect&, const RGBType&);

		void					HostPaintCase		(const EmScreenUpdateInfo&);
		void					HostPaintLCD		(const EmScreenUpdateInfo& info,
													 const EmRect& srcRect,
													 const EmRect& destRect,
													 Bool scaled);

		void					HostPalette			(void);
		void					HostDisplayChange	(void);

		void					HostGetDefaultSkin	(EmPixMap&, int scale);

	public:
		EmWindowHostData*		GetHostData			(void) { return fHostData; }
		Bool					PrevLCDColorsChanged(const RGBList& newLCDColors);
		void					SaveLCDColors		(const RGBList& newLCDColors);
		void					GetSystemColors		(const EmScreenUpdateInfo&,
													 RGBList&);
		EmRegion				GetSkinRegion		(void);

	private:
		friend class EmWindowHostData;

		static EmWindow*		fgWindow;

		EmWindowHostData*		fHostData;

		EmPixMap				fSkin;
		RGBList					fSkinColors1;
		RGBList					fSkinColors2;
		EmRegion				fSkinRegion;

		RGBList					fPrevLCDColors;

		SkinElementType			fCurrentButton;

		Bool					fNeedWindowReset;
		Bool					fNeedWindowInvalidate;

		Bool					fOldLCDOn;
		uint16					fOldLEDState;

		Bool					fWiggled;
};

#endif	// EmWindow_h
