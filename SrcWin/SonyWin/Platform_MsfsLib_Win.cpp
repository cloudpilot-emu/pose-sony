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

#include "EmCommon.h"

#include "Emulator.h"			// gInstance
#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"
#include "Platform.h"			// AllocateMemory
#include "UAE_Utils.h"
#include "Platform_MsfsLib.h"
#include "EmLowMem.h"			// EmLowMem_GetGlobal
#include "EmWindow.h"			// EmWindow
#include "EmWindowWin.h"		// EmWindowHostData
#include "SonyShared/TrapPatches_SlotDrvLib.h" 

#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <tchar.h>

#define MAX_FFS_PATH			256

#include "OpenFileList.h"		// for Sony & MemoryStick

void	LCD_SetStateJogButton(SkinElementType witch, Bool bPress, Bool bEnabled);	// LCDSony.inl
void	LCD_DrawButtonForPEG(HDC hDC, SkinElementType witch);

#define	PALM_DIR				"PALM"
#define	PROGRAMS_DIR			"PALM\\PROGRAMS"
#define	INFO_DIR				"PALM\\INFO"
#define	BACKUP_DIR				"PALM\\BACKUP"
#define	VOLLABEL_LONG_FILE		"PALM\\volInfo.txt"
#define	VOLLABEL_SHORT_FILE		"volInfo.txt"

Int16	g_nCardInserted = MSSTATE_UNKNOWN;		// メモリステイックのINSERT/REMOVEの状態
												// MSSTATE_REMOVED(-1):REMOVED
												// 0以上              :INSERTED

static	Bool					g_bReadyMsfsLib = false;
static	int						g_nMsHomeLength;
static	char					g_szMsHome[MAX_FFS_PATH + 1];
static	char					g_szVolumeLabel[MAX_FFS_PATH + 1];
static	DWORD					g_nSavedErrno = 0;
static	UInt32					g_nUsedSize, g_nTotalSize;
static	Bool					g_bUsedSizeUpdated = false;

static	VFSSlotMountParamType	g_infMountParam;
static	OpenFileList			g_lstOpenFiles;
static	struct _finddata_t		g_infFindData;
static	int						g_nCurrentDrv = 0;

const char *gszFileNameIllegals = "\"*/:;,<>?\\|";		// " * / : ; , < > ? \ |             ---- (+ \005)

#define PRINTF	if (1) ; else LogAppendMsg

extern	UInt32 TimDateTimeToSeconds(DateTimePtr dateTimeP);
extern	void TimSecondsToDateTime(UInt32 seconds, DateTimePtr dateTimeP);

Int32	CreateDir(char *pDirName);

void ENtoSLASH (LPSTR pPathName)
{
	while (*pPathName)
	{
		if (*pPathName == '\\')
			*pPathName = '/';
		*pPathName++;
	}
}

Err	CheckFileName(LPSTR pName, LPSTR pChgName)
{
	if (!pName || !strlen(pName))
		return vfsErrBadName;

	char	szTotal[_MAX_PATH], szFile[_MAX_PATH], szExt[_MAX_PATH];

	// BugFix-2000/11/30-maki
	// ルートディレクトリがオープンできないバグの修正
	if (strlen(pName) == 1 && (pName[0] == '/' || pName[0] == '\\'))
	{
		strcpy(pChgName, ".");
		return errNone;
	}

	_splitpath(pName, NULL, NULL, szFile, szExt);
	_makepath (szTotal, NULL, NULL, szFile, szExt);

	if (strpbrk(szTotal, gszFileNameIllegals))
		return vfsErrBadName;

	if (strlen(szTotal) > 0 && strspn(szTotal, ".") == strlen(szTotal))
		return vfsErrBadName;

	strcpy(szTotal, pName);
	ENtoSLASH(szTotal);		// '\\'を'/'へ変換する
	if (strstr(szTotal, "./"))
		return vfsErrBadName;

	// "\\\\"または"//"によるディレクトリ指定は無効だが、エラーはvfsErrDirectoryNotFoundを返す
	if (strstr(szTotal, "//"))
		return vfsErrDirectoryNotFound;

	// MSディレクトリをカレントディレクトリとした場合に扱えるように、先頭の'/'を削除する
	if (pChgName)
	{
		strcpy(pChgName, (szTotal[0] == '/') ? &szTotal[1] : szTotal);
		// 末尾の'/'も削除する
		if (pChgName[strlen(pChgName)-1] == '/')
			pChgName[strlen(pChgName)-1] = NULL;
	}
	return errNone;
}		

void ChangeToMSDrvDir()
{
	g_nCurrentDrv = _getdrive();
	int	nDrv = g_szMsHome[0];
	if (islower(nDrv))
		nDrv = _toupper(nDrv);
	int nResult = _chdrive(nDrv - 'A' + 1);
	nResult = _chdir(g_szMsHome);
}

void RestoreDrv()
{
	if (g_nCurrentDrv)
		_chdrive(g_nCurrentDrv);
}

/************************************************************
 *  FUNCTION    : IsExistFile (char* pPathName, WIN32_FIND_DATA* pFileInfo)
 *
 *  DESCRIPTION : ファイル検索
 *              
 *  PARAMETERS  : pPathName ... 検索するファイル名
 *				  pFileInfo ... ファァイル情報格納領域（省略可）
 *              
 *  RETURNS     : TRUE  ... ファイルが存在する
 *				  FALSE ... ファイルが見つからない
 *************************************************************/
BOOL IsExistFile (LPCSTR pPathName, struct _finddata_t *pFileInfo=NULL)
{
	long				hFind;
	struct _finddata_t 	fileInfo;
	char				szPath[_MAX_PATH];
	strcpy(szPath, pPathName);
	if (szPath[strlen(szPath)-1] == '\\' || szPath[strlen(szPath)-1] == '/')
		szPath[strlen(szPath)-1] = '\0';

	hFind = _findfirst(szPath, &fileInfo);
	if (hFind == -1)
		return FALSE;
	_findclose(hFind);
	if (pFileInfo)
		*pFileInfo = fileInfo;
	return TRUE;
}

/************************************************************
 *  FUNCTION    : GetSizeInDir (char* pDirName)
 *
 *  DESCRIPTION : ディレクトリの使用サイズの取得
 *              
 *  PARAMETERS  : pDirName ... サイズを取得するディレクトリ名
 *              
 *  RETURNS     : 使用サイズ
 *************************************************************/
