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

#ifndef EmRegs328_h
#define EmRegs328_h

#include "EmHAL.h"				// EmHALHandler
#include "EmRegs.h"				// EmRegs
#include "EmStructs.h"			// RGBList
#include "EmUARTDragonball.h"	// EmUARTDragonball::State

class EmScreenUpdateInfo;


class EmRegs328 : public EmRegs, public EmHALHandler
{
	public:
								EmRegs328				(void);
		virtual					~EmRegs328				(void);

		// EmRegs overrides
		virtual void			Initialize				(void);
		virtual void			Reset					(Bool hardwareReset);
		virtual void			Save					(SessionFile&);
		virtual void			Load					(SessionFile&);
		virtual void			Dispose					(void);

		virtual void			SetSubBankHandlers		(void);
		virtual uint8*			GetRealAddress			(emuptr address);
		virtual emuptr			GetAddressStart			(void);
		virtual uint32			GetAddressRange			(void);

		// EmHALHandler overrides
		virtual void			Cycle					(Bool sleeping);
		virtual void			CycleSlowly				(Bool sleeping);

		virtual void			ButtonEvent				(SkinElementType, Bool buttonIsDown);
		virtual void			TurnSoundOff			(void);
		virtual void			ResetTimer				(void);
		virtual void			ResetRTC				(void);

		virtual int32			GetInterruptLevel		(void);
		virtual int32			GetInterruptBase		(void);

		virtual Bool			GetLCDScreenOn			(void) = 0;
		virtual Bool			GetLCDBacklightOn		(void) = 0;
		virtual Bool			GetLCDHasFrame			(void);
		virtual void			GetLCDBeginEnd			(emuptr&, emuptr&);
		virtual void			GetLCDScanlines			(EmScreenUpdateInfo& info);

		virtual Bool			GetSerialPortOn			(int uartNum) = 0;
		virtual int32			GetDynamicHeapSize		(void);
		virtual int32			GetROMSize				(void);
		virtual emuptr			GetROMBaseAddress		(void);
		virtual Bool			ChipSelectsConfigured	(void);
		virtual int32			GetSystemClockFrequency	(void);
		virtual Bool			GetCanStop				(void);
		virtual Bool			GetAsleep				(void);

		virtual uint8			GetPortInputValue		(int);
		virtual uint8			GetPortInternalValue	(int);
		virtual void			PortDataChanged			(int, uint8, uint8);

		virtual void			LineDriverChanged		(int uartNum);

	private:
		uint32					pllFreqSelRead			(emuptr address, int size);
		uint32					portXDataRead			(emuptr address, int size);
		uint32					tmr1StatusRead			(emuptr address, int size);
		uint32					tmr2StatusRead			(emuptr address, int size);
		uint32					uartRead				(emuptr address, int size);
		uint32					rtcHourMinSecRead		(emuptr address, int size);

		void					csASelect1Write			(emuptr address, int size, uint32 value);
		void					csCSelect0Write			(emuptr address, int size, uint32 value);
		void					csCSelect1Write			(emuptr address, int size, uint32 value);
		void					intMaskHiWrite			(emuptr address, int size, uint32 value);
		void					intMaskLoWrite			(emuptr address, int size, uint32 value);
		void					intStatusHiWrite		(emuptr address, int size, uint32 value);
		void					portXDataWrite			(emuptr address, int size, uint32 value);
		void					portDIntReqEnWrite		(emuptr address, int size, uint32 value);
		void					tmr1StatusWrite			(emuptr address, int size, uint32 value);
		void					tmr2StatusWrite			(emuptr address, int size, uint32 value);
		void					wdCounterWrite			(emuptr address, int size, uint32 value);
		void					spiMasterControlWrite	(emuptr address, int size, uint32 value);
		void					uartWrite				(emuptr address, int size, uint32 value);
		void					lcdRegisterWrite		(emuptr address, int size, uint32 value);
		void					rtcControlWrite			(emuptr address, int size, uint32 value);
		void					rtcIntStatusWrite		(emuptr address, int size, uint32 value);
		void					rtcIntEnableWrite		(emuptr address, int size, uint32 value);

	protected:
		void					HotSyncEvent			(Bool buttonIsDown);

		virtual uint8			GetKeyBits				(void);
		virtual uint16			ButtonToBits			(SkinElementType);

	protected:
		void					UpdateInterrupts		(void);
		void					UpdatePortDInterrupts	(void);
		void					UpdateRTCInterrupts		(void);

	protected:
		Bool					IDDetectAsserted		(void);
		UInt8					GetHardwareID			(void);

	protected:
		void					UARTStateChanged		(Bool sendTxData);
		void					UpdateUARTState			(Bool refreshRxData);
		void					UpdateUARTInterrupts	(const EmUARTDragonball::State& state);
		void					MarshalUARTState		(EmUARTDragonball::State& state);
		void					UnmarshalUARTState		(const EmUARTDragonball::State& state);

	protected:
		int						GetPort					(emuptr address);
		void					PrvGetPalette			(RGBList& thePalette);

	protected:
		HwrM68328Type			f68328Regs;
		bool					fHotSyncButtonDown;
		uint32					fTmr2CurrentMilliseconds;
		uint32					fTmr2StartMilliseconds;
		uint16					fKeyBits;
		uint16					fLastTmr1Status;
		uint16					fLastTmr2Status;
		uint8					fPortDEdge;

		uint32					fHour;
		uint32					fMin;
		uint32					fSec;
		uint32					fTick;
		uint32					fCycle;

		EmUARTDragonball*		fUART;
};

#endif	/* EmRegs328_h */
