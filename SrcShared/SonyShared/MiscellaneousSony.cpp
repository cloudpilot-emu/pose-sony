#include "EmCommon.h"
#include "Miscellaneous.h"
#include "MiscellaneousSony.h"

/***********************************************************************
 *
 * FUNCTION:    GetMSSizeTextList
 *
 * DESCRIPTION: メモリ・ステイックのサイズ一覧を取得する
 *				for Sony & MemoryStick
 ***********************************************************************/

void GetMSSizeTextList (MemoryTextList& memoryList)
{
	memoryList.push_back (make_pair (MSSizeType (8), string ("8M")));
	memoryList.push_back (make_pair (MSSizeType (16), string ("16M")));
	memoryList.push_back (make_pair (MSSizeType (32), string ("32M")));
	memoryList.push_back (make_pair (MSSizeType (64), string ("64M")));
	memoryList.push_back (make_pair (MSSizeType (128), string ("128M")));
}