UInt32	GetSizeInDir(char *pDirName)
{
	char				szPath[MAX_FFS_PATH + 1];
	char				szSubDir[MAX_FFS_PATH + 1];
	long				hFind, hSaveFind;
    struct _finddata_t	cInfo;
	UInt32				nSize = 0;

	strcpy(szPath, pDirName);
	if (szPath[strlen(szPath)-1] != '\\' && szPath[strlen(szPath)-1] != '/')
			strcat(szPath, "\\");
	strcat(szPath, "*.*");

	hFind = hSaveFind = _findfirst(szPath, &cInfo);
	while (hFind != -1)
	{
		if (cInfo.attrib & _A_SUBDIR)
		{
			if (strcmp(cInfo.name, ".") && strcmp(cInfo.name, ".."))
			{
				strcpy(szSubDir, pDirName);
				if (szSubDir[strlen(szSubDir)-1] != '\\' && szSubDir[strlen(szSubDir)-1] != '/')
					strcat(szSubDir, "\\");
				strcat(szSubDir, cInfo.name);
				nSize += GetSizeInDir(szSubDir);
			}
		}
		else 
		{
			// MS\volInfo.txtのファイルサイズはカウントしない
			if (stricmp(pDirName, g_szMsHome) || stricmp(cInfo.name, VOLLABEL_SHORT_FILE))
				nSize += cInfo.size;	
		}
		hFind = _findnext(hSaveFind, &cInfo);
	}
	if (hSaveFind != -1)
		_findclose(hSaveFind);

	return nSize;
}


/************************************************************
 *  FUNCTION    : CreateDir (char* pDirName)
 *
 *  DESCRIPTION : ディレクトリの作成
 *              
 *  PARAMETERS  : pDirName ... 作成するディレクトリ名
 *              
 *  RETURNS     : エラー情報
 *************************************************************/
Int32	CreateDir(char *pDirName)
{
	char				szPath[MAX_FFS_PATH + 1];
//	char				szSubDir[MAX_FFS_PATH + 1];
//	char				szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
//	long				hSaveFind;
	char				*p;
	size_t				len;

	if (::IsExistFile(pDirName))
		return 0;

	p = strstr(pDirName, g_szMsHome);
	if (!p)
		return -1;

	szPath[0] = NULL;
	strncat(szPath, pDirName, strlen(g_szMsHome)+1);
	p += strlen(g_szMsHome) + 1;
	do 
	{
		if (len = strcspn(p, "/\\"))
		{
			strncat(szPath, p, len);
			if (!::IsExistFile(szPath))
			{
				SECURITY_ATTRIBUTES	dirAttr;
				dirAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
				dirAttr.lpSecurityDescriptor = NULL;
				dirAttr.bInheritHandle = FALSE;
				::CreateDirectory(szPath, &dirAttr);
			}
			p += len;
			strncat(szPath, p, 1);
			p ++;
		}

	} while (strlen(pDirName) != strlen(szPath) && len);
	return 0;
}


/************************************************************
 *  FUNCTION    : DeleteDir (char* pDirName)
 *
 *  DESCRIPTION : ディレクトリの削除
 *				  指定ディレクトリ下のサブディレクトリも削除する
 *				  ただし、bDeleteOwn=falseの場合、pDirName下のファイルおよび
 *				  ディレクトリを削除し、pDirNameは残す
 *              
 *  PARAMETERS  : pDirName   ... 削除するディレクトリ名
 *				  bDeleteOwn ... pDirNameディレクトリ自体を削除するかどうか
 *              
 *  RETURNS     : エラー情報
 *************************************************************/
Int32	DeleteDir(char *pDirName, BOOL bDeleteOwn = true)
{
	char				szPath[MAX_FFS_PATH + 1];
	char				szSubDir[MAX_FFS_PATH + 1];
	long				hFind, hSaveFind;
    struct _finddata_t	cInfo;
	UInt32				nSize = 0;
	int					nResult = 0;

	strcpy(szPath, pDirName);
	if (szPath[strlen(szPath)-1] != '\\' && szSubDir[strlen(szSubDir)-1] != '/')
			strcat(szPath, "\\");
	strcat(szPath, "*.*");

	hFind = hSaveFind = _findfirst(szPath, &cInfo);
	while (hFind != -1)
	{
		strcpy(szSubDir, pDirName);
		if (szSubDir[strlen(szSubDir)-1] != '\\' && szSubDir[strlen(szSubDir)-1] != '/')
			strcat(szSubDir, "\\");
		strcat(szSubDir, cInfo.name);
		if (cInfo.attrib & _A_SUBDIR)
		{
			if (strcmp(cInfo.name, ".") && strcmp(cInfo.name, ".."))
			{
				nResult = DeleteDir(szSubDir);
				if (nResult)
					break;
			}
		}
		else
		{
			if (cInfo.attrib & _A_RDONLY)
				_chmod(szSubDir, _S_IREAD | _S_IWRITE );
			nResult = remove(szSubDir);
			if (nResult)
				break;
			nSize += cInfo.size;
		}
		hFind = _findnext(hSaveFind, &cInfo);
	}
	if (hSaveFind != -1)
		_findclose(hSaveFind);

	if (!nResult && bDeleteOwn)
		nResult = _rmdir(pDirName);
	return nResult;
}


static char* ActualMsPath (char* pFileName)
{
	static char szActualPath[MAX_FFS_PATH * 2 + 1];

	strcpy (szActualPath, g_szMsHome);
	if (pFileName[0] != '\\' && pFileName[0] != '/')
		strcat (szActualPath, "\\");

	strcat (szActualPath, pFileName);
	return szActualPath;
}

Err Platform_MsfsLib::GetVfsErrorCode (Err defaultErrCode)
{
	Err	retval = defaultErrCode;
	switch (g_nSavedErrno)
	{
	case EACCES:    retval = vfsErrFilePermissionDenied; break;		// ファイルがオープンできない
	case EEXIST:    retval = vfsErrFileAlreadyExists; break;		// ファイルが既に存在する
	case ENOENT:    retval = vfsErrFileNotFound; break;				// ファイルが存在しない
	case EBADF :    retval = vfsErrFileBadRef; break;				// ファイルがオープンされていない
	case EINVAL:    retval = vfsErrBadName; break;					// ファイル名に無効な文字が使われている
	case ENOTEMPTY: retval = vfsErrDirNotEmpty; break;				// ディレクトリが空でない
	}
	return retval;
}

/************************************************************
 *  FUNCTION    : Initialize()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     :
 *************************************************************/
void Platform_MsfsLib::Initialize ()
{
	Preference<Configuration>	prefConfiguration (kPrefKeyLastConfiguration);
	Configuration				cfg = *prefConfiguration;
	Platform_MsfsLib::Initialize(cfg);
}

