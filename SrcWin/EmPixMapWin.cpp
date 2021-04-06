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

#include "EmCommon.h"
#include "EmPixMapWin.h"


static HBITMAP	PrvCreateBitmap		(HDC destDC, const EmPixMap& src,
									 int firstLine, int lastLine, Bool scale);
static HPALETTE PrvCreatePalette	(HDC destDC, const EmPixMap& src);
static int		PrvGetNumColors		(const BITMAPINFO* dib);
static uint8*	PrvGetBits			(const BITMAPINFO* dib);
static long		PrvGetRowBytes		(const BITMAPINFO* dib);
static long		PrvGetRowBytes		(long width, long bpp);
static EmPixMapFormat PrvGetFormat	(const BITMAPINFO* dib);


/***********************************************************************
 *
 * FUNCTION:	ConvertPixMapToHost
 *
 * DESCRIPTION:	Convert an EmPixMap object to an HDC, HBITMAP, and
 *				HPALETTE that can be used for drawing.  The caller
 *				is responsible for deleting the object when it's done
 *				with them.
 *
 *				It's possible to convert only a subrange of the source
 *				pixmap.  The actual range is specified by firstline
 *				and lastline.  When returned, the HBITMAP will have
 *				the same height as the source pixmap, but only the
 *				specified scanline range will have valid contents.
 *
 * PARAMETERS:	src - pixmap to be converted.
 *
 *				destDC - "master" DC used to create the new objects.
 *
 *				lcdDC - receives the newly created HDC.
 *
 *				lcdBM - receives the newly created HBITMAP.
 *
 *				lcdPalette - receives the newly created HPALETTE.
 *
 *				firstLine - first line of the source pixmap to convert.
 *
 *				lastLine - last line of the source pixmap to convert.
 *
 *				scale - whether or not the source pixmap should be
 *					scaled by a factor of two.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

void ConvertPixMapToHost (	const EmPixMap& src, HDC destDC,
							HDC& lcdDC, HBITMAP& lcdBM, HPALETTE& lcdPalette,
							int firstLine, int lastLine, Bool scale)
{
	lcdDC		= ::CreateCompatibleDC (destDC);
	lcdBM		= ::PrvCreateBitmap (destDC, src, firstLine, lastLine, scale);
	lcdPalette	= ::PrvCreatePalette (destDC, src);
}


/***********************************************************************
 *
 * FUNCTION:	ConvertPixMapToHost
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

void CreatePixMapFromHost (const BITMAPINFO* src, EmPixMap& dest)
{
	// Wrap up the BITMAPINFO in an EmPixMap.

	EmPixMap	pixMap;

	pixMap.SetSize (EmPoint (src->bmiHeader.biWidth, src->bmiHeader.biHeight));
	pixMap.SetFormat (::PrvGetFormat (src));
	pixMap.SetRowBytes (::PrvGetRowBytes (src));

	int	numColors = ::PrvGetNumColors (src);
	if (numColors)
	{
		RGBList	colors;
		::ConvertToRGBList (colors, src->bmiColors, numColors);
		pixMap.SetColorTable (colors);
	}

	pixMap.SetBits (::PrvGetBits (src));

	// Copy the data to the destination.

	dest = pixMap;

	// No stinkin' bottom-up bitmaps!

	if (src->bmiHeader.biHeight > 0)
	{
		dest.FlipScanlines ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	ConvertRGBList
 *
 * DESCRIPTION:	Converts an RGBList into platform-specific formats.
 *
 * PARAMETERS:	outColors - RGBQUAD array or LOGPALETTE to receive the
 *					colors.  These routines assume that the destination
 *					is large enough to hold all the source colors from
 *					"inColors".
 *
 *				inColors - RGBList containing the colors to convert.
 *
 * RETURNED:	New colors are written into "outColors".
 *
 ***********************************************************************/

