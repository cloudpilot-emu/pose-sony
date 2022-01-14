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

#ifndef EmDlgWin_h
#define EmDlgWin_h

#include "EmDlg.h"

void			CenterWindow	(HWND hWnd);

UINT CALLBACK	CenterDlgProc	(HWND hWnd,
								 UINT msg,
								 WPARAM wparam,
								 LPARAM lparam);

HWND			GetCurrentModelessDialog (void);

void			EmSetWindowText	(HWND hWndCtrl, const char* lpszNew);

//
// A C++ class for subclassing a window without any other framework:
//

class SubclassedWindow
{
	public:
								SubclassedWindow	(HWND hWnd);
		virtual 				~SubclassedWindow	(void);

	protected:
		const HWND				fWnd;
		WNDPROC 				fOldWndProc;

		static LRESULT CALLBACK StaticWndProc		(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
		virtual LRESULT 		WndProc				(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam) = 0;
};


// This class implements the hyperlink functionality.  The idea is
// quite simple: Draw the static text in the appropriate color.  When the
// user clicks in the control window, call ShellExecute with appropriate
// parameters to launch the URL.  Change the color after a successful
// browser launch.	This code grabs the URL right out of the window text.
// ShellExecute will understand both http: and mailto: URLs.

class StaticHyperlink : public SubclassedWindow
{
	public:
								StaticHyperlink		(HWND hWnd);
		virtual 				~StaticHyperlink	(void);

		HBRUSH					OnCtlColorStatic	(HWND parent, HDC dc, HWND control, int type);
		void					OnLButtonDown		(HWND hWnd, BOOL doubleclick, int x, int y, UINT flags);
		UINT					OnNcHitTest			(HWND hWnd, int x, int y);
		BOOL					OnSetCursor			(HWND hWnd, HWND hwndCursor, UINT codeHitTest, UINT msg);

	private:
		virtual LRESULT 		WndProc				(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);

		HFONT					fFont;
		HCURSOR 				fFinger;
		COLORREF				fColor;

};


class PopupMenuButton : public SubclassedWindow
{
	public:
								PopupMenuButton		(HWND hWnd);
		virtual 				~PopupMenuButton	(void);

		void					OnPaint				(HWND hWnd);
		void					OnLButtonDown		(HWND hWnd, BOOL doubleclick, int x, int y, UINT flags);

		INT						OnCBAddString		(HWND hWnd, LPCSTR);
		void					OnCBResetContent	(HWND hWnd);
		INT						OnCBGetCurSel		(HWND hWnd);
		INT						OnCBSetCurSel		(HWND hWnd, INT item);
		INT						OnCBSetItemData		(HWND hWnd, INT item, LONG data);

	private:
		virtual LRESULT 		WndProc				(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);

		HMENU					fMenu;
		HFONT					fFont;
		INT						fMenuItem;
		Bool					fSelected;
};


// Window subclass that passes on Control-Cs to the parent window.

class ControlCButton : public SubclassedWindow
{
	public:
								ControlCButton		(HWND hWnd);
		virtual 				~ControlCButton		(void);

	private:
		virtual LRESULT 		WndProc				(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

#endif	/* EmDlgWin_h */