void Platform_MsfsLib::Initialize(const Configuration& cfg)
{
	char	szDrive[_MAX_DRIVE], szDir[_MAX_DIR];

	g_bUsedSizeUpdated = false;
	g_szVolumeLabel[0] = NULL;
	g_nCardInserted = MSSTATE_UNKNOWN;

	::GetModuleFileName (gInstance, g_szMsHome, _MAX_PATH);
	_splitpath (g_szMsHome, szDrive, szDir, NULL, NULL);
	_makepath (g_szMsHome, szDrive, szDir, NULL, NULL);

	switch (cfg.fMSSize)
	{
	case   8: _tcscat(g_szMsHome, "MS8");   break;
	case  16: _tcscat(g_szMsHome, "MS16");  break;
	case  32: _tcscat(g_szMsHome, "MS32");  break;
	case  64: _tcscat(g_szMsHome, "MS64");  break;
	case 128: _tcscat(g_szMsHome, "MS128"); break;
	default:  _tcscat(g_szMsHome, "MS"); break;
	}
	
	g_nMsHomeLength = _tcslen(g_szMsHome);

	// メモリスティック用ディレクトリの作成
	if (!IsExistFile (g_szMsHome))
	{
		SECURITY_ATTRIBUTES	dirAttr;
		dirAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		dirAttr.lpSecurityDescriptor = NULL;
		dirAttr.bInheritHandle = FALSE;
		if (!::CreateDirectory(g_szMsHome, &dirAttr))
		{
			g_bReadyMsfsLib = FALSE;
			return;
		}
	}

	// ボリュームラベルの取得
	char	*pszLabel = ::ActualMsPath(VOLLABEL_SHORT_FILE);
	if (!::IsExistFile (pszLabel))
	{
		pszLabel = ::ActualMsPath(VOLLABEL_LONG_FILE);
		if (!::IsExistFile (pszLabel))
			pszLabel = NULL;
	}
	if (pszLabel)
	{
		int	nFile;
		nFile = _open(pszLabel, 
				_O_BINARY|_O_RDWR,
				_S_IREAD | _S_IWRITE);
		if (nFile != -1)
		{
			long nReadNum = _read(nFile, g_szVolumeLabel, 256);
			_close(nFile);
			if (nReadNum != -1)
				g_szVolumeLabel[nReadNum] = NULL;
		}
	}
	g_bReadyMsfsLib = TRUE;

	g_lstOpenFiles.ClearAllInfo();
}

/************************************************************
 *  FUNCTION    : Load()
 *
 *  DESCRIPTION : セッションファイル(PSF)からのMSライブラリ情報の取得
 *              
 *  PARAMETERS  : 
 *              
 *  RETURNS     : なし
 *************************************************************/
void Platform_MsfsLib::Load (SessionFile& f)
{
	Chunk	chunk;
	if (f.ReadMSfsInfo (chunk))
	{
		long			version;
		EmStreamChunk	s (chunk);

		s >> version;

		s >> g_nCardInserted;
		s >> g_infMountParam.vfsMountParam.volRefNum;
		s >> g_infMountParam.vfsMountParam.reserved;
		s >> g_infMountParam.vfsMountParam.mountClass;
		s >> g_infMountParam.slotLibRefNum;
		s >> g_infMountParam.slotRefNum;
	}
	else
	{
		f.SetCanReload (false);
	}
	
	::LCD_SetStateJogButton(kElement_MS_InOut, (g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE) ? false : true, true);
//	::LCD_DrawButtonForPEG(NULL, kElement_MS_InOut);
}

/************************************************************
 *  FUNCTION    : Save()
 *
 *  DESCRIPTION : セッションファイル(PSF)へのMSライブラリ情報の出力
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : なし
 *************************************************************/
void Platform_MsfsLib::Save (SessionFile& f)
{
	const long	kCurrentVersion = 1;

	Chunk			chunk;
	EmStreamChunk	s (chunk);

	s << kCurrentVersion;

	s << g_nCardInserted;
	s << g_infMountParam.vfsMountParam.volRefNum;
	s << g_infMountParam.vfsMountParam.reserved;
	s << g_infMountParam.vfsMountParam.mountClass;
	s << g_infMountParam.slotLibRefNum;
	s << g_infMountParam.slotRefNum;

	f.WriteMSfsInfo (chunk);
}

void Platform_MsfsLib::Reset ()
{
}

//--------------------------------------------------
// Library initialization, shutdown, sleep and wake
//--------------------------------------------------

Err Platform_MsfsLib::LibOpen (UInt16 libRefNum)
{
	UNUSED_PARAM(libRefNum)

	return errNone;
}

Err Platform_MsfsLib::LibClose (UInt16 libRefNum)
{
	UNUSED_PARAM(libRefNum)
	return errNone;
}

Err Platform_MsfsLib::LibSleep (UInt16 libRefNum)
{
	UNUSED_PARAM(libRefNum)
	return errNone;
}

Err Platform_MsfsLib::LibWake (UInt16 libRefNum)
{
	UNUSED_PARAM(libRefNum)

	if (g_nCardInserted == MSSTATE_REQ_INSERT)
		ExpCardInserted(1);
	if (g_nCardInserted == MSSTATE_REQ_REMOVE)
		ExpCardRemoved(1);

	return errNone;
}


/************************************************************
 *  FUNCTION    : FSFileCreate()
 *
 *  DESCRIPTION : ファイルを作成するがオープンはしない 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone
 *				  vfsErrVolumeBadRef
 *************************************************************/
