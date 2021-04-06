#include <list>
typedef struct _tagOpenFileInfo
{
	char	szFilePath[MAX_FFS_PATH+1];
	FileRef	nFileRef;
	UInt16	nOpenMode;
	BOOL	bIsDir;
	long	nDirEntry;

} OpenFileInfo, *OpenFileInfoPtr;
typedef list <OpenFileInfo>	OpenFileInfoList;

// ディレクトリをオープンした場合、ファイル番号として
// 30000以上の値を一時的に割り当てる
#define	DIR_HANDLE_START	30000

class	OpenFileList
{
public:
	OpenFileList() {};
	~OpenFileList()
	{
	}

private:
	// members value
	OpenFileInfoList	m_lstOpenFile;

public:
	// 
	UInt16 NewDirHandle()
	{
		BOOL	bFound = TRUE;
		FileRef	hNew = DIR_HANDLE_START;
		do
		{
			OpenFileInfoList::iterator i;
			for (i=m_lstOpenFile.begin(), bFound=FALSE; i!=m_lstOpenFile.end(); i++)
			{
				if ((*i).bIsDir && (*i).nFileRef == hNew)
				{
					bFound = TRUE;
					hNew ++;
					break;	
				}
			}
		} while (bFound);
		return hNew;
	};

	BOOL AddOpenFileInfo(char *pFilePath, UInt16 nOpenMode, FileRef nFileRef, BOOL bIsDir=FALSE)
	{
		OpenFileInfo	oinf;
		strcpy(oinf.szFilePath, pFilePath);
		oinf.nOpenMode = nOpenMode;
		oinf.nFileRef = nFileRef;
		oinf.bIsDir = bIsDir;
		oinf.nDirEntry = 0;
		
		char	*p = oinf.szFilePath;
		while (*p)
		{
			if (*p == '\\')
				*p = '/';
			*p++;
		}
		m_lstOpenFile.insert(m_lstOpenFile.begin(), oinf);
		return TRUE;
	}

	BOOL RemoveOpenFileInfo(FileRef nFileRef)
	{
		OpenFileInfoList::iterator i;
		for (i=m_lstOpenFile.begin(); i!=m_lstOpenFile.end(); i++)
		{
			if ((*i).nFileRef == nFileRef)
			{
				m_lstOpenFile.erase(i);	
				return TRUE;
			}
		}
		return FALSE;
	};

	BOOL GetOpenFileInfo(FileRef nFileRef, OpenFileInfo *pInfo)
	{
		OpenFileInfoList::iterator i;
	    for (i=m_lstOpenFile.begin(); i!=m_lstOpenFile.end(); i++)
		{
			if ((*i).nFileRef == nFileRef)
			{
				*pInfo = *i;	
				return TRUE;
			}
		}
		return FALSE;
	};

	BOOL GetOpenFileInfo(LPCSTR pszFilePath, OpenFileInfo *pInfo)
	{
		char	szBuff[MAX_FFS_PATH+1];

		strcpy(szBuff, pszFilePath);
		char	*p = szBuff;
		while (*p)
		{
			if (*p == '\\')
				*p = '/';
			*p++;
		}

		OpenFileInfoList::iterator i;
		for (i=m_lstOpenFile.begin(); i!=m_lstOpenFile.end(); i++)
		{
			if (!strcmp((*i).szFilePath, szBuff))
			{
				*pInfo = *i;	
				return TRUE;
			}
		}
		return FALSE;
	};

	BOOL SetDirEntryHandle(FileRef nFileRef, long nDirEntry)
	{
		OpenFileInfoList::iterator i;
	    for (i=m_lstOpenFile.begin(); i!=m_lstOpenFile.end(); i++)
		{
			if ((*i).nFileRef == nFileRef)
			{
				(*i).nDirEntry = nDirEntry;	
				return TRUE;
			}
		}
		return FALSE;
	};

	BOOL IsValidateFileRef(FileRef nFileRef)
	{
		OpenFileInfoList::iterator i;
	    for (i=m_lstOpenFile.begin(); i!=m_lstOpenFile.end(); i++)
		{
			if ((*i).nFileRef == nFileRef)
			{
				return TRUE;
			}
		}
		return FALSE;
	};

	void ClearAllInfo()
	{
		OpenFileInfoList::iterator i;
	    for (i=m_lstOpenFile.begin(); i!=m_lstOpenFile.end(); i++)
		{
			if (!(*i).bIsDir)
				_close((*i).nFileRef);
		}
		m_lstOpenFile.clear();	
	};
};

