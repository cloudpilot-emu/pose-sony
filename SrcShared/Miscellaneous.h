/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1999-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#ifndef _MISCELLANEOUS_H_
#define _MISCELLANEOUS_H_

#include "Byteswapping.h"		// Canonical
#include "EmStructs.h"			// DatabaseInfoList

class Chunk;
class EmRect;

void CommonStartup (void);
void CommonShutdown (void);


class StMemory
{
	public:
		StMemory		(	char*	inPtr = NULL);

		StMemory		(	long	inSize,
							Bool	inClearBytes = false);

		~StMemory		();

		operator char*	()			{ return mPtr; }

		char*	Get		() const	{ return mPtr; }
		Bool	IsOwner	() const	{ return mIsOwner; }
		Bool	IsValid	() const	{ return (mPtr != NULL); }

		void	Adopt	(	char*	inPtr);
		char*	Release	() const;
		void	Dispose	();

	protected:
		char*			mPtr;
		mutable Bool	mIsOwner;

	private:
		StMemory	( const StMemory &inPointerBlock);
		StMemory&	operator = (const StMemory &inPointerBlock);
};


class StMemoryMapper
{
	public:
		StMemoryMapper	(const void* memory, long size);
		~StMemoryMapper	(void);

	private:
		const void*	fMemory;
};

template <class T>
class StCanonical
{
	public:
		StCanonical (T& obj) :
			fObject (obj)
		{
			Canonical (fObject);
		}

		~StCanonical ()
		{
			Canonical (fObject);
		}

	private:
		T&	fObject;
};

class StWordSwapper
{
	public:
		StWordSwapper (void* memory, long length) :
			fMemory (memory),
			fLength (length)
		{
			::ByteswapWords (fMemory, fLength);
		}

		~StWordSwapper ()
		{
			::ByteswapWords (fMemory, fLength);
		}

	private:
		void*	fMemory;
		long	fLength;
};

// ================================================================================
//
//	EmValueChanger
//
//		Use EmValueChanger to temporarily change the value of a variable. The
//		constructor saves the old value and sets the new value. The destructor
//		restores the old value.
//
// ================================================================================

template <class T>
class EmValueChanger
{
	public:
								EmValueChanger(T& variable, T newValue) :
									fVariable(variable),
		  							fOrigValue(variable)
								{
									fVariable = newValue;
								}

								~EmValueChanger()
								{
									fVariable = fOrigValue;
								}
	
	private:
		T&						fVariable;
		T						fOrigValue;
};


void LoadAnyFiles (const EmFileRefList& fileList);

void CollectOKObjects(FormPtr frm, vector<UInt16>& okObjects, Bool reportErrors);

Bool PinRectInRect (EmRect& inner, const EmRect& outer);

Bool	IsBound				(void);
Bool	IsBoundPartially	(void);
Bool	IsBoundFully		(void);

// Inline function to turn a trap word (0xA###) into an index into the
// trap table.  The method used here (masking off the uppermost nybble
// instead of, say, subtracting sysTrapBase) matches the ROM.

inline uint16 SysTrapIndex (uint16 trapWord)
{
	return (uint16) (trapWord & ~0xF000);
}

inline uint16 LibTrapIndex (uint16 trapWord)
{
	return (uint16) (SysTrapIndex (trapWord) - SysTrapIndex (sysLibTrapBase));
}

inline Bool IsSystemTrap (uint16 trapWord)
{
	return SysTrapIndex (trapWord) < SysTrapIndex (sysLibTrapBase);
}

inline Bool IsLibraryTrap (uint16 trapWord)
{
	return !IsSystemTrap (trapWord);
}

const Bool	kAllDatabases = false;
const Bool	kApplicationsOnly = true;

void	GetDatabases			(DatabaseInfoList& appList, Bool applicationsOnly);

Bool	IsExecutable			(UInt32 dbType, UInt32 dbCreator, UInt16 dbAttrs);
Bool	IsVisible				(UInt32 dbType, UInt32 dbCreator, UInt16 dbAttrs);
void 	GetLoadableFileList 	(string directoryName, EmFileRefList& fileList);
void	GetFileContents			(const EmFileRef& file, Chunk& contents);

void	InstallCalibrationInfo	(void);
void	ResetCalibrationInfo	(void);
void	ResetClocks				(void);
void	SetHotSyncUserName		(const char*);

void	SeparateList			(StringList& stringList, string str, char delimiter);

void	RunLengthEncode			(void** srcPP, void** dstPP, long srcBytes, long dstBytes);
void	RunLengthDecode			(void** srcPP, void** dstPP, long srcBytes, long dstBytes);
long	RunLengthWorstSize		(long);

void	GzipEncode				(void** srcPP, void** dstPP, long srcBytes, long dstBytes);
void	GzipDecode				(void** srcPP, void** dstPP, long srcBytes, long dstBytes);
long	GzipWorstSize			(long);

int		CountBits				(uint32 v);
inline int		CountBits				(uint16 v) { return CountBits ((uint32) (uint16) v); }
inline int		CountBits				(uint8 v) { return CountBits ((uint32) (uint8) v); }

inline int		CountBits				(int32 v) { return CountBits ((uint32) (uint32) v); }
inline int		CountBits				(int16 v) { return CountBits ((uint32) (uint16) v); }
inline int		CountBits				(int8 v) { return CountBits ((uint32) (uint8) v); }


uint32	NextPowerOf2			(uint32 x);
uint32	DateToDays				(uint32 year, uint32 month, uint32 day);

string	GetLibraryName			(uint16 refNum);

Bool	GetSystemCallContext	(emuptr, SystemCallContext&);

void	GetHostTime				(long* hour, long* min, long* sec);
void	GetHostDate				(long* year, long* month, long* day);

Bool		StartsWith			(const char* s, const char* pattern);
Bool		EndsWith			(const char* s, const char* pattern);
string		Strip				(const char* s, const char*, Bool leading, Bool trailing);
string		Strip				(const string& s, const char*, Bool leading, Bool trailing);
string		ReplaceString		(const string& source,
								 const string& pattern,
								 const string& replacement);
void		FormatInteger		(char* dest, uint32 integer);
const char*	LaunchCmdToString	(UInt16 cmd);

typedef pair <RAMSizeType, string>	MemoryText;
typedef vector <MemoryText>			MemoryTextList;
void GetMemoryTextList (MemoryTextList& memoryList);

void GenerateStackCrawl (EmStackFrameList& frameList);

#endif	// _MISCELLANEOUS_H_
