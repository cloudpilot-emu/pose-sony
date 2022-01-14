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
#include "EmFileRefWin.h"

#include "EmDirRefWin.h"		// PrvMaybeResolveLink
#include "Miscellaneous.h"		// EndsWith
#include "Platform.h"			// Platform::AllocateMemory


const char*	kExtension[] =
{
	NULL,		// kFileTypeNone,
	".exe",		// kFileTypeApplication,
	".rom",		// kFileTypeROM,
	".psf",		// kFileTypeSession,
	".pev",		// kFileTypeEvents
	".ini",		// kFileTypePreference,
	".prc",		// kFileTypePalmApp,
	".pdb",		// kFileTypePalmDB,
	".pqa",		// kFileTypePalmQA,
	".txt",		// kFileTypeText,
	".bmp",		// kFileTypePicture,
	".skin",	// kFileTypeSkin,
	".mwp",		// kFileTypeProfile,
	NULL,		// kFileTypePalmAll,
	NULL		// kFileTypeAll
};


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::EmFileRef
 *
 * DESCRIPTION:	Various ways to make a file reference.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

EmFileRef::EmFileRef (void) :
	fFilePath ()
{
}


EmFileRef::EmFileRef (const EmFileRef& other) :
	fFilePath (other.fFilePath)
{
}


EmFileRef::EmFileRef (const char* path) :
	fFilePath (path)
{
	this->MaybePrependCurrentDirectory ();
	this->MaybeResolveLink ();
}


EmFileRef::EmFileRef (const string& path) :
	fFilePath (path)
{
	this->MaybePrependCurrentDirectory ();
	this->MaybeResolveLink ();
}


EmFileRef::EmFileRef (const EmDirRef& parent, const char* path) :
	fFilePath (parent.GetFullPath () + path)
{
	this->MaybeResolveLink ();
}


