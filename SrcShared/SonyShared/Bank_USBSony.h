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

#ifndef _BANK_USBSONY_H_
#define _BANK_USBSONY_H_

#include "EmRegs.h"
#include "EmPalmStructs.h"
#include "HwrUsbSony.h"

class SessionFile;

typedef struct tagHwrUsbCLIE
{
	UInt8	dataWrite;		// 0x0000
	UInt8	dataRead;		// 0x0001
	UInt8	cmdWrite;		// 0x0002
	UInt8	cmdRead;		// 0x0003
} HwrUsbCLIEType;

class EmRegsUsbCLIE : public EmRegs
{
	public:
								EmRegsUsbCLIE		();
								EmRegsUsbCLIE		(emuptr baseAddr, uint32 offset);
								EmRegsUsbCLIE		(uint32 offset);
		virtual					~EmRegsUsbCLIE		(void);

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

		int						ValidAddress		(emuptr iAddress, uint32 iSize);
		static void				AddressError		(emuptr address, long size, Bool forRead);
		static void				InvalidAccess		(emuptr address, long size, Bool forRead);
		
		HwrUsbCLIEType			fRegs;
		uint32					fOffsetAddr;
		emuptr					fBaseAddr;
};


typedef struct tagHwrUSBforPegN700C
{
	UInt8	dataWrite;		// 0x0000
	UInt8	dataRead;		// 0x0001
	UInt8	cmdWrite;		// 0x0002
	UInt8	cmdRead;		// 0x0003

	UInt8	_filler01[4];	// 0x0004-0007

	UInt8	USB0008;		// 0x0008
	UInt8	USB0009;		// 0x0009

	UInt8	_filler02[0x1F6];

	UInt8	USB0200;
	UInt8	USB0201;
	UInt8	USB0202;
	UInt8	USB0203;
	UInt8	USB0204;
	UInt8	USB0205;
	UInt8	USB0206;
	UInt8	USB0207;
	UInt8	USB0208;
	UInt8	USB0209;
	UInt8	_filler03[0x6];		// 0x020A-0x020F

	UInt8	_filler04[0x9F0];	// 0x0300-0x0BFF

	UInt8	USB0C00;
	UInt8	USB0C01;
	UInt8	USB0C02;
	UInt8	USB0C03;
	UInt8	USB0C04;
	UInt8	USB0C05;
	UInt8	USB0C06;
	UInt8	USB0C07;
	UInt8	USB0C08;
	UInt8	USB0C09;
	UInt8	USB0C0A;
	UInt8	USB0C0B;
	UInt8	USB0C0C;
	UInt8	USB0C0D;
	UInt8	USB0C0E;
	UInt8	USB0C0F;

	UInt8	_filler05[0x73F0];	// 0x0C10-0x7FFF

	UInt8	USB8000[0x7FFF];
} HwrUSBforPegN700C;

class EmRegsUSBforPegN700C : public EmRegs
{
	public:
								EmRegsUSBforPegN700C	(emuptr);
		virtual					~EmRegsUSBforPegN700C	(void);

		virtual void			Initialize				(void);
		virtual void			Reset					(Bool hardwareReset);
		virtual void			Save					(SessionFile&);
		virtual void			Load					(SessionFile&);
		virtual void			Dispose					(void);

		virtual void			SetSubBankHandlers		(void);
		virtual uint8*			GetRealAddress			(emuptr address);
		virtual emuptr			GetAddressStart			(void);
		virtual uint32			GetAddressRange			(void);

		void					UnsupportedWrite		(emuptr address, int size, uint32 value);

	private:
		uint32					Read					(emuptr address, int size);
		void					Write					(emuptr address, int size, uint32 value);

		void					WrireToAddr0x8000		(emuptr address, int size, uint32 value);
		uint32					ReadFromAddr0x8000		(emuptr address, int size);
		
		int						ValidAddress			(emuptr iAddress, uint32 iSize);
		static void				AddressError			(emuptr address, long size, Bool forRead);
		static void				InvalidAccess			(emuptr address, long size, Bool forRead);
		
		emuptr					fBaseAddr;
		HwrUSBforPegN700C		fRegs;
};

#define	PegN700C_USB_RegsRange		0x0000FFFF

#endif /* _BANK_USBSONY_H_ */
  
