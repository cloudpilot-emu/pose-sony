/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef EmCPUARM_h
#define EmCPUARM_h

#include "EmCPU.h"				// EmCPU

class EmCPUARM;
extern EmCPUARM*	gCPUARM;

class EmCPUARM : public EmCPU
{
	public:
		// -----------------------------------------------------------------------------
		// constructor / destructor
		// -----------------------------------------------------------------------------

								EmCPUARM 			(EmSession*);
		virtual 				~EmCPUARM			(void);

		// -----------------------------------------------------------------------------
		// public methods
		// -----------------------------------------------------------------------------

		// Standard sub-system methods:
		//		Reset:	Resets the state.  Called on hardware resets or on
		//				calls to SysReset.  Also called from constructor.
		//		Save:	Saves the state to the given file.
		//		Load:	Loads the state from the given file.  Can assume that
		//				Reset has been called first.

		virtual void 			Reset				(Bool hardwareReset);
		virtual void 			Save				(SessionFile&);
		virtual void 			Load				(SessionFile&);

		// Execute the main CPU loop until asked to stop.

		virtual void 			Execute 			(void);
		virtual void 			CheckAfterCycle		(void);

	private:
		void					DoReset				(Bool cold);
};

#endif	// EmCPUARM_h