void ConvertFromRGBList (RGBQUAD* outColors, const RGBList& inColors)
{
	int	numColors = inColors.size ();

	for (int ii = 0; ii < numColors; ++ii)
	{
		outColors[ii].rgbRed		= inColors[ii].fRed;
		outColors[ii].rgbGreen		= inColors[ii].fGreen;
		outColors[ii].rgbBlue		= inColors[ii].fBlue; 
		outColors[ii].rgbReserved	= 0;
	}
}


void ConvertFromRGBList (LOGPALETTE* outColors, const RGBList& inColors)
{
	int	numColors = inColors.size ();

	outColors->palVersion		= 0x300;
	outColors->palNumEntries	= numColors;

	for (int ii = 0; ii < numColors; ++ii)
	{
		outColors->palPalEntry[ii].peRed	= inColors[ii].fRed;
		outColors->palPalEntry[ii].peGreen	= inColors[ii].fGreen;
		outColors->palPalEntry[ii].peBlue	= inColors[ii].fBlue;
		outColors->palPalEntry[ii].peFlags	= 0;
	}
}

void ConvertToRGBList (RGBList& outColors, const RGBQUAD* inColors, int numColors)
{
	outColors.clear ();

	for (int ii = 0; ii < numColors; ++ii)
	{
		RGBType	rgb (inColors[ii].rgbRed, inColors[ii].rgbGreen, inColors[ii].rgbBlue);
		outColors.push_back (rgb);
	}
}


