/******************************************************************************
 *
 * Copyright (c) 2000 Palm Computing, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: ExpansionMgr.h
 *
 * Description:
 *		Header file for Expansion Manager.
 *
 * History:
 *		02/25/00	jed	Created by Jesse Donaldson.
 *
 *****************************************************************************/

#ifndef __EXPANSIONMGR_H__
#define __EXPANSIONMGR_H__

#include <PalmTypes.h>
#include <CoreTraps.h>
#include <SystemMgr.h>

#ifdef BUILDING_AGAINST_PALMOS35
	#define sysTrapExpansionMgr	sysTrapSysReserved2

	#define expErrorClass			0x2900			// Post 3.5 this is defined in ErrorBase.h
	
	// Post 3.5 these are defined in NotifyMgr.h.
	#define sysNotifyCardInsertedEvent	'crdi'	// Broadcast when an ExpansionMgr card is 
																// inserted into a slot, and the slot driver 
																// calls ExpCardInserted.  Always broadcast
																// from UI task.
																// ExpansionMgr will play a sound & attempt to
																// mount a volume unless 'handled' is set
																// to true by a notification handler.
																// PARAMETER: slot number cast as void*
																
	#define sysNotifyCardRemovedEvent	'crdo'	// Broadcast when an ExpansionMgr card is 
																// removed from a slot, and the slot driver 
																// calls ExpCardRemoved.  Always broadcast
																// from UI task.
																// ExpansionMgr will play a sound & attempt to
																// unmount a volume unless 'handled' is set
																// to true by a notification handler.
																// PARAMETER: slot number cast as void*

	#define sysNotifyVolumeMountedEvent	'volm'	// Broadcast when a VFSMgr volume is 
																// mounted, Always broadcast
																// from UI task.
																// PARAMETER: VFSAnyMountParamPtr cast as void*

	#define sysNotifyVolumeUnmountedEvent 'volu'	// Broadcast when a VFSMgr volume is 
																// unmounted, Always broadcast
																// from UI task.
																// PARAMETER: volume refNum cast as void*

#else
	#define sysTrapExpansionMgr	sysTrapExpansionDispatch
#endif


#ifndef USE_EXPMGR_TRAPS
	#if EMULATION_LEVEL == EMULATION_NONE
		#define	USE_EXPMGR_TRAPS	1
	#else
		#define	USE_EXPMGR_TRAPS	0
	#endif
#endif


#ifdef BUILDING_EXPMGR_DISPATCH
	#define EXPMGR_TRAP(expMgrSelectorNum)
#else
	#define EXPMGR_TRAP(sel) \
		_SYSTEM_API(_CALL_WITH_SELECTOR)(_SYSTEM_TABLE, sysTrapExpansionMgr, sel)
#endif



/********************************************************************
 * Type of Expansion Manager database
 ********************************************************************/
#define sysFileCExpansionMgr			'expn'

/********************************************************************
 * Type of SlotDrvr Library database
 ********************************************************************/
#define		sysFileTSlotDriver		'libs'			// file type for slot driver libraries



#define expFtrIDVersion				0	// ID of feature containing version of ExpansionMgr.
												// Check existence of this feature to see if ExpMgr is installed.

#define expMgrVersionNum			((UInt16)1)	// version of the ExpansionMgr, obtained from the feature

#define invalidSlotRefNum			0

typedef Err (*ExpPollingProcPtr)(UInt16 slotLibRefNum, 
			void *slotPollRefConP);


/************************************************************
 * Capabilities of the hardware device for ExpCardInfoType.capabilityFlags
 *************************************************************/
#define	expCapabilityHasStorage		0x00000001	// card supports reading (& maybe writing) sectors
#define	expCapabilityReadOnly		0x00000002	// card is read only

#define expCardInfoStringMaxLen		31

typedef struct ExpCardInfoTag
{
	UInt32	capabilityFlags;	// bits for different stuff the card supports
	Char		manufacturerStr[expCardInfoStringMaxLen+1];	// Manufacturer, e.g., "Palm", "Motorola", etc...
	Char		productStr[expCardInfoStringMaxLen+1];			// Name of product, e.g., "SafeBackup 32MB"
	Char		deviceClassStr[expCardInfoStringMaxLen+1];	// Type of product, e.g., "Backup", "Ethernet", etc.
	Char		deviceUniqueIDStr[expCardInfoStringMaxLen+1];// Unique identifier for product, e.g., a serial number.  Set to "" if no such identifier exists.
}	ExpCardInfoType, *ExpCardInfoPtr;