Err Platform_MsfsLib::FSFileCreate (UInt16 nVolRefNum, char* pPathName)
{
	int	nFile;
	Err retval = errNone;
	char	szLocalName[_MAX_PATH];

	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	if (!(retval = ::CheckFileName(pPathName, szLocalName))) 
	{
		// MSxxをカレントディレクトリにする
		::ChangeToMSDrvDir();

		char	szPath[_MAX_PATH], szDirPath[_MAX_PATH];
		_splitpath(szLocalName, NULL, szPath, NULL, NULL);
		_makepath (szDirPath, NULL, szPath, NULL, NULL);
		if (!::IsExistFile(szDirPath))
			return vfsErrDirectoryNotFound;		// ディレクトリが存在しない

		if (strnicmp(szLocalName, "palm/", 5))
			return vfsErrFileGeneric;

		if (::IsExistFile(szLocalName))
			return vfsErrFileAlreadyExists;		// 既にファイルが存在する

		nFile = _creat(szLocalName, _S_IREAD|_S_IWRITE);
		g_nSavedErrno = errno;
		if (nFile == -1)
			retval = GetVfsErrorCode(vfsErrBadName);
		else
			_close(nFile);

		::RestoreDrv();
	}

	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileOpen()
 *
 *  DESCRIPTION : ファイルをオープンする
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     :
 *************************************************************/
Err Platform_MsfsLib::FSFileOpen(UInt16 nVolRefNum, char *pPathName, UInt16 nOpenMode, FileRef* pFileRef)
{
	int	nFile;
	Err retval = errNone;

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	char	szLocalName[_MAX_PATH];
	if (!(retval = ::CheckFileName(pPathName, szLocalName))) 
	{
		struct _finddata_t 	fdata;
		if (::IsExistFile(g_szMsHome, &fdata) && (fdata.attrib & _A_RDONLY) && (nOpenMode & vfsModeWrite) == vfsModeWrite)
			return expErrCardReadOnly;			// メモリスティックが書込み禁止

		OpenFileInfo	oinf;
		if (g_lstOpenFiles.GetOpenFileInfo(szLocalName, &oinf))
		{
			if (oinf.nOpenMode & vfsModeExclusive)
				return vfsErrFilePermissionDenied;	// 既にvfsModeExclusiveでオープンされている
			if (nOpenMode & vfsModeExclusive)
				return vfsErrFilePermissionDenied;	// 既にオープンされているファイルをvfsModeExclusiveでオープンしようとしている
		}

		int	nFlags  = _O_BINARY | ((nOpenMode == vfsModeReadWrite) ? _O_RDWR : _O_RDONLY); 
		if ((nOpenMode & vfsModeReadWrite) == vfsModeReadWrite)
			nFlags |= _O_RDWR;
		else if ((nOpenMode & vfsModeRead) == vfsModeRead)
			nFlags |= _O_RDONLY;
		else if ((nOpenMode & vfsModeWrite) == vfsModeWrite)
			nFlags |= _O_WRONLY;
		else
			return vfsErrFilePermissionDenied;		// openMode 指定エラー

		::ChangeToMSDrvDir();		// MSxxディレクトリをカレントにする

		BOOL	bIsDir = FALSE;
		if (::IsExistFile(szLocalName, &fdata) && (fdata.attrib & _A_SUBDIR))
		{
			*pFileRef = g_lstOpenFiles.NewDirHandle();
			bIsDir = TRUE;
		}
		else 
		{
			nFile		  = _open(szLocalName, nFlags, _S_IREAD|_S_IWRITE);
			g_nSavedErrno = errno;
			if (nFile == -1)
				retval = GetVfsErrorCode(vfsErrFileNotFound);
			else
				*pFileRef = (FileRef)nFile;
		}

		if (retval == errNone)
			g_lstOpenFiles.AddOpenFileInfo(szLocalName, nOpenMode, *pFileRef, bIsDir);

		::RestoreDrv();
	}

	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileClose()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     :
 *************************************************************/
Err Platform_MsfsLib::FSFileClose (FileRef nFileRef)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	Err	retval = errNone;
	if (oinf.bIsDir)
	{
		if (oinf.nDirEntry)
			_findclose(oinf.nDirEntry);
		g_lstOpenFiles.SetDirEntryHandle(nFileRef, NULL);
	}
	else
	{
		int	nResult   = _close(nFileRef);
		g_nSavedErrno = errno;
		if (nResult == -1)
			retval = GetVfsErrorCode(vfsErrFileBadRef);
	}
	g_lstOpenFiles.RemoveOpenFileInfo(nFileRef);
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileReadData()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS		: errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err Platform_MsfsLib::FSFileReadData(FileRef nFileRef, UInt32 nNumBytes, void * pBufBase,
								   UInt32 nOffset, UInt32* pnNumByteRead)
{
	if (pnNumByteRead)
		*pnNumByteRead = 0;

	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	if ((oinf.nOpenMode & vfsModeRead) != vfsModeRead)
		return vfsErrFilePermissionDenied;

	Err retval = errNone;
	const char*	pBuff = (char*)pBufBase;
	int	nResult   = _read(nFileRef, (void*)&pBuff[nOffset], nNumBytes);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);
	else 
	{
		if (!nResult || nResult < nNumBytes)
			retval = vfsErrFileEOF;

		if (pnNumByteRead)
			*pnNumByteRead = nResult;
	}
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileRead()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS		: errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err Platform_MsfsLib::FSFileRead(FileRef nFileRef, UInt32 nNumBytes, void * pBuf,
								   UInt32* pnNumByteRead)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	if ((oinf.nOpenMode & vfsModeRead) != vfsModeRead)
		return vfsErrFilePermissionDenied;

	Err retval = errNone;
	int	nResult   = _read(nFileRef, pBuf, nNumBytes);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);
	else if (pnNumByteRead)
		*pnNumByteRead = nResult;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileWrite()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS		: errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *				  vfsErrFilePermissionDenied
 *				  Returns the number of bytes actually written in numBytesWrittenP if it is not NULL
 *************************************************************/
Err Platform_MsfsLib::FSFileWrite(FileRef nFileRef, UInt32 nNumBytes, void* pBufBase,
								  UInt32* pnNumByteWrite)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	if ((oinf.nOpenMode & vfsModeWrite) != vfsModeWrite)
		return vfsErrFilePermissionDenied;
	
	// 残サイズチェック
	Preference<Configuration>	prefConfiguration (kPrefKeyLastConfiguration);
	Configuration				cfg = *prefConfiguration;
	UInt32	nTotalSize = (cfg.fMSSize * 1024 * 1024); 
	if (!g_bUsedSizeUpdated)
		g_nUsedSize = GetSizeInDir(g_szMsHome);

	if (nNumBytes > nTotalSize - g_nUsedSize)
		return vfsErrVolumeFull;

	// 書き込み
	Err		retval = errNone;
	UInt32	nWrittenBytes = 0;
	const char*	pBuff = (char*)pBufBase;
	
	int	nResult   = _write(nFileRef, pBufBase, nNumBytes);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);
	else
		nWrittenBytes = nResult;
	
	if (pnNumByteWrite)
		*pnNumByteWrite = nWrittenBytes;

	g_bUsedSizeUpdated = false;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileDelete()
 *
 *  DESCRIPTION : ファイルまたはディレクトリの削除 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS		: errNone				-- no error
 *				  vfsErrFileNotFound	-- the fileref is invalid
 *************************************************************/
