/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef _PLATFORM_EXPMGRLIB_H_
#define _PLATFORM_EXPMGRLIB_H_

#include "SessionFile.h"		// SessionFile
#include "SonyShared\ExpansionMgr.h"

class Platform_ExpMgrLib
{
   public:
		static	Bool	IsUsable(void);
		static	void	CalledExpInit (void);
		static	void	Initialize (void);
		static	void	Reset (void);
		static	void	Load(SessionFile& f);
		static	void	Save(SessionFile& f);
};

#endif   // _PLATFORM_EXPMGRLIB_H_
