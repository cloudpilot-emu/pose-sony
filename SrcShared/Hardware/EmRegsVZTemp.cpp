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
#include "EmRegsVZTemp.h"
#include "EmRegsVZPrv.h"

#include "EmSPISlaveADS784x.h"	// EmSPISlaveADS784x
#include "EmTransportSerial.h"	// EmTransportSerial


const int		kNumButtonRows = 3;
const int		kNumButtonCols = 4;