Err Platform_MsfsLib::FSFileDelete (UInt16 nVolRefNum, char* pPathName)
{
	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	int	nResult;
	Err retval = errNone;

	char	szLocalName[_MAX_PATH];
	if (!(retval = ::CheckFileName(pPathName, szLocalName))) 
	{
		OpenFileInfo	oinf;
		if (g_lstOpenFiles.GetOpenFileInfo(szLocalName, &oinf))
			return vfsErrFileStillOpen;		// ファイルがオープンされている

		::ChangeToMSDrvDir();

		struct _finddata_t 	fdata;
		if (!::IsExistFile(szLocalName, &fdata))
			return vfsErrFileNotFound;		// ファイルが存在しない

		if (fdata.attrib & _A_RDONLY)
			return vfsErrFilePermissionDenied;	// pPathNameがReadOnly

		if (fdata.attrib & _A_SUBDIR)
		{
			nResult = _rmdir(szLocalName);
			if (nResult == -1)
				retval = vfsErrDirNotEmpty;
		}
		else
		{
			nResult = remove(szLocalName);
			g_nSavedErrno = errno;
			if (nResult == -1)
				retval = GetVfsErrorCode(vfsErrFileStillOpen);
		}

		::RestoreDrv();
	}

	g_bUsedSizeUpdated = false;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileRename()	 
 *
 *  DESCRIPTION : ファイルまたはディレクトリの名前変更
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     :
 *************************************************************/
Err Platform_MsfsLib::FSFileRename (UInt16 nVolRefNum, char* pPathName, char* pNewName)
{
	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	char	szLocalName[_MAX_PATH];
	Err		retval = errNone;
	if (!(retval = ::CheckFileName(pPathName, szLocalName)) && !(retval = ::CheckFileName(pNewName, NULL))) 
	{
		char	szPath[_MAX_PATH], szNewPath[_MAX_PATH];
		struct _finddata_t 	fdata;

		::ChangeToMSDrvDir();

		if (!IsExistFile(szLocalName, &fdata))
			return vfsErrFileNotFound;			// ファイルが存在しない

		if (fdata.attrib & _A_RDONLY)
			return vfsErrFilePermissionDenied;	// pPathNameがReadOnly

		OpenFileInfo	oinf;
		if (g_lstOpenFiles.GetOpenFileInfo(szLocalName, &oinf))
			return vfsErrFileStillOpen;			// ファイルがオープンされている

		_splitpath(szLocalName, NULL, szPath, NULL, NULL);
		_makepath (szNewPath, NULL, szPath, pNewName, NULL);
		
		if (::IsExistFile(szNewPath))
			return vfsErrFileAlreadyExists;		// pNewNameと同名のファイルが既に存在する

		int nResult = rename(szLocalName, szNewPath);	
		g_nSavedErrno = errno;
		if (nResult == -1)
			retval = GetVfsErrorCode(vfsErrFileStillOpen);

		::RestoreDrv();
	}

	return retval;
}

/************************************************************
 *  FUNCTION    : FSFileSeek()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err Platform_MsfsLib::FSFileSeek(FileRef nFileRef, FileOrigin nOrigin, Int32 nOffset)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	Err retval = errNone;
	int	nOriginOfLSeek;
	int	nPos;
	struct _stat	fState;
	int	nResult = _fstat(nFileRef, &fState);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);
	
	switch (nOrigin)
	{
	case fsOriginBeginning: nOriginOfLSeek = SEEK_SET; nPos = 0; break;
	case fsOriginCurrent:	nOriginOfLSeek = SEEK_CUR; nPos = _tell(nFileRef); break;
	case fsOriginEnd:		nOriginOfLSeek = SEEK_END; nPos = fState.st_size; break;
	default: return sysErrParamErr;
	}

	if (nPos + nOffset > fState.st_size)
	{
		nOffset = fState.st_size - nPos;
		retval = vfsErrFileEOF;
	}

	nResult   = _lseek(nFileRef, nOffset, nOriginOfLSeek);
	g_nSavedErrno = errno;
	if (nResult == -1 && g_nSavedErrno == EINVAL)
	{
		// 無効な位置や０よりも前の位置を指定した場合、戻り値はerrNoneで
		// ファイルポインタは先頭をさす
		_lseek(nFileRef, SEEK_SET, 0);
	}
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileTell()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err Platform_MsfsLib::FSFileTell (FileRef nFileRef, UInt32 *pFilePos)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	Err retval = errNone;

	int nResult = _tell(nFileRef);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);
	else
		*pFilePos = nResult;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileAttributesGet()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *************************************************************/
Err  Platform_MsfsLib::FSFileAttributesGet (FileRef nFileRef, UInt32 *pAttr)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;

	::ChangeToMSDrvDir();

	*pAttr = NULL;
	DWORD	dwAttr = GetFileAttributes (oinf.szFilePath);
	if (dwAttr & FILE_ATTRIBUTE_ARCHIVE)	*pAttr |= vfsFileAttrArchive;
	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)	*pAttr |= vfsFileAttrDirectory;
	if (dwAttr & FILE_ATTRIBUTE_HIDDEN)		*pAttr |= vfsFileAttrHidden;
	if (dwAttr & FILE_ATTRIBUTE_SYSTEM)		*pAttr |= vfsFileAttrSystem;
	if (dwAttr & FILE_ATTRIBUTE_READONLY)	*pAttr |= vfsFileAttrReadOnly;

	::RestoreDrv();
	return errNone;
}


/************************************************************
 *  FUNCTION    : FSFileAttributesSet()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *				  sysErrParamErr		-- attributes included vfsFileAttrDirectory or vfsFileAttrVolumeLabel
 *************************************************************/
Err  Platform_MsfsLib::FSFileAttributesSet (FileRef nFileRef, UInt32 nAttr)
{
	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	if ((nAttr & vfsFileAttrDirectory) || (nAttr & vfsFileAttrVolumeLabel))
		return sysErrParamErr;

	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;

	Err				retval = errNone;
	DWORD			dwAttr = 0;

	if (nAttr & vfsFileAttrReadOnly)
		dwAttr |= FILE_ATTRIBUTE_READONLY;
	if (nAttr & vfsFileAttrHidden)
		dwAttr |= FILE_ATTRIBUTE_HIDDEN;
	if (nAttr & vfsFileAttrSystem)
		dwAttr |= FILE_ATTRIBUTE_SYSTEM;
	if (nAttr & vfsFileAttrArchive)
		dwAttr |= FILE_ATTRIBUTE_ARCHIVE;

	::ChangeToMSDrvDir();

	if (!::SetFileAttributes(oinf.szFilePath, dwAttr))
	{
#ifdef _DEBUG
		DWORD	dwError = GetLastError();
#endif
		EmAssert (false);
	}

	::RestoreDrv();

	return retval;
}


