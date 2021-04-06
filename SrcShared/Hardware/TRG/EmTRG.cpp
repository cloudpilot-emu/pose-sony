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
#include "EmTRG.h"

#include "EmBankRegs.h"
#include "EmRegsEZTRGpro.h"
#include "EmTRGCF.h"


/***********************************************************************
 *
 * FUNCTION:    OEMCreateTRGRegObjs
 *
 * DESCRIPTION: This function is called by EmDevice::CreateRegs for TRG
 *              devices
 *
 * PARAMETERS:  'fHardwareSubID' specifies the device
 *
 * RETURNED:    None
 *
 ***********************************************************************/
void OEMCreateTRGRegObjs(long /*hardwareSubID*/)
{
	EmBankRegs::AddSubBank (new EmRegsEZTRGpro);

	// the CF emulation has only been tested on a Windows system
	#ifdef PLATFORM_WINDOWS
  	EmBankRegs::AddSubBank (new EmRegsCFMemCard);
	#endif
}
