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
#include "EmRegionWin.h"


HRGN ConvertRegionToHost (const EmRegion& rgn)
{
	HRGN	result = NULL;
	EmRect	rect;
	EmRegionRectIterator	iter (rgn);

	if (iter.Next (rect))
	{
		result = ::CreateRectRgn (	rect.fLeft,
									rect.fTop,
									rect.fRight,
									rect.fBottom);

		while (iter.Next (rect))
		{
			HRGN	tempRgn = ::CreateRectRgn (	rect.fLeft,
												rect.fTop,
												rect.fRight,
												rect.fBottom);

			::CombineRgn (result, result, tempRgn, RGN_OR);
			::DeleteObject (tempRgn);
		}
	}

	return result;
}
