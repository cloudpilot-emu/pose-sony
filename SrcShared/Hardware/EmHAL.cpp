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

#include "EmCommon.h"
#include "EmHAL.h"

EmHALHandler*		EmHAL::fgRootHandler;


// ---------------------------------------------------------------------------
//		¥ EmHAL::AddHandler
// ---------------------------------------------------------------------------

void EmHAL::AddHandler (EmHALHandler* handler)
{
	EmAssert (handler->fNextHandler == NULL);
	EmAssert (handler->fPrevHandler == NULL);
	EmAssert (fgRootHandler == NULL || fgRootHandler->fPrevHandler == NULL);

	if (fgRootHandler != NULL)
	{
		fgRootHandler->fPrevHandler = handler;
		handler->fNextHandler = fgRootHandler;
	}

	fgRootHandler = handler;
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::RemoveHandler
// ---------------------------------------------------------------------------

void EmHAL::RemoveHandler (EmHALHandler* handler)
{
	if (handler->fNextHandler)
	{
		handler->fNextHandler->fPrevHandler = handler->fPrevHandler;
	}

	if (handler->fPrevHandler)
	{
		handler->fPrevHandler->fNextHandler = handler->fNextHandler;
	}
	else
	{
		fgRootHandler = handler->fNextHandler;
	}

	handler->fNextHandler = NULL;
	handler->fPrevHandler = NULL;

	EmAssert (handler->fNextHandler == NULL);
	EmAssert (handler->fPrevHandler == NULL);
	EmAssert (fgRootHandler == NULL || fgRootHandler->fPrevHandler == NULL);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::RemoveHandler
// ---------------------------------------------------------------------------

void EmHAL::EnsureCoverage (void)
{
#if 0
	// Rats...can't get this work...

	Bool			isHandled[20];
	EmHALHandler*	thisHandler = fgRootHandler;
	EmHALHandler	baseHandler;

	while (thisHandler)
	{
		typedef void (EmHALHandler::*CycleHandler)(Bool);
		CycleHandler	p1 = (thisHandler->Cycle);
		CycleHandler	p2 = (baseHandler.Cycle);

		if (p1 != p2)
		{
			isHandled[0] = true;
		}

		thisHandler = thisHandler->fNextHandler;
	}

	for (int ii = 0; ii < sizeof (isHandled); ++ii)
	{
		EmAssert (isHandled[ii]);
	}
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::Cycle
// ---------------------------------------------------------------------------

#if 0	// It's inline
void EmHAL::Cycle (Bool sleeping)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->Cycle (sleeping);
}
#endif


// ---------------------------------------------------------------------------
//		¥ EmHAL::CycleSlowly
// ---------------------------------------------------------------------------

void EmHAL::CycleSlowly (Bool sleeping)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->CycleSlowly (sleeping);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::ButtonEvent
// ---------------------------------------------------------------------------

void EmHAL::ButtonEvent (SkinElementType button, Bool buttonIsDown)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->ButtonEvent (button, buttonIsDown);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::TurnSoundOff
// ---------------------------------------------------------------------------

void EmHAL::TurnSoundOff (void)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->TurnSoundOff ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::ResetTimer
// ---------------------------------------------------------------------------

void EmHAL::ResetTimer (void)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->ResetTimer ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::ResetRTC
// ---------------------------------------------------------------------------

void EmHAL::ResetRTC (void)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->ResetRTC ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetInterruptLevel
// ---------------------------------------------------------------------------

int32 EmHAL::GetInterruptLevel (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetInterruptLevel ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetInterruptBase
// ---------------------------------------------------------------------------

int32 EmHAL::GetInterruptBase (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetInterruptBase ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetLCDScreenOn
// ---------------------------------------------------------------------------
// Called in various portXDataWrite methods to determine if Screen::InvalidateAll
// needs to be called.  Typically implemented in the EmRegs<Product> or
// EmRegs<LCDDriver> subclass.

Bool EmHAL::GetLCDScreenOn (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetLCDScreenOn ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetLCDBacklightOn
// ---------------------------------------------------------------------------
// Called in various portXDataWrite methods to determine if Screen::InvalidateAll
// needs to be called.  Typically implemented in the EmRegs<Product> or
// EmRegs<LCDDriver> subclass.

Bool EmHAL::GetLCDBacklightOn (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetLCDBacklightOn ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetLCDHasFrame
// ---------------------------------------------------------------------------
// Called by host LCD code to know if it needs to draw a 2-pixel white frame.
// Typically implemented in the EmRegs<Processor> or EmRegs<LCDDriver> subclass.

Bool EmHAL::GetLCDHasFrame (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetLCDHasFrame ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetLCDBeginEnd
// ---------------------------------------------------------------------------
// Called by Screen class in order to mark the memory used for the framebuffer.
// Typically implemented in the EmRegs<Processor> or EmRegs<LCDDriver> subclass.

void EmHAL::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->GetLCDBeginEnd (begin, end);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetLCDScanlines
// ---------------------------------------------------------------------------
// Fill in the output fields of EmScreenUpdateInfo.  Typically implemented in
// the EmRegs<Processor> or EmRegs<LCDDriver> subclass.

void EmHAL::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->GetLCDScanlines (info);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetIRPortOn
// ---------------------------------------------------------------------------

Bool EmHAL::GetIRPortOn (int /*uartNum*/)
{
#if 0
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetIRPortOn (uartNum);
#else
	return false;	// For now, now IR port handling.
#endif
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmHAL::GetSerialPortOn (int uartNum)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetSerialPortOn (uartNum);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetDynamicHeapSize
// ---------------------------------------------------------------------------

int32 EmHAL::GetDynamicHeapSize (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetDynamicHeapSize ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetROMSize
// ---------------------------------------------------------------------------

int32 EmHAL::GetROMSize (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetROMSize ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetROMBaseAddress
// ---------------------------------------------------------------------------

emuptr EmHAL::GetROMBaseAddress (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetROMBaseAddress ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::ChipSelectsConfigured
// ---------------------------------------------------------------------------

Bool EmHAL::ChipSelectsConfigured (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->ChipSelectsConfigured ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetSystemClockFrequency
// ---------------------------------------------------------------------------

int32 EmHAL::GetSystemClockFrequency (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetSystemClockFrequency ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetCanStop
// ---------------------------------------------------------------------------

Bool EmHAL::GetCanStop (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetCanStop ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetAsleep
// ---------------------------------------------------------------------------

Bool EmHAL::GetAsleep (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetAsleep ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetPortInputValue
// ---------------------------------------------------------------------------

uint8 EmHAL::GetPortInputValue (int port)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetPortInputValue (port);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetPortInternalValue
// ---------------------------------------------------------------------------

uint8 EmHAL::GetPortInternalValue (int port)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetPortInternalValue (port);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::PortDataChanged
// ---------------------------------------------------------------------------

void EmHAL::PortDataChanged (int port, uint8 oldVal, uint8 newVal)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->PortDataChanged (port, oldVal, newVal);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetKeyInfo
// ---------------------------------------------------------------------------

void EmHAL::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->GetKeyInfo (numRows, numCols, keyMap, rows);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::LineDriverChanged
// ---------------------------------------------------------------------------
// Tell the UART manager for the given UART that the host transport needs to
// be opened or closed.

void EmHAL::LineDriverChanged (int uartNum)
{
	EmAssert (EmHAL::GetRootHandler());
	EmHAL::GetRootHandler()->LineDriverChanged (uartNum);
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetVibrateOn
// ---------------------------------------------------------------------------

Bool EmHAL::GetVibrateOn (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetVibrateOn ();
}


// ---------------------------------------------------------------------------
//		¥ EmHAL::GetLEDState
// ---------------------------------------------------------------------------

uint16 EmHAL::GetLEDState (void)
{
	EmAssert (EmHAL::GetRootHandler());
	return EmHAL::GetRootHandler()->GetLEDState ();
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ EmHALHandler::EmHALHandler
// ---------------------------------------------------------------------------

EmHALHandler::EmHALHandler (void) :
	fNextHandler (NULL),
	fPrevHandler (NULL)
{
	EmHAL::AddHandler (this);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::~EmHALHandler
// ---------------------------------------------------------------------------

EmHALHandler::~EmHALHandler (void)
{
	EmHAL::RemoveHandler (this);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::Cycle
// ---------------------------------------------------------------------------

void EmHALHandler::Cycle (Bool sleeping)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->Cycle (sleeping);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::CycleSlowly
// ---------------------------------------------------------------------------

void EmHALHandler::CycleSlowly (Bool sleeping)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->CycleSlowly (sleeping);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::ButtonEvent
// ---------------------------------------------------------------------------

void EmHALHandler::ButtonEvent (SkinElementType button, Bool buttonIsDown)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->ButtonEvent (button, buttonIsDown);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::TurnSoundOff
// ---------------------------------------------------------------------------

void EmHALHandler::TurnSoundOff (void)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->TurnSoundOff ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::ResetTimer
// ---------------------------------------------------------------------------

void EmHALHandler::ResetTimer (void)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->ResetTimer ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::ResetRTC
// ---------------------------------------------------------------------------

void EmHALHandler::ResetRTC (void)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->ResetRTC ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetInterruptLevel
// ---------------------------------------------------------------------------

int32 EmHALHandler::GetInterruptLevel (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetInterruptLevel ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetInterruptBase
// ---------------------------------------------------------------------------

int32 EmHALHandler::GetInterruptBase (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetInterruptBase ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetLCDScreenOn (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetLCDScreenOn ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetLCDBacklightOn
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetLCDBacklightOn (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetLCDBacklightOn ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetLCDHasFrame
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetLCDHasFrame (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetLCDHasFrame ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetLCDBeginEnd
// ---------------------------------------------------------------------------

void EmHALHandler::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->GetLCDBeginEnd (begin, end);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetLCDScanlines
// ---------------------------------------------------------------------------

void EmHALHandler::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->GetLCDScanlines (info);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetIRPortOn
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetIRPortOn (int uartNum)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetIRPortOn (uartNum);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetSerialPortOn (int uartNum)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetSerialPortOn (uartNum);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetDynamicHeapSize
// ---------------------------------------------------------------------------

int32 EmHALHandler::GetDynamicHeapSize (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetDynamicHeapSize ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetROMSize
// ---------------------------------------------------------------------------

int32 EmHALHandler::GetROMSize (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetROMSize ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetROMBaseAddress
// ---------------------------------------------------------------------------

emuptr EmHALHandler::GetROMBaseAddress (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetROMBaseAddress ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::ChipSelectsConfigured
// ---------------------------------------------------------------------------

Bool EmHALHandler::ChipSelectsConfigured (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->ChipSelectsConfigured ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetSystemClockFrequency
// ---------------------------------------------------------------------------

int32 EmHALHandler::GetSystemClockFrequency (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetSystemClockFrequency ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetCanStop
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetCanStop (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetCanStop ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetAsleep
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetAsleep (void)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetAsleep ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetPortInputValue
// ---------------------------------------------------------------------------

uint8 EmHALHandler::GetPortInputValue (int port)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetPortInputValue (port);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetPortInternalValue
// ---------------------------------------------------------------------------

uint8 EmHALHandler::GetPortInternalValue (int port)
{
	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetPortInternalValue (port);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::PortDataChanged
// ---------------------------------------------------------------------------

void EmHALHandler::PortDataChanged (int port, uint8 oldVal, uint8 newVal)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->PortDataChanged (port, oldVal, newVal);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetKeyInfo
// ---------------------------------------------------------------------------

void EmHALHandler::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->GetKeyInfo (numRows, numCols, keyMap, rows);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::LineDriverChanged
// ---------------------------------------------------------------------------
// Tell the UART manager for the given UART that the host transport needs to
// be opened or closed.

void EmHALHandler::LineDriverChanged (int uartNum)
{
	EmAssert (this->GetNextHandler());
	this->GetNextHandler()->LineDriverChanged (uartNum);
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetVibrateOn
// ---------------------------------------------------------------------------

Bool EmHALHandler::GetVibrateOn (void)
{
	if (!this->GetNextHandler())
		return false;

	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetVibrateOn ();
}


// ---------------------------------------------------------------------------
//		¥ EmHALHandler::GetLEDState
// ---------------------------------------------------------------------------

uint16 EmHALHandler::GetLEDState (void)
{
	if (!this->GetNextHandler())
		return 0;

	EmAssert (this->GetNextHandler());
	return this->GetNextHandler()->GetLEDState ();
}
