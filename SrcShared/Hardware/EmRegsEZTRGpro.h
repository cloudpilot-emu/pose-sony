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

#ifndef EmRegsEZTRGpro_h
#define EmRegsEZTRGpro_h

#include "EmRegsEZ.h"


class EmRegsEZTRGpro : public EmRegsEZ
{
	public:
								EmRegsEZTRGpro			(void);
		virtual					~EmRegsEZTRGpro			(void);

		virtual void			Initialize				(void);
		virtual Bool			GetLCDScreenOn			(void);
		virtual Bool			GetLCDBacklightOn		(void);
		virtual Bool			GetSerialPortOn			(int uartNum);

		virtual uint8			GetPortInputValue		(int);
		virtual uint8			GetPortInternalValue	(int);
		virtual void			GetKeyInfo				(int* numRows, int* numCols,
														 uint16* keyMap, Bool* rows);
		virtual void			PortDataChanged			(int, uint8, uint8);
		void 				SetSubBankHandlers(void);
	private:
		void				spiWrite(emuptr address, int size, uint32 value);
		uint16				spiLatchedVal, spiUnlatchedVal;
		

	protected:
		virtual EmSPISlave*		GetSPISlave				(void);

	private:
		EmSPISlave*				fSPISlaveADC;
};

#endif	/* EmRegsEZTRGpro_h */
