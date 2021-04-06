/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1999-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef EmDevice_h
#define EmDevice_h

#include <vector>				// vector
#include <utility>				// pair
#include <string>				// string

class EmCPU;
class EmRegs;
class EmSession;
struct DeviceInfo;

class LAS;
template <class A> class EmROMReader;

class EmDevice;
typedef vector<EmDevice>	EmDeviceList;


class EmDevice
{
	public:
								EmDevice			(void);
								EmDevice			(const char*);
								EmDevice			(const string&);
								EmDevice			(const EmDevice&);
								~EmDevice			(void);

		bool					operator==			(const EmDevice&) const;
		bool					operator!=			(const EmDevice&) const;

		Bool					Supported			(void) const;

		Bool					Supports68328		(void) const;
		Bool					Supports68EZ328		(void) const;
		Bool					Supports68VZ328		(void) const;
		Bool					Supports68SZ328		(void) const;
		Bool					SupportsARM			(void) const;

		Bool					PrismPlatinumEdgeHack	(void) const;
		Bool					EdgeHack				(void) const;

		Bool					SupportsROM			(const EmROMReader<LAS>& ROM) const;

		EmCPU*					CreateCPU			(EmSession*) const;
		void					CreateRegs			(void) const;

		Bool					HasFlash			(void) const;

		RAMSizeType				MinRAMSize			(void) const;

		long					HardwareID			(void) const;
		long					HardwareSubID		(void) const;

		string					GetMenuString		(void) const;
		string					GetIDString			(void) const;
		vector<string>			GetIDStrings		(void) const;

	public:
		static EmDeviceList		GetDeviceList		(void);

#ifdef SONY_ROM
		int						GetDeviceType		(void) const { return fDeviceID; };
#endif

	private:
								EmDevice			(int);
		const DeviceInfo*		GetDeviceInfo		(void) const;
		int						fDeviceID;

		int						GetDeviceID			(const char*) const;

};

enum	// DeviceType
{
	kDeviceUnspecified	= 0,
	kDevicePilot,
	kDevicePalmPilot,
	kDevicePalmIII,
	kDevicePalmIIIc,
	kDevicePalmIIIe,
	kDevicePalmIIIx,
	kDevicePalmV,
	kDevicePalmVx,
	kDevicePalmVII,
	kDevicePalmVIIEZ,
	kDevicePalmVIIx,
	kDevicePalmM100,
	kDevicePalmM500,
	kDevicePalmM505,
	kDeviceARMRef,
	kDeviceSymbol1500,
	kDeviceSymbol1700,
	kDeviceSymbol1740,
	kDeviceTRGpro,
	kDeviceVisor,
	kDeviceVisorPrism,
	kDeviceVisorPlatinum,
	kDeviceVisorEdge,

#ifdef SONY_ROM
	kDevicePEGS500C,
	kDevicePEGS300,
	kDevicePEGN700C,
	kDevicePEGS320,
	kDevicePEGS360,
	kDevicePEGN600C,
	kDevicePEGT400,
	kDevicePEGT600,
#endif

	kDeviceLast
};

#ifdef	SONY_ROM
#include	"SonyShared\SonyDevice.h"
#endif

#endif	/* EmDevice_h */
