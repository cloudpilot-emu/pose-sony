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
#include "EmWindow.h"

#include "EmHAL.h"				// EmHAL::GetVibrateOn
#include "EmJPEG.h"				// JPEGToPixMap
#include "EmPixMap.h"			// EmPixMap
#include "EmQuantizer.h"		// EmQuantizer
#include "EmRegion.h"			// EmRegion
#include "EmScreen.h"			// EmScreenUpdateInfo
#include "EmSession.h"			// PostPenEvent, PostButtonEvent, etc.
#include "EmStream.h"			// delete imageStream
#include "Platform.h"			// Platform::PinToScreen

#ifdef SONY_ROM
#include "ROMStubs.h"
#include "EmWindowWin.h"
#include "SonyWin/Platform_ExpMgrLib.h"
#include "SonyWin/SonyButtonProc.h"
#include "SonyShared/SonyChars.h"
extern	ScaleType		gCurrentScale;
#endif

/*
	EmWindow is the cross-platform object representing the window that
	displays the emulation.  It is not the actual thing that appears
	on the monitor as a window, but it is repsonsible for that object's
	creation, as well as dealing with cross-platform aspects of handling
	it, such as updating its contents, changing skins, etc.

	EmWindow is part of a group of three objects that together completely
	manage the Emulator's window.  The other two are EmWindowHostData,
	and the platform-specific window object--be it an HWND, an LWindow
	sub-class, or an Fl_Window sub-class.

	The three objects maintain references to each other as follows:

		EmWindow -----> EmWindowHostData -----> host-window
		   ^ ^                |                     |
		   | +----------------+                     |
		   +----------------------------------------+

	EmWindow handles all the cross-platform issues.  It can deal with
	the upper-level parts of updating the LCD area, wiggling the window
	if the vibrator is going, change skins, etc.

	EmWindow manages most of the platform-specific aspects by defining
	HostFoo methods, which are then defined differently on each platform.
	These HostFoo methods are implemented in EmWindow<Host>

	EmWindowHostData is also defined in EmWindow<Host>, and holds any
	platform-specific data (such as the HWND on Windows, etc.).  It
	declares and implements any support functions needed on that particular
	platform, but which can't be declared in EmWindow because either not
	all platforms need those functions, or they involve platform-specific
	types.

	The host window object is whatever makes sense for that platform,
	and is what one would create if one were programming natively for
	just that platform.  On Windows, it's an HWND.  On the Mac, it's an
	LWindow sub-class (which in turn wraps up a Mac OS WindowPtr).  On
	Unix, it's an Fl_Window sub-class (which in turn wraps up an 
	X Windows Window.

	One creates a window by calling "window = new EmWindow;" followed by
	"window->WindowCreate();".  Cross-platform code can then call high-level
	functions in EmWindow such as HandleUpdate and HandlePen.  Platform-
	specific code can perform platform-specific operations, getting the
	host window object by calling "window->GetHostData()->GetHostWindow();".
	If the platform specific code starts off with the host window object,
	it can get the cross-platform EmWindow* by calling the GetWindow (host-
	object) function in EmWindow<Host>.  When it's time to close the window,
	call "window->WindowDispose;" followed by "delete window;"
*/


EmWindow*	EmWindow::fgWindow;

// ---------------------------------------------------------------------------
//		¥ EmWindow::EmWindow
// ---------------------------------------------------------------------------
// Constructor.  Perform simple data member initialization.  The heavy stuff
// occurs in WindowCreate.

