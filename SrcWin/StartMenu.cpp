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

#include "EmCommon.h"
#include "StartMenu.h"			// AddStartMenuItem
#include "Platform.h"			// Platform::GetString
#include "resource.h"
#include "Strings.r.h"			// kStr_AppName

#include <shlobj.h>

/**************************************************************************
 *
 * Local Function Prototypes
 *
 *************************************************************************/

BOOL GetFolder(HWND, LPITEMIDLIST*, LPITEMIDLIST, LPSTR, LPCSTR);
static BOOL GetShortcut(HWND, HINSTANCE, LPSTR);
static BOOL GetShortcutName(HWND, HINSTANCE, LPSTR);
static BOOL CALLBACK GetShortcutNameDlgProc(HWND, UINT, WPARAM, LPARAM);
static HRESULT CreateLink(LPCSTR, LPSTR, LPSTR);

/***********************************************************************
 *
 * FUNCTION:	AddStartMenuItem
 *
 * DESCRIPTION:	Adds a link to the specified path to the start menu
 *
 * PARAMETERS:	Standard Windows interface parameters.
 *				szAppPath specifies the target of the link (usually the
 *					program)
 *				szLinkPath stores the name of the link after it is
 *					created.
 *
 * RETURNED:	TRUE if shortcut was created; FALSE if not
 *
 ***********************************************************************/

BOOL AddStartMenuItem(HWND hWnd, HINSTANCE hInstance, LPSTR szAppPath,
					  LPSTR szLinkPath)
{
	// get the location of the new shortcut
	if (!GetShortcut(hWnd, hInstance, szLinkPath))
	   return FALSE;

	// create the shortcut
	CreateLink(szAppPath, szLinkPath, "");

	return TRUE;
}

/***********************************************************************
 *
 * FUNCTION:	GetShortcut
 *
 * DESCRIPTION:	Starts the user interface elements to select a location
 *				and name for a shortcut on the start menu.
 *
 * PARAMETERS:	Standard Windows interface parameters.
 *				lpszPath returns the path of the LINK.
 *
 * RETURNED:	TRUE if shortcut location was completely specified;
 *				FALSE if not
 *
 ***********************************************************************/

static BOOL GetShortcut(HWND hWnd, HINSTANCE hInstance, LPSTR lpszPath)
{
	LPITEMIDLIST pidlStartMenu, pidlDestination;
	char szTemp[MAX_PATH];

	// get the pidl for the start menu
	SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidlStartMenu);

	// get the destination folder
	if (!GetFolder(hWnd, &pidlDestination, pidlStartMenu, szTemp,
				   "Select Location for Shortcut"))
	   return FALSE;

	// get the path for the folder
	SHGetPathFromIDList(pidlDestination, lpszPath);

	// append the shorcut filename
	if (!GetShortcutName(hWnd, hInstance, lpszPath))
		return FALSE;

	// add .LNK to the shortcut file name
	lstrcat(lpszPath, ".lnk");

	return TRUE;
}

/***********************************************************************
 *
 * FUNCTION:	GetFolder
 *
 * DESCRIPTION:	Displays standard dialog to let user select a folder
 *
 * PARAMETERS:	Standard Windows interface parameters.
 *				ppidlDestination stores the selected folder
 *				pidlRoot specifies the root of the dialog box's folder
 *					hierarchy
 *				lpszDisplayName stores the display name of the folder
 *					selected by the user.
 *				lpszTitle specifies the title of the dialog box
 *
 * RETURNED:	TRUE if shortcut name was typed; FALSE if user cancelled
 *
 ***********************************************************************/

BOOL GetFolder(   HWND hwndParent,
				  LPITEMIDLIST *ppidlDestination,
				  LPITEMIDLIST pidlRoot,
				  LPSTR lpszDisplayName,
				  LPCSTR lpszTitle)
{
	BROWSEINFO	BrInfo;

	ZeroMemory(&BrInfo, sizeof(BrInfo));
	BrInfo.hwndOwner		= hwndParent;
	BrInfo.pidlRoot 		= pidlRoot; // browse from the start menu down
	BrInfo.pszDisplayName	= lpszDisplayName;
	BrInfo.lpszTitle		= lpszTitle;

	// use the shell's folder browser
	*ppidlDestination = SHBrowseForFolder(&BrInfo);

	// did the user select the cancel button
	if(NULL == *ppidlDestination)
		return FALSE;

	return TRUE;
}

