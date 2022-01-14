
#ifndef	_EMPATCHMODULE_EXPMGR_H
#define _EMPATCHMODULE_EXPMGR_H

#include "SonyShared\VFSMgr.h"

#include "EmPatchModule.h"

class EmPatchModule_ExpMgr : public EmPatchModule
{
	public:
								EmPatchModule_ExpMgr();
		virtual					~EmPatchModule_ExpMgr(){}
};

// ======================================================================
//	Patches for ExpansionManager functions
// ======================================================================

class ExpMgrLibHeadpatch
{
	public:
		static Bool GetExpMgrPatches( const SystemCallContext& context,
									  HeadpatchProc& hp,
									  TailpatchProc& tp);
		static CallROMType		HandleExpMgrCall(long selector);
		static Err				SetCardInfo(UInt16 nSlotRefNum, ExpCardInfoType* pCardInfo);

		// Eemurate functions
		static CallROMType		Init(void);
		static CallROMType		SlotDriverInstall(void);
		static CallROMType		SlotDriverRemove(void);
		static CallROMType		SlotLibFind(void);
		static CallROMType		SlotRegister(void);
		static CallROMType		SlotUnregister(void);
		static CallROMType		SlotPollingProc(void);
		static CallROMType		CardInserted(void);
		static CallROMType		CardRemoved(void);
		static CallROMType		CardPresent(void);
		static CallROMType		CardInfo(void);
		static CallROMType		SlotEnumerate(void);
};

class ExpMgrLibTailpatch
{
	public:
		static void		ExpInit(void);
};

#define	SLOT_COUNT				1				// ÉXÉçÉbÉgêî

#endif	// _EMPATCHMODULE_EXPMGR_H
