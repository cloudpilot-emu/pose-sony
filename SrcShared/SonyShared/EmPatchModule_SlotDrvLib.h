
#ifndef	_EMPATCHMODULE_SLOTDRVLIB_H
#define _EMPATCHMODULE_SLOTDRVLIB_H

// ======================================================================
//	Patches for Slot Driver Liblary functions
// ======================================================================

#include "EmPatchModule.h"

class EmPatchModule_SlotDrvLib : public EmPatchModule
{
	public:
								EmPatchModule_SlotDrvLib();
		virtual					~EmPatchModule_SlotDrvLib(){}
};

class SlotDrvLibHeadpatch
{
	public:
		static ProtoPatchTableEntry* GetSlotDrvLibPatchesTable();

		static CallROMType		Selecter8(void);
		static CallROMType		Selecter11(void);
		static CallROMType		Selecter15(void);

		static CallROMType		Selecter4(void);
		static CallROMType		Selecter5(void);
		static CallROMType		Selecter6(void);
		static CallROMType		Selecter7(void);
		static CallROMType		Selecter9(void);
		static CallROMType		Selecter10(void);
};

#define	SlotDriverLibNameForPEGN700C	"Sony MsSlot Library"
#define	SlotDriverLibName				"Slot Driver Library"

#endif	// _EMPATCHMODULE_SLOTDRVLIB_H

