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

#ifndef EmRegsFMSound_h
#define EmRegsFMSound_h

#include "EmRegs.h"
#include "EmPalmStructs.h"

#define FMSound_BaseAddress			0x10980000
#define FMSound_Range				0x0000FFFF

class SessionFile;
typedef struct tagHwrFMSound
{
	UInt8	_filler01[FMSound_Range];	// 0x000-FFFF
} HwrFMSoundType;

class EmRegsFMSound : public EmRegs
{
	public:
								EmRegsFMSound		(void);
								EmRegsFMSound		(emuptr);
		virtual					~EmRegsFMSound		(void);

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
		
		HwrFMSoundType			fRegs;
		emuptr					fBaseAddr;
};

#endif /* EmRegsFMSound_h */
  