/***********************************************************************
 *
 * FUNCTION:	GetShortcutName
 *
 * DESCRIPTION:	Gets a shortcut name from the user
 *
 * PARAMETERS:	Standard Windows interface parameters.
 *				lpszPath stores the returned path
 *
 * RETURNED:	TRUE if shortcut name was typed; FALSE if user cancelled
 *
 ***********************************************************************/

static BOOL GetShortcutName(HWND hWnd, HINSTANCE hInstance, LPSTR lpszPath)
{
	return DialogBoxParam(	hInstance,
							MAKEINTRESOURCE(IDD_GET_SHORTCUT_NAME),
							hWnd,
							GetShortcutNameDlgProc,
							(LPARAM)lpszPath);
}

/***********************************************************************
 *
 * FUNCTION:	GetShortcutNameDlgProc
 *
 * DESCRIPTION:	Handle events for the dialog getting the shortcut name.
 *
 * PARAMETERS:	Standard Windows dialog box callback parameters.
 *
 * RETURNED:	Standard Windows dialog box callback result.
 *
 ***********************************************************************/

static BOOL CALLBACK GetShortcutNameDlgProc(  HWND hWnd,
											  UINT uMsg,
											  WPARAM wParam,
											  LPARAM lParam)
{
	static LPSTR   lpszShorcut;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			lpszShorcut = (LPSTR)lParam;
			string	appName = Platform::GetString (kStr_AppName);

			SetDlgItemText(hWnd, IDC_TEXT, appName.c_str ());
			return TRUE;
		}

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			{
				char szTemp[MAX_PATH];

				GetDlgItemText(hWnd, IDC_TEXT, szTemp, sizeof(szTemp));
				lstrcat(lpszShorcut, "\\");
				lstrcat(lpszShorcut, szTemp);
				EndDialog(hWnd, TRUE);
			}	   
			break;

		case IDCANCEL:
			EndDialog(hWnd, FALSE);
			break;
		}
		return TRUE;

	}
	return FALSE;
}

/**************************************************************************

   CreateLink()

   uses the shell's IShellLink and IPersistFile interfaces to create and
   store a shortcut to the specified object.

   Returns the result of calling the member functions of the interfaces.

   lpszSource - address of a buffer containing the path of the object

   lpszTarget - address of a buffer containing the path where the shell
	  link is to be stored

   lpszDesc - address of a buffer containing the description of the shell
	  link

**************************************************************************/

static HRESULT CreateLink(	LPCSTR lpszSource,
							LPSTR lpszTarget,
							LPSTR lpszDesc)
{
	HRESULT hres;
	IShellLink* pShellLink;

	// CoInitialize must be called before this
	// Get a pointer to the IShellLink interface.
	hres = CoCreateInstance(   CLSID_ShellLink,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IShellLink,
							   (LPVOID*)&pShellLink);
	if (SUCCEEDED(hres))
	   {
	   IPersistFile* pPersistFile;

	   // Set the path to the shortcut target, and add the description.
	   pShellLink->SetPath(lpszSource);
	   pShellLink->SetDescription(lpszDesc);

	   // Query IShellLink for the IPersistFile interface for saving the
	   // shortcut in persistent storage.
	   hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

	   if (SUCCEEDED(hres))
		  {
		  WCHAR wsz[MAX_PATH];

		  // Ensure that the string is ANSI.
		  MultiByteToWideChar( CP_ACP, 0, lpszTarget, -1, wsz, MAX_PATH);

		  // Save the link by calling IPersistFile::Save.
		  hres = pPersistFile->Save(wsz, TRUE);
 
			//if(FAILED(hres))
			//	 ErrorHandler();

		  pPersistFile->Release();
		  }

	   pShellLink->Release();
	   }

	return hres;
}