/************************************************************
 *  FUNCTION    : VFSFileEOF()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileEOF			-- file pointer is at end of file
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err Platform_MsfsLib::FSFileEOF(FileRef nFileRef)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	Err retval = errNone;
	int nResult = _eof(nFileRef);
	g_nSavedErrno = errno;
	if (nResult == 1)
		retval = vfsErrFileEOF;
	else if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);

	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileGetSize()
 *
 *  DESCRIPTION : オープンしているファイルサイズの取得
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err	 Platform_MsfsLib::FSFileSize(FileRef nFileRef, UInt32 *pFileSize)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	Err retval    = errNone;
	struct _stat	fState;
	int	nResult = _fstat(nFileRef, &fState);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);
	else 
		*pFileSize = fState.st_size;

	return retval;
}


/************************************************************
 *  FUNCTION    : FSFileResize()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *                vfsErrIsADirectory	-- the fileref points to a directory
 *************************************************************/
Err Platform_MsfsLib::FSFileResize (FileRef nFileRef, UInt32 nNewSize)
{
	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;
	
	if (oinf.bIsDir)
		return vfsErrIsADirectory;

	if ((oinf.nOpenMode & vfsModeWrite) != vfsModeWrite)
		return vfsErrFilePermissionDenied;

	Err		retval = errNone;
	UInt32	nAttr, nSize;

	if (retval = FSFileAttributesGet(nFileRef, &nAttr))	
		return retval;

	if (nAttr == vfsFileAttrReadOnly)
		return vfsErrFilePermissionDenied;

	if (retval = FSFileSize(nFileRef, &nSize))
		return retval;

	if (nNewSize == nSize)
		return retval;

	// 残サイズチェック
	if (nNewSize > nSize)
	{
		Preference<Configuration>	prefConfiguration (kPrefKeyLastConfiguration);
		Configuration				cfg = *prefConfiguration;
		UInt32	nTotalSize = (cfg.fMSSize * 1024 * 1024); 
		if (!g_bUsedSizeUpdated)
			g_nUsedSize = GetSizeInDir(g_szMsHome);
		if (nNewSize - nSize > nTotalSize - g_nUsedSize)
			return vfsErrVolumeFull;
	}

	// サイズ変更
	int	nResult = _chsize(nFileRef, nNewSize);
	g_nSavedErrno = errno;
	if (nResult == -1)
		retval = GetVfsErrorCode(vfsErrFileBadRef);

	g_bUsedSizeUpdated = false;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSDirCreate()
 *
 *  DESCRIPTION : 新規ディレクトリの作成
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS		: errNone					-- no error
 *				  vfsErrVolumeBadRef		-- the volume has not been mounted with FSVolumeMount
 *				  vfsErrFileAlreadyExists	-- a directory or file with this name already exists at this location
 *				  vfsErrBadName				-- the full path before the new directory does not exist
 *				  vfsErrDirectoryNotFound
 *************************************************************/
Err Platform_MsfsLib::FSDirCreate(UInt16 nVolRefNum, char *pDirName)
{
	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	int	nResult;
	Err retval = errNone;

	char	szLocalName[_MAX_PATH];
	if (!(retval = ::CheckFileName(pDirName, szLocalName)))
	{
		::ChangeToMSDrvDir();

		nResult = _mkdir (szLocalName);
		g_nSavedErrno = errno;
		if (nResult == -1)
		{
			if (g_nSavedErrno == ENOENT)
				retval = vfsErrDirectoryNotFound;
			else
				retval = GetVfsErrorCode(vfsErrFileAlreadyExists);
		}

		::RestoreDrv();
	}
	
	return retval;
}


/************************************************************
 *  FUNCTION    : FSDirEntryEnumerate()
 *
 *  DESCRIPTION : ディレクトリエントリのリストアップ
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS		: errNone					-- no error
 *				  vfsErrFileBadRef
 *				  vfsErrNotADirectory
 *************************************************************/
Err Platform_MsfsLib::FSDirEntryEnumerate(FileRef nDirRef, UInt32 *pDirEntryIterator, FileInfoType *pInfo)
{
	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nDirRef, &oinf))
		return vfsErrFileBadRef;	// nDirRefが無効な値

	if (!oinf.bIsDir)
		return vfsErrNotADirectory;	// nDirRefがディレクトリではない
	
	if (*pDirEntryIterator == expIteratorStop)
		return sysErrParamErr;	

	Err		retval = errNone;
	long	nResult;
	long	hFind;
	char	szPath[_MAX_PATH];

	::ChangeToMSDrvDir();

	strcpy(szPath, oinf.szFilePath);
	strcat(szPath, "\\*.*");

	if (*pDirEntryIterator == expIteratorStart)
	{
		// 検索開始の場合
		hFind = _findfirst(szPath, &g_infFindData);
		g_nSavedErrno = errno;
		if (hFind == -1)
		{
			*pDirEntryIterator = expIteratorStop;
			::RestoreDrv();
			return expErrEnumerationEmpty;
		}
		// ".."の読み飛ばし
		nResult = _findnext(hFind, &g_infFindData);

		// １つ目のエントリの読込み
		nResult = _findnext(hFind, &g_infFindData);	
		g_nSavedErrno = errno;
		if (nResult == -1)
		{
			// エントリが１つも存在しない
			_findclose(hFind);
			*pDirEntryIterator = expIteratorStop;
			::RestoreDrv();
			return expErrEnumerationEmpty;
		}
	}
	else
	{
		// 継続して検索の場合
		hFind = (long)*pDirEntryIterator;
	}

	if (pInfo)
	{
		// 初回の検索の場合、たった今検索したエントリ情報をセットする。
		// 継続検索の場合、前回検索した「次のエントリ」情報をセットする。
		pInfo->attributes = g_infFindData.attrib;
		if (pInfo->nameP && pInfo->nameBufLen > 0)
		{
			strncpy(pInfo->nameP, g_infFindData.name, pInfo->nameBufLen);
			pInfo->nameP[pInfo->nameBufLen-1] = NULL;
		}
	}

	// 次のエントリの読込み
	nResult = _findnext(hFind, &g_infFindData);	
	g_nSavedErrno = errno;

	if (nResult == -1)
	{
		_findclose(hFind);
		*pDirEntryIterator = expIteratorStop;
		g_lstOpenFiles.SetDirEntryHandle(nDirRef, NULL);	
	}
	else
	{
		*pDirEntryIterator = hFind;
		g_lstOpenFiles.SetDirEntryHandle(nDirRef, hFind);	
	}

	::RestoreDrv();
	return retval;
}


/************************************************************
 *  FUNCTION    : FSVolumeFormat()
 *
 *  DESCRIPTION : MS カードのフォーマット
 *              
 *  PARAMETERS  : 
 *                
 *  RETURNS     :
 *************************************************************/
