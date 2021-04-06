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

#include "Application.h"		// DisplayWndProc
#include "EmDlgWin.h"			// CenterWindow
#include "EmWindow.h"				// EmWindow
#include "EmPixMapWin.h"		// ConvertPixMapToHost
#include "EmRegion.h"			// Bounds
#include "EmRegionWin.h"		// ConvertRegionToHost
#include "EmQuantizer.h"		// EmQuantizer
#include "EmScreen.h"			// EmScreenUpdateInfo
#include "EmSession.h"			// EmSessionStopper
#include "Emulator.h"			// gInstance
#include "Platform.h"			// Platform::GetString
#include "resource.h"			// IDI_EMULATOR
#include "Strings.r.h"			// kStr_AppName

#ifdef SONY_ROM
#include "EmCPU68K.h"			// GetRegisters
#include "SonyWin/Platform_ExpMgrLib.h"
#include "SonyWin/SonyButtonProc.h"
#include "SonyShared/SonyChars.h"
extern	ScaleType		gCurrentScale;
#endif

const char* 	kClassName	= "PalmOSEmulatorDisplay";

#define WIDTH(r)	((r).right - (r).left)
#define HEIGHT(r)	((r).bottom - (r).top)


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostConstruct
// ---------------------------------------------------------------------------
// Create the host-specific data structure.

