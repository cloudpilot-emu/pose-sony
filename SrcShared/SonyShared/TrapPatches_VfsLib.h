
#ifndef	_TRAPPATCHES_VFSMGR_H
#define _TRAPPATCHES_VFSMGR_H

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

#endif	// _TRAPPATCHES_VFSMGR_H
