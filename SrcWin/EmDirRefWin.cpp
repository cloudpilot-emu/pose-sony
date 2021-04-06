/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "EmDirRefWin.h"

#include "EmFileRef.h"
#include "Emulator.h"			// gInstance
#include "Miscellaneous.h"		// EndsWith

#include <ShlObj.h>


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::EmDirRef
 *
 * DESCRIPTION:	Various ways to make a directory reference.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

EmDirRef::EmDirRef (void) :
	fDirPath ()
{
}


EmDirRef::EmDirRef (const EmDirRef& other) :
	fDirPath (other.fDirPath)
{
}


EmDirRef::EmDirRef (const char* path) :
	fDirPath (path)
{
	this->MaybeAppendSlash ();
	this->MaybeResolveLink ();
}


EmDirRef::EmDirRef (const string& path) :
	fDirPath (path)
{
	this->MaybeAppendSlash ();
	this->MaybeResolveLink ();
}


EmDirRef::EmDirRef (const EmDirRef& parent, const char* path) :
	fDirPath (parent.GetFullPath () + path)
{
	this->MaybeAppendSlash ();
	this->MaybeResolveLink ();
}


EmDirRef::EmDirRef (const EmDirRef& parent, const string& path) :
	fDirPath (parent.GetFullPath () + path)
{
	this->MaybeAppendSlash ();
	this->MaybeResolveLink ();
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::EmDirRef
 *
 * DESCRIPTION:	EmDirRef destructor.  Nothing special to do...
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

EmDirRef::~EmDirRef (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::operator=
 *
 * DESCRIPTION:	Assignment operator.  If "other" is not the same as
 *				the controlled object, copy the contents.
 *
 * PARAMETERS:	other - object to copy.
 *
 * RETURNED:	reference to self.
 *
 ***********************************************************************/

EmDirRef&
EmDirRef::operator= (const EmDirRef& other)
{
	if (&other != this)
	{
		fDirPath = other.fDirPath;
	}

	return *this;
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::IsSpecified
 *
 * DESCRIPTION:	Returns whether or not the controlled object has been
 *				pointed to a (possibly non-existant) file, or if it's
 *				empty (that it, it was created with the default ctor).
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	True if the object points to a file.
 *
 ***********************************************************************/

Bool
EmDirRef::IsSpecified (void) const
{
	return !fDirPath.empty ();
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::Exists
 *
 * DESCRIPTION:	Returns whether or not the controlled object points to
 *				an existing file.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	True if the referenced file exists.
 *
 ***********************************************************************/

Bool
EmDirRef::Exists (void) const
{
	if (this->IsSpecified ())
	{
		DWORD	result = ::GetFileAttributes (fDirPath.c_str ());

		return result != 0xFFFFFFFF;
	}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::Create
 *
 * DESCRIPTION:	Attempt to create the managed directory.  Does nothing
 *				if the directory already exists.  Throws an exception
 *				if the attempt to create the directory fails.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
EmDirRef::Create (void) const
{
	if (!this->Exists ())
	{
		if (!::CreateDirectory (fDirPath.c_str (), NULL))
		{
			// !!! throw...
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::GetName
 *
 * DESCRIPTION:	Returns the name of the referenced file.  Only the file
 *				*name* is returned, not the full path.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	A string containing the name.  If the file is not
 *				specified, an empty string is returned.  No checks are
 *				made to see if the file actually exists.
 *
 ***********************************************************************/

string
EmDirRef::GetName (void) const
{
	string	result;

	if (this->IsSpecified ())
	{
		// Make a copy of the path, and chop off the trailing '\'
		// in order to get _splitpath to think the thing at the
		// end is a file name.

		string	dirPath (fDirPath);
		dirPath.resize (dirPath.size () - 1);

		char	name[_MAX_FNAME];
		char	ext[_MAX_EXT];

		_splitpath (dirPath.c_str (), NULL, NULL, name, ext);

		strcat (name, ext);

		result = string (name);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::GetParent
 *
 * DESCRIPTION:	Returns an object representing the parent (or container)
 *				of the managed file.  If the managed file is the root
 *				directory, returns an unspecified EmDirRef.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	An object representing the file's parent.
 *
 ***********************************************************************/

EmDirRef
EmDirRef::GetParent (void) const
{
	EmDirRef	result;

	if (this->IsSpecified ())
	{
		// Make a copy of the path, and chop off the trailing '\'
		// in order to get _splitpath to think the thing at the
		// end is a file name.

		string	dirPath (fDirPath);
		dirPath.resize (dirPath.size () - 1);

		char	path[_MAX_PATH];
		char	drive[_MAX_DRIVE];
		char	dir[_MAX_DIR];
		char	name[_MAX_FNAME];
		char	ext[_MAX_EXT];

		_splitpath (dirPath.c_str (), drive, dir, name, ext);

		if (strlen (name) != 0)
		{
			_makepath (path, drive, dir, NULL, NULL);

			result = EmDirRef (path);
		}
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::GetFullPath
 *
 * DESCRIPTION:	Get a full (platform-specific) path to the object.  The
 *				path is canonicalized in that it will always have a
 *				trailing slash.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	An string representing the file's path.
 *
 ***********************************************************************/

string
EmDirRef::GetFullPath (void) const
{
	return fDirPath;
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::GetChildren
 *
 * DESCRIPTION:	Get a full (platform-specific) path to the object.  The
 *				path is canonicalized in that it will always have a
 *				trailing slash.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	An string representing the file's path.
 *
 ***********************************************************************/

void
EmDirRef::GetChildren (EmFileRefList* fileList, EmDirRefList* dirList) const
{
	if (!this->Exists ())
		return;

	WIN32_FIND_DATA	FindFileData;
	string			pattern	= fDirPath + '*';
	HANDLE			hFind	= ::FindFirstFile (pattern.c_str (), &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// If this is the current or parent directory shortcut,
			// skip it.

			if ((strcmp (FindFileData.cFileName, ".") == 0) ||
				(strcmp (FindFileData.cFileName, "..") == 0))
			{
				continue;
			}

			// Get the full path and file attributes.

			string	fullPath (fDirPath + FindFileData.cFileName);
			string	resolvedPath = ::PrvMaybeResolveLink (fullPath);
			DWORD	realAttributes = ::GetFileAttributes (resolvedPath.c_str ());

			// Determine if this is a directory or file, and push it
			// onto the appropriate list.

			if (((realAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
			{
				if (dirList)
				{
					dirList->push_back (EmDirRef (resolvedPath));
				}
			}
			else
			{
				if (fileList)
				{
					fileList->push_back (EmFileRef (resolvedPath));
				}
			}
		}
		while (::FindNextFile (hFind, &FindFileData));

		::FindClose(hFind);
	}
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::operator==
 * FUNCTION:	EmDirRef::operator!=
 * FUNCTION:	EmDirRef::operator>
 * FUNCTION:	EmDirRef::operator<
 *
 * DESCRIPTION:	Bogus operators for wiggy VC++ compiler which won't let
 *				us instantiate STL containers without them.
 *
 * PARAMETERS:	other - object to compare ourself to.
 *
 * RETURNED:	True if the requested condition is true.  Comparisons
 *				are based on the file's full path.
 *
 ***********************************************************************/

bool
EmDirRef::operator== (const EmDirRef& other) const
{
	return stricmp (fDirPath.c_str (), other.fDirPath.c_str ()) == 0;
}


bool
EmDirRef::operator!= (const EmDirRef& other) const
{
	return stricmp (fDirPath.c_str (), other.fDirPath.c_str ()) != 0;
}


bool
EmDirRef::operator> (const EmDirRef& other) const
{
	return stricmp (fDirPath.c_str (), other.fDirPath.c_str ()) < 0;
}


bool
EmDirRef::operator< (const EmDirRef& other) const
{
	return stricmp (fDirPath.c_str (), other.fDirPath.c_str ()) > 0;
}


/***********************************************************************
 *
 * FUNCTION:	FromPrefString
 *
 * DESCRIPTION:	Initialize this object from the string containing a file
 *				reference stored in a preference file.
 *
 * PARAMETERS:	s - the string from the preference file
 *
 * RETURNED:	True if we were able to carry out the initialization.
 *				False otherwise.  Note that the string is NOT validated
 *				to see if it refers to an existing file.
 *
 ***********************************************************************/

bool
EmDirRef::FromPrefString (const string& s)
{
	fDirPath = s;
	MaybeAppendSlash ();

	return true;
}


/***********************************************************************
 *
 * FUNCTION:	ToPrefString
 *
 * DESCRIPTION:	Produce a string that can be stored to a preference file
 *				and which can later be used to reproduce the current
 *				file reference object.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The string to be written to the preference file.
 *
 ***********************************************************************/

string
EmDirRef::ToPrefString (void) const
{
	return fDirPath;
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::GetEmulatorDirectory
 *
 * DESCRIPTION:	Return an EmDirRef for Poser's directory.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The desired EmDirRef.
 *
 ***********************************************************************/

EmDirRef
EmDirRef::GetEmulatorDirectory (void)
{
	return EmFileRef::GetEmulatorRef ().GetParent ();
}


/***********************************************************************
 *
 * FUNCTION:	EmDirRef::GetPrefsDirectory
 *
 * DESCRIPTION:	Return an EmDirRef for Poser's preferences directory.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The desired EmDirRef.
 *
 ***********************************************************************/

EmDirRef
EmDirRef::GetPrefsDirectory (void)
{
	char	buffer[_MAX_PATH];
	::GetWindowsDirectory (buffer, sizeof (buffer));

	return EmDirRef (buffer);
}


/***********************************************************************
 *
 * FUNCTION:	MaybeAppendSlash
 *
 * DESCRIPTION:	Append a trailing slash to the full path if there isn't
 *				one already there.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void
EmDirRef::MaybeAppendSlash (void)
{
	if (this->IsSpecified () && fDirPath[fDirPath.size () - 1] != '\\')
	{
		fDirPath += '\\';
	}
}


/***********************************************************************
 *
 * FUNCTION:	MaybeResolveLink
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void
EmDirRef::MaybeResolveLink (void)
{
	fDirPath = ::PrvMaybeResolveLink (fDirPath);

	this->MaybeAppendSlash ();
}


/***********************************************************************
 *
 * FUNCTION:	PrvMaybeResolveLink
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

string PrvMaybeResolveLink (const string& path)
{
	string	resolvedPath;
	string	tempPath (path);

	if (path.size () > 0)
	{
		// See if the passed in path is a path to a file link.  To start,
		// take off any trailing '\' in order to treat the entity like
		// a file.

		if (*(tempPath.end () - 1) == '\\')
		{
			tempPath.erase (tempPath.end () - 1);
		}

		// If it's a file, try resolving it.

		if (::PrvIsExistingFile (tempPath))
		{
			resolvedPath = ::PrvResolveLink (tempPath);
		}

		// If it's not an existing file, perhaps the entity was
		// specified without the ".lnk" at the end.  Try adding
		// that and see what happens.

		else
		{
			tempPath += ".lnk";

			if (::PrvIsExistingFile (tempPath))
			{
				resolvedPath = ::PrvResolveLink (tempPath);
			}
		}
	}

	// If both attempts failed, "resolvedPath" is still empty.
	// Set it to the original path passed in and return that to
	// the caller.

	if (resolvedPath.empty ())
	{
		resolvedPath = path;
	}

	return resolvedPath;
}


/***********************************************************************
 *
 * FUNCTION:	PrvIsExistingDirectory
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

BOOL PrvIsExistingDirectory (const string& path)
{
	DWORD	result = ::GetFileAttributes (path.c_str ());

	return (result != 0xFFFFFFFF) &&
		((result & FILE_ATTRIBUTE_DIRECTORY) != 0);
}


/***********************************************************************
 *
 * FUNCTION:	PrvIsExistingFile
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

BOOL PrvIsExistingFile (const string& path)
{
	DWORD	result = ::GetFileAttributes (path.c_str ());

	return (result != 0xFFFFFFFF) &&
		((result & FILE_ATTRIBUTE_DIRECTORY) == 0);
}


/***********************************************************************
 *
 * FUNCTION:	PrvResolveLink
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

string PrvResolveLink (const string& path)
{
	string	result;
	TCHAR	realPath [MAX_PATH];

	if (SUCCEEDED (::ResolveIt (path.c_str (), realPath)))
	{
		result = realPath;
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	ResolveIt
 *
 * DESCRIPTION:	.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

HRESULT ResolveIt (LPCSTR lpszLinkFile, LPSTR lpszPath)
{
	::CoInitialize(NULL);

	// Get a pointer to the IShellLink interface.

	IShellLink*	psl;
	HRESULT		hres = ::CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
										IID_IShellLink, (void**) &psl);
	if (SUCCEEDED (hres))
	{
		// Get a pointer to the IPersistFile interface.

		IPersistFile*	ppf;
		hres = psl->QueryInterface (IID_IPersistFile, (void**) &ppf);

		if (SUCCEEDED (hres))
		{
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode.
			::MultiByteToWideChar (CP_ACP, 0, lpszLinkFile, -1, wsz, MAX_PATH);

			// Load the shortcut.
			hres = ppf->Load (wsz, STGM_READ);

			if (SUCCEEDED (hres))
			{
				// Resolve the link.
				hres = psl->Resolve (NULL, 0);

				if (SUCCEEDED (hres))
				{
					// Get the path to the link target.
					WIN32_FIND_DATA	wfd;
					char			szGotPath[MAX_PATH];
					hres = psl->GetPath (szGotPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY);

					if (SUCCEEDED (hres))
					{
						lstrcpy (lpszPath, szGotPath);
					}
				}
			}

			// Release the pointer to the IPersistFile interface.
			ppf->Release ();
		}

		// Release the pointer to the IShellLink interface.
		psl->Release ();
	}

	::CoUninitialize();

	return hres;
}
