/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2000 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef _PLATFORM_MSFSLIB_H_
#define _PLATFORM_MSFSLIB_H_

#include "SonyShared\ExpansionMgr.h"
#include "SonyShared\VFSMgr.h"
#include "SessionFile.h"		// SessionFile

class Platform_MsfsLib
{
   public:
		static void		Initialize(const Configuration& cfg);
		static void		Initialize();
		static void		Load(SessionFile& f);
		static void		Save(SessionFile& f);
		static void		Reset();

		static Err		GetVfsErrorCode (Err defaultErrCode);
		static Bool		CardIsWriteProtect(UInt16 nVolRefNum = 0);
		static UInt16	GetSlotLibRefNum(void);

        static Err     LibOpen                 (UInt16 libRef);
        static Err     LibClose                (UInt16 libRef);
        static Err     LibSleep                (UInt16 libRef);
        static Err     LibWake                 (UInt16 libRef);
 
		static Err		FSFileCreate			(UInt16 volRefNum, char *pPathName);
		static Err		FSFileOpen				(UInt16 volRefNum, char *pPathName, UInt16 openMode, FileRef* pFileRef);
		static Err		FSFileClose				(FileRef nFileRef);
		static Err		FSFileReadData			(FileRef nFileRef, UInt32 nNumBytes, void*pBufBase, UInt32 nOffset, UInt32* pnNumByteRead);
		static Err		FSFileRead				(FileRef nFileRef, UInt32 nNumBytes, void*pBufBase, UInt32* pnNumByteRead);
		static Err		FSFileWrite				(FileRef nFileRef, UInt32 nNumBytes, void*pBufBase, UInt32* pnNumByteWrite);
		static Err		FSFileDelete			(UInt16 volRefNum, char* pPathName);
		static Err		FSFileRename			(UInt16 volRefNum, char* pPathName, char* pNewName);
		static Err		FSFileSeek				(FileRef nFileRef, FileOrigin nOrigin, Int32 nOffset);
		static Err		FSFileEOF				(FileRef nFileRef);
		static Err		FSFileTell				(FileRef nFileRef, UInt32 *pFilePos);
		static Err		FSFileAttributesGet		(FileRef nFileRef, UInt32 *pAttr);
		static Err		FSFileAttributesSet		(FileRef nFileRef, UInt32 nAttr);
		static Err		FSFileDateGet			(FileRef nFileRef, UInt16 nWhichDate, UInt32* pDate);
		static Err		FSFileDateSet			(FileRef nFileRef, UInt16 nWhichDate, UInt32 nDate);

		static Err		FSFileSize				(FileRef nFileRef, UInt32 *pFileSize);
		static Err		FSFileResize			(FileRef nFileRef, UInt32 nNewSize);
		static Err		FSDirCreate				(UInt16 nVolRefNum, char *pDirName);
		static Err		FSDirEntryEnumerate		(FileRef nDirRef, UInt32 *pDir, FileInfoType *pInfo);
		static Err		FSVolumeFormat			(VFSAnyMountParamPtr pVfsMountParam);
		static Err		FSVolumeInfo			(UInt16 nVolRefNum, VolumeInfoType* pVolInfo);
		static Err		FSVolumeMount			(VFSAnyMountParamPtr pVfsMountParam);
		static Err		FSVolumeUnmount			(UInt16 nVolRefNum);
		static Err		FSVolumeSize			(UInt16 nVolRefNum, UInt32* pVolUsed, UInt32* pVolTotal);

		static Err		FSVolumeLabelGet		(UInt16 nVolRefNum, LPSTR pVolumeLabel, UInt16 nBufLen);
		static Err		FSVolumeLabelSet		(UInt16 nVolRefNum, LPSTR pVolumeLabel);
};

#define	MSSTATE_REMOVED			-1	// MSカードのREMOVED状態
#define	MSSTATE_REQ_REMOVE		-5	// MSカードのREMOVE要求
									// 電源OFF中にMSカードが抜かれた場合にセットされる。電源ONになった時、
									// この値がセットされていれば、ExpCardRemovedを実行する。
#define	MSSTATE_REQ_INSERT		-6	// MSカードのINSERTE要求
									// 電源OFF中にMSカードが挿入された場合にセットされる。電源ONになった時、
									// この値がセットされていれば、ExpCardInsertedを実行する。
#define	MSSTATE_UNKNOWN			-10	// Unknown


extern Int16	g_nCardInserted;	// メモリステイックのINSERT/REMOVEの状態
									// -1:REMOVE    0以上:INSERT

#endif   // _PLATFORM_MSFSLIB_H_
