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


#ifndef EmPatchLoaderWin_h
#define EmPatchLoaderWin_h

#include "EmPatchLoader.h"


// ===========================================================================
//		PatchLoader
// ===========================================================================


class EmPatchLoaderWin : public EmPatchLoader
{
	public:

// ==============================================================================
// *  constructors
// ==============================================================================
		EmPatchLoaderWin() : EmPatchLoader()
		{
		}


// ==============================================================================
// *  interface implementations
// ==============================================================================


// ==============================================================================
// *  OVERRIDE IEmPatchLoader
// ==============================================================================
		virtual Err LoadAllModules	(void);
		virtual Err LoadModule		(const string& url);

	private:
};

#endif // EmPatchLoaderWin_h