EmFileRef::EmFileRef (const EmDirRef& parent, const string& path) :
	fFilePath (parent.GetFullPath () + path)
{
	this->MaybeResolveLink ();
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::EmFileRef
 *
 * DESCRIPTION:	EmFileRef destructor.  Nothing special to do...
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	nothing.
 *
 ***********************************************************************/

EmFileRef::~EmFileRef (void)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::operator=
 *
 * DESCRIPTION:	Assignment operator.  If "other" is not the same as
 *				the controlled object, copy the contents.
 *
 * PARAMETERS:	other - object to copy.
 *
 * RETURNED:	reference to self.
 *
 ***********************************************************************/

EmFileRef&
EmFileRef::operator= (const EmFileRef& other)
{
	if (&other != this)
	{
		fFilePath = other.fFilePath;
	}

	return *this;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::IsSpecified
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
EmFileRef::IsSpecified (void) const
{
	return !fFilePath.empty ();
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::Exists
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
EmFileRef::Exists (void) const
{
	if (this->IsSpecified ())
	{
		DWORD	result = ::GetFileAttributes (fFilePath.c_str ());

		return result != 0xFFFFFFFF;
	}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::Delete
 *
 * DESCRIPTION:	Delete the managed file from the file system.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
EmFileRef::Delete (void) const
{
	::DeleteFile (fFilePath.c_str ());
}


/***********************************************************************
 *
 * FUNCTION:    EmFileRef::IsType
 *
 * DESCRIPTION: DESCRIPTION
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

Bool
EmFileRef::IsType (EmFileType type) const
{
	if (fFilePath.size () > 4 &&
		kExtension[type] != NULL &&
		::EndsWith (fFilePath.c_str (), kExtension[type]))
	{
		return true;
	}

	// Add special hacks for ROM files.
	if (type == kFileTypeROM &&
		(::StartsWith (fFilePath.c_str(), "rom.") ||
		::EndsWith (fFilePath.c_str(), ".widebin")))
	{
		return true;
	}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::SetCreatorAndType
 *
 * DESCRIPTION: Set the Finder type and creator information of the
 *				managed file.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
EmFileRef::SetCreatorAndType (EmFileCreator creator, EmFileType fileType) const
{
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::GetAttr
 *
 * DESCRIPTION: Get basic file attributes of the managed file.
 *
 * PARAMETERS:	A pointer to an integer where the mode bits will be stored.
 *
 * RETURNED:	An integer containing an errno style error result, 0 for no error.
 *
 ***********************************************************************/

int
EmFileRef::GetAttr (int * mode) const
{
	EmAssert(mode);
	
	*mode = 0;
	
	if (!IsSpecified())
		return ENOENT;
	
	DWORD attr = GetFileAttributes(GetFullPath().c_str());

	if (attr == 0xFFFFFFFF)
		return ENOENT; // We should translate more error codes. But which ones?

	if (attr & FILE_ATTRIBUTE_SYSTEM)
		*mode |= kFileAttrSystem;

	if (attr & FILE_ATTRIBUTE_HIDDEN)
		*mode |= kFileAttrHidden;

	if (attr & FILE_ATTRIBUTE_READONLY)
		*mode |= kFileAttrReadOnly;
	
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::SetAttr
 *
 * DESCRIPTION: Set basic file attributes of the managed file.
 *
 * PARAMETERS:	An integer containing bits from the EmFileAttr enum.
 *
 * RETURNED:	An integer containing an errno style error result, 0 for no error.
 *
 ***********************************************************************/

int
EmFileRef::SetAttr (int mode) const
{
	if (!IsSpecified())
		return ENOENT;
			
	DWORD attr = GetFileAttributes(GetFullPath().c_str());

	if (attr == 0xFFFFFFFF)
		return ENOENT; // We should translate more error codes. But which ones?

	attr &= ~(FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);

	if (mode & kFileAttrSystem)
		attr |= FILE_ATTRIBUTE_SYSTEM;
		
	if (mode & kFileAttrHidden)
		attr |= FILE_ATTRIBUTE_HIDDEN;

	if (mode & kFileAttrReadOnly)
		attr |= FILE_ATTRIBUTE_READONLY;

	if (SetFileAttributes(GetFullPath().c_str(), attr)==0)
		return EACCES; // We should translate more error codes. But which ones?
		
	return 0;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::GetName
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
EmFileRef::GetName (void) const
{
	string	result;

	if (this->IsSpecified ())
	{
		char	name[_MAX_FNAME];
		char	ext[_MAX_EXT];

		_splitpath (fFilePath.c_str (), NULL, NULL, name, ext);

		strcat (name, ext);

		result = string (name);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::GetParent
 *
 * DESCRIPTION:	Returns an object representing the parent (or container)
 *				of the managed file.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	An object representing the file's parent.
 *
 ***********************************************************************/

EmDirRef
EmFileRef::GetParent (void) const
{
	EmDirRef	result;

	if (this->IsSpecified ())
	{
		char	path[_MAX_PATH];
		char	drive[_MAX_DRIVE];
		char	dir[_MAX_DIR];
		char	name[_MAX_FNAME];
		char	ext[_MAX_EXT];

		_splitpath (fFilePath.c_str (), drive, dir, name, ext);
		_makepath (path, drive, dir, NULL, NULL);

		result = EmDirRef (path);
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::GetFullPath
 *
 * DESCRIPTION:	Get a full (platform-specific) path to the object.
 *
 * PARAMETERS:	none.
 *
 * RETURNED:	An string representing the file's path.
 *
 ***********************************************************************/

string
EmFileRef::GetFullPath (void) const
{
	return fFilePath;
}


/***********************************************************************
 *
 * FUNCTION:	EmFileRef::operator==
 * FUNCTION:	EmFileRef::operator!=
 * FUNCTION:	EmFileRef::operator>
 * FUNCTION:	EmFileRef::operator<
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
EmFileRef::operator== (const EmFileRef& other) const
{
	return stricmp (fFilePath.c_str (), other.fFilePath.c_str ()) == 0;
}


bool
EmFileRef::operator!= (const EmFileRef& other) const
{
	return stricmp (fFilePath.c_str (), other.fFilePath.c_str ()) != 0;
}


bool
EmFileRef::operator> (const EmFileRef& other) const
{
	return stricmp (fFilePath.c_str (), other.fFilePath.c_str ()) < 0;
}


bool
EmFileRef::operator< (const EmFileRef& other) const
{
	return stricmp (fFilePath.c_str (), other.fFilePath.c_str ()) > 0;
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
EmFileRef::FromPrefString (const string& s)
{
	fFilePath = s;

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
EmFileRef::ToPrefString (void) const
{
	return fFilePath;
}

/***********************************************************************
 *
 * FUNCTION:	MaybePrependCurrentDirectory
 *
 * DESCRIPTION:	Prepend the current working directory if the managed
 *				path is not a full path.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void
EmFileRef::MaybePrependCurrentDirectory (void)
{
	size_t	bufSize = 256;
	char*	buffer = (char*) Platform::AllocateMemory (bufSize);
	LPTSTR	dummy;
	DWORD	numChars;

	while ((numChars = ::GetFullPathName (fFilePath.c_str (), bufSize, buffer, &dummy)) >= bufSize)
	{
		bufSize *= 2;
		buffer = (char*) Platform::ReallocMemory (buffer, bufSize);
	}

	fFilePath = buffer;
	Platform::DisposeMemory (buffer);
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
EmFileRef::MaybeResolveLink (void)
{
	fFilePath = ::PrvMaybeResolveLink (fFilePath);
}