EmWindow::EmWindow (void) :
	fHostData (NULL),
	fSkin (),
	fSkinColors1 (),
	fSkinColors2 (),
	fSkinRegion (),
	fPrevLCDColors (),
	fCurrentButton (kElement_None),
	fNeedWindowReset (false),
	fNeedWindowInvalidate (false),
	fOldLCDOn (false),
	fOldLEDState (0),
	fWiggled (false)
{
	this->HostConstruct ();

	EmAssert (fgWindow == NULL);
	fgWindow = this;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::~EmWindow
// ---------------------------------------------------------------------------
// Destructor.  Simple resource clean-up.  The heavy stuff occurs in
// WindowDispose.

EmWindow::~EmWindow (void)
{
	this->HostDestruct ();

	EmAssert (fgWindow == this);
	fgWindow = NULL;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetWindow
// ---------------------------------------------------------------------------

EmWindow* EmWindow::GetWindow (void)
{
	return fgWindow;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::WindowCreate
// ---------------------------------------------------------------------------
// Create the LCD window, along with any host data and host window objects.
// Set the window's skin, register for any notification regarding the window's
// appearance, reposition the window to where it should live, ensure that
// the window is still on the screen, and then finally make the window
// visible.
//
// This function should be called immediately after the EmWindow object is
// created.

void EmWindow::WindowCreate (void)
{
	// Create the Host version of the window.

	this->HostWindowCreate ();

	// Establish the window's skin.

	this->WindowReset ();

	// Install notification callbacks for when the skin changes.

	gPrefs->AddNotification (&EmWindow::PrefsChangedCB, kPrefKeySkins, this);
	gPrefs->AddNotification (&EmWindow::PrefsChangedCB, kPrefKeyScale, this);
	gPrefs->AddNotification (&EmWindow::PrefsChangedCB, kPrefKeyBackgroundColor, this);

	// Restore the window location.

	Preference<EmPoint>	pref (kPrefKeyWindowLocation);

	if (pref.Loaded ())
	{
		this->HostWindowMoveTo (*pref);
	}

	// No previously saved position, so center it.

	else
	{
		this->HostWindowCenter ();
	}

	// Ensure the window is on the screen.

	EmRect	rect (this->HostWindowBoundsGet ());

	if (Platform::PinToScreen (rect))
	{
		this->HostWindowMoveTo (rect.TopLeft ());
	}

	// Show the window.

	this->HostWindowShow ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::WindowDispose
// ---------------------------------------------------------------------------
// Dispose of the window and all its data.  Unregister for notification, save
// the window location, and destroy the host window object.
//
// This function should be called just before the EmWindow object is deleted.

void EmWindow::WindowDispose (void)
{
	gPrefs->RemoveNotification (EmWindow::PrefsChangedCB);

	// Save the window location.

	{
		EmRect	rect (this->HostWindowBoundsGet ());

		if (!rect.IsEmpty ())
		{
			Preference <EmPoint>	pref (kPrefKeyWindowLocation);
			pref = rect.TopLeft ();
		}
	}

	this->HostWindowDispose ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::WindowReset
// ---------------------------------------------------------------------------

void EmWindow::WindowReset (void)
{
	// Can only set the skin if we have a session running...

	if (!gSession)
		return;

	// Configure our Skin engine.

	::SkinSetSkin ();

	// Get the specified skin, or the default skin if the specified
	// one cannot be found.

	if (!this->GetSkin (fSkin))
	{
		this->GetDefaultSkin (fSkin);
	}

	// Get and cache the colors for the skin.  We'll quantize at two
	// levels.  One will be used whenever we can get away with it and
	// will specify as many case colors as possible for an 8-bit host
	// display.  The other will be more polite, using few colors in order
	// to allow more for the LCD area if it's in 8- or 16-bit mode.

	{
		EmQuantizer q (220, 6);	// Leaves 256 - 220 - 20 = 16 for LCD
		q.ProcessImage (fSkin);
		q.GetColorTable (fSkinColors1);
	}

	{
		EmQuantizer q (64, 6);	// Leaves 256 - 64 - 20 = 172 for LCD
		q.ProcessImage (fSkin);
		q.GetColorTable (fSkinColors2);
	}

	// Clear out the previously saved LCD colors.  This will force the
	// any host palettes to be regenerated, because they'll think the
	// Palm OS LCD palette has changed.

	fPrevLCDColors.clear ();

	// Create a one-bpp mask of the skin.

	EmPixMap	mask;
	fSkin.CreateMask (mask);

	// Convert it to a region.

	fSkinRegion = mask.CreateRegion ();

	// Now convert this all to platform-specific data structures.
	// This will also resize the window.

	this->HostWindowReset ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HandlePenEvent
// ---------------------------------------------------------------------------

void EmWindow::HandlePenEvent (const EmPoint& where, Bool down)
{
	// Find out what part of the case we clicked on.

	SkinElementType	what;

#ifdef SONY_ROM
	if (ControlButtonForCLIE(where, down))
		return ;
#endif
	
	// If the mouse button is down and we aren't currently tracking
	// anything, then find out what the user clicked on.

	if (down && (fCurrentButton == kElement_None))
	{
		what = ::SkinTestPoint (where);

		if ((what != kElement_Frame) && (what != kElement_None))
		{
			this->HostMouseCapture ();
		}
	}

	// If the pen is up, or if we were already in the progress of tracking
	// something, then stick with it.

	else
	{
		what = fCurrentButton;
	}

	// If we clicked in the touchscreen area (which includes the LCD area
	// and the silkscreen area), start the mechanism for tracking the mouse.

	if (what == kElement_Touchscreen)
	{
#ifdef SONY_ROM
		if (!EmHAL::GetLCDScreenOn ())
			return;	// Power Off ó‘Ô
#endif

		EmPoint		whereLCD = ::SkinWindowToTouchscreen (where);
		EmPenEvent	event (whereLCD, down);

		EmAssert (gSession);
		gSession->PostPenEvent (event);
	}

	// If we clicked in the frame, start dragging the window.

	else if (what == kElement_Frame)
	{
		if (down)
		{
			this->HostWindowDrag ();

			// We normally record the mouse button going up in
			// EventMouseUp.  However, that method is not called
			// after ClickInDrag, so say the mouse button is up here.

			down = false;
		}
	}

	// Otherwise, if we clicked on a button, start tracking it.

	else if (what != kElement_None)
	{
		EmButtonEvent	event (what, down);

		EmAssert (gSession);
		gSession->PostButtonEvent (event);
	}

	// Set or clear what we're tracking.

	if (down)
		fCurrentButton = what;
	else
		fCurrentButton = kElement_None;

	if (fCurrentButton == kElement_None)
	{
		this->HostMouseRelease ();
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HandleUpdate
// ---------------------------------------------------------------------------
// Called to handle update events.

void EmWindow::HandleUpdate (void)
{
	this->HostUpdateBegin ();

	this->PaintScreen (true, true);

	this->HostUpdateEnd ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HandlePalette
// ---------------------------------------------------------------------------
// Called when we need to tell the OS what our preferred palette is.  Usually
// called when window orders change.

void EmWindow::HandlePalette (void)
{
	this->HostPalette ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HandleDisplayChange
// ---------------------------------------------------------------------------
// Called when the display depth changes.

void EmWindow::HandleDisplayChange (void)
{
	this->HostDisplayChange ();
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::HandleIdle
// ---------------------------------------------------------------------------
// Called several times a second to perform idle time events such as
// refreshing the LCD area and vibrating the window.

void EmWindow::HandleIdle (const EmPoint& where)
{
	// Track the pen if it's down.

	if (fCurrentButton == kElement_Touchscreen)
	{
		this->HandlePenEvent (where, true);
	}

	// Refresh the LCD area.

	this->HostDrawingBegin ();

	if (fNeedWindowReset)
	{
		this->WindowReset ();
	}
	else if (fNeedWindowInvalidate)
	{
		this->PaintScreen (false, true);
	}
	else
	{
		this->PaintScreen (false, false);
	}

	fNeedWindowReset		= false;
	fNeedWindowInvalidate	= false;

	this->HostDrawingEnd ();


	// Do the Wiggle Walk.

	{
		const int	kWiggleOffset = 2;

		EmSessionStopper	stopper (gSession, kStopNow);

		if (stopper.Stopped ())
		{
			if (EmHAL::GetVibrateOn ())
			{
				if (!fWiggled)
				{
					fWiggled = true;
					this->HostWindowMoveBy (EmPoint (kWiggleOffset, 0));
				}
				else
				{
					fWiggled = false;
					this->HostWindowMoveBy (EmPoint (-kWiggleOffset, 0));
				}
			}

			// If the vibrator just went off, then put the window
			// back to where it was.

			else if (fWiggled)
			{
				fWiggled = false;
				this->HostWindowMoveBy (EmPoint (-kWiggleOffset, 0));
			}
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetLCDContents
// ---------------------------------------------------------------------------
// Return the full contents of the LCD area.  Used for screenshots.

void EmWindow::GetLCDContents (EmScreenUpdateInfo& info)
{
	EmSessionStopper	stopper (gSession, kStopNow);

	if (stopper.Stopped ())
	{
		EmScreen::InvalidateAll ();
		EmScreen::GetBits (info);
		EmScreen::InvalidateAll ();
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PaintScreen
// ---------------------------------------------------------------------------
// Paint the entire window.  Could be called in response to an update event
// or as part of our regular idle time handling.

void EmWindow::PaintScreen (Bool drawCase, Bool wholeLCD)
{
	EmScreenUpdateInfo	info;
	Bool				bufferDirty;
	Bool				drawFrame = false;
	Bool				drawLED = false;
	Bool				drawLCD = false;
	Bool				lcdOn;
	uint16				ledState;

	{
		// Pause the CPU so that we can get the current hardware state.

		EmSessionStopper	stopper (gSession, kStopNow);

		if (!stopper.Stopped ())
			return;

		// Determine if we have to force the redrawing of the background.
		// We have to do that if the LCD or LEDs have turned off.

		lcdOn		= EmHAL::GetLCDScreenOn ();
		ledState	= EmHAL::GetLEDState ();

		if (!lcdOn && fOldLCDOn)
		{
			drawCase = true;
		}

		if (!ledState && fOldLEDState)
		{
			drawCase = true;
		}

		// Determine if we have to draw the LCD or LED.  We'd have to do that
		// if their state has changed or if we had to draw the background.

		if (drawCase || (lcdOn != fOldLCDOn))
		{
			drawLCD		= lcdOn;
		}

		if (drawCase || (lcdOn != fOldLCDOn))
		{
			drawFrame	= lcdOn && EmHAL::GetLCDHasFrame ();
		}

		if (drawCase || (ledState != fOldLEDState))
		{
			drawLED		= ledState != 0;
		}

		fOldLCDOn		= lcdOn;
		fOldLEDState	= ledState;

		// If we're going to be drawing the whole LCD, then invalidate
		// the entire screen.

		if (wholeLCD || drawLCD)
		{
			EmScreen::InvalidateAll ();
		}

		// Get the current LCD state.

		bufferDirty = EmScreen::GetBits (info);

		// If the buffer was dirty, that's another reason to draw the LCD.

		if (bufferDirty)
		{
			drawLCD		= lcdOn;
		}

		// If there is no width or height to the buffer, then that's a
		// great reason to NOT draw the LCD.

		EmPoint	size = info.fImage.GetSize ();
		if (size.fX == 0 || size.fY == 0)
		{
			drawLCD		= false;
		}
	}

	// Set up the graphics palette.

	Bool	drawAnything = drawCase || drawFrame || drawLED || drawLCD;

	if (drawAnything)
	{
		this->HostPaletteSet (info);
	}


	// Draw the case.

	if (drawCase)
	{
		this->PaintCase (info);
	}

	// Draw any LCD frame.

	if (drawFrame)
	{
		this->PaintLCDFrame (info);
	}

	// Draw any LEDs.

	if (drawLED)
	{
		this->PaintLED (ledState);
	}

	// Draw the LCD area.

	if (drawLCD)
	{
		this->PaintLCD (info);
	}

#if 0
	// Draw frames around the buttons.	This is handy when determining their
	// locations initially.

	if (drawCase)
	{
		this->PaintButtonFrames ();
	}
#endif

	// Restore the palette that was changed in HostSetPalette.

	if (drawAnything)
	{
		this->HostPaletteRestore ();
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PaintCase
// ---------------------------------------------------------------------------
// Draw the skin.

void EmWindow::PaintCase (const EmScreenUpdateInfo& info)
{
	this->HostPaintCase (info);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PaintLCDFrame
// ---------------------------------------------------------------------------
// Draw any frame around the LCD area required in order to faithfully
// emulate the appearance of the real LCD.

void EmWindow::PaintLCDFrame (const EmScreenUpdateInfo&)
{
	RGBType	backlightColor	(0xff, 0xff, 0xff);
	EmRect	lcdRect			(this->GetLCDBounds ());

#ifdef SONY_ROM
	EmPoint	penSize			(1, 1);
#else
	EmPoint	penSize			(2, 2);
#endif

	penSize = ::SkinScaleUp (penSize);

	lcdRect.Inset (-penSize.fX, -penSize.fY);

	this->HostRectFrame (lcdRect, penSize, backlightColor);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PaintLCD
// ---------------------------------------------------------------------------
// Draw the LCD portion of the window.

void EmWindow::PaintLCD (const EmScreenUpdateInfo& info)
{
	// Get the bounds of the LCD area.

	EmRect	lcdRect = this->GetLCDBounds ();

	// Limit the vertical range to the lines that have changed.

#ifdef SONY_ROM
	EmRect	destRect;
	EmPoint	screenSize = info.fImage.GetSize();
	bool	HiRez = false;
	HiRez = (( gSession->GetDevice().GetDeviceType() == kDevicePEGT400 
			|| gSession->GetDevice().GetDeviceType() == kDevicePEGT600 
			|| gSession->GetDevice().GetDeviceType() == kDevicePEGN700C 
			|| gSession->GetDevice().GetDeviceType() == kDevicePEGN600C)
			&& screenSize.fX == 320);
	
	if (HiRez)
	{
		destRect = lcdRect;

		if (gCurrentScale == 2)
		{
			destRect.fBottom	= destRect.fTop + info.fLastLine ;
			destRect.fTop		= destRect.fTop + info.fFirstLine;
		}
		else
		{
			destRect.fBottom	= destRect.fTop + (info.fLastLine / (screenSize.fX / 160)) ;
			destRect.fTop		= destRect.fTop + (info.fFirstLine / (screenSize.fX / 160)) ;
		}
	}
	else
	{	
		destRect = ::SkinScaleDown (lcdRect);
		destRect.fBottom	= destRect.fTop + info.fLastLine;
		destRect.fTop		= destRect.fTop + info.fFirstLine;
		destRect = ::SkinScaleUp (destRect);
	}
#else
	EmRect	destRect = ::SkinScaleDown (lcdRect);

	destRect.fBottom	= destRect.fTop + info.fLastLine;
	destRect.fTop		= destRect.fTop + info.fFirstLine;

	destRect = ::SkinScaleUp (destRect);
#endif

	// Get the bounds of the area we'll be blitting from.

	EmRect	srcRect (destRect);
	srcRect -= lcdRect.TopLeft ();

#ifdef SONY_ROM
	if (HiRez)
	{
		srcRect.fBottom	*= (screenSize.fX / 160) ;
		srcRect.fTop	*= (screenSize.fX / 160) ;
		srcRect.fRight	*= (screenSize.fX / 160) ;
		srcRect.fLeft	*= (screenSize.fX / 160) ;
	}
#endif

	// Determine if the image needs to be scaled.

	EmPoint	before (1, 1);
	EmPoint	after = ::SkinScaleUp (before);

	// Setup is done. Let the host-specific routines handle the rest.

	this->HostPaintLCD (info, srcRect, destRect, before.fX != after.fX);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PaintLED
// ---------------------------------------------------------------------------

void EmWindow::PaintLED (uint16 ledState)
{
	// Get the color for the LED.

	RGBType	ledColor;

	if ((ledState & (kLEDRed | kLEDGreen)) == (kLEDRed | kLEDGreen))
	{
#ifdef SONY_ROM
		ledColor = RGBType (237, 84, 17);	// Orange
#else
		ledColor = RGBType (255, 128, 0);	// Orange
#endif
	}
	else if ((ledState & kLEDRed) == kLEDRed)
	{
		ledColor = RGBType (255, 0, 0);		// Red
	}
	else /* green */
	{
		ledColor = RGBType (0, 255, 0);		// Green
	}

	EmRect	bounds (this->GetLEDBounds ());

	this->HostOvalPaint (bounds, ledColor);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PaintButtonFrames
// ---------------------------------------------------------------------------
// Draw frames around the buttons.	This is handy when determining their
// locations initially.

void EmWindow::PaintButtonFrames (void)
{
	EmPoint			penSize (1, 1);
	RGBType			color (0, 0, 0);

	int				index = 0;
	SkinElementType	type;
	EmRect			bounds;

	while (::SkinGetElementInfo (index, type, bounds))
	{
		this->HostRectFrame (bounds, penSize, color);

		++index;
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PrefsChangedCB
// ---------------------------------------------------------------------------
// Respond to a preference change.  This version of the function retrieves
// the EmWindow object pointer and calls a non-static member function of the
// object.

void EmWindow::PrefsChangedCB (PrefKeyType key, void* data)
{
	EmAssert (data);

	EmWindow* lcd = static_cast<EmWindow*>(data);
	lcd->PrefsChanged (key);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PrefsChanged
// ---------------------------------------------------------------------------
// Respond to a preference change.  If the sking or display size has changed,
// we need to rebuild our skin image.  If the background color preference
// has changed, then just flag that the LCD area needs to be redrawn.

void EmWindow::PrefsChanged (PrefKeyType key)
{
	if ((strcmp (key, kPrefKeyScale) == 0) || (strcmp (key, kPrefKeySkins) == 0))
	{
		fNeedWindowReset = true;
	}
	else if (strcmp (key, kPrefKeyBackgroundColor) == 0)
	{
		fNeedWindowInvalidate = true;
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetLCDBounds
// ---------------------------------------------------------------------------
// Get the bounds for the LCD.

EmRect EmWindow::GetLCDBounds (void)
{
	int				ii = 0;
	SkinElementType	type = kElement_None;
	EmRect			bounds (10, 10, 170, 170);

	while (::SkinGetElementInfo (ii, type, bounds))
	{
		if (type == kElement_LCD)
		{
			break;
		}

		++ii;
	}

	return bounds;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetLEDBounds
// ---------------------------------------------------------------------------
// Get the bounds for the LED.  Put it in the power button.

EmRect EmWindow::GetLEDBounds (void)
{
	int				ii = 0;
	SkinElementType	type = kElement_None;
	EmRect			bounds (10, 10, 20, 20);

#if SONY_ROM
	if (SkinGetElementEmRect(kElement_AlermLED, &bounds)
	 || SkinGetElementEmRect(kElement_PowerButton, &bounds))  
	{ 
	} 
#else
	while (::SkinGetElementInfo (ii++, type, bounds))
	{
		if (type == kElement_PowerButton)
		{
			// Create a square centered in the power button.

			if (bounds.Width () > bounds.Height ())
			{
				bounds.fLeft	= bounds.fLeft + (bounds.Width () - bounds.Height ()) / 2;
				bounds.fRight	= bounds.fLeft + bounds.Height ();
			}
			else
			{
				bounds.fTop		= bounds.fTop + (bounds.Height () - bounds.Width ()) / 2;
				bounds.fBottom	= bounds.fTop + bounds.Width ();
			}

			// Inset it a little -- looks better this way.

			bounds.Inset (2, 2);

			break;
		}
	}
#endif
	return bounds;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetSkin
// ---------------------------------------------------------------------------
// Get the currently selected skin as a PixMap.

Bool EmWindow::GetSkin (EmPixMap& pixMap)
{
	EmStream*	imageStream = ::SkinGetSkinStream ();

	if (!imageStream)
		return false;

	// Turn the JPEG image into BMP format.

	::JPEGToPixMap (*imageStream, pixMap);

	// Free up the resource info.

	delete imageStream;
	imageStream = NULL;

	return true;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetDefaultSkin
// ---------------------------------------------------------------------------
// Get the default, built-in skin as a PixMap.

void EmWindow::GetDefaultSkin (EmPixMap& pixMap)
{
	Preference<ScaleType>	scalePref (kPrefKeyScale);

	EmAssert ((*scalePref == 1) || (*scalePref == 2));

	this->HostGetDefaultSkin (pixMap, *scalePref);
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::PrevLCDColorsChanged
// ---------------------------------------------------------------------------

Bool EmWindow::PrevLCDColorsChanged (const RGBList& newLCDColors)
{
	// If the incoming image does not have a color table, then it's a
	// direct-pixel image.  It can have any set of colors in it, so
	// we'll have to create an 8-bit palette for it no matter what.
	//
	// If the incoming image's color table has a different size than
	// the previously saved color table, then we'll need to regenerate
	// our cached palette.

	if (newLCDColors.size () == 0 ||
		newLCDColors.size () != fPrevLCDColors.size ())
	{
		return true;
	}

	// The incoming color table size is the same as the previously saved
	// color table size, and neither of them are zero.  Check the contents
	// of the two tables to see if they've changed.

	for (size_t ii = 0; ii < newLCDColors.size (); ++ii)
	{
		if (newLCDColors[ii] != fPrevLCDColors[ii])
		{
			return true;
		}
	}

	return false;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::SaveLCDColors
// ---------------------------------------------------------------------------

void EmWindow::SaveLCDColors (const RGBList& newLCDColors)
{
	fPrevLCDColors = newLCDColors;
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetSystemColors
// ---------------------------------------------------------------------------

void EmWindow::GetSystemColors (const EmScreenUpdateInfo& info, RGBList& colors)
{
	// If this is a 1, 2, or 4 bit LCD, then we can use the whole
	// palette from it along with our beefy skin color table.

	if (info.fImage.GetDepth () <= 4)
	{
		colors = info.fImage.GetColorTable ();
		colors.insert (colors.end (),
			fSkinColors1.begin (),
			fSkinColors1.end ());
	}

	// If this is an 8-bit image, then we have an interesting problem.  We
	// want to generate a palette that we can cache until the Palm OS
	// palette changes.  The palette we generate shouldn't be based on the
	// current image, as that can change and use different colors without
	// the Palm OS palette changing.  So if we were to take that route,
	// the palette we generated would be based on the handful of colors on
	// the screen and not what's in the palette.  It would be nice to use
	// the Palm OS palette itself, but there are 256 colors in there, and
	// we need to cut that down a little so that we can share time with
	// the skin.  Therefore, we run the Palm OS palette itself through the
	// color quantizer itself to generate a nice subset of the palette.
	// Since we have to merge 84 or so colors together, and since all
	// colors in the Palm OS palette are weighted evenly, we don't want
	// those 84 merged colors to appear at one end of the spectrum or the
	// other.  Therefore, we'll randomize the Palm OS palette before
	// offering its colors to the quantizer.

	else if (info.fImage.GetDepth () == 8)
	{
		srand (1);	// Randomize it consistantly...  :-)

		colors = info.fImage.GetColorTable ();

		for (size_t ii = 0; ii < colors.size (); ++ii)
		{
			size_t	jj = rand () % colors.size ();

			swap (colors[ii], colors[jj]);
		}

		EmQuantizer q (172, 6);
		q.ProcessColorTable (colors);
		q.GetColorTable (colors);

		colors.insert (colors.end (),
			fSkinColors2.begin (),
			fSkinColors2.end ());
	}

	// Otherwise, if this is a 16 or 24 bit LCD, then we have to
	// get the best 172 colors and combine that with our 64-entry
	// skin color table.

	else
	{
		EmQuantizer q (172, 6);
		q.ProcessImage (info.fImage);
		q.GetColorTable (colors);

		colors.insert (colors.end (),
			fSkinColors2.begin (),
			fSkinColors2.end ());
	}
}


// ---------------------------------------------------------------------------
//		¥ EmWindow::GetSkinRegion
// ---------------------------------------------------------------------------

EmRegion EmWindow::GetSkinRegion (void)
{
	return fSkinRegion;
}
