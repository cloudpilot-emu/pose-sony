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
#include "EmWindow.h"			// EmWindow

/*
	EmWindowWin is a Windows-specific sub-class of EmWindow.  It is
	responsible for translating platform-specific window-related
	actions into cross-platform actions, making use of the the cross-
	platform EmWindow implementations.
*/

class EmPoint;
class EmRect;
class EmScreenUpdateInfo;
struct EmKeyEvent;

class EmWindowWin : public EmWindow
{
	public:
								EmWindowWin			(void);
								~EmWindowWin		(void);

		void					CacheFlush			(void);
		void					GenerateSkinCache	(void);

		void					DrawDDB				(HDC, HBITMAP, HPALETTE,
													 const EmRect&,
													 const EmPoint&);

		void					GetSkin				(HDC&, HBITMAP&, HPALETTE&);
		HPALETTE				GetSystemPalette	(const EmScreenUpdateInfo&);

		HWND					GetHwnd				(void) { return fWnd; }

#ifdef SONY_ROM
		HDC						fDestDC;
		void					DrawStretchDDB (HDC, HBITMAP, HPALETTE, const EmRect&, const EmRect&);
#endif //SONY_ROM

	private:
		static LRESULT CALLBACK	PrvWndProc			(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		void					PrvOnActivate		(HWND hWnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
		void					PrvOnChar			(HWND hWnd, TCHAR ch, int cRepeat);
		void					PrvOnClose			(HWND hWnd);
		void					PrvOnCommand		(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify);
		void					PrvOnContextMenu	(HWND hWnd, HWND hwndContext, int xPos, int yPos);
		void					PrvOnDestroy		(HWND hWnd);
		void					PrvOnDisplayChange	(HWND hWnd, UINT bitsPerPixel, UINT cxScreen, UINT cyScreen);
		void					PrvOnDropFiles		(HWND hWnd, HDROP hdrop);
		void					PrvOnKeyDown		(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
		void					PrvOnKeyUp			(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
		void					PrvOnLButtonDown	(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
		void					PrvOnLButtonUp		(HWND hWnd, int x, int y, UINT keyFlags);
		void					PrvOnMouseMove		(HWND hWnd, int x, int y, UINT keyFlags);
		UINT					PrvOnNcHitTest		(HWND hWnd, int x, int y);
		void					PrvOnNcRButtonUp	(HWND hWnd, int x, int y, UINT codeHitTest);
		void					PrvOnPaint			(HWND hWnd);
		void					PrvOnPaletteChanged	(HWND hWnd, HWND hwndPaletteChange);
		BOOL					PrvOnQueryNewPalette(HWND hWnd);
		void					PrvOnSysKeyDown		(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);

	private:
		void					PrvPopupMenu		(HWND hWnd, int x, int y);
		void					PrvGetModifiers		(EmKeyEvent& event);

	private:
		virtual void			HostWindowReset		(void);

		virtual void			HostMouseCapture	(void);
		virtual void			HostMouseRelease	(void);

		virtual void			HostUpdateBegin		(void);
		virtual void			HostUpdateEnd		(void);

		virtual void			HostDrawingBegin	(void);
		virtual void			HostDrawingEnd		(void);

		virtual void			HostPaletteSet		(const EmScreenUpdateInfo&);
		virtual void			HostPaletteRestore	(void);

		virtual void			HostWindowMoveBy	(const EmPoint&);
		virtual void			HostWindowMoveTo	(const EmPoint&);
		virtual EmRect			HostWindowBoundsGet	(void);
		virtual void			HostWindowCenter	(void);
		virtual void			HostWindowShow		(void);
		virtual void			HostWindowDrag		(void);
		virtual void			HostWindowStayOnTop	(void);

		virtual void			HostRectFrame		(const EmRect&, const EmPoint&, const RGBType&);
		virtual void			HostOvalPaint		(const EmRect&, const RGBType&);

		virtual void			HostPaintCase		(const EmScreenUpdateInfo&);
		virtual void			HostPaintLCD		(const EmScreenUpdateInfo& info,
													 const EmRect& srcRect,
													 const EmRect& destRect,
													 Bool scaled);

		virtual void			HostPalette			(void);
		virtual void			HostDisplayChange	(void);

		virtual void			HostGetDefaultSkin	(EmPixMap&, int scale);
		virtual EmPoint			HostGetCurrentMouse	(void);

	private:
		HWND					fWnd;

		EmPoint					fLastPen;

		// Used when drawing/updating.
		PAINTSTRUCT				fPaintStruct;
#ifndef SONY_ROM
		HDC						fDestDC;
#endif //!SONY_ROM
		HPALETTE				fDestPalette;
		HPALETTE				fOldDestPalette;

		HPALETTE				fCachedSystemPalette;

		HDC						fCachedSkinDC;
		HBITMAP 				fCachedSkinBitmap;
		HPALETTE				fCachedSkinPalette;
};

extern EmWindowWin*	gHostWindow;

#endif	// EmLCDWin_h