/************************************************************
 * Iterator start and stop constants.
 * Used by FSVolumeEnumerate, VFSDirEntryEnumerate, FSDirEntryEnumerate and ExpSlotEnumerate
 *************************************************************/
#define expIteratorStart              0L
#define expIteratorStop               0xffffffffL


/************************************************************
 * Error codes
 *************************************************************/
#define expErrUnsupportedOperation			(expErrorClass | 1)		// unsupported or undefined opcode and/or creator
#define expErrNotEnoughPower					(expErrorClass | 2)		// the required power is not available

#define expErrCardNotPresent					(expErrorClass | 3)		// no card is present
#define expErrInvalidSlotRefNumber			(expErrorClass | 4)		// slot reference number is bad
#define expErrSlotDeallocated					(expErrorClass | 5)		// slot reference number is within valid range, but has been deallocated.
#define expErrCardNoSectorReadWrite			(expErrorClass | 6)		// the card does not support the 
																						// SlotDriver block read/write API
#define expErrCardReadOnly						(expErrorClass | 7)		// the card does support R/W API
																						// but the card is read only
#define expErrCardBadSector					(expErrorClass | 8)		// the card does support R/W API
																						// but the sector is bad
#define expErrCardProtectedSector			(expErrorClass | 9)		// The card does support R/W API
																						// but the sector is protected
#define expErrNotOpen							(expErrorClass | 10)		// slot driver library has not been opened
#define expErrStillOpen							(expErrorClass | 11)		// slot driver library is still open - maybe it was opened > once
#define expErrUnimplemented					(expErrorClass | 12)		// Call is unimplemented
#define expErrEnumerationEmpty				(expErrorClass | 13)		// No values remaining to enumerate


/************************************************************
 * Common media types.  Used by SlotCardMediaType and SlotMediaType.
 *************************************************************/
#define ExpMediaType_Any				'wild'	// matches all media types when looking up a default directory
#define ExpMediaType_MemoryStick		'mstk'
#define ExpMediaType_CompactFlash	'cfsh'
#define ExpMediaType_SecureDigital	'sdig'
#define ExpMediaType_MultiMediaCard	'mmcd'
#define ExpMediaType_SmartMedia		'smed'
#define ExpMediaType_RAMDisk			'ramd'	// a RAM disk based media
#define ExpMediaType_PoserHost		'pose'	// Host filesystem emulated by Poser


/************************************************************
 * Selectors for routines found in the Expansion manager. The order
 * of these selectors MUST match the jump table in ExpansionMgr.c.
 *************************************************************/
typedef enum {
	expInit = 0,
	expSlotDriverInstall,	// 1
	expSlotDriverRemove,	// 2
	expSlotLibFind,			// 3
	expSlotRegister,		// 4
	expSlotUnregister,		// 5
	expCardInserted,		// 6
	expCardRemoved,			// 7
	expCardPresent,			// 8
	expCardInfo,			// 9
	expSlotEnumerate,		// 10
	
	expMaxSelector = expSlotEnumerate,
	expBigSelector = 0x7FFF	// Force ExpansionMgrSelector to be 16 bits.
} ExpansionMgrSelector;


Err ExpInit(void)
		EXPMGR_TRAP(expInit);

Err ExpSlotDriverInstall(UInt32 dbCreator, UInt16 *slotLibRefNumP)
		EXPMGR_TRAP(expSlotDriverInstall);

Err ExpSlotDriverRemove(UInt16 slotLibRefNum)
		EXPMGR_TRAP(expSlotDriverRemove);

Err ExpSlotLibFind(UInt16 slotRefNumber, UInt16 *slotLibRefNum)
		EXPMGR_TRAP(expSlotLibFind);

Err ExpSlotRegister(UInt16 slotLibRefNum, UInt16 *slotRefNum)
		EXPMGR_TRAP(expSlotRegister);

Err ExpSlotUnregister(UInt16 slotRefNumber)
		EXPMGR_TRAP(expSlotUnregister);

Err ExpCardInserted(UInt16 slotRefNumber)
		EXPMGR_TRAP(expCardInserted);

Err ExpCardRemoved(UInt16 slotRefNumber)
		EXPMGR_TRAP(expCardRemoved);

Err ExpCardPresent(UInt16 slotRefNumber)
		EXPMGR_TRAP(expCardPresent);

Err ExpCardInfo(UInt16 slotRefNumber, ExpCardInfoType* infoP)
		EXPMGR_TRAP(expCardInfo);

Err ExpSlotEnumerate(UInt16 *slotRefNumP, UInt32 *slotIteratorP)
		EXPMGR_TRAP(expSlotEnumerate);



#endif	// __EXPANSIONMGR_H__
