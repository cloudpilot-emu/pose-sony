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
#include "Platform_VfsLib.h"

#include "Emulator.h"			// gInstance
#include "Logging.h"			// LogAppendMsg
#include "Marshal.h"
#include "Platform.h"			// AllocateMemory
#include "UAE_Utils.h"

#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <tchar.h>
#include "Platform_ExpMgrLib.h"
#include "Platform_MsfsLib.h"
#include "EmWindow.h"		// EmWindow
#include "EmWindowWin.h"	// EmWindowHostData

void	LCD_SetStateJogButton(SkinElementType witch, Bool bPress, Bool bEnabled);	// LCDSony.inl
void	LCD_DrawButtonForPEG(HDC hDC, SkinElementType witch);

static	UInt16	g_bReadyExpMgrLib = FALSE;

#define PRINTF	if (1) ; else LogAppendMsg

/************************************************************
 *  FUNCTION    : IsUsable()
 *
 *  DESCRIPTION : ExpantionMgr関数の使用が可能(初期化済)かどうかを返す
 *              
 *  PARAMETERS  : 
 *              
 *  RETURNS     : なし
 *************************************************************/
Bool Platform_ExpMgrLib::IsUsable ()
{
	return g_bReadyExpMgrLib;
}

void Platform_ExpMgrLib::Initialize ()
{
	g_bReadyExpMgrLib = false;
}

/************************************************************
 *  FUNCTION    : Reset()
 *
 *  DESCRIPTION : 
 *              
 *  PARAMETERS  : 
 *              
 *  RETURNS     : なし
 *************************************************************/
void Platform_ExpMgrLib::Reset ()
{
	g_bReadyExpMgrLib = false;
}

/************************************************************
 *  FUNCTION    : CalledExpInit()
 *
 *  DESCRIPTION : ExpMgrライブラリ自体が初期化された場合に呼び出す
 *              
 *  PARAMETERS  : 
 *              
 *  RETURNS     : なし
 *************************************************************/
void Platform_ExpMgrLib::CalledExpInit ()
{
	g_bReadyExpMgrLib = true;
}

/************************************************************
 *  FUNCTION    : Load()
 *
 *  DESCRIPTION : セッションファイル(PSF)からのExpMgrライブラリ情報の取得
 *              
 *  PARAMETERS  : 
 *              
 *  RETURNS     : なし
 *************************************************************/
void Platform_ExpMgrLib::Load(SessionFile& f)
{
	Chunk	chunk;
	if (f.ReadMSfsInfo (chunk))
	{
		long			version;
		EmStreamChunk	s (chunk);

		s >> version;
		s >> g_bReadyExpMgrLib;
	}
	else
	{
		f.SetCanReload (false);
	}
	
	extern	Int16	g_nCardInserted ;
	
	::LCD_SetStateJogButton(kElement_MS_InOut, 
							(g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE) ? false : true, 
							true);
//	::LCD_DrawButtonForPEG(NULL, kElement_MS_InOut);	
}

/************************************************************
 *  FUNCTION    : Save()
 *
 *  DESCRIPTION : セッションファイル(PSF)へのExpMgrライブラリ情報の出力
 *              
 *  PARAMETERS  :
 *              
 *  RETURNS     : なし
 *************************************************************/
void Platform_ExpMgrLib::Save(SessionFile& f)
{
	const long	kCurrentVersion = 1;

	Chunk			chunk;
	EmStreamChunk	s (chunk);

	s << kCurrentVersion;
	s << g_bReadyExpMgrLib;

	f.WriteMSfsInfo (chunk);
}


