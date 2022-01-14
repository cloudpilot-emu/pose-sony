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

#include "EmCommon.h"
#include "EmDocumentWin.h"

#include "EmWindow.h"			// gWindow, GetLCDContents
#include "EmPixMapWin.h"		// ConvertFromRGBList
#include "EmScreen.h"			// EmScreenUpdateInfo

EmDocumentWin*	gHostDocument;

// ---------------------------------------------------------------------------
//		¥ EmDocument::HostCreateDocument
// ---------------------------------------------------------------------------
// Create our document instance.  This is the one and only function that
// creates the document.  Being in a platform-specific file, it can create
// any subclass of EmDocument it likes (in particular, one specific to our
// platform).

EmDocument* EmDocument::HostCreateDocument (void)
{
	return new EmDocumentWin;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmDocumentWin::EmDocumentWin
 *
 * DESCRIPTION:	Constructor.  Sets the global host application variable
 *				to point to us.
 *
 * PARAMETERS:	None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

EmDocumentWin::EmDocumentWin (void) :
	EmDocument ()
{
	EmAssert (gHostDocument == NULL);
	gHostDocument = this;
}


/***********************************************************************
 *
 * FUNCTION:	EmDocumentWin::~EmDocumentWin
 *
 * DESCRIPTION:	Destructor.  Closes our window and sets the host
 *				application variable to NULL.
 *
 * PARAMETERS:	None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

EmDocumentWin::~EmDocumentWin (void)
{
	// Delete the window along with the document.  We do this in the
	// platform destructor instead of the EmDocument destructor because
	// we don't necessarily want the window closed on all platforms
	// (e.g., Unix).

	delete gWindow;

	EmAssert (gHostDocument == this);
	gHostDocument = NULL;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmDocumentWin::HostCanSaveBound
// ---------------------------------------------------------------------------
// Return whether or not we can carry out the "bind" operation.  If not,
// report an error message and return false.  Otherwise, return true.

Bool EmDocumentWin::HostCanSaveBound (void)
{
	OSVERSIONINFO	version = { sizeof (version) };
	::GetVersionEx (&version);

	if (version.dwPlatformId != VER_PLATFORM_WIN32_NT)
	{
		::MessageBox (NULL, 
			"This operation requires Windows NT or better.  Windows\n"
			"95, 98 and Millennium Edition don't provide the facilities\n"
			"for creating bound applications.", "Error", MB_ICONHAND | MB_OK);

		return false;
	}

	return true;
}


// ---------------------------------------------------------------------------
//		¥ EmDocumentWin::HostSaveScreen
// ---------------------------------------------------------------------------
// Save the current contents of the LCD buffer to the given file.

void EmDocumentWin::HostSaveScreen (const EmFileRef& destRef)
{
	if (!gWindow)
		return;

	// Create and open the file.

	HANDLE	file = ::CreateFile (destRef.GetFullPath ().c_str (),
								GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL, NULL);

	if (file != INVALID_HANDLE_VALUE)
	{
		// Get the LCD pixels.

		EmScreenUpdateInfo	info;
		gWindow->GetLCDContents (info);

		// Convert the pixmap into some easily managed standard BMP format.

		if (info.fImage.GetDepth () < 8)
		{
			info.fImage.ConvertToFormat (kPixMapFormat8);
		}
		else if (info.fImage.GetDepth () == 16)
		{
			info.fImage.ConvertToFormat (kPixMapFormat16RGB555);
		}
		else if (info.fImage.GetDepth () == 24)
		{
			info.fImage.ConvertToFormat (kPixMapFormat24BGR);
		}

		// Calculate some handy values (depth, number of colors, and image buffer size).

		long	depth					= info.fImage.GetDepth ();
		long	numColors				= (depth <= 8) ? (1 << depth) : 0;
		EmPoint	size					= info.fImage.GetSize ();

		// Calculate various offsets into the BMP file.

		DWORD	fileHeaderSize			= sizeof (BITMAPFILEHEADER);
		DWORD	bitmapHeaderSize		= sizeof (BITMAPINFOHEADER);
		DWORD	colorTableSize			= numColors * sizeof (RGBQUAD);
		DWORD	bitmapOffset			= fileHeaderSize + bitmapHeaderSize + colorTableSize;
		DWORD	bitmapSize				= info.fImage.GetRowBytes () * size.fY;
		DWORD	fileSize				= bitmapOffset + bitmapSize;

		// Prepare the BMP file header.

		BITMAPFILEHEADER	fileHeader;

		fileHeader.bfType				= 'MB';
		fileHeader.bfSize				= fileSize;
		fileHeader.bfReserved1			= 0;
		fileHeader.bfReserved2			= 0;
		fileHeader.bfOffBits			= bitmapOffset;

		// Prepare the BMP information header.

		BITMAPINFOHEADER	bitmapHeader;

		bitmapHeader.biSize 			= sizeof (BITMAPINFOHEADER);
		bitmapHeader.biWidth			= size.fX;
		bitmapHeader.biHeight			= size.fY;
		bitmapHeader.biPlanes			= 1;
		bitmapHeader.biBitCount 		= depth;
		bitmapHeader.biCompression		= BI_RGB;
		bitmapHeader.biSizeImage		= 0;
		bitmapHeader.biXPelsPerMeter	= 0;
		bitmapHeader.biYPelsPerMeter	= 0;
		bitmapHeader.biClrUsed			= numColors;
		bitmapHeader.biClrImportant 	= 0;

		// Prepare the color table.

		RGBQUAD*	colors = (RGBQUAD*) _alloca (colorTableSize);
		::ConvertFromRGBList (colors, info.fImage.GetColorTable ());

		// Prepare the bit image (BMP files are stored upside down).

		info.fImage.FlipScanlines ();

		// Write the BMP file header, BMP information header, color table,
		// and pixels to the file.

		DWORD	written;

		::WriteFile (file, &fileHeader, fileHeaderSize, &written, NULL);
		::WriteFile (file, &bitmapHeader, bitmapHeaderSize, &written, NULL);
		::WriteFile (file, colors, colorTableSize, &written, NULL);
		::WriteFile (file, info.fImage.GetBits (), bitmapSize, &written, NULL);

		// Close the file.

		::CloseHandle (file);
	}
}
