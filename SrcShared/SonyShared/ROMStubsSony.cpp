#include "EmCommon.h"
#include "ROMStubs.h"

#include "ATraps.h"				// ATrap
#include "EmBankMapped.h"		// EmBankMapped::GetEmulatedAddress
#include "PalmPack.h"
#include "PalmPackPop.h"

#ifdef SONY_ROM

#define PushParm(p)	PushParameter (theTrap, p)

inline void PushParameter (ATrap& theTrap, uint32 p)	{ theTrap.PushLong (p); }
inline void PushParameter (ATrap& theTrap, int32 p)		{ theTrap.PushLong (p); }
inline void PushParameter (ATrap& theTrap, uint16 p)	{ theTrap.PushWord (p); }
inline void PushParameter (ATrap& theTrap, int16 p)		{ theTrap.PushWord (p); }
inline void PushParameter (ATrap& theTrap, uint8 p)		{ theTrap.PushByte (p); }
inline void PushParameter (ATrap& theTrap, int8 p)		{ theTrap.PushByte (p); }

template <typename T>
inline void PushParameter (ATrap& theTrap, const T* p)	{ theTrap.PushLong ((uint32) p); }

#define CallROM(trapWord)				\
	theTrap.Call (trapWord)

#define PUSH_VALUE(v)					\
	PushParm(v)

#define PUSH_PALM_PTR(p)				\
	PushParm((uint32) p)

#define PUSH_HOST_PTR(p)								\
	StMemoryMapper	mapper##p (p, sizeof (*(p)));		\
	PushParm(EmBankMapped::GetEmulatedAddress (p))


// ---------------------------------------------------------------------------
//		¥ Canonical_If_Not_Null
// ---------------------------------------------------------------------------
// Pointer parameters often need a little extra handling.  If they are not
// NULL, they generally point to either stack or heap buffers.  If they're on
// the stack, then on Windows they're in little-endian format and need to be
// converted into big-endian format.  Fortunately, we don't have to worry
// about the goofy word-swapping done for ROM and RAM memory, as the memory
// accessors that deal with data on the stack doesn't do any of that.
//
// For now, we don't actually check to see if data is on the stack or not.
// We just use this function on pointers that we _expect_ point to
// stack-resident data.  So far we haven't run into any problems with that
// approach.

template <class T>
inline void Canonical_If_Not_Null (T* p)
{
	if (p)
		Canonical (*p);
}
#pragma mark -


// ---------------------------------------------------------------------------
//		¥ Expansion Manager functions
// ---------------------------------------------------------------------------
#include "SonyShared\ExpansionMgr.h"

#define CallExpansionMgr(selector)		\
	theTrap.SetNewDReg (2, selector);	\
	theTrap.Call (sysTrapExpansionMgr)

Err ExpInit(void)
{
	ATrap	theTrap;
	CallExpansionMgr (expInit);
	return (Err) theTrap.GetD0 ();
}

Err ExpCardInserted(UInt16 slotRefNumber)
{
	// Call the ROM.
	ATrap	theTrap;
	PUSH_VALUE	(slotRefNumber);

	CallExpansionMgr (expCardInserted);

	return (Err) theTrap.GetD0 ();
}


Err ExpCardRemoved(UInt16 slotRefNumber)
{
	// Call the ROM.
	ATrap	theTrap;
	PUSH_VALUE	(slotRefNumber);

	CallExpansionMgr (expCardRemoved);

	return (Err) theTrap.GetD0 ();
}

Err ExpSlotLibFind(UInt16 slotRefNumber, UInt16 * slotLibRefNum)
{
	// Call the ROM.
	ATrap	theTrap;
	PUSH_VALUE		(slotRefNumber);
	PUSH_PALM_PTR	(slotLibRefNum);

	CallExpansionMgr (expSlotLibFind);

	return (Err) theTrap.GetD0 ();
}

// ---------------------------------------------------------------------------
//		¥ Date and Time calculations for MsFsLib
// ---------------------------------------------------------------------------
#include <DateTime.h>

UInt32 TimDateTimeToSeconds(DateTimePtr dateTimeP)
{
	Canonical_If_Not_Null (dateTimeP);

	ATrap theTrap;
	PUSH_HOST_PTR	(dateTimeP);
	CallROM (sysTrapTimDateTimeToSeconds);

	Canonical_If_Not_Null (dateTimeP);

	return (UInt32)  theTrap.GetD0 ();
}

void TimSecondsToDateTime(UInt32 seconds, DateTimePtr dateTimeP)
{
	Canonical_If_Not_Null (dateTimeP);

	ATrap theTrap;
	PUSH_HOST_PTR	(dateTimeP);
	PUSH_VALUE		(seconds);
	CallROM (sysTrapTimSecondsToDateTime);

	Canonical_If_Not_Null (dateTimeP);
}



#endif

