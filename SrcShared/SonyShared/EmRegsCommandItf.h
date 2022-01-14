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

#ifndef EmRegsCommandItf_h
#define EmRegsCommandItf_h

#include "EmRegs.h"
#include "EmPalmStructs.h"

#define CommandItf_BaseAddress			0x11000C00
//#define CommandItf_Range				0x000001FF
#define CommandItf_Range				0x00000200

class SessionFile;
typedef struct tagHwrCommandItf
{
//	UInt8	_filler01[CommandItf_Range];	// 0x0C00-0DFF
	UInt8	_filler01[0x06];		//	0x0C00-0C06
	UInt8	CmdItf0C06;				//	0x0C06
	UInt8	_filler02[0x01F8];		//	0x0C0F-0DFF
} HwrCommandItfType;

class EmRegsCommandItf : public EmRegs
{
	public:
								EmRegsCommandItf		(void);
								EmRegsCommandItf		(emuptr);
		virtual					~EmRegsCommandItf		(void);

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
		
		HwrCommandItfType			fRegs;
		emuptr					fBaseAddr;
};

#endif /* EmRegsCommandItf_h */
  
