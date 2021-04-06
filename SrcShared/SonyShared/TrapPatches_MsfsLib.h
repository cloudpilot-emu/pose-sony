
#ifndef	_TRAPPATCHES_MSFSLIB_H
#define _TRAPPATCHES_MSFSLIB_H
// ======================================================================
//	Patches for MSFS(Memory Stick File System) Liblary functions
// ======================================================================

class MsfsLibHeadpatch
{
	public:
		static ProtoPatchTableEntry* GetMsfsLibPatchesTable();

		static CallROMType		LibOpen	(void);
		static CallROMType		LibClose (void);
		static CallROMType		LibSleep (void);
		static CallROMType		LibWake (void);

//		static CallROMType		FSTrapLibAPIVersion(void);
//		static CallROMType		FSTrapCustomControl(void);
		static CallROMType		FSTrapFilesystemType(void);

		static CallROMType		FSTrapFileCreate(void);
		static CallROMType		FSTrapFileOpen(void);
		static CallROMType		FSTrapFileClose(void);
		static CallROMType		FSTrapFileRead(void);
		static CallROMType		FSTrapFileWrite(void);
		static CallROMType		FSTrapFileDelete(void);
		static CallROMType		FSTrapFileRename(void);
		static CallROMType		FSTrapFileSeek(void);
		static CallROMType		FSTrapFileEOF(void);
		static CallROMType		FSTrapFileTell(void);
		static CallROMType		FSTrapFileResize(void);
		static CallROMType		FSTrapFileAttributesGet(void);
		static CallROMType		FSTrapFileAttributesSet(void);
		static CallROMType		FSTrapFileDateGet(void);
		static CallROMType		FSTrapFileDateSet(void);
		static CallROMType		FSTrapFileSize(void);

		static CallROMType		FSTrapDirCreate(void);
		static CallROMType		FSTrapDirEntryEnumerate(void);

		static CallROMType		FSTrapVolumeFormat(void);
		static CallROMType		FSTrapVolumeMount(void);
		static CallROMType		FSTrapVolumeUnmount(void);
		static CallROMType		FSTrapVolumeInfo(void);
		static CallROMType		FSTrapVolumeLabelGet(void);
		static CallROMType		FSTrapVolumeLabelSet(void);
		static CallROMType		FSTrapVolumeSize(void);
};

#define	MsFsLibNameForPEGN700C	"Sony MsFs Library"
#define	MsFsLibName				"MS FileSystem Library"
#define	SoundLibName			"Sony Sound Library"
#define	IrcLibName				"Sony Irc Library"

#endif	// _TRAPPATCHES_MSFSLIB_H

