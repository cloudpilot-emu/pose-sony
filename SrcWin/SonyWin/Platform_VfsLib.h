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

#ifndef _PLATFORM_VFSLIB_H_
#define _PLATFORM_VFSLIB_H_

#include "SonyShared\Ffs.h"
#include "SessionFile.h"		// SessionFile

class Platform_VfsLib
{
   public:
	   
//	static Err		VFSVolumeInfo			(UInt16 nVolRefNum, VolumeInfoType* pVolInfo);
//	static Err		VFSVolumeSize			(UInt16 nVolRefNum, UInt32* pVolUsed, UInt32* pVolTotal);
	static Err		VFSImportDatabaseFromFile(UInt16 nVolRefNum, char*pPathName, UInt16 *pCardNo, LocalID *pDbID);
//	static Err		VFSExportDatabaseToFile(UInt16 nVolRefNum, char*pPathName, UInt16 nCardNo, LocalID nDbID);
	
};

#endif   // _PLATFORM_VFSLIB_H_
