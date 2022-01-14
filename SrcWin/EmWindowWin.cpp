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

#include "EmCommon.h"
#include "EmWindowWin.h"

#include "EmApplicationWin.h"	// gInstance
#include "EmDlgWin.h"			// CenterWindow
#include "EmDocument.h"			// gDocument
#include "EmMenus.h"			// EmMenu, MenuFindMenu, etc.
#include "EmMenusWin.h"			// HostCreatePopupMenu
#include "EmPixMapWin.h"		// ConvertPixMapToHost
#include "EmRegion.h"			// Bounds
#include "EmRegionWin.h"		// ConvertRegionToHost
#include "EmQuantizer.h"		// EmQuantizer
#include "EmScreen.h"			// EmScreenUpdateInfo
#include "EmSession.h"			// EmSessionStopper
#include "Platform.h"			// Platform::GetString
#include "resource.h"			// IDI_EMULATOR
#include "Sounds.h"				// gSounds->NextSound
#include "Strings.r.h"			// kStr_AppName

#ifdef SONY_ROM
#include "EmCPU68K.h"			// GetRegisters
#include "SonyWin/Platform_ExpMgrLib.h"
#include "SonyWin/SonyButtonProc.h"
#include "SonyShared/SonyChars.h"
extern	ScaleType		gCurrentScale;
#endif //SONY_ROM

const char* 	kClassName	= "PalmOSEmulatorDisplay";


#define WIDTH(r)	((r).right - (r).left)
#define HEIGHT(r)	((r).bottom - (r).top)


EmWindowWin*	gHostWindow;


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmWindow::NewWindow
// ---------------------------------------------------------------------------

