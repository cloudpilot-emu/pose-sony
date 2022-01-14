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

#ifndef EmRegsClockIRQCntrl_h
#define EmRegsClockIRQCntrl_h

#include "EmRegs.h"
#include "EmPalmStructs.h"

#define ClockIRQCntrl_BaseAddress			0x11000000
#define ClockIRQCntrl_Range					0x00000400

class SessionFile;
typedef struct tagHwrClockIRQCntrl
{
	UInt8	_filler01[0x200];	// 0x000-3FF

	UInt8	dataWrite;		// 0x0000
	UInt8	dataRead;		// 0x0001
	UInt8	cmdWrite;		// 0x0002
	UInt8	cmdRead;		// 0x0003

	UInt8	_filler02[0x4];	// 0x0004-0007

	UInt8	ExpCard0008;	// 0x0008
	UInt8	ExpCard0009;	// 0x0009

	UInt8	_filler03[0x3F0];	// 0x0010-400
} HwrClockIRQCntrlType;

class EmRegsClockIRQCntrl : public EmRegs
{
	public:
								EmRegsClockIRQCntrl		(void);
								EmRegsClockIRQCntrl		(emuptr);
		virtual					~EmRegsClockIRQCntrl		(void);

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
		
		HwrClockIRQCntrlType	fRegs;
		emuptr					fBaseAddr;
};

#endif /* EmRegsClockIRQCntrl_h */
  
