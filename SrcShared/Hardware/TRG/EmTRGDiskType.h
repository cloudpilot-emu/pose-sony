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

#ifndef EmTRGDiskType_h
#define EmTRGDiskType_h

#include "EmTRGCFDefs.h"

class EmDiskType 
{
	public:
					EmDiskType				(void);
	 virtual			~EmDiskType				(void);

	// EmRegs overrides
	EmSector *		GetTuple(void);
	EmSector *		GetDriveID(void);
	EmSector *		GetEmptySector(void);

private:
};

class EmGeneric8MB : public EmDiskType
{
	public:
					EmGeneric8MB				(void);
		virtual			~EmGeneric8MB				(void);

		EmSector *	GetTuple(void);
		EmSector *	GetDriveID(void);
		uint32		GetNumSectors(void);
		EmSector *	GetSector(LogicalBlockAddr lba);
	private:
};

class EmCurrDiskType 
{
	public:
					EmCurrDiskType				(void);
	 virtual			~EmCurrDiskType				(void);

	void				GetTuple(EmDiskTypeID ID,
                                 EmSector * s);
	void				GetDriveID(EmDiskTypeID ID,
                                 EmSector * s);
	uint32 				GetNumSectors(EmDiskTypeID ID);
	void				GetSector(EmDiskTypeID ID,
					          LogicalBlockAddr lba,
                                 EmSector * s);
private:
	EmDiskType			UnknownDisk;
	EmGeneric8MB			Generic8MB;
};

#endif	/* EmTRGDiskType_h */
