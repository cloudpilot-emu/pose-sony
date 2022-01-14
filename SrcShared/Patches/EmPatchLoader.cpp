/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2001 PocketPyro, Inc.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmPatchLoader.h"

#include "EmPatchMgr.h"
#include "EmPatchModule.h"
#include "EmPatchModuleHtal.h"
#include "EmPatchModuleMap.h"
#include "EmPatchModuleNetLib.h"
#include "EmPatchModuleSys.h"
#include "EmPatchState.h"

#ifdef SONY_ROM
#include "SonyShared\EmPatchModule_ExpMgr.h"
#include "SonyShared\EmPatchModule_MsfsLib.h"
#include "SonyShared\EmPatchModule_SlotDrvLib.h"
#include "SonyShared\EmPatchModule_VfsLib.h"
#endif //SONY_ROM
// ======================================================================
// This is where you might say all the good stuff actually happens... 
//
//		we create the instances of all the relevant objects
//		we create the instances of all the statically linked patch modules
//		we add the statically linked patch modules to the PatchMap
//
//	NOTE: 
//		If USE_HOST_SPECIFIC_LOADER is defined no EmPatchLoader object is 
//		created here, a derived object will be created for the Host specific
//		implementation.  
//
//		Either way the EmPatchLoader will be exposed globally through the 
//		global public interface pointer "gTheLoaderIP"
// ======================================================================


// ======================================================================
//	Private Object instances
// ======================================================================
//
static EmPatchMgr           gPatchManager;
static EmPatchModuleMap     gPatchMap;

#ifndef USE_HOST_SPECIFIC_LOADER
static EmPatchLoader		gLoader;
#endif //USE_HOST_SPECIFIC_LOADER


// ======================================================================
//	Static linked PatchModule instances
// ======================================================================
static EmPatchModuleHtal	gPatchModuleHtal;
static EmPatchModuleSys		gPatchModuleSys;
static EmPatchModuleNetLib	gPatchModuleNetLib;

#ifdef SONY_ROM
static EmPatchModule_ExpMgr			gExpMgrPatchTable;
static EmPatchModule_MsfsLib		gMsfsLibPatchTable;		// MS FileSystem Lib for Sony's ROM
static EmPatchModule_SlotDrvLib		gSlotDrvLibPatchTable;	// Slot driver Lib for Sony's ROM
static EmPatchModule_VfsLib			gVfsLibPatchTable;
#endif //SONY_ROM

// ======================================================================
//	Global Public interfaces
// ======================================================================

IEmPatchModuleMap*	gPatchMapIP			= &gPatchMap;
IEmPatchContainer*	gPatchContainerIP	= &gPatchManager;

#ifndef USE_HOST_SPECIFIC_LOADER

IEmPatchLoader*		gTheLoaderIP		= &gLoader;

#endif //USE_HOST_SPECIFIC_LOADER




// ==============================================================================
// *  BEGIN IEmPatchLoader
// ==============================================================================

Err EmPatchLoader::InitializePL (void)
{
	return kPatchErrNone;
}

Err EmPatchLoader::ResetPL (void)
{
	return kPatchErrNone;
}

Err EmPatchLoader::DisposePL (void)
{
	return kPatchErrNone;
}

Err EmPatchLoader::ClearPL (void)
{
	return kPatchErrNone;
}

Err EmPatchLoader::LoadPL (void)
{
	return kPatchErrNone;
}


Err EmPatchLoader::LoadAllModules(void)
{
	gPatchMap.AddModule (&gPatchModuleSys);
	gPatchMap.AddModule (&gPatchModuleNetLib);
	gPatchMap.AddModule (&gPatchModuleHtal);

#ifdef SONY_ROM
	gPatchMap.AddModule (&gExpMgrPatchTable);
	gPatchMap.AddModule (&gMsfsLibPatchTable);
	gPatchMap.AddModule (&gSlotDrvLibPatchTable);
	gPatchMap.AddModule (&gVfsLibPatchTable);
#endif //SONY_ROM

	gPatchMap.InitializeAll (*gPatchContainerIP);
	
	return kPatchErrNone;
}


Err EmPatchLoader::LoadModule (const string& /*url*/)
{
	return kPatchErrNotImplemented;
}

// ==============================================================================
// *  END IEmPatchLoader
// ==============================================================================

