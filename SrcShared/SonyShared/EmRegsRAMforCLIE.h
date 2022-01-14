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

#ifndef EmRegsRAMforCLIE_h
#define EmRegsRAMforCLIE_h

#include "EmRegs.h"
#include "EmPalmStructs.h"

#define RAMforCLIE_BaseAddress			0x1F052000
#define RAMforCLIE_Range				0x0002E000	// 0x1F052000-1F07FFFF

class SessionFile;
typedef struct tagHwrRAMforCLIE
{
	UInt8	_filler01[RAMforCLIE_Range];			// 0x1F052000-1F07FFFF
} HwrRAMforCLIEType;

class EmRegsRAMforCLIE : public EmRegs
{
	public:
								EmRegsRAMforCLIE		(void);
								EmRegsRAMforCLIE		(emuptr);
		virtual					~EmRegsRAMforCLIE		(void);

		virtual void			Initialize		(void);
		virtual void			Reset			(Bool hardwareReset);
		virtual void			Save			(SessionFile&);
		virtual void			Load			(SessionFile&);
		virtual void			Dispose			(void);

		virtual void			SetSubBankHandlers	(void);
		virtual uint8*			GetRealAddress		(emuptr address);
		virtual emuptr			GetAddressStart		(void);
		virtual uint32			GetAddressRange		(void);

		void					UnsupportedWrite	(emuptr address, int size, uint32 value);

	private:
		uint32					Read	(emuptr address, int size);
		void					Write	(emuptr address, int size, uint32 value);

		uint32					ReadFromDummy	(emuptr address, int size);
		void					WrireToDummy	(emuptr address, int size, uint32 value);

		int						ValidAddress		(emuptr iAddress, uint32 iSize);
		static void				AddressError		(emuptr address, long size, Bool forRead);
		static void				InvalidAccess		(emuptr address, long size, Bool forRead);
		
		HwrRAMforCLIEType			fRegs;
		emuptr							fBaseAddr;
};

#endif /* EmRegsRAMforCLIE_h */
  