void ConvertToRGBList (RGBList& outColors, const LOGPALETTE* inColors)
{
	outColors.clear ();

	for (int ii = 0; ii < inColors->palNumEntries; ++ii)
	{
		RGBType	rgb (inColors->palPalEntry[ii].peRed,
					inColors->palPalEntry[ii].peRed,
					inColors->palPalEntry[ii].peRed);
		outColors.push_back (rgb);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmHostLCD::IsPaletteDevice
// ---------------------------------------------------------------------------
// Returns whether or not the given device is palette-based or not

BOOL IsPaletteDevice (HDC hDC)
{
	int 	rasterCaps;

	if (hDC)
	{
		rasterCaps = ::GetDeviceCaps (hDC, RASTERCAPS);
	}
	else
	{
		hDC = ::GetDC (NULL);
		rasterCaps = ::GetDeviceCaps (hDC, RASTERCAPS);
		::ReleaseDC (NULL, hDC);
	}

	BOOL	isPaletteDevice = RC_PALETTE == (rasterCaps & RC_PALETTE);

	return isPaletteDevice;
}


/***********************************************************************
 *
 * FUNCTION:	PrvCreateBitmap
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

HBITMAP PrvCreateBitmap (HDC destDC, const EmPixMap& src,
							int firstLine, int lastLine, Bool scale)
{
	HBITMAP lcdBM = NULL;

	// Determine a lot of the values we'll need.

	EmPoint				srcSize			= src.GetSize ();
	EmPixMapDepth		srcDepth		= src.GetDepth ();
	EmCoord				srcWidth		= srcSize.fX;
	EmCoord				srcHeight		= srcSize.fY;

	EmCoord				destWidth		= srcWidth * (scale ? 2 : 1);
	EmCoord				destHeight		= srcHeight * (scale ? 2 : 1);
	long				destDepth		= srcDepth <= 8 ? 8 : 24;

	// Determine how many colors are involved.  Create an appropriately
	// sized BITMAPINFO and fill in the color table, if necessary.

	int	numColors = 0;

	if (srcDepth <= 8)
	{
		numColors = 1 << srcDepth;
	}

	BITMAPINFO* bmi = (BITMAPINFO*) _alloca (sizeof (BITMAPINFOHEADER) +
											numColors * sizeof (RGBQUAD));

	if (numColors > 0)
	{
		const RGBList&	colorTable = src.GetColorTable ();
		::ConvertFromRGBList (bmi->bmiColors, colorTable);
	}

	// Now fill in all the BITMAPINFOHEADER fields.

	bmi->bmiHeader.biSize			= sizeof (BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth			= destWidth;
	bmi->bmiHeader.biHeight 		= -destHeight;
	bmi->bmiHeader.biPlanes 		= 1;
	bmi->bmiHeader.biBitCount		= destDepth;
	bmi->bmiHeader.biCompression	= BI_RGB;
	bmi->bmiHeader.biSizeImage		= 0;
	bmi->bmiHeader.biXPelsPerMeter	= 0;
	bmi->bmiHeader.biYPelsPerMeter	= 0;
	bmi->bmiHeader.biClrUsed		= numColors;
	bmi->bmiHeader.biClrImportant	= 0;

	// Create the DIB section.	The last two parameters to CreateDIBSection
	// are hSection and dwOffset; by setting them to NULL/0, we tell Windows
	// to allocate the memory for us.

	VOID*	bits;
	lcdBM = ::CreateDIBSection (destDC, bmi, DIB_RGB_COLORS, &bits, NULL, 0);

	if (!lcdBM || !bits)
	{
		// If we couldn't get that, just return.  I'm not sure why CreateDIBSection
		// would fail, but I've seen it happen.  It usually happens when another
		// copy of the emulator contacts this one in order to download the ROM.
		// Perhaps the intense serial port activity is getting in the way.
		// Whatever.  But subsequent attempts to call CreateDIBSection appear to
		// work.  GetLastError is returning zero.

//		DWORD	error = ::GetLastError ();

		return NULL;
	}


	// Finally, copy the bits, converting to 8 or 24 bit format along the way.

	BITMAP	bm;
	::GetObject (lcdBM, sizeof (bm), &bm);

	EmPixMap	pixMap;

	pixMap.SetSize (EmPoint (destWidth, destHeight));
	pixMap.SetFormat (destDepth == 8 ? kPixMapFormat8 : kPixMapFormat24BGR);
	pixMap.SetRowBytes (::PrvGetRowBytes (bmi));
	pixMap.SetColorTable (src.GetColorTable ());
	pixMap.SetBits (bits);

	EmRect	srcBounds (0, firstLine, srcSize.fX, lastLine);
	EmRect	destBounds (srcBounds);

	if (scale)
	{
		destBounds *= EmPoint (2, 2);
	}

	EmPixMap::CopyRect (pixMap, src, destBounds, srcBounds);

	return lcdBM;
}


/***********************************************************************
 *
 * FUNCTION:	PrvCreatePalette
 *
 * DESCRIPTION:	Create an HPALETTE from the color information in the
 *				source pixmap.  Assumes that the source pixmap uses
 *				a color table and is not a "direct" pixmap.
 *
 * PARAMETERS:	destDC - the DC responsible for the actual palette
 *					object creation.
 *
 *				src - the cross-platform pixmap containing the color
 *					information to be converted.
 *
 * RETURNED:	The new HPALETTE.  The caller is responsible for
 *				disposing of it when it's done with it.
 *
 ***********************************************************************/

HPALETTE PrvCreatePalette (HDC destDC, const EmPixMap& src)
{
	// Get the size of the palette to create.  This will be based on the
	// depth of the image.  !!! What happens if there's a mismatch between
	// the depth and the number of entries in the PixMap's color table?

	int 	numColors = 1 << src.GetDepth ();

	// Get the source color list.

	const RGBList&	colors = src.GetColorTable ();

	if (numColors > 256)
		return NULL;

	EmAssert (numColors <= 256);
	EmAssert (numColors == colors.size ());

	// Create the destination LOGPALETTE.

	LOGPALETTE* logPal = (LOGPALETTE*) _alloca (
		offsetof (LOGPALETTE, palPalEntry) +
		sizeof (PALETTEENTRY) * numColors);

	// Convert the source to the destination.

	::ConvertFromRGBList (logPal, colors);

	// Create the palette object from the LOGPALETTE.

	HPALETTE	lcdPalette = ::CreatePalette (logPal);

	return lcdPalette;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetNumColors
 *
 * DESCRIPTION:	Determine the number of colors in the color table for
 *				the given BITMAPINFO.  The number of colors is
 *				determined by the depth of the bitmap, and is even
 *				optional if the bit depth is greater than 8 bpp.
 *
 * PARAMETERS:	dib - a pointer to the BITMAPINFO containing the
 *					bitmap and color information.
 *
 * RETURNED:	For 1-, 4-, and 8-bpp images returns the number of
 *				colors in the contained color table, as determined by
 *				the image's bit depth and the biClrUsed field.	For
 *				images with more bpp, there is no color table, so
 *				this function returns zero.
 *
 ***********************************************************************/

int PrvGetNumColors (const BITMAPINFO* dib)
{
	int numColors;

	if (dib->bmiHeader.biBitCount <= 8)
		numColors = (1 << dib->bmiHeader.biBitCount);
	else
		numColors = 0;	// No palette needed for 24 BPP DIB

	if (dib->bmiHeader.biClrUsed > 0)
		numColors = dib->bmiHeader.biClrUsed;  // Use biClrUsed

	return numColors;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetBits
 *
 * DESCRIPTION:	Calculate the location of the pixels of a BITMAPINFO
 *				object.
 *
 * PARAMETERS:	dib - the pointer to the BITMAPINFO describing the
 *					bitmap.  This function assumes that the pixels
 *					are stored in memory right after the variable-
 *					length (and optional) color table.
 *
 * RETURNED:	A pointer to the pixels.
 *
 ***********************************************************************/

uint8* PrvGetBits (const BITMAPINFO* dib)
{
	return ((uint8*) dib) + dib->bmiHeader.biSize +
		::PrvGetNumColors (dib) * sizeof (RGBQUAD);
}



/***********************************************************************
 *
 * FUNCTION:	PrvGetRowBytes
 *
 * DESCRIPTION:	Determine the number of bytes in a scanline, given the
 *				number of pixels in the line and assuming long-word
 *				rounding.
 *
 * PARAMETERS:	dib - BITMAPINFO describing the bitmap whose rowbytes
 *					we want to calculate.
 *
 *				width - the width in pixels of the image whose rowbytes
 *					we want to calculate.
 *
 *				bpp - the bit depth (bits per pixel) of the image whose
 *					rowbytes we want to calculate.
 *
 * RETURNED:	The number of bytes between the start of successive
 *				rows in the image.
 *
 ***********************************************************************/

long PrvGetRowBytes (const BITMAPINFO* dib)
{
	LONG	biWidth		= dib->bmiHeader.biWidth;
	WORD	biBitCount	= dib->bmiHeader.biBitCount;

	return ::PrvGetRowBytes (biWidth, biBitCount);
}

long PrvGetRowBytes (long width, long bpp)
{
	return ((width * bpp + 31) & ~31) / 8;
}


/***********************************************************************
 *
 * FUNCTION:	PrvGetFormat
 *
 * DESCRIPTION:	Determine the format used by the DIB, in terms of the
 *				values used by EmPixMap.
 *
 * PARAMETERS:	dib - BITMAPINFO describing the bitmap whose format
 *					we want to determine.
 *
 * RETURNED:	An EmPixMapFormat with the format value.
 *
 ***********************************************************************/

EmPixMapFormat PrvGetFormat (const BITMAPINFO* dib)
{
	EmPixMapFormat	format;

	if (dib->bmiHeader.biBitCount == 1)
		format = kPixMapFormat1;

	else if (dib->bmiHeader.biBitCount == 2)
		format = kPixMapFormat2;

	else if (dib->bmiHeader.biBitCount == 4)
		format = kPixMapFormat4;

	else if (dib->bmiHeader.biBitCount == 8)
		format = kPixMapFormat8;

	else if (dib->bmiHeader.biBitCount == 24)
		format = kPixMapFormat24BGR;

	else if (dib->bmiHeader.biBitCount == 32)
		format = kPixMapFormat32BGRA;

	else
		EmAssert (false);

	return format;
}
