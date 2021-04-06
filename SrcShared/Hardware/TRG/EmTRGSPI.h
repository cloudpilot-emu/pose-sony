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

#ifndef EmTRGSPI_h
#define EmTRGSPI_h

typedef uint16 SpiValue;

typedef uint16 BusState;
#define BUS_ENABLED  1
#define BUS_16BIT    2
#define BUS_SWAP     4

void		SpiInitialize				(void);
void		SpiSetUnlatchedVal(SpiValue val);
void		SpiLatch(void);
Boolean		SpiIsBacklightOn(void);
BusState	SpiGetBusState(void);

#define NUM_KEY_ROWS 3

typedef struct {
	uint16 val[NUM_KEY_ROWS];
} KeyRowsType;

void		SpiGetKeyRows(KeyRowsType * keyRows);


#endif	/* EmTRGSPI_h */