void EmWindow::HostConstruct (void)
{
	EmAssert (!fHostData);

	fHostData = new EmWindowHostData (this);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostDestruct
// ---------------------------------------------------------------------------
// Dispose of the host-specific data structure.

void EmWindow::HostDestruct (void)
{
	EmAssert (fHostData != NULL);

	delete fHostData;
	fHostData = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowCreate
// ---------------------------------------------------------------------------
// Create the host-specific window object.

void EmWindow::HostWindowCreate (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow == NULL);

	// Create the window class for the emulator window.

	static bool	registered;
	if (!registered)
	{
		WNDCLASS	wc;
		::ZeroMemory (&wc, sizeof (wc));

		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= &::DisplayWndProc;
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

	fHostData->fHostWindow = ::CreateWindow (	kClassName, app.c_str (),
									WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUP,
									CW_USEDEFAULT, CW_USEDEFAULT,
									CW_USEDEFAULT, CW_USEDEFAULT,
									NULL, NULL,			// parent, menu
									gInstance, NULL);	// lpParam

	::SetWindowLong (fHostData->fHostWindow, 0, (LONG) this);

	::DragAcceptFiles (fHostData->fHostWindow, TRUE);

	// Set a timer so that we periodically refresh the window contents.

	::SetTimer (fHostData->fHostWindow, 1, 100, NULL);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowDispose
// ---------------------------------------------------------------------------
// Dispose of the host-specific window object.

void EmWindow::HostWindowDispose (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	::DragAcceptFiles (fHostData->fHostWindow, FALSE);

	// Windows has already destroyed all child windows at this point and the 
	// GCW's WM_DESTROY handler sets the global handle to the window to NULL,
	// so we're already set. This call wouldn't work anyway since it posts a
	// message that won't be answered because the GCW is already gone.

	// Platform::GCW_Close ();

	::KillTimer (fHostData->fHostWindow, 1);

	::DestroyWindow (fHostData->fHostWindow);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowReset
// ---------------------------------------------------------------------------
// Update the window's appearance due to a skin change.

void EmWindow::HostWindowReset (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	// Delete the old bitmap and palette.

	fHostData->CacheFlush ();

	// Change the window to accomodate the settings and bitmap.

	// Get the current window size.  We get the "window" rect (the outer
	// window bounds) and the "client" rect (the inner window bounds), so
	// that we can subtract the two and determine the frame size.

	RECT wr, cr;
	::GetWindowRect (fHostData->fHostWindow, &wr);
	::GetClientRect (fHostData->fHostWindow, &cr);

	// Get the desired client size.

	EmRect	newBounds = fSkinRegion.Bounds ();

	// Resize the window.

	::MoveWindow (fHostData->fHostWindow, wr.left, wr.top,
			newBounds.Width () + WIDTH (wr) - WIDTH (cr),
			newBounds.Height () + HEIGHT (wr) - HEIGHT (cr), TRUE);

	// Set the framing region for the window.

	HRGN	rgn = ::ConvertRegionToHost (fSkinRegion);
	::SetWindowRgn (fHostData->fHostWindow, rgn, TRUE);

	// Invalidate the window contents now (necessary?).

	::InvalidateRect (fHostData->fHostWindow, NULL, FALSE);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostMouseCapture
// ---------------------------------------------------------------------------
// Capture the mouse so that all mouse events get sent to this window.

void EmWindow::HostMouseCapture (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	::SetCapture (fHostData->fHostWindow);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostMouseRelease
// ---------------------------------------------------------------------------
// Release the mouse so that mouse events get sent to the window the
// cursor is over.

void EmWindow::HostMouseRelease (void)
{
	::ReleaseCapture ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostUpdateBegin
// ---------------------------------------------------------------------------
// Prepare the host window object for updating.

void EmWindow::HostUpdateBegin (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);
	EmAssert (fHostData->fDestDC == NULL);

	fHostData->fDestDC = ::BeginPaint (fHostData->fHostWindow, &fHostData->fPaintStruct);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostUpdateEnd
// ---------------------------------------------------------------------------
// Finalize the host window object after it's been updated.

void EmWindow::HostUpdateEnd (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	::EndPaint (fHostData->fHostWindow, &fHostData->fPaintStruct);
	fHostData->fDestDC = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostDrawingBegin
// ---------------------------------------------------------------------------
// Prepare the host window object for drawing outside of an update event.

void EmWindow::HostDrawingBegin (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);
	EmAssert (fHostData->fDestDC == NULL);

	fHostData->fDestDC = ::GetDC (fHostData->fHostWindow);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostDrawingEnd
// ---------------------------------------------------------------------------
// Finalize the host window object after drawing outside of an update event.

void EmWindow::HostDrawingEnd (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	::ReleaseDC (fHostData->fHostWindow, fHostData->fDestDC);
	fHostData->fDestDC = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostPaletteSet
// ---------------------------------------------------------------------------
// Establish the palette to be used for drawing.

void EmWindow::HostPaletteSet (const EmScreenUpdateInfo& info)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	// Get the palette we'll be using.

	EmAssert (fHostData->fDestPalette == NULL);
	EmAssert (fHostData->fOldDestPalette == NULL);

	fHostData->fDestPalette = fHostData->GetSystemPalette (info);

	if (fHostData->fDestPalette)
	{
		fHostData->fOldDestPalette = ::SelectPalette (fHostData->fDestDC, fHostData->fDestPalette, FALSE);
		::RealizePalette (fHostData->fDestDC);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostPaletteRestore
// ---------------------------------------------------------------------------
// Clean up after HostPaletteSet.

void EmWindow::HostPaletteRestore (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	if (fHostData->fOldDestPalette)
	{
		::SelectPalette (fHostData->fDestDC, fHostData->fOldDestPalette, FALSE);
//		::RealizePalette (fHostData->fDestDC);

		fHostData->fDestPalette		= NULL;
		fHostData->fOldDestPalette	= NULL;
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowMoveBy
// ---------------------------------------------------------------------------
// Move the host window object by the given offset.

void EmWindow::HostWindowMoveBy (const EmPoint& offset)
{
	this->HostWindowMoveTo (this->HostWindowBoundsGet ().TopLeft () + offset);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowMoveTo
// ---------------------------------------------------------------------------
// Move the host window object to the given location.

void EmWindow::HostWindowMoveTo (const EmPoint& loc)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	::SetWindowPos (fHostData->fHostWindow, NULL, loc.fX, loc.fY,
		0, 0, SWP_NOZORDER | SWP_NOSIZE);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowBoundsGet
// ---------------------------------------------------------------------------
// Get the global bounds of the host window object.

EmRect EmWindow::HostWindowBoundsGet (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	RECT	r;
	::GetWindowRect (fHostData->fHostWindow, &r);

	return EmRect (r);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowCenter
// ---------------------------------------------------------------------------
// Center the window to the main display.

void EmWindow::HostWindowCenter (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	::CenterWindow (fHostData->fHostWindow);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowShow
// ---------------------------------------------------------------------------
// Make the host window object visible.

void EmWindow::HostWindowShow (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

	::ShowWindow (fHostData->fHostWindow, SW_SHOWNORMAL);
	::SetForegroundWindow (fHostData->fHostWindow);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostWindowDrag
// ---------------------------------------------------------------------------
// The user has clicked in a region of the host window object that causes
// the window to be dragged.  Drag the window around.

void EmWindow::HostWindowDrag (void)
{
	// Not needed on Windows.  The detection and handling of the window
	// drag is taken care of before the mouse event is passed off to the
	// EmWindow object.
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostRectFrame
// ---------------------------------------------------------------------------
// Draw a rectangle frame with the given width in the given color.

void EmWindow::HostRectFrame (const EmRect& r, const EmPoint& pen, const RGBType& color)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	HBRUSH		theBrush	= (HBRUSH) ::GetStockObject (HOLLOW_BRUSH);
	HBRUSH		oldBrush	= SelectBrush (fHostData->fDestDC, theBrush);

	COLORREF	rgb			= RGB (color.fRed, color.fGreen, color.fBlue);
	HPEN		thePen		= ::CreatePen (PS_INSIDEFRAME, pen.fX, rgb);
	HPEN		oldPen		= SelectPen (fHostData->fDestDC, thePen);

	::Rectangle(fHostData->fDestDC, r.fLeft, r.fTop, r.fRight, r.fBottom);

	SelectPen (fHostData->fDestDC, oldPen);
	SelectBrush (fHostData->fDestDC, oldBrush);

	::DeleteObject (thePen);
	::DeleteObject (theBrush);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostOvalPaint
// ---------------------------------------------------------------------------
// Fill an oval with the given color.

void EmWindow::HostOvalPaint (const EmRect& r, const RGBType& color)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	COLORREF	rgb			= RGB (color.fRed, color.fGreen, color.fBlue);
	HBRUSH		brush		= ::CreateSolidBrush (rgb);
	HBRUSH		oldBrush	= (HBRUSH) ::SelectObject (fHostData->fDestDC, brush);

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGT400 
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGT600) 
	{
		RECT rc;
		::SetRect(&rc, r.fLeft, r.fTop, r.fRight, r.fBottom);
		::FillRect(fHostData->fDestDC, &rc, brush);
	}
	else 
		::Ellipse (fHostData->fDestDC, r.fLeft, r.fTop, r.fRight, r.fBottom);
#else
	::Ellipse (fHostData->fDestDC, r.fLeft, r.fTop, r.fRight, r.fBottom);
#endif
	::SelectObject (fHostData->fDestDC, oldBrush);
	::DeleteObject (brush);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostPaintCase
// ---------------------------------------------------------------------------
// Draw the skin.

void EmWindow::HostPaintCase (const EmScreenUpdateInfo& info)
{
	EmAssert (fHostData != NULL);

	HDC			srcDC;
	HBITMAP		srcBitmap;
	HPALETTE	srcPalette;

	EmRect		from (EmPoint (0, 0), fSkin.GetSize ());
	EmPoint		to (0, 0);

	fHostData->GetSkin (srcDC, srcBitmap, srcPalette);
	fHostData->DrawDDB (srcDC, srcBitmap, srcPalette, from, to);

#ifdef SONY_ROM
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGS500C 
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS300
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGN700C
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS320
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS360
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGT400
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGT600
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGN600C)
	{	
		for (int i=0; i<BUTTON_NUM; i++)
			if (gJogButtonState[i].nButton != kElement_PowerButton)
				LCD_DrawButtonForPEG(fHostData->fDestDC, gJogButtonState[i].nButton);
	}

	// redraw PowerButton
	if (gSession->GetDevice().GetDeviceType() == kDevicePEGS500C 
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS300
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS320
	 || gSession->GetDevice().GetDeviceType() == kDevicePEGS360)
	{
		LCD_SetStateJogButton(kElement_PowerButton, info.fLCDOn, true);
		LCD_DrawButtonForPEG(fHostData->fDestDC, kElement_PowerButton);
	}
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostPaintLCD
// ---------------------------------------------------------------------------
// Draw the LCD area.

void EmWindow::HostPaintLCD (const EmScreenUpdateInfo& info, const EmRect& srcRect,
						  const EmRect& destRect, Bool scaled)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fDestDC != NULL);

	HDC 		srcDC		= NULL;
	HBITMAP 	srcBitmap	= NULL;
	HPALETTE	srcPalette	= NULL;

	::ConvertPixMapToHost (	info.fImage, fHostData->fDestDC,
							srcDC, srcBitmap, srcPalette,
							info.fFirstLine, info.fLastLine,
							scaled);

#ifdef SONY_ROM
	fHostData->DrawStretchDDB (srcDC, srcBitmap, srcPalette, srcRect, destRect);
#else
	fHostData->DrawDDB (srcDC, srcBitmap, srcPalette, srcRect, destRect.TopLeft ());
#endif

	::DeleteObject (srcPalette);
	::DeleteObject (srcBitmap);
	::DeleteDC (srcDC);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostPalette
// ---------------------------------------------------------------------------
// Tell the system about the palette we want to use.  Called when Windows
// is updating its system palette.

void EmWindow::HostPalette (void)
{
	EmAssert (fHostData != NULL);
	EmAssert (fHostData->fHostWindow != NULL);

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

	HPALETTE	pal = fHostData->GetSystemPalette (info);

	if (pal)
	{
		HDC 		dc = ::GetDC (fHostData->fHostWindow);
		HPALETTE	op = ::SelectPalette (dc, pal, FALSE);
		UINT		n = ::RealizePalette (dc);

		if (n != GDI_ERROR)
		{
			if (n > 0)
			{
				::InvalidateRect (fHostData->fHostWindow, NULL, FALSE);
			}
		}

		::SelectPalette (dc, op, TRUE);
		::RealizePalette (dc);

		::ReleaseDC (fHostData->fHostWindow, dc);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostDisplayChange
// ---------------------------------------------------------------------------
// Respond to the display's bit depth changing.  All we do here is flush our
// caches of the skin information.  It will get regenerated when it's needed.

void EmWindow::HostDisplayChange (void)
{
	fHostData->CacheFlush ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HostGetDefaultSkin
// ---------------------------------------------------------------------------
// Get the default (built-in) skin image.

void EmWindow::HostGetDefaultSkin (EmPixMap& pixMap, int scale)
{
	UINT	bitmapID	= scale == 1 ? IDB_DEFAULT_SMALL : IDB_DEFAULT_LARGE;

	HRSRC	hResInfo	= ::FindResource (gInstance, MAKEINTRESOURCE (bitmapID), RT_BITMAP);
	HGLOBAL	hBitmap		= ::LoadResource (gInstance, hResInfo);
	LPVOID	pBitmap		= ::LockResource (hBitmap);

	::CreatePixMapFromHost ((BITMAPINFO*) pBitmap, pixMap);
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::EmWindowHostData
// ---------------------------------------------------------------------------

EmWindowHostData::EmWindowHostData (EmWindow* lcd) :
	fWindow (lcd),
	fHostWindow (NULL),
	fPaintStruct (),
	fDestDC (NULL),
	fDestPalette (NULL),
	fOldDestPalette (NULL),
	fCachedSystemPalette (NULL),
	fCachedSkinDC (NULL),
	fCachedSkinBitmap (NULL),
	fCachedSkinPalette (NULL)
{
}


// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::~EmWindowHostData
// ---------------------------------------------------------------------------

EmWindowHostData::~EmWindowHostData (void)
{
	this->CacheFlush ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::~EmWindowHostData
// ---------------------------------------------------------------------------

HWND EmWindowHostData::GetHostWindow (void)
{
	return fHostWindow;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::CacheFlush
// ---------------------------------------------------------------------------

void EmWindowHostData::CacheFlush (void)
{
	// Delete the objects.

	::DeleteObject (fCachedSystemPalette);
	::DeleteObject (fCachedSkinBitmap);
	::DeleteObject (fCachedSkinPalette);
	::DeleteDC (fCachedSkinDC);

	// Drop our references.

	fCachedSystemPalette = NULL;
	fCachedSkinDC		= NULL;
	fCachedSkinBitmap	= NULL;
	fCachedSkinPalette	= NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::DrawDDB
// ---------------------------------------------------------------------------

void EmWindowHostData::DrawDDB (HDC srcDC,
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


void EmWindowHostData::DrawStretchDDB (HDC srcDC,
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


// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::GetSkin
// ---------------------------------------------------------------------------
// Get the DDB for the skin, creating the DDB and caching it if necessary.

void EmWindowHostData::GetSkin (HDC& dc, HBITMAP& bm, HPALETTE& pal)
{
	if (!fCachedSkinDC)
	{
		EmAssert (fDestDC != NULL);

		EmPoint	size = fWindow->fSkin.GetSize ();

		::ConvertPixMapToHost (	fWindow->fSkin, fDestDC,
								fCachedSkinDC, fCachedSkinBitmap, fCachedSkinPalette,
								0, size.fY, false);
	}

	dc = this->fCachedSkinDC;
	bm = this->fCachedSkinBitmap;
	pal = this->fCachedSkinPalette;
}


// ---------------------------------------------------------------------------
//		¥ EmWindowHostData::GetSystemPalette
// ---------------------------------------------------------------------------
// Return a palette to be used for all our drawing.  This palette includes
// entries for both the case graphics and the colors in the LCD area.

HPALETTE EmWindowHostData::GetSystemPalette (const EmScreenUpdateInfo& info)
{
	// If the dest is not a palette device, we don't need to do anything.

	if (!::IsPaletteDevice (fDestDC))
		return NULL;

	// Check to see if we need to flush our cached palette.

	if (fCachedSystemPalette)
	{
		const RGBList& newColorTable = info.fImage.GetColorTable ();

		if (fWindow->PrevLCDColorsChanged (newColorTable))
		{
			::DeleteObject (fCachedSystemPalette);
			fCachedSystemPalette = NULL;
		}

		fWindow->SaveLCDColors (newColorTable);
	}

	// If the screen device is palette-based, return a palette, creating
	// and caching it if necessary.

	if (!fCachedSystemPalette)
	{
		RGBList		colors;
		fWindow->GetSystemColors (info, colors);

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


// ---------------------------------------------------------------------------
//		¥ GetWindow
// ---------------------------------------------------------------------------
// Return the EmWindow object associated with the host window object.

EmWindow* GetWindow (HWND hWnd)
{
	EmAssert (hWnd);

	if (!hWnd)
		return NULL;

	EmWindow*	result = (EmWindow*) ::GetWindowLong (hWnd, 0);

	EmAssert (result);
	EmAssert (result->GetHostData ());
	EmAssert (result->GetHostData ()->GetHostWindow () == hWnd);

	return result;
}
