
#ifndef	_EMPATCHMODULE_VFSMGR_H
#define _EMPATCHMODULE_VFSMGR_H

#include "EmPatchModule.h"

class EmPatchModule_VfsLib : public EmPatchModule
{
	public:
								EmPatchModule_VfsLib();
		virtual					~EmPatchModule_VfsLib(){}
};

// ======================================================================
//	Patches for VFS Manager functions
// ======================================================================

class VfsMgrLibHeadpatch
{
	public:
		static Bool GetVfsMgrPatches( const SystemCallContext& context,
									  HeadpatchProc& hp,
									  TailpatchProc& tp);
		static CallROMType		HandleVfsMgrCall(long selector);

		static CallROMType		VfsLibTrapVolumeMount(void);
		static CallROMType		VfsLibTrapVolumeSize(void);
		static CallROMType		VfsLibTrapVolumeEnumerate(void);
/*
	{vfsTrapImportDatabaseFromFile,		VfsLibHeadpatch::VfsLibTrapImportDatabaseFromFile,	NULL},
	{vfsTrapExportDatabaseToFile,		VfsLibHeadpatch::VfsLibTrapExportDatabaseToFile,	NULL},
*/
};

#endif	// _EMPATCHMODULE_VFSMGR_H
