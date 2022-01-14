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

#ifndef EmDocumentWin_h
#define EmDocumentWin_h

#include "EmDocument.h"

/*
	EmDocumentWin is a Windows-specific sub-class of EmDocument.  It is
	responsible for translating platform-specific document-related
	actions into cross-platform actions, making use of the the cross-
	platform EmDocument implementations.
*/

class EmDocumentWin : public EmDocument
{
	public:
								EmDocumentWin		(void);
		virtual					~EmDocumentWin		(void);

	public:
		// I'd like these to be private, but at least one part of Poser
		// needs access to HostSaveScreen.
		virtual Bool			HostCanSaveBound	(void);
		virtual void			HostSaveScreen		(const EmFileRef&);
};

extern EmDocumentWin*	gHostDocument;

#endif	// EmDocumentWin_h
