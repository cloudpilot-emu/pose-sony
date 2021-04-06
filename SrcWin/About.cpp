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
#include "About.h"

#include "EmDlgWin.h"			// CenterWindow, StaticHyperlink
#include "Emulator.h"			// gInstance
#include "ErrorHandling.h"		// Errors::SetParameter
#include "Platform.h"			// Platform::GetString
#include "Strings.r.h"			// kStr_ values
#include "resource.h"			// IDC_HTTP, IDC_MAILTO, etc.


/***********************************************************************
 *
 * FUNCTION:	AboutDlgProc
 *
 * DESCRIPTION:	Handle events for the About box dialog.  Sets the
 *				application name and version.  Establishes the
 *				URL hyperlinks.  Hides the Cancel button.
 *
 * PARAMETERS:	Standard Windows dialog box callback parameters.
 *
 * RETURNED:	Standard Windows dialog box callback result.
 *
 ***********************************************************************/

BOOL CALLBACK AboutDlgProc(HWND hwnd,
						   UINT msg,
						   WPARAM wparam,
						   LPARAM lparam)
{
	// Except in response to the WM_INITDIALOG message, the dialog box procedure
	// should return nonzero if it processes the message, and zero if it does not.
	// In response to a WM_INITDIALOG message, the dialog box procedure should
	// return zero if it calls the SetFocus function to set the focus to one of
	// the controls in the dialog box. Otherwise, it should return nonzero, in
	// which case the system sets the focus to the first control in the dialog
	// box that can be given the focus.

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			HWND	hdlg = ::GetParent (hwnd);

			::CenterWindow (hdlg);

			Errors::SetParameter ("%application", Platform::GetString (kStr_AppName));
			Errors::SetParameter ("%version", Platform::GetShortVersionString ());

			string	appAndVersion = Errors::ReplaceParameters (kStr_AppAndVers);
			::SetDlgItemText (hwnd, IDC_ABOUT, appAndVersion.c_str ());

			// Hide the Cancel button and move the OK button over.

			HWND	cancelButton	= ::GetDlgItem (hdlg, IDCANCEL);
			HWND	okButton		= ::GetDlgItem (hdlg, IDOK);

			::ShowWindow (cancelButton, SW_HIDE);

			RECT	cancelRect, okRect;
			::GetWindowRect (cancelButton, &cancelRect);
			::GetWindowRect (okButton, &okRect);

			long	shift = cancelRect.right - okRect.right;

			::ScreenToClient (hdlg, (POINT*) &okRect.left);
			::ScreenToClient (hdlg, (POINT*) &okRect.right);

			::MoveWindow (okButton, okRect.left + shift, okRect.top,
						okRect.right - okRect.left, okRect.bottom - okRect.top, true);

			// Subclass the URLs so that they act like hyperlinks.

			new StaticHyperlink (::GetDlgItem (hwnd, IDC_HTTP));
			new StaticHyperlink (::GetDlgItem (hwnd, IDC_MAILTO));
			if (::GetDlgItem (hwnd, IDC_HTTP2) != NULL)
				new StaticHyperlink (::GetDlgItem (hwnd, IDC_HTTP2));

			return TRUE;
		}

		case WM_CTLCOLORSTATIC:
		{
			// Reflect this one back down to the child.

			LONG		wnd = ::GetWindowLong ((HWND) lparam, GWL_USERDATA);
			if (wnd)
				return (BOOL) ::SendMessage ((HWND) lparam, msg, wparam, lparam);

			return FALSE;
		}
	}

	return FALSE;
}


/***********************************************************************
 *
 * FUNCTION:	DoAbout
 *
 * DESCRIPTION:	Show the application's About box dialog.
 *
 * PARAMETERS:	hwnd - parent window
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void DoAbout (HWND hwnd)
{
	PROPSHEETPAGE	psp[4];
	::ZeroMemory (psp, sizeof (psp));

	psp[0].dwSize		= sizeof (PROPSHEETPAGE);
	psp[0].dwFlags		= PSP_DEFAULT;
	psp[0].hInstance	= gInstance;
	psp[0].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_PALM);
	psp[0].pfnDlgProc	= AboutDlgProc;

	psp[1].dwSize		= sizeof (PROPSHEETPAGE);
	psp[1].dwFlags		= PSP_DEFAULT;
	psp[1].hInstance	= gInstance;
	psp[1].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_WINDOWS);
	psp[1].pfnDlgProc	= AboutDlgProc;

	psp[2].dwSize		= sizeof (PROPSHEETPAGE);
	psp[2].dwFlags		= PSP_DEFAULT;
	psp[2].hInstance	= gInstance;
	psp[2].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_MAC);
	psp[2].pfnDlgProc	= AboutDlgProc;

	psp[3].dwSize		= sizeof (PROPSHEETPAGE);
	psp[3].dwFlags		= PSP_DEFAULT;
	psp[3].hInstance	= gInstance;
	psp[3].pszTemplate	= MAKEINTRESOURCE (IDD_ABOUT_UAE);
	psp[3].pfnDlgProc	= AboutDlgProc;

	char	aboutApp[200];
	string	about (Platform::GetString (IDS_ABOUT));
	string	app (Platform::GetString (kStr_AppName));
	sprintf (aboutApp, about.c_str (), app.c_str ());

	PROPSHEETHEADER psh;
	::ZeroMemory (&psh, sizeof (psh));

	psh.dwSize			= sizeof (psh);
	psh.dwFlags 		= PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent		= hwnd;
	psh.hInstance		= gInstance;
	psh.pszCaption		= aboutApp;
	psh.nPages			= 4;
	psh.ppsp			= psp;

	::PropertySheet (&psh);
}
