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
class EmFileRef;
class EmRegs;
class EmSession;
struct DeviceInfo;

class EmROMReader;

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

		Bool					SupportsROM			(const EmFileRef&) const;
		Bool					SupportsROM			(const EmROMReader&) const;

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
		int						GetDeviceType		(void) const { return fDeviceID; }
#endif //SONY_ROM

	private:
								EmDevice			(int);
		const DeviceInfo*		GetDeviceInfo		(void) const;
		int						GetDeviceID			(const char*) const;

		int						fDeviceID;
};

#ifdef SONY_ROM 
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
	kDeviceHandEra330,
	kDeviceVisor,
	kDeviceVisorPrism,
	kDeviceVisorPlatinum,
	kDeviceVisorEdge,

	kDevicePEGS500C,
	kDevicePEGS300,
	kDevicePEGN700C,
	kDevicePEGS320,
	kDevicePEGN600C,
	kDevicePEGT400,
	kDevicePEGT600,
	kDeviceYSX1100,
	kDeviceYSX1230,
	kDeviceLast
};
#endif //SONY_ROM

#ifdef	SONY_ROM
#include	"SonyShared\SonyDevice.h"
#endif	//SONY_ROM

#endif	/* EmDevice_h */