EmWindow* EmWindow::NewWindow (void)
{
	// Create the new Windows window.

	EmWindowWin*	theWindow = new EmWindowWin;

	// Cross-platform post-creation initialization.

	theWindow->WindowInit ();

	// Return the window.

	return theWindow;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::EmWindowWin
// ---------------------------------------------------------------------------

EmWindowWin::EmWindowWin (void) :
	EmWindow (),
	fWnd (NULL),
	fPaintStruct (),
	fDestDC (NULL),
	fDestPalette (NULL),
	fOldDestPalette (NULL),
	fCachedSystemPalette (NULL),
	fCachedSkinDC (NULL),
	fCachedSkinBitmap (NULL),
	fCachedSkinPalette (NULL)
{
	EmAssert (gHostWindow == NULL);
	gHostWindow = this;

	// Create the window class for the emulator window.  It's important
	// to create this after setting gHostWindow so that our WndProc
	// can use gHostWindow to dispatch messages.

	static bool	registered;
	if (!registered)
	{
		WNDCLASS	wc;
		::ZeroMemory (&wc, sizeof (wc));

		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= &EmWindowWin::PrvWndProc;
		wc.cbWndExtra		= sizeof (void*);
		wc.hInstance		= gInstance;
		wc.hIcon			= ::LoadIcon (gInstance, MAKEINTRESOURCE (IDI_EMULATOR));
		wc.hCursor			= ::LoadCursor (NULL, IDC_ARROW);
		wc.lpszClassName	= kClassName;

		::RegisterClass (&wc);

		registered = true;
	}

	// Create the window.  Use WS_POPUP to create a captionless window.
	// Otherwise, we'd have to remove the caption style bit in LCD_ResetWindow.
	// See MSDN article "Win32 Window Hierarchy and Styles".

	string	app (Platform::GetString (kStr_AppName));

	fWnd = ::CreateWindow (
							kClassName, app.c_str (),
							WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUP,
							CW_USEDEFAULT, CW_USEDEFAULT,
							CW_USEDEFAULT, CW_USEDEFAULT,
							NULL, NULL,			// parent, menu
							gInstance, NULL);	// lpParam

	// Initialize the window.  !!! We may want to do this in a
	// WM_CREATE handler instead of here after it's created.

	::DragAcceptFiles (fWnd, TRUE);

	// Associate the HWND with the EmWindow.

	::SetWindowLong (fWnd, 0, (LONG) this);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::~EmWindowWin
// ---------------------------------------------------------------------------

EmWindowWin::~EmWindowWin (void)
{
	this->PreDestroy ();

	if (fWnd)
	{
		::DestroyWindow (fWnd);
		fWnd = NULL;
	}

	this->CacheFlush ();

	EmAssert (gHostWindow == this);
	gHostWindow = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::CacheFlush
// ---------------------------------------------------------------------------

void EmWindowWin::CacheFlush (void)
{
	// Delete the objects.

	::DeleteObject (fCachedSystemPalette);
	::DeleteObject (fCachedSkinBitmap);
	::DeleteObject (fCachedSkinPalette);
	::DeleteDC (fCachedSkinDC);

	// Drop our references.

	fCachedSystemPalette	= NULL;
	fCachedSkinDC			= NULL;
	fCachedSkinBitmap		= NULL;
	fCachedSkinPalette		= NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::DrawDDB
// ---------------------------------------------------------------------------

void EmWindowWin::DrawDDB (HDC srcDC,
						 HBITMAP srcBitmap,
						 HPALETTE srcPalette,
						 const EmRect& from,
						 const EmPoint& to)
{
	EmAssert (fDestDC != NULL);

	HBITMAP		oldBitmap	= SelectBitmap (srcDC, srcBitmap);
	HPALETTE	oldPalette	= ::SelectPalette (srcDC, srcPalette, FALSE);

	::BitBlt (
		fDestDC,						// handle to destination device context
		to.fX, to.fY,					// y-coordinate of destination rectangle's upper-left corner
		from.Width (), from.Height (),	// width/height of destination rectangle
		srcDC,							// handle to source device context
		from.fLeft, from.fTop,			// x/y-coordinate of source rectangle's upper-left corner
		SRCCOPY);						// raster operation code

	::SelectPalette (srcDC, oldPalette, FALSE);
	SelectBitmap (srcDC, oldBitmap);
}

#ifdef SONY_ROM
void EmWindowWin::DrawStretchDDB (HDC srcDC,
						 HBITMAP srcBitmap,
						 HPALETTE srcPalette,
						 const EmRect& from,
						 const EmRect& to)
{
	EmAssert (fDestDC != NULL);

	HBITMAP		oldBitmap	= SelectBitmap (srcDC, srcBitmap);
	HPALETTE	oldPalette	= ::SelectPalette (srcDC, srcPalette, FALSE);

	::StretchBlt (
				fDestDC,						// handle to destination device context
				to.fLeft, to.fTop,					// y-coordinate of destination rectangle's upper-left corner
				to.Width (), to.Height (),	// width/height of destination rectangle
				srcDC,							// handle to source device context
				from.fLeft, from.fTop,			// x/y-coordinate of source rectangle's upper-left corner
				from.Width (), from.Height (),	// width/height of destination rectangle
				SRCCOPY);						// raster operation code

	::SelectPalette (srcDC, oldPalette, FALSE);
	SelectBitmap (srcDC, oldBitmap);
}
#endif //SONY_ROM

// ---------------------------------------------------------------------------
//		¥ EmWindowWin::GetSkin
// ---------------------------------------------------------------------------
// Get the DDB for the skin, creating the DDB and caching it if necessary.

void EmWindowWin::GetSkin (HDC& dc, HBITMAP& bm, HPALETTE& pal)
{
	if (!fCachedSkinDC)
	{
		EmAssert (fDestDC != NULL);

		EmPoint	size = this->GetCurrentSkin ().GetSize ();

		::ConvertPixMapToHost (	this->GetCurrentSkin (), fDestDC,
								fCachedSkinDC, fCachedSkinBitmap, fCachedSkinPalette,
								0, size.fY, false);
	}

	dc	= fCachedSkinDC;
	bm	= fCachedSkinBitmap;
	pal	= fCachedSkinPalette;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::GetSystemPalette
// ---------------------------------------------------------------------------
// Return a palette to be used for all our drawing.  This palette includes
// entries for both the case graphics and the colors in the LCD area.

HPALETTE EmWindowWin::GetSystemPalette (const EmScreenUpdateInfo& info)
{
	// If the dest is not a palette device, we don't need to do anything.

	if (!::IsPaletteDevice (fDestDC))
		return NULL;

	// Check to see if we need to flush our cached palette.

	if (fCachedSystemPalette)
	{
		const RGBList& newColorTable = info.fImage.GetColorTable ();

		if (this->PrevLCDColorsChanged (newColorTable))
		{
			::DeleteObject (fCachedSystemPalette);
			fCachedSystemPalette = NULL;
		}

		this->SaveLCDColors (newColorTable);
	}

	// If the screen device is palette-based, return a palette, creating
	// and caching it if necessary.

	if (!fCachedSystemPalette)
	{
		RGBList		colors;
		this->GetSystemColors (info, colors);

		size_t		lpalSize	= offsetof (LOGPALETTE, palPalEntry) +
									sizeof (PALETTEENTRY) * colors.size ();

		// Allocate memory for the palette and copy the colors.

		LOGPALETTE* lpPal		= (LOGPALETTE*) _alloca (lpalSize);
		::ConvertFromRGBList (lpPal, colors);

		// Create the logical palette.

		fCachedSystemPalette = ::CreatePalette (lpPal);
	}

	EmAssert (fCachedSystemPalette);

	return fCachedSystemPalette;
}


/***********************************************************************
 *
 * FUNCTION:	EmWindowWin::PrvWndProc
 *
 * DESCRIPTION:	Dispatch the command to the right handler via the
 *				message cracker macros.
 *
 * PARAMETERS:	Standard Window DefProc parameters.
 *
 * RETURNED:	Result code specific to the command.
 *
 ***********************************************************************/

LRESULT CALLBACK EmWindowWin::PrvWndProc (HWND hWnd,
											UINT msg,
											WPARAM wParam,
											LPARAM lParam)
{
#if defined (_DEBUG) && defined (TRACE_MSG)
	MSG	_msg;

	_msg.hwnd = hWnd;
	_msg.lParam = lParam;
	_msg.message = msg;
	_msg.wParam = wParam;

	_AfxTraceMsg ("EmWindow", &_msg);
#endif

	// Redefine HANDLE_MSG to know about gHostWindow.

	#undef HANDLE_MSG
	#define HANDLE_MSG(hwnd, message, fn)	\
		case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (gHostWindow->fn))

	//dgm -- the define for HANDLE_WM_CONTEXTMENU in windowsx.h is bogus.
	//       It forces the cursor position to unsigned, which causes menu
	//       problems on multi-monitor setups.
	//
	//	#define HANDLE_WM_CONTEXTMENU(hwnd, wParam, lParam, fn) \
	//	    ((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)

	#undef  HANDLE_WM_CONTEXTMENU
	#define HANDLE_WM_CONTEXTMENU(hwnd, wParam, lParam, fn) \
		((fn)((hwnd), (HWND)(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), 0L)

	EmAssert (gHostWindow);

	switch (msg)
	{
		HANDLE_MSG (hWnd, WM_ACTIVATE,			PrvOnActivate);
		HANDLE_MSG (hWnd, WM_CHAR,				PrvOnChar);
		HANDLE_MSG (hWnd, WM_CLOSE,				PrvOnClose);
		HANDLE_MSG (hWnd, WM_COMMAND,			PrvOnCommand);
		HANDLE_MSG (hWnd, WM_CONTEXTMENU,		PrvOnContextMenu);
		HANDLE_MSG (hWnd, WM_DESTROY,			PrvOnDestroy);
		HANDLE_MSG (hWnd, WM_DISPLAYCHANGE,		PrvOnDisplayChange);
		HANDLE_MSG (hWnd, WM_DROPFILES,			PrvOnDropFiles);
		HANDLE_MSG (hWnd, WM_KEYDOWN,			PrvOnKeyDown);
		HANDLE_MSG (hWnd, WM_KEYUP,				PrvOnKeyUp);
		HANDLE_MSG (hWnd, WM_LBUTTONDOWN,		PrvOnLButtonDown);
		HANDLE_MSG (hWnd, WM_LBUTTONUP,			PrvOnLButtonUp);
		HANDLE_MSG (hWnd, WM_MOUSEMOVE,			PrvOnMouseMove);
		HANDLE_MSG (hWnd, WM_NCHITTEST,			PrvOnNcHitTest);
		HANDLE_MSG (hWnd, WM_NCRBUTTONUP,		PrvOnNcRButtonUp);
		HANDLE_MSG (hWnd, WM_PAINT,				PrvOnPaint);
		HANDLE_MSG (hWnd, WM_PALETTECHANGED,	PrvOnPaletteChanged);
		HANDLE_MSG (hWnd, WM_QUERYNEWPALETTE,	PrvOnQueryNewPalette);
		HANDLE_MSG (hWnd, WM_SYSKEYDOWN,		PrvOnSysKeyDown);

		case MM_WOM_DONE:
			gSounds->NextSound();
			return NULL;
	}

	return ::DefWindowProc (hWnd, msg, wParam, lParam);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnActivate
 *
 * DESCRIPTION: WM_ACTIVATE handler.
 *
 *				The WM_ACTIVATE message is sent to both the window being
 *				activated and the window being deactivated. If the
 *				windows use the same input queue, the message is sent
 *				synchronously, first to the window procedure of the
 *				top-level window being deactivated, then to the window
 *				procedure of the top-level window being activated. If
 *				the windows use different input queues, the message is
 *				sent asynchronously, so the window is activated immediately.
 *
 *				We hamdle this message by forwarding it to the cross-
 *				platform window.
 *
 * PARAMETERS:  Standard WM_ACTIVATE parameters:
 *
 *				hWnd - handle to window.
 *
 *				state - whether the window is being activated or
 *					deactivated.
 *
 *				hwndActDeact - Handle to the window being activated or
 *					deactivated, depending on the value of the state
 *					parameter. If the state is WA_INACTIVE, hwndActDeact
 *					is the handle to the window being activated. If
 *					state is WA_ACTIVE or WA_CLICKACTIVE, hwndActDeact
 *					is the handle to the window being deactivated. This
 *					handle can be NULL.
 *
 *				fMinimized - the minimized state of the window being
 *					activated or deactivated.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnActivate (HWND hWnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (hwndActDeact);
	UNUSED_PARAM (fMinimized);

	this->HandleActivate (state != WA_INACTIVE);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnChar
 *
 * DESCRIPTION: WM_CHAR handler.
 *
 *				The WM_CHAR message is posted to the window with the
 *				keyboard focus when a WM_KEYDOWN message is translated
 *				by the TranslateMessage function. The WM_CHAR message
 *				contains the character code of the key that was pressed.
 *
 *				We handle this message by translating the character into
 *				either a key event or a button event and forwarding it
 *				to the cross-platform implementation.
 *
 * PARAMETERS:  Standard WM_CHAR parameters:
 *
 *				hWnd - handle to window.
 *
 *				ch - the character code of the key.
 *
 *				cRepeat - the repeat count. Other values associated with
 *					a WM_CHAR event (repeat count, scan code,
 *					extended-key flag, context code, previous
 *					key-state flag, and transition-state flag) are NOT
 *					passed to this function.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnChar (HWND hWnd, TCHAR ch, int cRepeat)
{
	UNUSED_PARAM (cRepeat);

	if (!gDocument)
		return;

	switch (ch)
	{
		case VK_RETURN: ch = chrLineFeed;		break;

			// These are handled in the key up/down handlers
			// below, so they shouldn't result in a key being
			// inserted into the event queue.
			//
			// Actually...pressing most of these keys doesn't
			// result in a WM_CHAR event.  We only need to check
			// for VK_ESCAPE.
//		case VK_UP: 	return;
//		case VK_DOWN:	return;
//		case VK_LEFT:	return;
//		case VK_RIGHT:	return;
		case VK_ESCAPE: return;
//		case VK_PRIOR:	return;
//		case VK_NEXT:	return;
//		case VK_F1: 	return;
//		case VK_F2: 	return;
//		case VK_F3: 	return;
//		case VK_F4: 	return;
	}

	EmKeyEvent	event ((unsigned char) ch);
	this->PrvGetModifiers (event);
	gDocument->HandleKey (event);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnClose
 *
 * DESCRIPTION: WM_CLOSE handler.
 *
 *				The WM_CLOSE message is sent as a signal that a window
 *				or an application should terminate.
 *
 *				We handle this message by passing a kCommandSessionClose
 *				message to the application object.
 *
 * PARAMETERS:  Standard WM_CLOSE parameters:
 *
 *				hWnd - handle to window.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnClose (HWND hWnd)
{
	// Send a kCommandClose message to the application.

	UINT	id			= kCommandSessionClose;
	UINT	codeNotify	= 0;
	HWND	hwndCtl		= NULL;

	::PostMessage (
		gHostApplication->GetWindow (),	// Send to the application's window
		WM_COMMAND,
		MAKEWPARAM((UINT)(id),(UINT)(codeNotify)),
		(LPARAM)(HWND)(hwndCtl));
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnCommand
 *
 * DESCRIPTION: WM_COMMAND handler.
 *
 *				The WM_COMMAND message is sent when the user selects a
 *				command item from a menu, when a control sends a
 *				notification message to its parent window, or when an
 *				accelerator keystroke is translated.
 *
 *				We handle this message by passing it to the cross-
 *				platform implementation.  If it doesn't handle it, we
 *				post it to the application object.
 *
 * PARAMETERS:  Standard WM_COMMAND parameters:
 *
 *				hWnd - handle to window.
 *
 *				id - the identifier of the menu item, control, or
 *					accelerator.
 *
 *				hwndCtl - Handle to the control sending the message if
 *					the message is from a control. Otherwise, this
 *					parameter is NULL.
 *
 *				codeNotify - the notification code if the message is
 *					from a control. If the message is from an
 *					accelerator, this value is 1. If the message is from
 *					a menu, this value is zero.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnCommand (HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
	// We need to see if the command was issued because of a match
	// with the accelerator table.  If so, the command may be disabled.

	if (codeNotify == 1)	// Indicates command from accelerator
	{
		// Get an up-to-date menu list.

		EmMenu*	menu = ::MenuFindMenu (kMenuPopupMenuPreferred);
		EmAssert (menu);

		::MenuUpdateMruMenus (*menu);
		::MenuUpdateMenuItemStatus (*menu);

		// Get the menu item for this command.

		EmMenuItem*		selected = MenuFindMenuItemByCommandID (*menu, (EmCommandID) id, true);
		EmAssert (selected);

		// If we couldn't find it, leave.

		if (!selected)
			return;

		// If disabled, leave.

		if (!selected->GetIsActive ())
			return;

		// Otherwise, it should be OK to execute this command.
		// Fall through to the command handler.
	}

	// See if the document wants to handle it.

	if (!gDocument || !gDocument->HandleCommand ((EmCommandID) id))
	{
		// Otherwise, let the application have a shot at it.  We post the command
		// instead of calling gApplication->HandleCommand in case the command
		// results in the closing of this window.

		// Translate our parameters back into something that can be passed
		// PostMessage.  Model this after FORWARD_WM_COMMAND:
		//
		//	#define FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
		//		(void)(fn)((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))

		::PostMessage (
			gHostApplication->GetWindow (),	// Send to the application's window
			WM_COMMAND,
			MAKEWPARAM((UINT)(id),(UINT)(codeNotify)),
			(LPARAM)(HWND)(hwndCtl));
	}
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnContextMenu
 *
 * DESCRIPTION: WM_CONTEXTMENU handler.
 *
 *				The WM_CONTEXTMENU message notifies a window that the
 *				user clicked the right mouse button (right clicked) in
 *				the window.
 *
 *				DefWindowProc generates the WM_CONTEXTMENU message when
 *				it processes the WM_RBUTTONUP or WM_NCRBUTTONUP message
 *				or when the user types SHIFT+F10. The WM_CONTEXTMENU
 *				message is also generated when the user presses and
 *				releases the VK_APPS key.
 *
 *				If the context menu is generated from the keyboard --
 *				for example, if the user types SHIFT+F10 -- then the
 *				x- and y-coordinates are -1 and the application should
 *				display the context menu at the location of the current
 *				selection rather than at (xPos, yPos).
 *
 *				We handle this message by popping up our standard user
 *				menu.  Any command generated is posted back to this
 *				window as a WM_COMMAND message.
 *
 * PARAMETERS:  Standard WM_CONTEXTMENU parameters:
 *
 *				hWnd - handle to window.
 *
 *				hwndContext - Handle to the window in which the user
 *					right clicked the mouse. This can be a child window
 *					of the window receiving the message.
 *
 *				xPos - the horizontal position of the cursor, in screen
 *					coordinates, at the time of the mouse click.
 *
 *				yPos - the vertical position of the cursor, in screen
 *					coordinates, at the time of the mouse click.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnContextMenu (HWND hWnd, HWND hwndContext, int xPos, int yPos)
{
	UNUSED_PARAM (hwndContext);

	// When the user clicks with the right mouse button, popup our menu.
	//
	// (-1, -1) means that they pressed Shift-F10

	if (xPos == -1 && xPos == -1)
	{
		xPos = 0;
		yPos = 0;
	}
	else
	{
		POINT local = { xPos, yPos };
		::ScreenToClient (hWnd, &local);

		xPos = local.x;
		yPos = local.y;
	}

	this->PrvPopupMenu (hWnd, xPos, yPos);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnDestroy
 *
 * DESCRIPTION: WM_DESTROY handler.
 *
 *				The WM_DESTROY message is sent when a window is being
 *				destroyed. It is sent to the window procedure of the
 *				window being destroyed after the window is removed from
 *				the screen.
 *
 *				We handle this message by calling PostQuitMessage to
 *				cause the main event loop to exit.
 *
 * PARAMETERS:  Standard WM_DESTROY parameters:
 *
 *				hWnd - handle to window
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnDestroy (HWND hWnd)
{
	UNUSED_PARAM (hWnd);

	::PostQuitMessage (0);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnDisplayChange
 *
 * DESCRIPTION: WM_DISPLAYCHANGE handler.
 *
 *				The WM_DISPLAYCHANGE message is sent to all windows when
 *				the display resolution has changed.
 *
 *				We handle this message by xxx.
 *
 * PARAMETERS:  Standard WM_DISPLAYCHANGE parameters:
 *
 *				hWnd - handle to window.
 *
 *				bitsPerPixel - the new image depth of the display, in
 *					bits per pixel.
 *
 *				cxScreen - the horizontal resolution of the screen.
 *
 *				cyScreen - the vertical resolution of the screen.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnDisplayChange (HWND hWnd, UINT bitsPerPixel,
										UINT cxScreen, UINT cyScreen)
{
	UNUSED_PARAM (hWnd);
	UNUSED_PARAM (bitsPerPixel);
	UNUSED_PARAM (cxScreen);
	UNUSED_PARAM (cyScreen);

	this->HandleDisplayChange ();
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnDropFiles
 *
 * DESCRIPTION: WM_DROPFILES handler.
 *
 *				Sent when the user drops a file on the window of an
 *				application that has registered itself as a recipient
 *				of dropped files.
 *
 *				We handle this message by re-posting it to the
 *				application object.
 *
 * PARAMETERS:  Standard WM_DROPFILES parameters:
 *
 *				hWnd - handle to window.
 *
 *				hdrop - Handle to an internal structure describing the
 *					dropped files. Pass this handle DragFinish,
 *					DragQueryFile, or DragQueryPoint to retrieve
 *					information about the dropped files.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnDropFiles (HWND hWnd, HDROP hdrop)
{
	// Forward this operation to the application.

	// Translate our parameters back into something that can be passed
	// PostMessage.  Model this after FORWARD_WM_DROPFILES:
	//
	//	#define FORWARD_WM_DROPFILES(hwnd, hdrop, fn) \
	//		(void)(fn)((hwnd), WM_DROPFILES, (WPARAM)(HDROP)(hdrop), 0L)

	::PostMessage (
		gHostApplication->GetWindow (),	// Send to the application's window
		WM_DROPFILES,
		(WPARAM)(HDROP)(hdrop),
		0L);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnKeyDown
 *
 * DESCRIPTION: WM_KEYDOWN handler.
 *
 *				The WM_KEYDOWN message is posted to the window with the
 *				keyboard focus when a nonsystem key is pressed. A
 *				nonsystem key is a key that is pressed when the ALT key
 *				is not pressed.
 *
 *				We handle this message by translating the character into
 *				either a key event or a button event and forwarding it
 *				to the cross-platform implementation.
 *
 * PARAMETERS:  Standard WM_KEYDOWN parameters:
 *
 *				hWnd - handle to window.
 *
 *				vk - the virtual-key code of the nonsystem key.
 *
 *				fDown - always true.
 *
 *				cRepeat - the repeat count for the current message.
 *
 *				flags - scan code, extended-key flag, context code,
 *					previous key-state flag, and transition-state flag.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnKeyDown (HWND hWnd, UINT vk, BOOL fDown,
								  int cRepeat, UINT flags)
{
	if (!gDocument)
		return;

	unsigned char		whichKey	= 0;
	SkinElementType 	whichButton = kElement_None;

	switch (vk)
	{
		case VK_UP: 	whichKey = chrUpArrow;		break;
		case VK_DOWN:	whichKey = chrDownArrow;	break;
		case VK_LEFT:	whichKey = chrLeftArrow;	break;
		case VK_RIGHT:	whichKey = chrRightArrow;	break;

		case VK_ESCAPE: whichButton = kElement_PowerButton; 	break;
		case VK_PRIOR:	whichButton = kElement_UpButton;		break;
		case VK_NEXT:	whichButton = kElement_DownButton;		break;
		case VK_F1: 	whichButton = kElement_App1Button;		break;
		case VK_F2: 	whichButton = kElement_App2Button;		break;
		case VK_F3: 	whichButton = kElement_App3Button;		break;
		case VK_F4: 	whichButton = kElement_App4Button;		break;
	}

	if (whichKey != 0)
	{
		EmKeyEvent	event ((unsigned char) whichKey);
		this->PrvGetModifiers (event);
		gDocument->HandleKey (event);
	}

	if (whichButton != kElement_None)
	{
		gDocument->HandleButton (whichButton, true);
	}
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnKeyUp
 *
 * DESCRIPTION: WM_KEYUP handler.
 *
 *				The WM_KEYUP message is posted to the window with the
 *				keyboard focus when a nonsystem key is released. A
 *				nonsystem key is a key that is pressed when the ALT key
 *				is not pressed, or a keyboard key that is pressed when
 *				a window has the keyboard focus.
 *
 *				We handle this message by translating the character into
 *				either a key event or a button event and forwarding it
 *				to the cross-platform implementation.
 *
 * PARAMETERS:  Standard WM_KEYUP parameters:
 *
 *				hWnd - handle to window.
 *
 *				vk - the virtual-key code of the nonsystem key.
 *
 *				fDown - always false.
 *
 *				cRepeat - the repeat count for the current message.
 *
 *				flags - scan code, extended-key flag, context code,
 *					previous key-state flag, and transition-state flag.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnKeyUp (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	if (!gDocument)
		return;

	SkinElementType 	whichButton = kElement_None;

	switch (vk)
	{
		case VK_ESCAPE: whichButton = kElement_PowerButton; 	break;
		case VK_PRIOR:	whichButton = kElement_UpButton;		break;
		case VK_NEXT:	whichButton = kElement_DownButton;		break;
		case VK_F1: 	whichButton = kElement_App1Button;		break;
		case VK_F2: 	whichButton = kElement_App2Button;		break;
		case VK_F3: 	whichButton = kElement_App3Button;		break;
		case VK_F4: 	whichButton = kElement_App4Button;		break;
	}

	if (whichButton != kElement_None)
	{
		gDocument->HandleButton (whichButton, false);
	}
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnLButtonDown
 *
 * DESCRIPTION: WM_LBUTTONDOWN handler.
 *
 *				The WM_LBUTTONDOWN message is posted when the user
 *				presses the left mouse button while the cursor is in the
 *				client area of a window. If the mouse is not captured,
 *				the message is posted to the window beneath the cursor.
 *				Otherwise, the message is posted to the window that has
 *				captured the mouse.
 *
 *				We handle this message by forwarding it to the cross-
 *				platform implementation.
 *
 * PARAMETERS:  Standard WM_LBUTTONDOWN parameters:
 *
 *				hWnd - handle to window.
 *
 *				fDoubleClick - always false.
 *
 *				x - the x-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the client area.
 *
 *				y - the y-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the client area.
 *
 *				keyFlags - Indicates whether various virtual keys are
 *					down. This parameter can be one or more of the
 *					following values: MK_CONTROL, MK_LBUTTON, MK_MBUTTON,
 *					MK_RBUTTON, MK_SHIFT, MK_XBUTTON1, MK_XBUTTON2.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnLButtonDown (HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	fLastPen = EmPoint (x, y);
	this->HandlePenEvent (fLastPen, true);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnLButtonUp
 *
 * DESCRIPTION: WM_LBUTTONUP handler.
 *
 *				The WM_LBUTTONUP message is posted when the user
 *				releases the left mouse button while the cursor is in
 *				the client area of a window. If the mouse is not
 *				captured, the message is posted to the window beneath
 *				the cursor. Otherwise, the message is posted to the
 *				window that has captured the mouse.
 *
 *				We handle this message by forwarding it to the cross-
 *				platform implementation.
 *
 * PARAMETERS:  Standard WM_LBUTTONUP parameters:
 *
 *				hWnd - handle to window.
 *
 *				x - the x-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the client area.
 *
 *				y - the y-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the client area.
 *
 *				keyFlags - Indicates whether various virtual keys are
 *					down. This parameter can be one or more of the
 *					following values: MK_CONTROL, MK_MBUTTON,
 *					MK_RBUTTON, MK_SHIFT, MK_XBUTTON1, MK_XBUTTON2.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnLButtonUp (HWND hWnd, int x, int y, UINT keyFlags)
{
	fLastPen = EmPoint (x, y);
	this->HandlePenEvent (fLastPen, false);
	fLastPen = EmPoint (-1, -1);	// Invalidate last pen value.
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnMouseMove
 *
 * DESCRIPTION: WM_MOUSEMOVE handler.
 *
 *				The WM_MOUSEMOVE message is posted to a window when the
 *				cursor moves. If the mouse is not captured, the message
 *				is posted to the window that contains the cursor.
 *				Otherwise, the message is posted to the window that has
 *				captured the mouse.
 *
 *				We handle this message by remembering the pen location.
 *				We later pass that location to the cross-platform
 *				implementation in its idle handler
 *
 * PARAMETERS:  Standard WM_MOUSEMOVE parameters:
 *
 *				hWnd - handle to window.
 *
 *				x - the x-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the client area.
 *
 *				y - the y-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the client area.
 *
 *				keyFlags - Indicates whether various virtual keys are
 *					down. This parameter can be one or more of the
 *					following values: MK_CONTROL, MK_LBUTTON, MK_MBUTTON,
 *					MK_RBUTTON, MK_SHIFT, MK_XBUTTON1, MK_XBUTTON2.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnMouseMove (HWND hWnd, int x, int y, UINT keyFlags)
{
	if (::GetCapture () == hWnd)
	{
		fLastPen = EmPoint (x, y);
	}
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnNcHitTest
 *
 * DESCRIPTION: WM_NCHITTEST handler.
 *
 *				The WM_NCHITTEST message is sent to a window when the
 *				cursor moves, or when a mouse button is pressed or
 *				released. If the mouse is not captured, the message is
 *				sent to the window beneath the cursor. Otherwise, the
 *				message is sent to the window that has captured the
 *				mouse.
 *
 *				We handle this message by reporting that any hits on
 *				any part of the window other than active parts (like
 *				the LCD area or any of the buttons) is a hit on the
 *				caption.  This allows the user to drag the window
 *				around by clicking on any inactive area.
 *
 * PARAMETERS:  Standard WM_NCHITTEST parameters:
 *
 *				hWnd - handle to window.
 *
 *				x - the x-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the screen.
 *
 *				y - the y-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the screen.
 *
 * RETURNED:    The return value of the DefWindowProc function is one of
 *				the following values, indicating the position of the
 *				cursor hot spot: HTBORDER, HTBOTTOM, etc.
 *
 ***********************************************************************/

UINT EmWindowWin::PrvOnNcHitTest (HWND hWnd, int x, int y)
{
	POINT	clientPt = { x, y };
	::ScreenToClient (hWnd, &clientPt);

	EmPoint			pt (clientPt.x, clientPt.y);
	SkinElementType what = ::SkinTestPoint (pt);

	if (what == kElement_Frame)
		return HTCAPTION;

	return FORWARD_WM_NCHITTEST (hWnd, x, y, DefWindowProc);
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnNcRButtonUp
 *
 * DESCRIPTION: WM_NCRBUTTONUP handler.
 *
 *				The WM_NCRBUTTONUP message is posted when the user
 *				releases the right mouse button while the cursor is
 *				within the nonclient area of a window. This message is
 *				posted to the window that contains the cursor. If a
 *				window has captured the mouse, this message is not
 *				posted.
 *
 *				We handle this message by reposting it as a
 *				WM_CONTEXTMENU message.	We need to post this message
 *				explicitly, because Windows won't do it.  Because of
 *				our handling of WM_NCHITTEST, Windows thinks that
 *				right-clicks in the skin frame are in the caption.  In
 *				that case, Windows is supposed to show the "window"
 *				(or "system") menu.  However, we don't have such a menu,
 *				so Windows actually does nothing.  Therefore, we need to
 *				post the WM_CONTEXTMENU ourselves.
 *
 * PARAMETERS:  Standard WM_NCRBUTTONUP parameters:
 *
 *				hWnd - handle to window.
 *
 *				x - the x-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the screen.
 *
 *				y - the y-coordinate of the cursor. The coordinate is
 *					relative to the upper-left corner of the screen.
 *
 *				codeHitTest - the hit-test value returned by the
 *					DefWindowProc function as a result of processing
 *					the WM_NCHITTEST message.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnNcRButtonUp (HWND hWnd, int x, int y, UINT codeHitTest)
{
	// Model this after FORWARD_WM_CONTEXTMENU:
	//
	//	#define FORWARD_WM_CONTEXTMENU(hwnd, hwndContext, xPos, yPos, fn) \
	//		(void)(fn)((hwnd), WM_CONTEXTMENU, (WPARAM)(HWND)(hwndContext), MAKELPARAM((UINT)(xPos), (UINT)(yPos)))

	::PostMessage (
		hWnd,
		WM_CONTEXTMENU,
		(WPARAM)(HWND)(hWnd),
		MAKELPARAM((UINT)(x), (UINT)(y)));
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnPaint
 *
 * DESCRIPTION: WM_PAINT handler.
 *
 *				The WM_PAINT message is sent when the system or another
 *				application makes a request to paint a portion of an
 *				application's window. The message is sent when the
 *				UpdateWindow or RedrawWindow function is called, or by
 *				the DispatchMessage function when the application obtains
 *				a WM_PAINT message by using the GetMessage or PeekMessage
 *				function.
 *
 *				We handle this message by forwarding it to the cross-
 *				platform implementation.
 *
 * PARAMETERS:  Standard WM_PAINT parameters:
 *
 *				hWnd - handle to window.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnPaint (HWND hWnd)
{
	this->HandleUpdate ();
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnPaletteChanged
 *
 * DESCRIPTION: WM_PALETTECHANGED handler.
 *
 *				The WM_PALETTECHANGED message is sent to all top-level
 *				and overlapped windows after the window with the
 *				keyboard focus has realized its logical palette, thereby
 *				changing the system palette. This message enables a
 *				window that uses a color palette but does not have the
 *				keyboard focus to realize its logical palette and update
 *				its client area.
 *
 *				We handle this message by forwarding it to the cross-
 *				platform implementation.
 *
 * PARAMETERS:  Standard WM_PALETTECHANGED parameters:
 *
 *				hWnd - handle to window.
 *
 *				hwndPaletteChange - Handle to the window that caused
 *					the system palette to change.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnPaletteChanged (HWND hWnd, HWND hwndPaletteChange)
{
	if (hwndPaletteChange != hWnd)
	{
		this->HandlePalette ();
	}
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnQueryNewPalette
 *
 * DESCRIPTION: WM_QUERYNEWPALETTE handler.
 *
 *				The WM_QUERYNEWPALETTE message informs a window that it
 *				is about to receive the keyboard focus, giving the
 *				window the opportunity to realize its logical palette
 *				when it receives the focus.
 *
 *				We handle this message by forwarding it to the cross-
 *				platform implementation.
 *
 * PARAMETERS:  Standard WM_QUERYNEWPALETTE parameters:
 *
 *				hWnd - handle to window.
 *
 * RETURNED:    If the window realizes its logical palette, it must
 *				return TRUE; otherwise, it must return FALSE.
 *
 ***********************************************************************/

BOOL EmWindowWin::PrvOnQueryNewPalette (HWND hWnd)
{
	this->HandlePalette ();
	return true;
}


/***********************************************************************
 *
 * FUNCTION:    EmWindowWin::PrvOnSysKeyDown
 *
 * DESCRIPTION: WM_SYSKEYDOWN handler.
 *
 *				The WM_SYSKEYDOWN message is posted to the window with
 *				the keyboard focus when the user presses the F10 key
 *				(which activates the menu bar) or holds down the ALT key
 *				and then presses another key. It also occurs when no
 *				window currently has the keyboard focus; in this case,
 *				the WM_SYSKEYDOWN message is sent to the active window.
 *				The window that receives the message can distinguish
 *				between these two contexts by checking the context code
 *				in the lParam parameter.
 *
 *				We handle this message by checking for ALT+F11, which
 *				we use to minimize the window.  Otherwise, we let the
 *				default window handler handle it.
 *
 * PARAMETERS:  Standard WM_SYSKEYDOWN parameters:
 *
 *				hWnd - handle to window.
 *
 *				vk - the virtual-key code of the key being pressed.
 *
 *				fDown - always true.
 *
 *				cRepeat - the repeat count for the current message.
 *
 *				flags - the scan code, extended-key flag, context code,
 *					previous key-state flag, and transition-state flag.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvOnSysKeyDown (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch (vk)
	{
		// When the user presses Alt+F11, minimize our window.

		case VK_F11:
			if (flags & (1 << (29 - 16)))
			{
				::ShowWindow (hWnd, SW_MINIMIZE);
			}

			return;
	}

	FORWARD_WM_SYSKEYDOWN (hWnd, vk, cRepeat, flags, DefWindowProc);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmWindowWin::PrvPopupMenu
 *
 * DESCRIPTION:	Shows standard menu.
 *
 * PARAMETERS:	The same parameters passed to PrvOnContextMenu, sans
 *				the context window parameter.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvPopupMenu (HWND hWnd, int x, int y)
{
	// Update the menu.

	EmMenu*	menu = ::MenuFindMenu (kMenuPopupMenuPreferred);
	EmAssert (menu);

	::MenuUpdateMruMenus (*menu);
	::MenuUpdateMenuItemStatus (*menu);

	// Create and show the menu.

	HMENU	hostMenu = ::HostCreatePopupMenu (*menu);

	POINT	p = { x,  y };
	::ClientToScreen (hWnd, &p);

	::TrackPopupMenu (hostMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, 0, hWnd, NULL);

	::DestroyMenu (hostMenu);
}


/***********************************************************************
 *
 * FUNCTION:	EmWindowWin::PrvGetModifiers
 *
 * DESCRIPTION:	Translates the currently held-down set of keyboard
 *				modifiers into a cross-platform set of flags.
 *
 * PARAMETERS:	event - the event structure to receive the modifier
 *					flags.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void EmWindowWin::PrvGetModifiers (EmKeyEvent& event)
{
	if ((::GetKeyState (VK_SHIFT) & 0x80) != 0)
		event.fShiftDown = true;

	if ((::GetKeyState (VK_CAPITAL) & 0x01) != 0)
		event.fCapsLockDown = true;

	if ((::GetKeyState (VK_NUMLOCK) & 0x01) != 0)
		event.fNumLockDown = true;

//	if ((::GetKeyState (0) & 0x80) != 0)
//		event.fCommandDown = true;

//	if ((::GetKeyState (0) & 0x80) != 0)
//		event.fOptionDown = true;

	if ((::GetKeyState (VK_CONTROL) & 0x80) != 0)
		event.fControlDown = true;

	if ((::GetKeyState (VK_MENU) & 0x80) != 0)
		event.fAltDown = true;

	if ((::GetKeyState (VK_LWIN) & 0x80) != 0 ||
		(::GetKeyState (VK_RWIN) & 0x80) != 0)
		event.fWindowsDown = true;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowReset
// ---------------------------------------------------------------------------
// Update the window's appearance due to a skin change.

void EmWindowWin::HostWindowReset (void)
{
	EmAssert (fWnd != NULL);

	// Delete the old bitmap and palette.

	this->CacheFlush ();

	// Change the window to accomodate the settings and bitmap.

	// Get the current window size.  We get the "window" rect (the outer
	// window bounds) and the "client" rect (the inner window bounds), so
	// that we can subtract the two and determine the frame size.

	RECT wr, cr;
	::GetWindowRect (fWnd, &wr);
	::GetClientRect (fWnd, &cr);

	// Get the desired client size.

	EmRect	newBounds = this->GetCurrentSkinRegion ().Bounds ();

	// Determine the new width/height (of the window, not the client).

	UINT	width	= newBounds.Width () + WIDTH (wr) - WIDTH (cr);
	UINT	height	= newBounds.Height () + HEIGHT (wr) - HEIGHT (cr);

	// Resize the window.

	::SetWindowPos (fWnd, NULL, 0, 0, width, height,
		SWP_ASYNCWINDOWPOS |
		SWP_NOACTIVATE |
		SWP_NOMOVE |
		SWP_NOOWNERZORDER |
		SWP_NOZORDER);

	// Set the framing region for the window.

	HRGN	rgn = ::ConvertRegionToHost (this->GetCurrentSkinRegion ());
	::SetWindowRgn (fWnd, rgn, TRUE);

	// Invalidate the window contents now (necessary?).

	::InvalidateRect (fWnd, NULL, FALSE);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostMouseCapture
// ---------------------------------------------------------------------------
// Capture the mouse so that all mouse events get sent to this window.

void EmWindowWin::HostMouseCapture (void)
{
	EmAssert (fWnd != NULL);

	::SetCapture (fWnd);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostMouseRelease
// ---------------------------------------------------------------------------
// Release the mouse so that mouse events get sent to the window the
// cursor is over.

void EmWindowWin::HostMouseRelease (void)
{
	::ReleaseCapture ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostUpdateBegin
// ---------------------------------------------------------------------------
// Prepare the host window object for updating.

void EmWindowWin::HostUpdateBegin (void)
{
	EmAssert (fWnd != NULL);
	EmAssert (fDestDC == NULL);

	fDestDC = ::BeginPaint (fWnd, &fPaintStruct);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostUpdateEnd
// ---------------------------------------------------------------------------
// Finalize the host window object after it's been updated.

void EmWindowWin::HostUpdateEnd (void)
{
	EmAssert (fWnd != NULL);
	EmAssert (fDestDC != NULL);

	::EndPaint (fWnd, &fPaintStruct);
	fDestDC = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostDrawingBegin
// ---------------------------------------------------------------------------
// Prepare the host window object for drawing outside of an update event.

void EmWindowWin::HostDrawingBegin (void)
{
	EmAssert (fWnd != NULL);
	EmAssert (fDestDC == NULL);

	fDestDC = ::GetDC (fWnd);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostDrawingEnd
// ---------------------------------------------------------------------------
// Finalize the host window object after drawing outside of an update event.

void EmWindowWin::HostDrawingEnd (void)
{
	EmAssert (fWnd != NULL);
	EmAssert (fDestDC != NULL);

	::ReleaseDC (fWnd, fDestDC);
	fDestDC = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostPaletteSet
// ---------------------------------------------------------------------------
// Establish the palette to be used for drawing.

void EmWindowWin::HostPaletteSet (const EmScreenUpdateInfo& info)
{
	EmAssert (fDestDC != NULL);

	// Get the palette we'll be using.

	EmAssert (fDestPalette == NULL);
	EmAssert (fOldDestPalette == NULL);

	fDestPalette = this->GetSystemPalette (info);

	if (fDestPalette)
	{
		fOldDestPalette = ::SelectPalette (fDestDC, fDestPalette, FALSE);
		::RealizePalette (fDestDC);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostPaletteRestore
// ---------------------------------------------------------------------------
// Clean up after HostPaletteSet.

void EmWindowWin::HostPaletteRestore (void)
{
	EmAssert (fDestDC != NULL);

	if (fOldDestPalette)
	{
		::SelectPalette (fDestDC, fOldDestPalette, FALSE);
//		::RealizePalette (fDestDC);

		fDestPalette	= NULL;
		fOldDestPalette	= NULL;
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowMoveBy
// ---------------------------------------------------------------------------
// Move the host window object by the given offset.

void EmWindowWin::HostWindowMoveBy (const EmPoint& offset)
{
	this->HostWindowMoveTo (this->HostWindowBoundsGet ().TopLeft () + offset);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowMoveTo
// ---------------------------------------------------------------------------
// Move the host window object to the given location.

void EmWindowWin::HostWindowMoveTo (const EmPoint& loc)
{
	EmAssert (fWnd != NULL);

	::SetWindowPos (fWnd, NULL, loc.fX, loc.fY, 0, 0,
		SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowBoundsGet
// ---------------------------------------------------------------------------
// Get the global bounds of the host window object.

EmRect EmWindowWin::HostWindowBoundsGet (void)
{
	EmAssert (fWnd != NULL);

	RECT	r;
	::GetWindowRect (fWnd, &r);

	return EmRect (r);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowCenter
// ---------------------------------------------------------------------------
// Center the window to the main display.

void EmWindowWin::HostWindowCenter (void)
{
	EmAssert (fWnd != NULL);

	::CenterWindow (fWnd);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowShow
// ---------------------------------------------------------------------------
// Make the host window object visible.

void EmWindowWin::HostWindowShow (void)
{
	EmAssert (fWnd != NULL);

	::ShowWindow (fWnd, SW_SHOWNORMAL);
	::SetForegroundWindow (fWnd);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowStayOnTop
// ---------------------------------------------------------------------------
// Make the host window stay on top.

void EmWindowWin::HostWindowStayOnTop (void)
{
	EmAssert (fWnd != NULL);

	Preference<bool>	prefStayOnTop (kPrefKeyStayOnTop);

	if (prefStayOnTop.Loaded ())
	{
		HWND hWnd = NULL;

		if (*prefStayOnTop.GetValue ())
			hWnd = HWND_TOPMOST;
		else
			hWnd = HWND_NOTOPMOST;

		::SetWindowPos (fWnd, hWnd, 0, 0, 0, 0,
						SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostWindowDrag
// ---------------------------------------------------------------------------
// The user has clicked in a region of the host window object that causes
// the window to be dragged.  Drag the window around.

void EmWindowWin::HostWindowDrag (void)
{
	// Not needed on Windows.  The detection and handling of the window
	// drag is taken care of before the mouse event is passed off to the
	// EmWindow object.
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostRectFrame
// ---------------------------------------------------------------------------
// Draw a rectangle frame with the given width in the given color.

void EmWindowWin::HostRectFrame (const EmRect& r, const EmPoint& pen, const RGBType& color)
{
	EmAssert (fDestDC != NULL);

	HBRUSH		theBrush	= (HBRUSH) ::GetStockObject (HOLLOW_BRUSH);
	HBRUSH		oldBrush	= SelectBrush (fDestDC, theBrush);

	COLORREF	rgb			= RGB (color.fRed, color.fGreen, color.fBlue);
	HPEN		thePen		= ::CreatePen (PS_INSIDEFRAME, pen.fX, rgb);
	HPEN		oldPen		= SelectPen (fDestDC, thePen);

	::Rectangle (fDestDC, r.fLeft, r.fTop, r.fRight, r.fBottom);

	SelectPen (fDestDC, oldPen);
	SelectBrush (fDestDC, oldBrush);

	::DeleteObject (thePen);
	::DeleteObject (theBrush);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostOvalPaint
// ---------------------------------------------------------------------------
// Fill an oval with the given color.

void EmWindowWin::HostOvalPaint (const EmRect& r, const RGBType& color)
{
	EmAssert (fDestDC != NULL);

	COLORREF	rgb			= RGB (color.fRed, color.fGreen, color.fBlue);
	HBRUSH		brush		= ::CreateSolidBrush (rgb);
	HBRUSH		oldBrush	= (HBRUSH) ::SelectObject (fDestDC, brush);

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGT400 
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGT600
	 || gSession->GetDevice().GetDeviceType() == kDeviceYSX1100
	 || gSession->GetDevice().GetDeviceType() == kDeviceYSX1230
	 ) 
	{
		RECT rc;
		::SetRect(&rc, r.fLeft, r.fTop, r.fRight, r.fBottom);
		::FillRect(fDestDC, &rc, brush);
	}
	else 
		::Ellipse (fDestDC, r.fLeft, r.fTop, r.fRight, r.fBottom);
#else //!SONY_ROM
	::Ellipse (fDestDC, r.fLeft, r.fTop, r.fRight, r.fBottom);
#endif //SONY_ROM
	

	::SelectObject (fDestDC, oldBrush);
	::DeleteObject (brush);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostPaintCase
// ---------------------------------------------------------------------------
// Draw the skin.

void EmWindowWin::HostPaintCase (const EmScreenUpdateInfo& info)
{
	HDC			srcDC;
	HBITMAP		srcBitmap;
	HPALETTE	srcPalette;

	EmRect		from (EmPoint (0, 0), this->GetCurrentSkin ().GetSize ());
	EmPoint		to (0, 0);

	this->GetSkin (srcDC, srcBitmap, srcPalette);
	this->DrawDDB (srcDC, srcBitmap, srcPalette, from, to);

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGS500C 
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS300
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGN700C
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS320
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGT400
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGT600
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGN600C
	 || gSession->GetDevice().GetDeviceType() == kDeviceYSX1100
	 || gSession->GetDevice().GetDeviceType() == kDeviceYSX1230)
	{	
		for (int i=0; i<BUTTON_NUM; i++)
			if (gJogButtonState[i].nButton != kElement_PowerButton)
				LCD_DrawButtonForPEG(this->fDestDC, gJogButtonState[i].nButton);
	}

	// redraw PowerButton
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGS500C 
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS300
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS320)
	{
		LCD_SetStateJogButton(kElement_PowerButton, info.fLCDOn, true);
		LCD_DrawButtonForPEG(fDestDC, kElement_PowerButton);
	}
#endif //SONY_ROM
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostPaintLCD
// ---------------------------------------------------------------------------
// Draw the LCD area.

void EmWindowWin::HostPaintLCD (const EmScreenUpdateInfo& info, const EmRect& srcRect,
						  const EmRect& destRect, Bool scaled)
{
	EmAssert (fDestDC != NULL);

	HDC 		srcDC		= NULL;
	HBITMAP 	srcBitmap	= NULL;
	HPALETTE	srcPalette	= NULL;

	::ConvertPixMapToHost (	info.fImage, fDestDC,
							srcDC, srcBitmap, srcPalette,
							info.fFirstLine, info.fLastLine,
							scaled);

#ifdef SONY_ROM
	this->DrawStretchDDB (srcDC, srcBitmap, srcPalette, srcRect, destRect);
#else
	this->DrawDDB (srcDC, srcBitmap, srcPalette, srcRect, destRect.TopLeft ());
#endif //SONY_ROM
	

	::DeleteObject (srcPalette);
	::DeleteObject (srcBitmap);
	::DeleteDC (srcDC);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostPalette
// ---------------------------------------------------------------------------
// Tell the system about the palette we want to use.  Called when Windows
// is updating its system palette.

void EmWindowWin::HostPalette (void)
{
	EmAssert (fWnd != NULL);

	// Get information about the screen.

	// Problem here: getting the bits at this point could mess up
	// PrvPaintScreen when *it* gets the bits.	It could be that our
	// call here picks up on a change in the LCD bits, which could prevent
	// PrvPaintScreen from seeing the change.  To avoid this problem, we
	// re-invalidate the screen after we get its information.

	EmScreenUpdateInfo	info;

	{
		EmSessionStopper	stopper (gSession, kStopNow);

		if (!stopper.Stopped ())
			return;

		EmScreen::InvalidateAll ();
		EmScreen::GetBits (info);
		EmScreen::InvalidateAll ();
	}

	// Get the palette we prefer.

	HPALETTE	pal = this->GetSystemPalette (info);

	if (pal)
	{
		HDC 		dc = ::GetDC (fWnd);
		HPALETTE	op = ::SelectPalette (dc, pal, FALSE);
		UINT		n = ::RealizePalette (dc);

		if (n != GDI_ERROR)
		{
			if (n > 0)
			{
				::InvalidateRect (fWnd, NULL, FALSE);
			}
		}

		::SelectPalette (dc, op, TRUE);
		::RealizePalette (dc);

		::ReleaseDC (fWnd, dc);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostDisplayChange
// ---------------------------------------------------------------------------
// Respond to the display's bit depth changing.  All we do here is flush our
// caches of the skin information.  It will get regenerated when it's needed.

void EmWindowWin::HostDisplayChange (void)
{
	this->CacheFlush ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostGetDefaultSkin
// ---------------------------------------------------------------------------
// Get the default (built-in) skin image.

void EmWindowWin::HostGetDefaultSkin (EmPixMap& pixMap, int scale)
{
	UINT	bitmapID	= scale == 1 ? IDB_DEFAULT_SMALL : IDB_DEFAULT_LARGE;

	HRSRC	hResInfo	= ::FindResource (gInstance, MAKEINTRESOURCE (bitmapID), RT_BITMAP);
	HGLOBAL	hBitmap		= ::LoadResource (gInstance, hResInfo);
	LPVOID	pBitmap		= ::LockResource (hBitmap);

	::CreatePixMapFromHost ((BITMAPINFO*) pBitmap, pixMap);
}


// ---------------------------------------------------------------------------
//		¥ EmWindowWin::HostGetCurrentMouse
// ---------------------------------------------------------------------------
// Get the current mouse location.

EmPoint EmWindowWin::HostGetCurrentMouse (void)
{
	return fLastPen;
}
