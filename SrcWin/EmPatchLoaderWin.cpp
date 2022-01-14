/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) PocketPyro, Inc.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmPatchLoaderWin.h"

#include "Platform.h"

#include <tchar.h>


typedef EcmErr (*ECMLoadPfn)(IEcmContainer*, IEcmPackage**);
typedef EcmErr (*ECMUnloadPfn)(void);

typedef EcmErr (*ECMdInitializePfn)(IEmPatchContainer* containerIP);
typedef EcmErr (*ECMdDeposePfn)(void);
typedef EcmErr (*ECMdCreatePfn)(const EcmCompName& compName,
								const EcmIfName& ifName, void** compIP);

struct WinDllEntry
{
	WinDllEntry (ECMLoadPfn loadP, ECMUnloadPfn unloadP) : 
		Load (loadP),
		Unload (unloadP)
	{}

	WinDllEntry (ECMdInitializePfn initializeP, ECMdDeposePfn deposeP, ECMdCreatePfn createP) : 
		Initialize (initializeP),
		Depose (deposeP),
		Create (createP)
	{}

	ECMLoadPfn			Load;
	ECMUnloadPfn		Unload;
	IEcmPackage*		PackageIP;

	ECMdInitializePfn	Initialize;
	ECMdDeposePfn		Depose;
	ECMdCreatePfn		Create;
};


// ======================================================================
//	Private Object instances
// ======================================================================

static EmPatchLoaderWin		gWinLoader;
static list<WinDllEntry*>	gDllList;


// ======================================================================
//	Global Public Object Interfaces
// ======================================================================

#ifdef USE_HOST_SPECIFIC_LOADER

IEmPatchLoader*	gTheLoaderIP = &gWinLoader;

#endif //USE_HOST_SPECIFIC_LOADER


extern IEmPatchModuleMap*	gPatchMapIP;
extern IEmPatchContainer*	gPatchContainerIP;


Err EmPatchLoaderWin::LoadAllModules()
{
	TCHAR path[MAX_PATH];
	path[0] = 0;

	if (GetCurrentDirectory(MAX_PATH, path) != 0)
	{
		HANDLE hSearch = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA findData = {0};

		_tcscat(path, _T("\\*.epm"));

		hSearch = FindFirstFile(path, &findData);

		if (hSearch != INVALID_HANDLE_VALUE)
		{
			string patchModuleName = findData.cFileName;

			LoadModule(patchModuleName);

			while (FindNextFile(hSearch, &findData) == TRUE)
			{
				patchModuleName = findData.cFileName;

				LoadModule(patchModuleName);
			}

			FindClose(hSearch);
		}
	}

	return EmPatchLoader::LoadAllModules();
}




Err EmPatchLoaderWin::LoadModule(const string &url)
{
	HMODULE hModule		= ::LoadLibrary (url.c_str());

	FARPROC	loadP		= ::GetProcAddress (hModule, "ECMLoadPackage");
	FARPROC	unloadP		= ::GetProcAddress (hModule, "ECMUnloadPackage");

	FARPROC	initializeP	= ::GetProcAddress (hModule, "ECMdInitialize");
	FARPROC	deposeP		= ::GetProcAddress (hModule, "ECMdDepose");
	FARPROC	createP		= ::GetProcAddress (hModule, "ECMdCreate");

	WinDllEntry *entryP	= NULL;

	if (loadP && unloadP)
	{
		entryP = new WinDllEntry (
						(ECMLoadPfn) loadP,
						(ECMUnloadPfn) unloadP);
	}
	else if (initializeP && deposeP && createP)
	{
		entryP = new WinDllEntry (
						(ECMdInitializePfn) initializeP,
						(ECMdDeposePfn) deposeP,
						(ECMdCreatePfn) createP);
	}

	if (entryP)
	{
		IEmPatchModule*	modIP = NULL;

		gDllList.push_back (entryP);

		if (entryP->Load)
		{
			entryP->Load (gPatchContainerIP, (IEcmPackage**) &entryP->PackageIP);

			if (entryP->PackageIP)
			{
				entryP->PackageIP->RequestInterface (kEmPatchModuleIfn, (void**) &modIP);
			}
		}
		else if (entryP->Initialize)
		{
			entryP->Initialize (gPatchContainerIP);
			entryP->Create (string (""), kEmPatchModuleIfn, (void**) &modIP);
		}

		if (modIP != NULL)
			gPatchMapIP->AddModule (modIP);
	}

	return kPatchErrNone;
}