Err Platform_MsfsLib::FSVolumeFormat (VFSAnyMountParamPtr pVfsMountParam)
{
	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card id read only

	if (pVfsMountParam->mountClass != VFSMountClass_SlotDriver)
		return expErrUnsupportedOperation;

	if (pVfsMountParam->volRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeStillMounted;

	Err retval = errNone;
	retval = ::DeleteDir(g_szMsHome, false);
	g_szVolumeLabel[0] = NULL;
	g_bUsedSizeUpdated = false;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSVolumeMount()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  : 
 *                
 *  RETURNS     :
 *************************************************************/
Err Platform_MsfsLib::FSVolumeMount (VFSAnyMountParamPtr pVfsMountParam)
{
	Err retval = errNone;
	g_infMountParam.vfsMountParam = *pVfsMountParam;
	return retval;
}


/************************************************************
 *  FUNCTION    : FSVolumeUnmount()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  : 
 *                
 *  RETURNS     : errNone			 -- no error
 *				  vfsErrVolumeBadRef -- the volume has not been mounted with FSVolumeMount
 *************************************************************/
Err Platform_MsfsLib::FSVolumeUnmount (UInt16 nVolRefNum)
{
	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	g_infMountParam.vfsMountParam.volRefNum = 0;
	return errNone;
}


/************************************************************
 *  FUNCTION    : FSVolumeInfo()
 *
 *  DESCRIPTION : ボリューム情報の取得
 *              
 *  PARAMETERS  : 
 *                
 *  RETURNS     : errNone			 -- no error
 *				  vfsErrVolumeBadRef -- the volume has not been mounted with FSVolumeMount
 *************************************************************/
Err Platform_MsfsLib::FSVolumeInfo (UInt16 nVolRefNum, VolumeInfoType* pVolInfo)
{
	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	Err retval = errNone;
	pVolInfo->attributes = vfsVolumeAttrSlotBased;
	if (CardIsWriteProtect())
		pVolInfo->attributes |= vfsVolumeAttrReadOnly;
	pVolInfo->fsType     = fsFilesystemType_VFAT;
	pVolInfo->fsCreator  = 'MSfs';
	pVolInfo->mountClass = g_infMountParam.vfsMountParam.mountClass;
	pVolInfo->slotLibRefNum = GetSlotLibRefNum();
	pVolInfo->slotRefNum = 1;
	pVolInfo->mediaType  = ExpMediaType_MemoryStick;
	pVolInfo->reserved   = 0;

	return retval;
}


/************************************************************
 *  FUNCTION    : FSVolumeSize()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone	-- no error
 *************************************************************/
Err Platform_MsfsLib::FSVolumeSize(UInt16 nVolRefNum, UInt32* pVolUsed, UInt32* pVolTotal)
{
	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	Preference<Configuration>	prefConfiguration (kPrefKeyLastConfiguration);
	Configuration				cfg = *prefConfiguration;
	*pVolTotal = (cfg.fMSSize * 1024 * 1024); 
	Err retval = errNone;
	if (!g_bUsedSizeUpdated)
	{
		g_nUsedSize = ::GetSizeInDir(g_szMsHome);
		g_bUsedSizeUpdated = true;
	}
	*pVolUsed = g_nUsedSize;
	if (*pVolUsed > *pVolTotal)
		*pVolUsed = *pVolTotal;
	return retval;
}

/************************************************************
 *  FUNCTION    : CardIsWriteProtect()
 *
 *  DESCRIPTION : メモリスティックカード(MSディレクトリ)の書き込み禁止
 *              　状態の取得
 *  PARAMETERS  :
 *              
 *  RETURNS     : true - 書き込み禁止／false - 書き込み可
 *************************************************************/
Bool Platform_MsfsLib::CardIsWriteProtect(UInt16 nSlotNum)
{
	struct _finddata_t fileInfo;
	IsExistFile (g_szMsHome, &fileInfo);
	if (fileInfo.attrib & _A_RDONLY)
		return true;
	return false;
}


/************************************************************
 *  FUNCTION    : GetSlotLibRefNum()
 *
 *  DESCRIPTION : スロット・ドライバ・ライブラリ参照番号の取得
 *              
 *  PARAMETERS  : なし
 *              
 *  RETURNS     : ライブラリの参照番号
 *************************************************************/
UInt16	Platform_MsfsLib::GetSlotLibRefNum( )
{
	static	UInt16	gSlotLibRefNum;
	if (gSlotLibRefNum)
		return gSlotLibRefNum;
	
	UInt16	sysLibTableEntries	= EmLowMem_GetGlobal (sysLibTableEntries);
	for (UInt16 i=0; i<sysLibTableEntries; i++)
	{
		string	libName = ::GetLibraryName (i);
		if (libName == SlotDriverLibName
		 || libName == SlotDriverLibNameForPEGN700C)
		{	
			gSlotLibRefNum = i;
			return i;
		}
	}
	return 0;
}


/************************************************************
 *  FUNCTION    : FSFileDateGet()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *************************************************************/
Err  Platform_MsfsLib::FSFileDateGet (FileRef nFileRef, UInt16 nWhichDate, UInt32* pDate)
{
	if (nWhichDate != vfsFileDateCreated && nWhichDate != vfsFileDateModified
		&& nWhichDate != vfsFileDateAccessed)
		return sysErrParamErr;	// パラメータエラー

	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;

	::ChangeToMSDrvDir();

	WIN32_FILE_ATTRIBUTE_DATA	attrData;
	if (!::GetFileAttributesEx(oinf.szFilePath, GetFileExInfoStandard, &attrData))
	{
		return vfsErrFileNotFound;
	}

	::RestoreDrv();

	FILETIME	ftTime;
	switch (nWhichDate)
	{
	case vfsFileDateCreated:  ftTime = attrData.ftCreationTime; break;
	case vfsFileDateAccessed: ftTime = attrData.ftLastAccessTime; break;
	case vfsFileDateModified: ftTime = attrData.ftLastWriteTime; break;
	}

	SYSTEMTIME	sysTime;
	FILETIME	localTime;
	::FileTimeToLocalFileTime(&ftTime, &localTime);
	::FileTimeToSystemTime(&localTime, &sysTime);
	
	DateTimeType	dtTime;
	dtTime.year    = sysTime.wYear;
	dtTime.month   = sysTime.wMonth;
	dtTime.day     = sysTime.wDay;
	dtTime.hour    = sysTime.wHour;
	dtTime.minute  = sysTime.wMinute;
	dtTime.second  = sysTime.wSecond;
	dtTime.weekDay = sysTime.wDayOfWeek;

	*pDate = TimDateTimeToSeconds(&dtTime);
	return errNone;
}


/************************************************************
 *  FUNCTION    : FSFileDateSet()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrFileBadRef		-- the fileref is invalid
 *				  sysErrParamErr		-- attributes included fsAttribDirectory or fsAttribVolumeLabel
 *************************************************************/
Err  Platform_MsfsLib::FSFileDateSet (FileRef nFileRef, UInt16 nWhichDate, UInt32 nDate)
{
	if (nWhichDate != vfsFileDateCreated && nWhichDate != vfsFileDateModified
		&& nWhichDate != vfsFileDateAccessed)
		return sysErrParamErr;	// パラメータエラー

	// 1980 check
	if(nDate < 0x8EF45680)
		return 	sysErrParamErr;

	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	OpenFileInfo	oinf;
	if (!g_lstOpenFiles.GetOpenFileInfo(nFileRef, &oinf))
		return vfsErrFileBadRef;

	::ChangeToMSDrvDir();

	DWORD	dwAttr = ::GetFileAttributes (oinf.szFilePath);
	if (dwAttr & FILE_ATTRIBUTE_READONLY)
		return vfsErrFilePermissionDenied;

	DateTimeType	dtTime;
	TimSecondsToDateTime(nDate, &dtTime);

	SYSTEMTIME	sysTime;
	FILETIME	ftTime, localTime;

	sysTime.wYear      = dtTime.year;
	sysTime.wMonth     = dtTime.month;
	sysTime.wDay       = dtTime.day;
	sysTime.wHour      = dtTime.hour;
	sysTime.wMinute    = dtTime.minute;
	sysTime.wSecond    = dtTime.second;
	sysTime.wDayOfWeek = dtTime.weekDay;
	sysTime.wMilliseconds = 0;
	
	if (::SystemTimeToFileTime(&sysTime, &localTime))
		if (!::LocalFileTimeToFileTime(&localTime, &ftTime))
			ftTime = localTime;

	HANDLE	hFile = ::CreateFile(oinf.szFilePath, 
							 GENERIC_READ|GENERIC_WRITE,
							 FILE_SHARE_READ|FILE_SHARE_WRITE,
							 NULL,
							 OPEN_EXISTING,
							 (!oinf.bIsDir) ? FILE_ATTRIBUTE_NORMAL : FILE_FLAG_BACKUP_SEMANTICS,
							 NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		EmAssert(false);
		return vfsErrFilePermissionDenied;
	}

	BOOL bSucceed = ::SetFileTime(hFile, 
					(nWhichDate == vfsFileDateCreated)  ? &ftTime : NULL,
					(nWhichDate == vfsFileDateAccessed) ? &ftTime : NULL,
					(nWhichDate == vfsFileDateModified) ? &ftTime : NULL);
	::CloseHandle(hFile);

	::RestoreDrv();
	return errNone;
}

/************************************************************
 *  FUNCTION    : FSVolumeLabelGet
 *
 *  DESCRIPTION : ボリュームラベルの設定
 *              　ラベルの長さが12文字以上になった場合には、/Palmディレクトリ下に
 *				  volInfo.txtを作成しその中にラベル情報を出力する
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrVolumeBadRef	-- the volume has not been mounted with FSVolumeMount
 *				  vfsErrBufferOverflow	-- nBufLen is not big enough to receive the full volume label
 *										   nVolRefNum is filled in on return with the volume label for the specified volume
 *************************************************************/
Err Platform_MsfsLib::FSVolumeLabelGet(UInt16 nVolRefNum, LPSTR pVolumeLabel, UInt16 nBufLen)
{
	Err retval = errNone;

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	// nBufLenが12未満の場合、vfsErrBufferOverflowを返す
	if (nBufLen <= strlen(g_szVolumeLabel) || nBufLen < 12)
		return vfsErrBufferOverflow;

	strcpy(pVolumeLabel, g_szVolumeLabel);

	return retval;
}

/************************************************************
 *  FUNCTION    : FSVolumeLabelSet
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : errNone				-- no error
 *				  vfsErrVolumeBadRef	-- the volume has not been mounted with FSVolumeMount
 *				  vfsErrBadName			-- pVolumeLabel is invalid
************************************************************/
Err Platform_MsfsLib::FSVolumeLabelSet(UInt16 nVolRefNum, LPSTR pVolumeLabel)
{
	Err retval = errNone;
	const char *short_illegals="\"*+,/:;<=>?[\\]|"; // " * + , / : ; < = > ? [ \ ] | ---- (+ ')
	const char *long_illegals = "\"*/:<>?\\|";		// " * / : < > ? \ |             ---- (+ \005)

	if (CardIsWriteProtect())
		return expErrCardReadOnly;		// MS Card is read only

	if (nVolRefNum != g_infMountParam.vfsMountParam.volRefNum)
		return vfsErrVolumeBadRef;

	if (strlen(pVolumeLabel) > 255)
		return vfsErrBadName;

	BOOL	bLongLabel = (strlen(pVolumeLabel) >= 12) || 
			strspn(pVolumeLabel, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != strlen(pVolumeLabel);			
	
	char	*p;
	if (bLongLabel)
		p = strpbrk(pVolumeLabel, long_illegals);
	else
		p = strpbrk(pVolumeLabel, short_illegals);

	if (p)
		return vfsErrBadName;

	char	szNewVolFile[MAX_FFS_PATH+1], szDelVolFile[MAX_FFS_PATH+1];

	szDelVolFile[0] = NULL;
	if (bLongLabel)
	{
		strcpy(szNewVolFile, ::ActualMsPath(VOLLABEL_LONG_FILE));
		strcpy(szDelVolFile, ::ActualMsPath(VOLLABEL_SHORT_FILE));

		// Palmディレクトリの作成
		::CreateDir(::ActualMsPath(PALM_DIR));
	}
	else
	{
		strcpy(szNewVolFile, ::ActualMsPath(VOLLABEL_SHORT_FILE));
		strcpy(szDelVolFile, ::ActualMsPath(VOLLABEL_LONG_FILE));
	}

	strcpy(g_szVolumeLabel, pVolumeLabel);
	if (::IsExistFile(szDelVolFile))
		remove(szDelVolFile);

	int	nFile;
	nFile = _open(szNewVolFile, 
					_O_BINARY|_O_TRUNC|_O_CREAT|_O_RDWR,
					_S_IREAD | _S_IWRITE);
	g_nSavedErrno = errno;
	if (nFile == -1)
		retval = GetVfsErrorCode(vfsErrFileNotFound);
	_write(nFile, pVolumeLabel, strlen(pVolumeLabel));
	_close(nFile);

	return retval;
}
