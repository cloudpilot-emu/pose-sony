/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
	Copyright (c) 1998-2001 Palm, Inc. or its subsidiaries.
	All rights reserved.

	This file is part of the Palm OS Emulator.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
\* ===================================================================== */

#include "EmCommon.h"
#include "HostControl.h"
#include "HostControlPrv.h"

#include "EmBankMapped.h"		// EmBankMapped::GetEmulatedAddress
#include "EmCPU68K.h"			// gCPU68K, gStackHigh, etc.
#include "EmDirRef.h"			// EmDirRefList
#include "EmDlg.h"				// DoGetFile, DoPutFile, DoGetDirectory
#include "EmErrCodes.h"			// kError_NoError
#include "EmExgMgr.h"			// EmExgMgr::GetExgMgr
#include "EmFileImport.h"		// EmFileImport::LoadPalmFileList
#include "EmFileRef.h"			// EmFileRefList
#include "EmPalmStructs.h"		// EmAliasErr
#include "EmRPC.h"				// RPC::HandlingPacket, RPC::DeferCurrentPacket
#include "EmSession.h"			// ResumeByExternal
#include "EmStreamFile.h"		// EmStreamFile
#include "EmStructs.h"			// StringList, ByteList
#include "Hordes.h"				// Hordes::IsOn
#include "LoadApplication.h"	// SavePalmFile
#include "Logging.h"			// LogFile
#include "Miscellaneous.h"		// GetDeviceTextList, GetMemoryTextList
#include "Platform.h"			// Platform::GetShortVersionString
#include "Profiling.h"			// ProfileInit, ProfileStart, ProfileStop, etc.
#include "ROMStubs.h"			// EvtWakeup
#include "Startup.h"			// Startup::ScheduleNewHorde
#include "Strings.r.h"			// kStr_ProfileResults
#include "UAE.h"				// m68k_areg, m68k_dreg, regs
#include "UAE_Utils.h"			// uae_strlen, uae_strcpy

#if HAS_TRACER
#include "TracerPlatform.h"		// Tracer
#endif


#if PLATFORM_WINDOWS

#include <direct.h>				// _mkdir, _rmdir
#include <sys/types.h>			// 
#include <sys/stat.h>			// struct _stat
#include <sys/utime.h>			// _utime, _utimbuf
#include <io.h>					// _open
#include <fcntl.h>				// O_RDWR
#include <time.h>				// asctime, clock, clock_t, etc.

typedef struct _stat	_stat_;
typedef struct _utimbuf _utimbuf_;

#else

#include "sys/stat.h"			// mkdir(???), stat
#include "unistd.h"				// rmdir
#include "fcntl.h"				// O_RDWR
#include "utime.h"				// utime
#include <time.h>				// asctime, clock, clock_t, etc.

#define _O_BINARY	O_BINARY
#define _O_RDWR		O_RDWR

typedef struct stat		_stat_;
typedef struct utimbuf	_utimbuf_;

inline int	_mkdir (const char* path)
{
#if PLATFORM_MAC
	return mkdir (path);
#else
	return mkdir (path, 0777);
#endif
}

inline int	_rmdir (const char* path)
{
	return rmdir (path);
}

inline int	_stat (const char * path, struct stat * buf)
{
	return stat (path, buf);
}

inline int	_open (const char * path, int mode)
{
	return open (path, mode);
}

inline int	_chsize (int s, off_t offset)
{
	return ftruncate (s, offset);
}

inline int	_close (int s)
{
	return close (s);
}

inline int	_utime (const char * path, const utimbuf * times)
{
	return utime (path, times);
}

#endif

struct MyDIR
{
	EmDirRefList	fDirs;
	EmFileRefList	fFiles;

	EmDirRefList::iterator	fDirIter;
	EmFileRefList::iterator	fFileIter;

	int				fState;		// 0 = new, 1 = iterating dirs, 2 = iterating files, 3 = done
};


typedef void		(*HostHandler) (void);

static HostHandler	HostGetHandler (HostControlSelectorType selector);

static Bool 		CollectParameters (int stackOffset, const string& fmt, ByteList& stackData, StringList& stringData);
static void 		PushShort (int& callerStackOffset, ByteList& stackData);
static void 		PushLong (int& callerStackOffset, ByteList& stackData);
static void 		PushDouble (int& callerStackOffset, ByteList& stackData);
static void 		PushLongDouble (int& callerStackOffset, ByteList& stackData);
static void 		PushString (int& callerStackOffset, ByteList& stackData, StringList& stringData);

static string		ToString (emuptr);
static FILE*		ToFILE (emuptr);

static void			GetHostTmType (HostTmType& dest, emuptr src);
static void			PutHostTmType (emuptr dest, const HostTmType& src);
static void			TmFromHostTm (struct tm& dest, const HostTmType& src);
static void			HostTmFromTm (HostTmType& dest, const struct tm& src);

static void			MapAndReturn (const void* p, long size);
static void			MapAndReturn (string& s);
static void			PrvReturnString (const char* p);
static void			PrvReturnString (const string& s);

static void			PrvReleaseAllResources (void);

static emuptr		PrvMalloc (long size);
static emuptr		PrvRealloc (emuptr p, long size);
static void			PrvFree (emuptr p);


// Write to this "file" if you want to intertwine your
// output with that created by any host logging facilities
// (such as event logging).

#define hostLogFile ((HostFILEType*) -1)
#define hostLogFILE ((FILE*) -1)


// Macros for extracting parameters from the emulated stack

#define PARAMETER_SIZE(x)	\
	(sizeof (((StackFrame*) 0)->x))

#define PARAMETER_OFFSET(x) \
	(m68k_areg (regs, 7) + sizeof (HostControlSelectorType) + offsetof (StackFrame, x))

#define GET_PARAMETER(x)		\
	((PARAMETER_SIZE(x) == sizeof (char)) ? EmMemGet8 (PARAMETER_OFFSET(x)) :	\
	 (PARAMETER_SIZE(x) == sizeof (short)) ? EmMemGet16 (PARAMETER_OFFSET(x)) :	\
											EmMemGet32 (PARAMETER_OFFSET(x)))

inline int __fclose (FILE* f)
{
	if (f == hostLogFILE)
	{
		return 0;
	}

	return fclose (f);
}

inline int __feof (FILE* f)
{
	return feof (f);
}

inline int __ferror (FILE* f)
{
	return ferror (f);
}

inline int __fflush (FILE* f)
{
	if (f == hostLogFILE)
	{
		LogDump ();
		return 0;
	}

	return fflush (f);
}

inline int __fgetc (FILE* f)
{
	if (f == hostLogFILE)
	{
		return EOF;
	}

	return fgetc (f);
}

inline char* __fgets (char* s, int n, FILE* f)
{
	if (f == hostLogFILE)
	{
		return NULL;
	}

	return fgets (s, n, f);
}

inline int __vfprintf (FILE* f, const char* fmt, va_list args)
{
	if (f == hostLogFILE)
	{
		return LogGetStdLog ()->VPrintf (fmt, args);
	}

	return vfprintf (f, fmt, args);
}

inline int __fputc (int c, FILE* f)
{
	if (f == hostLogFILE)
	{
		return LogGetStdLog ()->Printf ("%c", c);
	}

	return fputc (c, f);
}

inline int __fputs (const char* s, FILE* f)
{
	if (f == hostLogFILE)
	{
		return LogGetStdLog ()->Printf ("%s", s);
	}

	return fputs (s, f);
}

inline size_t __fread (void* buffer, size_t size, size_t count, FILE* f)
{
	if (f == hostLogFILE)
	{
		return 0;
	}

	return fread (buffer, size, count, f);
}

inline int __fseek (FILE* f, long offset, int origin)
{
	if (f == hostLogFILE)
	{
		return -1;
	}

	return fseek (f, offset, origin);
}

inline long __ftell (FILE* f)
{
	if (f == hostLogFILE)
	{
		return -1;
	}

	return ftell (f);
}

inline size_t __fwrite (const void* buffer, size_t size, size_t count, FILE* f)
{
	if (f == hostLogFILE)
	{
		return LogGetStdLog ()->Write (buffer, size * count);
	}

	return fwrite (buffer, size, count, f);
}

static int translate_err_no(int err_no)
{
	switch (err_no)
	{
		case 0:				return hostErrNone;
		case EACCES:
		case EPERM:			return hostErrPermissions;
		case ENOENT:		return hostErrFileNotFound;
		case EIO:			return hostErrDiskError;
		case EBADF:			return hostErrInvalidParameter;
		case ENOMEM:		return hostErrOutOfMemory;
		case EFAULT:		return hostErrInvalidParameter;
		case EEXIST:		return hostErrExists;
		case ENOTDIR:		return hostErrNotADirectory;
		case EISDIR:		return hostErrIsDirectory;
		case EINVAL:		return hostErrInvalidParameter;
		case ENFILE:
		case EMFILE:		return hostErrTooManyFiles;
		case EFBIG:			return hostErrFileTooBig;
		case ENOSPC:		return hostErrDiskFull;
		case EROFS:			return hostErrReadOnlyFS;
		case ENAMETOOLONG:	return hostErrFileNameTooLong;
		case ENOTEMPTY:		return hostErrDirNotEmpty;
		case ENOSYS:
		case ENODEV:		return hostErrOpNotAvailable;
		default:			return hostErrUnknownError;
	}
}

// The following functions define a bunch of StackFrame structs.
// These structs need to mirror the format of parameters pushed
// onto the stack by the emulated code, and so need to be packed
// to 2-byte boundaries.
//
// The pragmas are reversed at the end of the file.

#include "PalmPack.h"



HostHandler		gHandlerTable [hostSelectorLastTrapNumber];

vector<FILE*>	gOpenFiles;
vector<MyDIR*>	gOpenDirs;
vector<void*>	gAllocatedBlocks;
HostDirEntType	gHostDirEnt;
string			gResultString;
HostTmType		gGMTime;
HostTmType		gLocalTime;


// ---------------------------------------------------------------------------
//		¥ HandleHostControlCall
// ---------------------------------------------------------------------------

CallROMType HandleHostControlCall (void)
{
	HostControlSelectorType selector = EmMemGet16 (m68k_areg (regs, 7) + 0);
	HostHandler				fn = HostGetHandler (selector);

	if (fn)
	{
		fn ();
	}
	else
	{
		// Return NULL/0 for unknown selectors
		m68k_areg (regs, 0) = 0;
		m68k_dreg (regs, 0) = 0;
	}

	return kSkipROM;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostGetHostVersion
// ---------------------------------------------------------------------------

static void _HostGetHostVersion (void)
{
	// long HostHostGetVersion (void)

	struct StackFrame
	{
	};

	enum { kMajor, kMinor, kFix, kBuild };

	int major		= 0;
	int minor		= 0;
	int fix 		= 0;
	int stage		= sysROMStageRelease;
	int buildNum	= 0;
	int state		= kMajor;

	string				version (Platform::GetShortVersionString ());
	string::iterator	iter;

	for (iter = version.begin (); iter != version.end (); ++iter)
	{
		char	ch = *iter;

		switch (state)
		{
			case kMajor:
				if (isdigit (ch))
					major = major * 10 + ch - '0';
				else if (ch == '.')
					state = kMinor;
				else
					goto VersionParseDone;
				break;

			case kMinor:
			case kFix:
				if (isdigit (ch))
				{
					if (state == kMinor)
						minor = minor * 10 + ch - '0';
					else
						fix = fix * 10 + ch - '0';
				}
				else if (ch == '.')
				{
					if (state == kMinor)
						state = kFix;
					else
						goto VersionParseDone;
				}
				else if (ch == 'd')
				{
					stage = sysROMStageDevelopment;
					state = kBuild;
				}
				else if (ch == 'a')
				{
					stage = sysROMStageAlpha;
					state = kBuild;
				}
				else if (ch == 'b')
				{
					stage = sysROMStageBeta;
					state = kBuild;
				}
				else
					goto VersionParseDone;
				break;

			case kBuild:
				if (isdigit (ch))
					buildNum = buildNum * 10 + ch - '0';
				else
					goto VersionParseDone;
				break;
		}
	}

VersionParseDone:
	
	m68k_dreg (regs, 0) = sysMakeROMVersion (major, minor, fix, stage, buildNum);
}


// ---------------------------------------------------------------------------
//		¥ _HostGetHostID
// ---------------------------------------------------------------------------

static void _HostGetHostID (void)
{
	// HostHostID HostHostGetID (void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = hostIDPalmOSEmulator;
}


// ---------------------------------------------------------------------------
//		¥ _HostGetHostPlatform
// ---------------------------------------------------------------------------

static void _HostGetHostPlatform (void)
{
	// HostPlatform HostHostGetPlatform (void)

	struct StackFrame
	{
	};

#if PLATFORM_WINDOWS
	m68k_dreg (regs, 0) = hostPlatformWindows;
#elif PLATFORM_MAC
	m68k_dreg (regs, 0) = hostPlatformMacintosh;
#elif PLATFORM_UNIX
	m68k_dreg (regs, 0) = hostPlatformUnix;
#else
	#error "Unsupported platform"
#endif
}


// ---------------------------------------------------------------------------
//		¥ _HostIsSelectorImplemented
// ---------------------------------------------------------------------------

static void _HostIsSelectorImplemented (void)
{
	// HostBoolType HostHostIsSelectorImplemented (long)

	struct StackFrame
	{
		long	selector;
	};

	HostControlSelectorType	selector = GET_PARAMETER (selector);
	HostHandler				fn = HostGetHandler (selector);

	m68k_dreg (regs, 0) = (fn != NULL);
}


// ---------------------------------------------------------------------------
//		¥ _HostGestalt
// ---------------------------------------------------------------------------

static void _HostGestalt (void)
{
	// HostErr HostHostGestalt (long gestSel, long* response)

	struct StackFrame
	{
		long	gestSel;
		long*	response;
	};

	m68k_dreg (regs, 0) = hostErrUnknownGestaltSelector;
}


// ---------------------------------------------------------------------------
//		¥ _HostIsCallingTrap
// ---------------------------------------------------------------------------

static void _HostIsCallingTrap (void)
{
	// HostBoolType HostIsCallingTrap (void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = gSession->IsNested ();
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostProfileInit
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileInit (void)
{
	// HostErr HostProfileInit (long maxCalls, long maxDepth)

	struct StackFrame
	{
		long	maxCalls;
		long	maxDepth;
	};

	if (!::ProfileCanInit ())
	{
		m68k_dreg (regs, 0) = hostErrProfilingNotReady;
		return;
	}

	long	maxCalls = GET_PARAMETER (maxCalls);
	long	maxDepth = GET_PARAMETER (maxDepth);

	::ProfileInit (maxCalls, maxDepth);

	m68k_dreg (regs, 0) = hostErrNone;
}

#endif


// ---------------------------------------------------------------------------
//		¥ _HostProfileStart
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileStart (void)
{
	// HostErr HostProfileStart (void)

	struct StackFrame
	{
	};

	if (!::ProfileCanStart ())
	{
		m68k_dreg (regs, 0) = hostErrProfilingNotReady;
		return;
	}

	::ProfileStart ();

	m68k_dreg (regs, 0) = hostErrNone;
}

#endif


// ---------------------------------------------------------------------------
//		¥ _HostProfileStop
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileStop (void)
{
	// HostErr HostProfileStop (void)

	struct StackFrame
	{
	};

	if (!::ProfileCanStop ())
	{
		m68k_dreg (regs, 0) = hostErrProfilingNotReady;
		return;
	}

	::ProfileStop ();

	m68k_dreg (regs, 0) = hostErrNone;
}

#endif


// ---------------------------------------------------------------------------
//		¥ _HostProfileDump
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileDump (void)
{
	// HostErr HostProfileDump (const char* filename)

	struct StackFrame
	{
		const char*	fileNameP;
	};

	if (!::ProfileCanDump ())
	{
		m68k_dreg (regs, 0) = hostErrProfilingNotReady;
		return;
	}

	// Get the caller's parameters.

	emuptr fileNameP = GET_PARAMETER (fileNameP);

	// Check the parameters.

	if (fileNameP)
	{
		string	fileName;
		fileName = ToString (fileNameP);
		::ProfileDump (fileName.c_str ());
	}
	else
	{
		::ProfileDump (NULL);
	}


	m68k_dreg (regs, 0) = 0;	// !!! TBD
}

#endif


// ---------------------------------------------------------------------------
//		¥ _HostProfileCleanup
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileCleanup (void)
{
	// HostErr HostProfileCleanup (void)

	struct StackFrame
	{
	};

	if (!::ProfileCanInit ())
	{
		m68k_dreg (regs, 0) = hostErrProfilingNotReady;
		return;
	}

	// ProfileCleanup is now performed at the end of a dump.
//	::ProfileCleanup ();

	m68k_dreg (regs, 0) = hostErrNone;
}

#endif


// ---------------------------------------------------------------------------
//		¥ _HostProfileDetailFn
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileDetailFn (void)
{
	// HostErr _HostProfileDetailFn (void* addr, HostBoolType logDetails)

	struct StackFrame
	{
		void*			addr;
		HostBoolType	logInstructions;
	};

	emuptr	addr = GET_PARAMETER (addr);
	Bool	logInstructions = GET_PARAMETER (logInstructions);

	ProfileDetailFn (addr, logInstructions);

	m68k_dreg (regs, 0) = hostErrNone;
}

#endif


// ---------------------------------------------------------------------------
//		¥ _HostProfileGetCycles
// ---------------------------------------------------------------------------
#if HAS_PROFILING

static void _HostProfileGetCycles (void)
{
	// long	HostProfileGetCycles(void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = gClockCycles;
}

#endif


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostErrNo
// ---------------------------------------------------------------------------

static void _HostErrNo (void)
{
	// long HostErrNo (void)

	struct StackFrame
	{
	};

	// Return the result.

	m68k_dreg (regs, 0) = (uint32) translate_err_no(errno);
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostFClose
// ---------------------------------------------------------------------------

static void _HostFClose (void)
{
	// long HostFClose (HostFILEType*)

	struct StackFrame
	{
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = (uint32) EOF;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result = __fclose (f);

	vector<FILE*>::iterator	iter = gOpenFiles.begin ();
	while (iter != gOpenFiles.end ())
	{
		if (*iter == f)
		{
			gOpenFiles.erase (iter);
			break;
		}

		++iter;
	}

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFEOF
// ---------------------------------------------------------------------------

static void _HostFEOF (void)
{
	// long HostFEOF (HostFILEType*)

	struct StackFrame
	{
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = 1;	// At end of file (right choice?)
		return;
	}

	// Call the function.

	int 	result = __feof (f);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFError
// ---------------------------------------------------------------------------

static void _HostFError (void)
{
	// long HostFError (HostFILEType*)

	struct StackFrame
	{
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result = __ferror (f);

	// Return the result.

	m68k_dreg (regs, 0) = translate_err_no(result);
}


// ---------------------------------------------------------------------------
//		¥ _HostFFlush
// ---------------------------------------------------------------------------

static void _HostFFlush (void)
{
	// long HostFFlush (HostFILEType*)

	struct StackFrame
	{
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = (uint32) EOF;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result = __fflush (f);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFGetC
// ---------------------------------------------------------------------------

static void _HostFGetC (void)
{
	// long HostFGetC (HostFILEType*)

	struct StackFrame
	{
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = (uint32) EOF;	// No file, no data...
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result = __fgetc (f);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFGetPos
// ---------------------------------------------------------------------------

static void _HostFGetPos (void)
{
	// long HostFGetPos (HostFILEType*, long*)

	struct StackFrame
	{
		HostFILEType*	f;
		long*			posP;
	};

	// Get the caller's parameters.

	FILE*	f		= ToFILE (GET_PARAMETER (f));
	emuptr	posP	= GET_PARAMETER (posP);

	// Check the parameters.

	if (!f || !posP)
	{
		m68k_dreg (regs, 0) = 1;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	long	pos = __ftell (f);

	// If the function succeeded, return the position in
	// the memory location pointed to by "posP".

	if (pos >= 0)	// success
	{
		EmMemPut32 (posP, pos);
	}

	// Return the result.

	m68k_dreg (regs, 0) = pos == -1 ? 1 : 0;
}


// ---------------------------------------------------------------------------
//		¥ _HostFGetS
// ---------------------------------------------------------------------------

static void _HostFGetS (void)
{
	// char* HostFGetS (char*, long n, HostFILEType*)

	struct StackFrame
	{
		char*			s;
		long			n;
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	emuptr	s = GET_PARAMETER (s);
	uint32	n = GET_PARAMETER (n);
	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f || !s)
	{
		m68k_dreg (regs, 0) = EmMemNULL;
		errno = hostErrInvalidParameter;
		return;
	}

	// Create a temporary string and read the data into it
	// so that we can later sort out the wordswapping.

	if (n > 0)
	{
		string	tempString (n, 'a');

		// Call the function.

		char*	result = __fgets (&tempString[0], (int) n, f);

		// If we were able to read the string, copy it into the
		// user's buffer (using uae_strcpy to take care of real
		// <-> emulated memory mapping.  If the read failed,
		// return NULL.

		if (result != 0)
		{
			uae_strcpy (s, tempString.c_str ());
		}
		else
		{
			s = EmMemNULL;
		}
	}

	// Return the result.

	m68k_areg (regs, 0) = s;
}


// ---------------------------------------------------------------------------
//		¥ _HostFOpen
// ---------------------------------------------------------------------------

static void _HostFOpen (void)
{
	// HostFILEType* HostFOpen (const char*, const char*)

	struct StackFrame
	{
		const char*	name;
		const char*	mode;
	};

	// Get the caller's parameters.

	emuptr	name = GET_PARAMETER (name);
	emuptr	mode = GET_PARAMETER (mode);

	// Check the parameters.

	if (!name || !mode)
	{
		m68k_areg (regs, 0) = EmMemNULL;
		errno = hostErrInvalidParameter;
		return;
	}

	// Copy the strings from emulated memory into real
	// memory; that way, we un-wordswap the strings
	// on little-endian platforms.

	string	name2 (ToString (name));
	string	mode2 (ToString (mode));

	// Call the function.

	FILE*	result = fopen (name2.c_str (), mode2.c_str ());

	if (result)
	{
		gOpenFiles.push_back (result);
	}

	// Return the result.

	m68k_areg (regs, 0) = (uint32) result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFPrintF
// ---------------------------------------------------------------------------

static void _HostFPrintF (void)
{
	// long HostFPrintF (HostFILEType*, const char*, ...)

	struct StackFrame
	{
		HostFILEType*	f;
		const char*		fmt;
	};

	// Get the caller's parameters.

	FILE*	f	= ToFILE (GET_PARAMETER (f));
	emuptr	fmt = GET_PARAMETER (fmt);

	// Check the parameters.

	if (!f || !fmt)
	{
		m68k_dreg (regs, 0) = (uint32) EOF;
		errno = hostErrInvalidParameter;
		return;
	}

	// Make a copy of the format string. We'll need it eventually
	// when we call vfprintf, and getting it now will allow us to
	// access the format string characters more quickly.

	string	fmt2 (ToString (fmt));

	// Collect the specified parameters. We need to make copies of everything
	// so that it's in the right endian order and to reverse any effects
	// of wordswapping.  The values specified on the stack (the integers,
	// chars, doubles, pointers, etc.) get converted and placed in stackData.
	// The data pointed to by the pointers gets converted and placed in stringData.

	ByteList	stackData;
	StringList stringData;

	if (!::CollectParameters (	sizeof (HostControlSelectorType) +
								sizeof (HostFILEType*) +
								sizeof (char*), fmt2, stackData, stringData))
	{
		m68k_dreg (regs, 0) = (uint32) EOF;
		errno = hostErrInvalidParameter;
		return;
	}

	// Write everything out to the file using vfprintf.

	int 	result = __vfprintf (f, fmt2.c_str (), (va_list) &stackData[0]);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFPutC
// ---------------------------------------------------------------------------

static void _HostFPutC (void)
{
	// long HostFPutC (long c, HostFILEType*)

	struct StackFrame
	{
		long			c;
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	uint32	ch	= GET_PARAMETER (c);
	FILE*	f	= ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = (uint32) EOF;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result = __fputc ((int) ch, f);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFPutS
// ---------------------------------------------------------------------------

static void _HostFPutS (void)
{
	// long HostFPutS (const char*, HostFILEType*)

	struct StackFrame
	{
		const char*		s;
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	emuptr	str = GET_PARAMETER (s);
	FILE*	f	= ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f || !str)
	{
		m68k_dreg (regs, 0) = (uint32) EOF;
		errno = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	str2 (ToString (str));

	// Call the function.

	int 	result = __fputs (str2.c_str (), f);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFRead
// ---------------------------------------------------------------------------

static void _HostFRead (void)
{
	// long HostFRead (void*, long, long, HostFILEType*)

	struct StackFrame
	{
		void*			buffer;
		long			size;
		long			count;
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	emuptr	buffer	= GET_PARAMETER (buffer);
	uint32	size	= GET_PARAMETER (size);
	uint32	count	= GET_PARAMETER (count);
	FILE*	f		= ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f || !buffer)
	{
		m68k_dreg (regs, 0) = 0;
		errno = hostErrInvalidParameter;
		return;
	}

	// Allocate a temporary buffer to hold the data read from
	// disk. We use a temporary buffer so that we can sort out
	// real <-> emulated memory mapping issues with uae_memcpy.

	void*	tempBuffer = malloc (size * count);

	// Call the function.

	size_t	result = __fread (tempBuffer, size, count, f);

	// If the read succeeded, copy the data into the user's buffer.
	
	if (result)
	{
		uae_memcpy (buffer, tempBuffer, size * result);
	}
	
	free (tempBuffer);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFReopen
// ---------------------------------------------------------------------------

#if 0
static void _HostFReopen (void)
{
}
#endif


// ---------------------------------------------------------------------------
//		¥ _HostFScanF
// ---------------------------------------------------------------------------

#if 0
static void _HostFScanF (void)
{
}
#endif


// ---------------------------------------------------------------------------
//		¥ _HostFSeek
// ---------------------------------------------------------------------------

static void _HostFSeek (void)
{
	// long HostFSeek (HostFILEType*, long offset, long origin)

	struct StackFrame
	{
		HostFILEType*	f;
		long			offset;
		long			origin;
	};

	// Get the caller's parameters.

	FILE*	f		= ToFILE (GET_PARAMETER (f));
	uint32	offset	= GET_PARAMETER (offset);
	uint32	origin	= GET_PARAMETER (origin);

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = (uint32) -1;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result	= __fseek (f, offset, (int) origin);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFSetPos
// ---------------------------------------------------------------------------

static void _HostFSetPos (void)
{
	// long HostFSetPos (HostFILEType*, long)

	struct StackFrame
	{
		HostFILEType*		f;
		long*				posP;
	};

	// Get the caller's parameters.

	FILE*	f		= ToFILE (GET_PARAMETER (f));
	emuptr	posP	= GET_PARAMETER (posP);

	long	pos		= EmMemGet32 (posP);

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = 1;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	int 	result = __fseek (f, pos, SEEK_SET);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFTell
// ---------------------------------------------------------------------------

static void _HostFTell (void)
{
	// long HostFTell (HostFILEType*)

	struct StackFrame
	{
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	FILE*	f = ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f)
	{
		m68k_dreg (regs, 0) = (uint32) -1;
		errno = hostErrInvalidParameter;
		return;
	}

	// Call the function.

	long	result = __ftell (f);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostFWrite
// ---------------------------------------------------------------------------

static void _HostFWrite (void)
{
	// long HostFWrite (const void*, long, long, HostFILEType*)

	struct StackFrame
	{
		const void*		buffer;
		long			size;
		long			count;
		HostFILEType*	f;
	};

	// Get the caller's parameters.

	emuptr	buffer	= GET_PARAMETER (buffer);
	uint32	size	= GET_PARAMETER (size);
	uint32	count	= GET_PARAMETER (count);
	FILE*	f		= ToFILE (GET_PARAMETER (f));

	// Check the parameters.

	if (!f || !buffer)
	{
		m68k_dreg (regs, 0) = 0;
		errno = hostErrInvalidParameter;
		return;
	}

	// Allocate a temporary buffer to hold the data being written
	// to disk. We use a temporary buffer so that we can sort out
	// real <-> emulated memory mapping issues with uae_memcpy.

	void*	tempBuffer = malloc (size * count);
	uae_memcpy (tempBuffer, buffer, size * count);

	// Call the function.

	size_t	result = __fwrite (tempBuffer, size, count, f);

	free (tempBuffer);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostRemove
// ---------------------------------------------------------------------------

static void _HostRemove (void)
{
	// long			HostRemove(const char* name)

	struct StackFrame
	{
		const char*	name;
	};

	// Get the caller's parameters.

	emuptr		name = GET_PARAMETER (name);
	const char*	nameP = NULL;
	string		nameStr;

	if (name)
	{
		nameStr = ToString (name);
		nameP = nameStr.c_str ();
	}

	// Call the function.

	int	result = remove (nameP);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostRename
// ---------------------------------------------------------------------------

static void _HostRename (void)
{
	// long			HostRename(const char* oldName, const char* newName)

	struct StackFrame
	{
		const char*	oldName;
		const char*	newName;
	};

	// Get the caller's parameters.

	emuptr		oldName = GET_PARAMETER (oldName);
	const char*	oldNameP = NULL;
	string		oldNameStr;

	if (oldName)
	{
		oldNameStr = ToString (oldName);
		oldNameP = oldNameStr.c_str ();
	}

	emuptr		newName = GET_PARAMETER (newName);
	const char*	newNameP = NULL;
	string		newNameStr;

	if (newName)
	{
		newNameStr = ToString (newName);
		newNameP = newNameStr.c_str ();
	}

	// Call the function.

	int	result = rename (oldNameP, newNameP);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostTmpFile
// ---------------------------------------------------------------------------

static void _HostTmpFile (void)
{
	// HostFILEType* HostTmpFile (void)

	struct StackFrame
	{
	};

	// Get the caller's parameters.

	// Call the function.

	FILE*		result = tmpfile ();

	// Return the result.

	m68k_areg (regs, 0) = (uint32) result;
}


// ---------------------------------------------------------------------------
//		¥ _HostTmpNam
// ---------------------------------------------------------------------------

static void _HostTmpNam (void)
{
	struct StackFrame
	{
		char*	nameP;
	};

	emuptr	nameP = GET_PARAMETER (nameP);

	char*	result = tmpnam (NULL);

	if (!result)
	{
		m68k_areg (regs, 0) = EmMemNULL;
	}
	else
	{
		if (nameP)
		{
			uae_strcpy (nameP, result);
			m68k_areg (regs, 0) = nameP;
		}
		else
		{
			::PrvReturnString (result);
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostGetEnv
// ---------------------------------------------------------------------------

static void _HostGetEnv (void)
{
	// char* HostGetEnv (const char*)

	struct StackFrame
	{
		const char*	name;
	};

	// Get the caller's parameters.

	emuptr	name	= GET_PARAMETER (name);

	// Check the parameters.

	if (!name)
	{
		m68k_areg (regs, 0) = EmMemNULL;
		errno = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	name2 (ToString (name));

	// Call the function.

	char*	value = getenv (name2.c_str ());

	::PrvReturnString (value);
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostMalloc
// ---------------------------------------------------------------------------

static void _HostMalloc (void)
{
	// void* HostMalloc(long size)

	struct StackFrame
	{
		long	size;
	};

	// Get the caller's parameters.

	long	size = GET_PARAMETER (size);

	m68k_areg (regs, 0) = ::PrvMalloc (size);
}


// ---------------------------------------------------------------------------
//		¥ _HostRealloc
// ---------------------------------------------------------------------------

static void _HostRealloc (void)
{
	// void* HostRealloc(void* p, long size)


	struct StackFrame
	{
		void*	p;
		long	size;
	};

	// Get the caller's parameters.

	emuptr	p = GET_PARAMETER (p);
	long	size = GET_PARAMETER (size);

	if (!p)
	{
		m68k_areg (regs, 0) = ::PrvMalloc (size);
	}
	else if (!size)
	{
		::PrvFree (p);
		m68k_areg (regs, 0) = EmMemNULL;
	}
	else
	{
		m68k_areg (regs, 0) = ::PrvRealloc (p, size);
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostFree
// ---------------------------------------------------------------------------

static void _HostFree (void)
{
	// void HostFree(void* p)

	struct StackFrame
	{
		void*	p;
	};

	// Get the caller's parameters.

	emuptr	p = GET_PARAMETER (p);

	::PrvFree (p);
}


// ---------------------------------------------------------------------------
//		¥ _HostAscTime
// ---------------------------------------------------------------------------

static void _HostAscTime (void)
{
	struct StackFrame
	{
		const HostTmType*	tm;
	};

	emuptr		hisTmP = GET_PARAMETER (tm);
	HostTmType	hisTm;
	struct tm	myTm;

	GetHostTmType (hisTm, hisTmP);
	TmFromHostTm (myTm, hisTm);

	char*	result = asctime (&myTm);

	::PrvReturnString (result);
}


// ---------------------------------------------------------------------------
//		¥ _HostClock
// ---------------------------------------------------------------------------

static void _HostClock (void)
{
	struct StackFrame
	{
	};

	clock_t	result = clock ();

	m68k_dreg (regs, 0) = (HostClockType) result;
}


// ---------------------------------------------------------------------------
//		¥ _HostCTime
// ---------------------------------------------------------------------------

static void _HostCTime (void)
{
	struct StackFrame
	{
		const HostTimeType*	time;
	};

	emuptr	timeP = GET_PARAMETER (time);

	time_t	myTime = EmMemGet32 (timeP);

	char*	result = ctime (&myTime);

	::PrvReturnString (result);
}


// ---------------------------------------------------------------------------
//		¥ _HostDiffTime
// ---------------------------------------------------------------------------

#if 0
static void _HostDiffTime (void)
{
}
#endif


// ---------------------------------------------------------------------------
//		¥ _HostGMTime
// ---------------------------------------------------------------------------

static void _HostGMTime (void)
{
	struct StackFrame
	{
		const HostTimeType*	time;
	};

	emuptr	timeP = GET_PARAMETER (time);

	time_t	myTime = EmMemGet32 (timeP);

	struct tm*	result = gmtime (&myTime);

	HostTmFromTm (gGMTime, *result);

	Canonical (gGMTime.tm_sec_);
	Canonical (gGMTime.tm_min_);
	Canonical (gGMTime.tm_hour_);
	Canonical (gGMTime.tm_mday_);
	Canonical (gGMTime.tm_mon_);
	Canonical (gGMTime.tm_year_);
	Canonical (gGMTime.tm_wday_);
	Canonical (gGMTime.tm_yday_);
	Canonical (gGMTime.tm_isdst_);

	MapAndReturn (&gGMTime, sizeof (gGMTime));
}


// ---------------------------------------------------------------------------
//		¥ _HostLocalTime
// ---------------------------------------------------------------------------

static void _HostLocalTime (void)
{
	struct StackFrame
	{
		const HostTimeType*	time;
	};

	emuptr	timeP = GET_PARAMETER (time);

	time_t	myTime = EmMemGet32 (timeP);

	struct tm*	result = localtime (&myTime);

	HostTmFromTm (gLocalTime, *result);

	Canonical (gLocalTime.tm_sec_);
	Canonical (gLocalTime.tm_min_);
	Canonical (gLocalTime.tm_hour_);
	Canonical (gLocalTime.tm_mday_);
	Canonical (gLocalTime.tm_mon_);
	Canonical (gLocalTime.tm_year_);
	Canonical (gLocalTime.tm_wday_);
	Canonical (gLocalTime.tm_yday_);
	Canonical (gLocalTime.tm_isdst_);

	MapAndReturn (&gLocalTime, sizeof (gLocalTime));
}


// ---------------------------------------------------------------------------
//		¥ _HostMkTime
// ---------------------------------------------------------------------------

static void _HostMkTime (void)
{
	struct StackFrame
	{
		const HostTmType*	tm;
	};

	emuptr		hisTmP = GET_PARAMETER (tm);
	HostTmType	hisTm;
	struct tm	myTm;

	GetHostTmType (hisTm, hisTmP);
	TmFromHostTm (myTm, hisTm);

	time_t	result = mktime (&myTm);

	m68k_dreg (regs, 0) = (HostTimeType) result;
}


// ---------------------------------------------------------------------------
//		¥ _HostStrFTime
// ---------------------------------------------------------------------------

static void _HostStrFTime (void)
{
	struct StackFrame
	{
		char*				strDest;
		HostSizeType		maxsize;
		const char*			format;
		const HostTmType*	timeptr;
	};

	emuptr			strDest	= GET_PARAMETER (strDest);
	HostSizeType	maxsize	= GET_PARAMETER (maxsize);
	emuptr			format	= GET_PARAMETER (format);
	emuptr			timeptr	= GET_PARAMETER (timeptr);

	if (!strDest || !format || !timeptr)
	{
		m68k_dreg (regs, 0) = 0;
		return;
	}

	void*	myDest = malloc (maxsize);

	string	formatStr (ToString (format));
	
	HostTmType	hisTime;
	struct tm	myTime;

	GetHostTmType (hisTime, timeptr);
	TmFromHostTm (myTime, hisTime);

	size_t	result = strftime ((char*) myDest, maxsize, formatStr.c_str (), &myTime);

	uae_memcpy (strDest, myDest, maxsize);

	free (myDest);

	m68k_dreg (regs, 0) = (HostSizeType) result;
}


// ---------------------------------------------------------------------------
//		¥ _HostTime
// ---------------------------------------------------------------------------

static void _HostTime (void)
{
	struct StackFrame
	{
		HostTimeType*	timeP;
	};

	emuptr	timeP = GET_PARAMETER (timeP);

	time_t	result2;
	time_t	result = time (&result2);

	if (timeP)
	{
		EmMemPut32 (timeP, result2);
	}

	m68k_dreg (regs, 0) = (HostTimeType) result;
}


// ---------------------------------------------------------------------------
//		¥ _HostMkDir
// ---------------------------------------------------------------------------

static void _HostMkDir (void)
{
	// int HostMkDir (const char*)

	struct StackFrame
	{
		const char*	name;
	};

	// Get the caller's parameters.

	emuptr		name = GET_PARAMETER (name);
	string		nameStr;
	const char*	nameP = NULL;

	// Check the parameters.

	if (name)
	{
		nameStr = ToString (name);
		nameP = nameStr.c_str ();
	}

	// Call the function.

	int	result = _mkdir (nameP);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostRmDir
// ---------------------------------------------------------------------------

static void _HostRmDir (void)
{
	// int HostRmDir (const char*)

	struct StackFrame
	{
		const char*	name;
	};

	// Get the caller's parameters.

	emuptr		name = GET_PARAMETER (name);
	string		nameStr;
	const char*	nameP = NULL;

	// Check the parameters.

	if (name)
	{
		nameStr = ToString (name);
		nameP = nameStr.c_str ();
	}

	// Call the function.

	int	result = _rmdir (nameP);

	// Return the result.

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostOpenDir
// ---------------------------------------------------------------------------

static void _HostOpenDir (void)
{
	// HostDIRType* HostOpenDir(const char*)

	struct StackFrame
	{
		const char*	name;
	};

	// Get the caller's parameters.

	emuptr		name = GET_PARAMETER (name);
	string		nameStr;
	const char*	nameP = NULL;

	// Check the parameters.

	if (name)
	{
		nameStr = ToString (name);
		nameP = nameStr.c_str ();
	}

	// See if the directory exists.

	EmDirRef	dirRef (nameP);
	if (!dirRef.Exists ())
	{
		m68k_areg (regs, 0) = EmMemNULL;
		return;
	}

	// Create a new MyDIR object to pass back as the HostDIRType.

	MyDIR*	dir = new MyDIR;
	if (!dir)
	{
		m68k_areg (regs, 0) = EmMemNULL;
		return;
	}

	// Initialize the MyDIR with the child entries.

	dirRef.GetChildren (&dir->fFiles, &dir->fDirs);
	dir->fState = 0;

	// Remember this on our list of open directories and
	// pass it back to the user.

	gOpenDirs.push_back (dir);

	m68k_areg (regs, 0) = (uint32) dir;
}


// ---------------------------------------------------------------------------
//		¥ _HostReadDir
// ---------------------------------------------------------------------------

static void _HostReadDir(void)
{
	// HostDirEntType* HostReadDir(HostDIRType*)

	struct StackFrame
	{
		HostDIRType*	hostDir;
	};

	MyDIR*	dir = (MyDIR*) GET_PARAMETER (hostDir);

	// Make sure dir is valid.

	vector<MyDIR*>::iterator	iter = gOpenDirs.begin ();
	while (iter != gOpenDirs.end ())
	{
		// It appears to be on our list, so let's use it.
		if (*iter == dir)
			goto FoundIt;

		++iter;
	}

	// hostDir was not on our list.  Return NULL.

	m68k_areg (regs, 0) = EmMemNULL;
	return;

FoundIt:

	// Initialize our result in case of failure.

	gHostDirEnt.d_name[0] = 0;

	// Get the next dir or file, as the case may be.

	switch (dir->fState)
	{
		// Just starting out.  Initialize the directory iterator
		// and fall through to the code that uses it.

		case 0:
			dir->fDirIter = dir->fDirs.begin ();
			dir->fState = 1;
			// Fall thru

		// Iterating over directories; get the next one.  If there
		// are no more, start iterating over files.

		case 1:
			if (dir->fDirIter != dir->fDirs.end ())
			{
				strcpy (gHostDirEnt.d_name, dir->fDirIter->GetName().c_str());
				++dir->fDirIter;
				break;
			}

			dir->fFileIter = dir->fFiles.begin ();
			dir->fState = 2;
			// Fall thru

		// Iterating over files; get the next one.  If there
		// are no more, stop iterating.

		case 2:
			if (dir->fFileIter != dir->fFiles.end ())
			{
				strcpy (gHostDirEnt.d_name, dir->fFileIter->GetName().c_str());
				++dir->fFileIter;
				break;
			}
			dir->fState = 3;

		// No longer iterating. Just return NULL.

		case 3:
			m68k_areg (regs, 0) = EmMemNULL;
			break;
	}

	// If we're returning a value in gHostDirEnt, make
	// sure it's mapped into emulated space.

	if (gHostDirEnt.d_name[0] != 0)
	{
		emuptr	result = EmBankMapped::GetEmulatedAddress (&gHostDirEnt);
		if (result == EmMemNULL)
		{
			EmBankMapped::MapPhysicalMemory (&gHostDirEnt, sizeof (gHostDirEnt));
			result = EmBankMapped::GetEmulatedAddress (&gHostDirEnt);
		}

		m68k_areg (regs, 0) = result;
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostCloseDir
// ---------------------------------------------------------------------------

static void _HostCloseDir (void)
{
	// long HostCloseDir(HostDIRType*)

	struct StackFrame
	{
		HostDIRType*	hostDir;
	};

	MyDIR*	dir = (MyDIR*) GET_PARAMETER (hostDir);

	// Make sure dir is valid.

	vector<MyDIR*>::iterator	iter = gOpenDirs.begin ();
	while (iter != gOpenDirs.end ())
	{
		if (*iter == dir)
		{
			// It's on our list.  Remove it from the list and delete it.

			gOpenDirs.erase (iter);
			delete dir;
			break;
		}

		++iter;
	}

	// Unmap any mapped gHostDirEnt if we're not iterating over any
	// other directories.

	if (gOpenDirs.size () == 0)
	{
		EmBankMapped::UnmapPhysicalMemory (&gHostDirEnt);
	}

	// Return no error (should we return an error if DIR was not found?)

	m68k_dreg (regs, 0) = 0;
}


// ---------------------------------------------------------------------------
//		¥ _HostStat
// ---------------------------------------------------------------------------

static void _HostStat(void)
{
	// long	HostStat(const char*, HostStat*)

	struct StackFrame
	{
		const char*		nameP;
		HostStatType*	statP;
	};

	// Get the caller's parameters.

	emuptr	nameP	= GET_PARAMETER (nameP);
	emuptr	statP	= GET_PARAMETER (statP);

	// Check the parameters.

	if (!nameP || !statP)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	nameStr (ToString (nameP));

	_stat_	local_stat;
	int	result = _stat (nameStr.c_str (), &local_stat);

	if (result == 0)
	{
		EmMemPut32 (statP + offsetof (HostStatType, st_dev_),			local_stat.st_dev);
		EmMemPut32 (statP + offsetof (HostStatType, st_ino_),			local_stat.st_ino);
		EmMemPut32 (statP + offsetof (HostStatType, st_mode_),		local_stat.st_mode);
		EmMemPut32 (statP + offsetof (HostStatType, st_nlink_),		local_stat.st_nlink);
		EmMemPut32 (statP + offsetof (HostStatType, st_uid_),			local_stat.st_uid);
		EmMemPut32 (statP + offsetof (HostStatType, st_gid_),			local_stat.st_gid);
		EmMemPut32 (statP + offsetof (HostStatType, st_rdev_),		local_stat.st_rdev);
		EmMemPut32 (statP + offsetof (HostStatType, st_atime_),		local_stat.st_atime);
		EmMemPut32 (statP + offsetof (HostStatType, st_mtime_),		local_stat.st_mtime);
		EmMemPut32 (statP + offsetof (HostStatType, st_ctime_),		local_stat.st_ctime);
		EmMemPut32 (statP + offsetof (HostStatType, st_size_),		local_stat.st_size);
#if PLATFORM_WINDOWS
		EmMemPut32 (statP + offsetof (HostStatType, st_blksize_),		0);
		EmMemPut32 (statP + offsetof (HostStatType, st_blocks_),		0);
		EmMemPut32 (statP + offsetof (HostStatType, st_flags_),		0);
#elif PLATFORM_MAC
		EmMemPut32 (statP + offsetof (HostStatType, st_blksize_),		local_stat.st_blksize);
		EmMemPut32 (statP + offsetof (HostStatType, st_blocks_),		local_stat.st_blocks);
		EmMemPut32 (statP + offsetof (HostStatType, st_flags_),		local_stat.st_flags);
#else
		EmMemPut32 (statP + offsetof (HostStatType, st_blksize_),		local_stat.st_blksize);
		EmMemPut32 (statP + offsetof (HostStatType, st_blocks_),		local_stat.st_blocks);
		EmMemPut32 (statP + offsetof (HostStatType, st_flags_),		0);
#endif
	}

	m68k_dreg (regs, 0) = result;
}

// ---------------------------------------------------------------------------
//		¥ _HostTruncate
// ---------------------------------------------------------------------------

static void _HostTruncate (void)
{
	// long HostTruncate(const char*, long)

	struct StackFrame
	{
		const char*		nameP;
		long			size;
	};

	// Get the caller's parameters.

	emuptr	nameP	= GET_PARAMETER (nameP);
	long	size	= GET_PARAMETER (size);

	// Check the parameters.

	if (!nameP || size < 0)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	nameStr (ToString (nameP));

	// Make the call

	int	result;

	int	fd = _open (nameStr.c_str (), _O_BINARY | _O_RDWR);
	if (fd)
	{
		result = _chsize (fd, size);
		_close (fd);
	}
	else
	{
		result = errno;
	}

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostUTime
// ---------------------------------------------------------------------------

static void _HostUTime (void)
{
	struct StackFrame
	{
		const char*		name;
		HostUTimeType*	time;
	};

	emuptr	nameP = GET_PARAMETER (name);
	emuptr	timeP = GET_PARAMETER (time);

	if (!nameP)
	{
		m68k_dreg (regs, 0) = (uint32) -1;
		errno = hostErrInvalidParameter;
		return;
	}

	string	name = ToString (nameP);

	_utimbuf_	buf;
	_utimbuf_*	bufP = NULL;

	if (timeP)
	{
//		buf.crtime	= EmMemGet32 (timeP + offsetof (HostUTimeType, crtime_));
		buf.actime	= EmMemGet32 (timeP + offsetof (HostUTimeType, actime_));
		buf.modtime	= EmMemGet32 (timeP + offsetof (HostUTimeType, modtime_));

		bufP = &buf;
	}

	int	result = _utime (name.c_str (), bufP);

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostGetFileAttr
// ---------------------------------------------------------------------------

static void _HostGetFileAttr(void)
{
	// long	HostGetFileAttr(const char*, unsigned long * attr)

	struct StackFrame
	{
		const char*		nameP;
		unsigned long *	attrP;
	};

	// Get the caller's parameters.

	emuptr	nameP	= GET_PARAMETER (nameP);
	emuptr	attrP	= GET_PARAMETER (attrP);

	// Check the parameters.

	if (!nameP || !attrP)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	nameStr (ToString (nameP));

	m68k_dreg (regs, 0) = hostErrNone;
	
	EmFileRef fRef(nameStr.c_str());
	
	int attr = 0;
	errno = fRef.GetAttr(&attr);
	
	if (errno != 0)
		m68k_dreg (regs, 0) = (uint32) -1;
	
	EmMemPut32 (attrP, attr);
}


// ---------------------------------------------------------------------------
//		¥ _HostSetFileAttr
// ---------------------------------------------------------------------------

static void _HostSetFileAttr(void)
{
	// long	HostSetFileAttr(const char*, unsigned long attr)

	struct StackFrame
	{
		const char*		nameP;
		unsigned long attr;
	};

	// Get the caller's parameters.

	emuptr	nameP	= GET_PARAMETER (nameP);
	unsigned long attr = GET_PARAMETER (attr);

	// Check the parameters.

	if (!nameP)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	nameStr (ToString (nameP));

	m68k_dreg (regs, 0) = hostErrNone;
	
	EmFileRef fRef(nameStr.c_str());
	
	errno = fRef.SetAttr(attr);
	
	if (errno != 0)
		m68k_dreg (regs, 0) = (uint32) -1;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostGremlinIsRunning
// ---------------------------------------------------------------------------

static void _HostGremlinIsRunning (void)
{
	// HostBoolType HostGremlinIsRunning (void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = Hordes::IsOn () ? 1 : 0;
}


// ---------------------------------------------------------------------------
//		¥ _HostGremlinNumber
// ---------------------------------------------------------------------------

static void _HostGremlinNumber (void)
{
	// long HostGremlinNumber (void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = Hordes::GremlinNumber ();
}


// ---------------------------------------------------------------------------
//		¥ _HostGremlinCounter
// ---------------------------------------------------------------------------

static void _HostGremlinCounter (void)
{
	// long HostGremlinCounter (void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = Hordes::EventCounter ();
}


// ---------------------------------------------------------------------------
//		¥ _HostGremlinLimit
// ---------------------------------------------------------------------------

static void _HostGremlinLimit (void)
{
	// long HostGremlinLimit (void)

	struct StackFrame
	{
	};

	m68k_dreg (regs, 0) = Hordes::EventLimit ();
}


// ---------------------------------------------------------------------------
//		¥ _HostGremlinNew
// ---------------------------------------------------------------------------

static void _HostGremlinNew (void)
{
	// HostErr HostGremlinNew (void)

	struct StackFrame
	{
		const HostGremlinInfoType*	info;
	};

	emuptr		infoP		= GET_PARAMETER (info);

	// Get the easy parts.

	HordeInfo	info;

	info.fStartNumber		= EmMemGet32 (infoP + offsetof(HostGremlinInfoType, fFirstGremlin));
	info.fStopNumber		= EmMemGet32 (infoP + offsetof(HostGremlinInfoType, fLastGremlin));
	info.fSaveFrequency		= EmMemGet32 (infoP + offsetof(HostGremlinInfoType, fSaveFrequency));
	info.fSwitchDepth		= EmMemGet32 (infoP + offsetof(HostGremlinInfoType, fSwitchDepth));
	info.fMaxDepth 			= EmMemGet32 (infoP + offsetof(HostGremlinInfoType, fMaxDepth));

	info.OldToNew ();	// Transfer the old fields to the new fields.

	// Get the list of installed applications so that we can compare it to
	// the list of applications requested by the caller.

	DatabaseInfoList	installedAppList;
	::GetDatabases (installedAppList, kApplicationsOnly);

	// Get the list of applications requested by the caller.  Break up the
	// string into a list of individual names, and check to see if the whole
	// thing was preceded by a '-'.

	string		appNames	= ToString (infoP + offsetof(HostGremlinInfoType, fAppNames));

	Bool		exclude = false;
	if (appNames[0] == '-')
	{
		exclude = true;
		appNames = appNames.substr(1);
	}

	StringList	requestedAppList;
	::SeparateList (requestedAppList, appNames, ',');

	// For each requested application, see if it's installed in the device.
	// If so, add it to the list of applications Gremlins should be run on.

	StringList::iterator	iter1 = requestedAppList.begin();
	while (iter1 != requestedAppList.end())
	{
		// Get the application info based on the given name.

		DatabaseInfoList::iterator	iter2 = installedAppList.begin ();
		while (iter2 != installedAppList.end ())
		{
			Bool	addIt;

			if (exclude)
				addIt = *iter1 != iter2->name;
			else
				addIt = *iter1 == iter2->name;

			if (addIt)
			{
				info.fAppList.push_back (*iter2);
			}

			++iter2;
		}

		++iter1;
	}

	// Start up the gremlin sub-system.

//	Hordes::New (info);

	Startup::ScheduleNewHorde (info, StringList());

	m68k_dreg (regs, 0) = errNone;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostImportFile
// ---------------------------------------------------------------------------

static void _HostImportFile (void)
{
	// HostErr HostImportFile (const char* fileName)

	struct StackFrame
	{
		const char*	fileName;
	};

	// Get the caller's parameters.

	emuptr	str = GET_PARAMETER (fileName);

	// Check the parameters.

	if (!str)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	fileName (ToString (str));

	ErrCode result = kError_NoError;
	try
	{
		EmFileRef		fileRef (fileName);
		EmFileRefList	fileList;
		fileList.push_back (fileRef);
		EmFileImport::LoadPalmFileList (fileList, kMethodBest);
	}
	catch (ErrCode errCode)
	{
		result = errCode;
	}

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExportFile
// ---------------------------------------------------------------------------

static void _HostExportFile (void)
{
	// HostErr HostExportFile (const char* dbName, long cardNum, const char* fileName)

	struct StackFrame
	{
		const char*	fileName;
		long		cardNum;
		const char*	dbName;
	};

	// Get the caller's parameters.

	emuptr	fName	= GET_PARAMETER (fileName);
	long	cardNum = GET_PARAMETER (cardNum);
	emuptr	dbName	= GET_PARAMETER (dbName);

	// Check the parameters.

	if (!fName || !dbName)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	fileName (ToString (fName));
	string	databaseName (ToString (dbName));

	ErrCode result = kError_NoError;
	try
	{
		EmFileRef		ref (fileName);
		EmStreamFile	stream (ref, kCreateOrEraseForUpdate,
								kFileCreatorInstaller, kFileTypePalmApp);
		::SavePalmFile (stream, cardNum, databaseName.c_str ());
	}
	catch (ErrCode errCode)
	{
		result = errCode;
	}

	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostSaveScreen
// ---------------------------------------------------------------------------

static void _HostSaveScreen (void)
{
	// HostErr HostSaveScreen (const char* fileName)

	struct StackFrame
	{
		const char*	fileName;
	};

	// Get the caller's parameters.

	emuptr	fName	= GET_PARAMETER (fileName);

	// Check the parameters.

	if (!fName)
	{
		m68k_dreg (regs, 0) = hostErrInvalidParameter;
		return;
	}

	// Copy the string from emulated memory into real
	// memory; that way, we un-wordswap the string
	// on little-endian platforms.

	string	fileName (ToString (fName));

	ErrCode result = kError_NoError;
	try
	{
		EmFileRef		ref (fileName);
		
		Platform::SaveScreen(ref);
	}
	catch (ErrCode errCode)
	{
		result = errCode;
	}

	m68k_dreg (regs, 0) = result;
}


#pragma mark -

#define exgErrBadData			(exgErrorClass | 8)  // internal data was not valid

// ---------------------------------------------------------------------------
//		¥ _HostExgLibOpen
// ---------------------------------------------------------------------------

static void _HostExgLibOpen (void)
{
	// Err ExgLibOpen (UInt16 libRefNum)

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibOpen (libRefNum);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibClose
// ---------------------------------------------------------------------------

static void _HostExgLibClose (void)
{
	// Err ExgLibClose (UInt16 libRefNum)

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibClose (libRefNum);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibSleep
// ---------------------------------------------------------------------------

static void _HostExgLibSleep (void)
{
	// Err ExgLibSleep (UInt16 libRefNum)

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibSleep (libRefNum);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibWake
// ---------------------------------------------------------------------------

static void _HostExgLibWake (void)
{
	// Err ExgLibWake (UInt16 libRefNum)

	struct StackFrame
	{
		UInt16	libRefNum;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibWake (libRefNum);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibHandleEvent
// ---------------------------------------------------------------------------

static void _HostExgLibHandleEvent (void)
{
	// Boolean ExgLibHandleEvent (UInt16 libRefNum, void* eventP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	eventP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	eventP		= GET_PARAMETER (eventP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Boolean	result = exgMgr->ExgLibHandleEvent (libRefNum, eventP);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibConnect
// ---------------------------------------------------------------------------

static void _HostExgLibConnect (void)
{
	// Err ExgLibConnect (UInt16 libRefNum, void* exgSocketP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibConnect (libRefNum, exgSocketP);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibAccept
// ---------------------------------------------------------------------------

static void _HostExgLibAccept (void)
{
	// Err ExgLibAccept (UInt16 libRefNum, void* exgSocketP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibAccept (libRefNum, exgSocketP);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibDisconnect
// ---------------------------------------------------------------------------

static void _HostExgLibDisconnect (void)
{
	// Err ExgLibDisconnect (UInt16 libRefNum, void* exgSocketP, Err error)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
		Err		err;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);
	Err		err			= GET_PARAMETER (err);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibDisconnect (libRefNum, exgSocketP, err);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibPut
// ---------------------------------------------------------------------------

static void _HostExgLibPut (void)
{
	// Err ExgLibPut (UInt16 libRefNum, void* exgSocketP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibPut (libRefNum, exgSocketP);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibGet
// ---------------------------------------------------------------------------

static void _HostExgLibGet (void)
{
	// Err ExgLibGet (UInt16 libRefNum, void* exgSocketP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibGet (libRefNum, exgSocketP);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibSend
// ---------------------------------------------------------------------------

static void _HostExgLibSend (void)
{
	// UInt32 ExgLibSend (UInt16 libRefNum, void* exgSocketP,
	//			const void* const bufP, const UInt32 bufLen, Err* errP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
		void*	bufP;
		UInt32	bufLen;
		Err*	errP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);
	emuptr	bufP		= GET_PARAMETER (bufP);
	UInt32	bufLen		= GET_PARAMETER (bufLen);
	emuptr	errP		= GET_PARAMETER (errP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	StMemory	buffer (bufLen);
	Err			myErr;

	uae_memcpy ((void*) buffer.Get (), bufP, bufLen);

	UInt32	result = exgMgr->ExgLibSend (libRefNum, exgSocketP, buffer.Get (), bufLen, &myErr);
	m68k_dreg (regs, 0) = result;

	EmAliasErr<PAS>	err (errP);
	err = myErr;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibReceive
// ---------------------------------------------------------------------------

static void _HostExgLibReceive (void)
{
	// UInt32 ExgLibReceive (UInt16 libRefNum, void* exgSocketP,
	//			void* bufP, const UInt32 bufLen, Err* errP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
		void*	bufP;
		UInt32	bufLen;
		Err*	errP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);
	emuptr	bufP		= GET_PARAMETER (bufP);
	UInt32	bufLen		= GET_PARAMETER (bufLen);
	emuptr	errP		= GET_PARAMETER (errP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	StMemory	buffer (bufLen);
	Err			myErr;

	UInt32	result = exgMgr->ExgLibReceive (libRefNum, exgSocketP, buffer.Get (), bufLen, &myErr);
	m68k_dreg (regs, 0) = result;

	uae_memcpy (bufP, (const void*) buffer.Get (), bufLen);

	EmAliasErr<PAS>	err (errP);
	err = myErr;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibControl
// ---------------------------------------------------------------------------

static void _HostExgLibControl (void)
{
	// Err ExgLibControl (UInt16 libRefNum, UInt16 op,
	//							 void* valueP, UInt16* valueLenP)

	struct StackFrame
	{
		UInt16	libRefNum;
		UInt16	op;
		void*	valueP;
		UInt16*	valueLenP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	UInt16	op			= GET_PARAMETER (op);
	emuptr	valueP		= GET_PARAMETER (valueP);
	emuptr	valueLenP	= GET_PARAMETER (valueLenP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibControl (libRefNum, op, valueP, valueLenP);
	m68k_dreg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ _HostExgLibRequest
// ---------------------------------------------------------------------------

static void _HostExgLibRequest (void)
{
	// Err ExgLibRequest (UInt16 libRefNum, void* exgSocketP)

	struct StackFrame
	{
		UInt16	libRefNum;
		void*	exgSocketP;
	};

	// Get the caller's parameters.

	UInt16	libRefNum	= GET_PARAMETER (libRefNum);
	emuptr	exgSocketP	= GET_PARAMETER (exgSocketP);

	// Check the parameters.

	EmExgMgr*	exgMgr	= EmExgMgr::GetExgMgr (libRefNum);
	if (!exgMgr)
	{
		m68k_dreg (regs, 0) = exgErrBadData;
		return;
	}

	Err	result = exgMgr->ExgLibRequest (libRefNum, exgSocketP);
	m68k_dreg (regs, 0) = result;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostGetPreference
// ---------------------------------------------------------------------------

static void _HostGetPreference (void)
{
	// HostBoolType HostGetPreference (const char*, char*);

	struct StackFrame
	{
		const char*	key;
		char*		value;
	};

	string	key 	= ToString (GET_PARAMETER (key));
	emuptr	value	= GET_PARAMETER (value);

	Preference<string>	pref (key.c_str ());

	if (pref.Loaded ())
	{
		uae_strcpy (value, pref->c_str ());
		m68k_dreg (regs, 0) = 1;
	}
	else
	{
		m68k_dreg (regs, 0) = 0;
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostSet1Preference
// ---------------------------------------------------------------------------

static void _HostSetPreference (void)
{
	// void HostSetPreference (const char*, const char*);

	struct StackFrame
	{
		const char*	key;
		const char*	value;
	};

	string	key 	= ToString (GET_PARAMETER (key));
	string	value	= ToString (GET_PARAMETER (value));

	Preference<string>	pref (key.c_str ());
	pref = value;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostLogFile
// ---------------------------------------------------------------------------

static void _HostLogFile (void)
{
	m68k_areg (regs, 0) = (uint32) hostLogFile;
}


// ---------------------------------------------------------------------------
//		¥ _HostSetLogFileSize
// ---------------------------------------------------------------------------

static void _HostSetLogFileSize (void)
{
	// void HostSetLogFileSize (long)

	struct StackFrame
	{
		long	newSize;
	};

	long	newSize = GET_PARAMETER (newSize);

	LogGetStdLog ()->SetLogSize (newSize);

	m68k_dreg (regs, 0) = 0;
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostSessionCreate
// ---------------------------------------------------------------------------

#if 0
static void _HostSessionCreate (void)
{
	// HostErr HostSessionCreate(const char* device, long ramSize, const char* romPath)

	struct StackFrame
	{
		const char*	device;
		long		ramSize;
		const char*	romPath;
	};

	string		deviceStr	= ToString (GET_PARAMETER (device));
	RAMSizeType	ramSize		= GET_PARAMETER (ramSize);
	string		romPathStr	= ToString (GET_PARAMETER (romPath));

	// See if it's OK to create a session.

	if (Platform::SessionRunning())
	{
		m68k_dreg (regs, 0) = hostErrSessionRunning;
		return;
	}

	// Convert the device string into a DeviceType

	DeviceType						device = kDeviceUnspecified;
	EmDeviceMenuItemList			devices = EmDevice::GetMenuItemList ();
	EmDeviceMenuItemList::iterator	deviceIter = devices.begin();

	while (deviceIter != devices.end())
	{
		if (_stricmp (deviceStr.c_str(), deviceIter->second.c_str()) == 0)
		{
			device = deviceIter->first;
			break;
		}
	}

	if (device == kDeviceUnspecified)
	{
		m68k_dreg (regs, 0) = hostErrInvalidDeviceType;
		return;
	}

	// Validate the RAM size

	Bool			sizeOK = false;
	MemoryTextList	sizes;
	::GetMemoryTextList (sizes);

	MemoryTextList::iterator	sizeIter = sizes.begin();
	while (sizeIter != sizes.end())
	{
		if (ramSize == sizeIter->first)
		{
			sizeOK = true;
			break;
		}
	}

	if (!sizeOK)
	{
		m68k_dreg (regs, 0) = hostErrInvalidRAMSize;
		return;
	}

	// Convert the ROM file string into a EmFileRef.

	EmFileRef	romRef(romPathStr);
	if (!romRef.Exists())
	{
		m68k_dreg (regs, 0) = hostErrFileNotFound;
		return;
	}

	// Kick this all off.

	Configuration	cfg (device, ramSize, romRef);
	Startup::ScheduleCreateSession (cfg);
	m68k_dreg (regs, 0) = errNone;
}
#endif


// ---------------------------------------------------------------------------
//		¥ _HostSessionOpen
// ---------------------------------------------------------------------------

#if 0
static void _HostSessionOpen (void)
{
	// HostErr HostSessionOpen (const char* psfFileName);

	struct StackFrame
	{
		const char*	psfFileName;
	};

	string	psfFileName	= ToString (GET_PARAMETER (psfFileName));

	// See if it's OK to open a session.

	if (Platform::SessionRunning())
	{
		m68k_dreg (regs, 0) = hostErrSessionRunning;
		return;
	}

	// Validate the file to open.

	EmFileRef	psfFileRef(psfFileName);
	if (!psfFileRef.Exists())
	{
		m68k_dreg (regs, 0) = hostErrFileNotFound;
		return;
	}

	// Kick this all off.

	Startup::ScheduleOpenSession (psfFileRef);
	m68k_dreg (regs, 0) = errNone;
}
#endif


// ---------------------------------------------------------------------------
//		¥ _HostSessionSave
// ---------------------------------------------------------------------------

static void _HostSessionSave (void)
{
	// HostBoolType HostSessionSave (const char* saveFileName)

	struct StackFrame
	{
		const char*	saveFileName;
	};

	string	saveFileName	= ToString (GET_PARAMETER (saveFileName));

	// See if it's OK to close a session.

	if (!gSession)
	{
		m68k_dreg (regs, 0) = hostErrSessionNotRunning;
		return;
	}

	EmFileRef	saveFileRef (saveFileName);

	// Saving will save the PC at the current instruction, which saves the state.  
	// Upon resumption, the current instruction will cause a save again.  This 
	// will repeat infinitely.  So advance the PC to the following instruction
	// before saving.

	// First, get the address of the function that got us here.
	// If the system call is being made by a TRAP $F, the PC has already
	// been bumped past the opcode.  If being made with a JSR via the
	// SYS_TRAP_FAST macro, the PC has not been adjusted.

	emuptr	startPC = m68k_getpc ();
	if ((EmMemGet16 (startPC) & 0xF000) == 0xA000)
	{
		startPC -= 2;	// Back us up to the TRAP $F
	}

	SystemCallContext	context;
	if (GetSystemCallContext (startPC, context))
	{
		m68k_setpc (context.fNextPC);

		// Set the return value to something else so that the code that is restored can distinguish
		// the saved case from not saved case.

		m68k_dreg (regs, 0) = true;

		gSession->Save (saveFileRef, false);
	}

	m68k_dreg (regs, 0) = false;
}


// ---------------------------------------------------------------------------
//		¥ _HostSessionClose
// ---------------------------------------------------------------------------

static void _HostSessionClose (void)
{
	// HostErr HostSessionClose (const char* saveFileName)

	struct StackFrame
	{
		const char*	saveFileName;
	};

	string	saveFileName	= ToString (GET_PARAMETER (saveFileName));

	// See if it's OK to close a session.

	if (!gSession)
	{
		m68k_dreg (regs, 0) = hostErrSessionNotRunning;
		return;
	}

	EmFileRef	saveFileRef(saveFileName);

	// Kick this all off.

	Startup::ScheduleCloseSession (saveFileName);
	m68k_dreg (regs, 0) = errNone;
}


// ---------------------------------------------------------------------------
//		¥ _HostSessionQuit
// ---------------------------------------------------------------------------

static void _HostSessionQuit (void)
{
	// HostErr HostSessionQuit (void)

	struct StackFrame
	{
	};

	// See if it's OK to quit Poser.

	if (gSession)
	{
//		m68k_dreg (regs, 0) = hostErrSessionRunning;
//		return;
	}

	// Kick this all off.

	Startup::ScheduleQuit ();

	m68k_dreg (regs, 0) = errNone;
}


// ---------------------------------------------------------------------------
//		¥ _HostSignalSend
// ---------------------------------------------------------------------------
// Called by anyone wanting to send a signal to any waiting scripts.

static void _HostSignalSend (void)
{
	// HostErr HostSignalSend (HostSignalType signalNumber)

	struct StackFrame
	{
		HostSignalType	signalNumber;
	};

	HostSignalType	signalNumber = GET_PARAMETER(signalNumber);

	RPC::SignalWaiters (signalNumber);

	m68k_dreg (regs, 0) = errNone;
}


// ---------------------------------------------------------------------------
//		¥ _HostSignalWait
// ---------------------------------------------------------------------------
// Called by scripts that want to get a signal sent from HostSignalSend.

static void _HostSignalWait (void)
{
	// HostErr HostSignalWait (long timeout)

	struct StackFrame
	{
		long	timeout;
	};

	long	timeout = GET_PARAMETER (timeout);

	// Unblock the CPU thread if it's suspended from a previous
	// HostSignalSend call.

	EmAssert (gSession);
	gSession->ScheduleResumeExternal ();

	if (RPC::HandlingPacket ())
	{
		RPC::DeferCurrentPacket (timeout);

		if (Patches::UIInitialized ())
		{
			::EvtWakeup ();	// Wake up the process in case the caller is looking
							// for an idle event (which would never otherwise
							// happen if EvtGetEvent has already been called and
							// is blocking).
		}
	}

	m68k_dreg (regs, 0) = errNone;
}


// ---------------------------------------------------------------------------
//		¥ _HostSignalResume
// ---------------------------------------------------------------------------
// Called by scripts to restart the emulator after it has sent a signal and
// then suspended itself.

static void _HostSignalResume (void)
{
	// HostErr HostSignalResume (void)

	struct StackFrame
	{
	};

	EmAssert (gSession);
	gSession->ScheduleResumeExternal ();

	m68k_dreg (regs, 0) = errNone;
}


#pragma mark -

#if HAS_TRACER
// ---------------------------------------------------------------------------
//		¥ _HostTraceInit
// ---------------------------------------------------------------------------

static void _HostTraceInit (void)
{	
	gTracer.InitOutputPort ();
}


// ---------------------------------------------------------------------------
//		¥ _HostTraceClose
// ---------------------------------------------------------------------------

static void _HostTraceClose (void)
{
	gTracer.CloseOutputPort ();
}


// ---------------------------------------------------------------------------
//		¥ _HostTraceOutputT
// ---------------------------------------------------------------------------

static void _HostTraceOutputT (void)
{
	// void HostTraceOutputT (unsigned short, const char*, ...)

	struct StackFrame
	{
		unsigned short	module;
		const char*		fmt;
	};

	// Get the caller's parameters.

	unsigned short	module = GET_PARAMETER (module);
	emuptr			fmt = GET_PARAMETER (fmt);

	// Check the parameters.

	if (!fmt)
	{
		errno = hostErrInvalidParameter;
		return;
	}

	// Make a copy of the format string.

	string	fmt2 (ToString (fmt));

	// Collect the specified parameters. We need to make copies of everything
	// so that it's in the right endian order and to reverse any effects
	// of wordswapping.  The values specified on the stack (the integers,
	// chars, doubles, pointers, etc.) get converted and placed in stackData.
	// The data pointed to by the pointers gets converted and placed in stringData.

	ByteList	stackData;
	StringList stringData;

	if (!::CollectParameters (	sizeof (HostControlSelectorType) +
								sizeof (short) +
								sizeof (char*), fmt2, stackData, stringData))
	{
		m68k_dreg (regs, 0) = (uint32) hostErrInvalidParameter;
		return;
	}

	// Write everything out

	gTracer.OutputVT( module, fmt2.c_str (), (va_list) &stackData[0]);
}


// ---------------------------------------------------------------------------
//		¥ _HostTraceOutputTL
// ---------------------------------------------------------------------------

static void _HostTraceOutputTL (void)
{
	// void HostTraceOutputTL (unsigned short, const char*, ...)

	struct StackFrame
	{
		unsigned short	module;
		const char*		fmt;
	};

	// Get the caller's parameters.
	
	unsigned short	module = GET_PARAMETER (module);
	emuptr			fmt = GET_PARAMETER (fmt);

	// Check the parameters.

	if (!fmt)
	{
		errno = hostErrInvalidParameter;
		return;
	}

	// Make a copy of the format string.

	string	fmt2 (ToString (fmt));

	// Collect the specified parameters. We need to make copies of everything
	// so that it's in the right endian order and to reverse any effects
	// of wordswapping.  The values specified on the stack (the integers,
	// chars, doubles, pointers, etc.) get converted and placed in stackData.
	// The data pointed to by the pointers gets converted and placed in stringData.

	ByteList	stackData;
	StringList stringData;

	if (!::CollectParameters (	sizeof (HostControlSelectorType) +
								sizeof (short) +
								sizeof (char*), fmt2, stackData, stringData))
	{
		m68k_dreg (regs, 0) = (uint32) hostErrInvalidParameter;
		return;
	}

	// Write everything out

	gTracer.OutputVTL( module, fmt2.c_str (), (va_list) &stackData[0]);
}


// ---------------------------------------------------------------------------
//		¥ _HostOutputVT
// ---------------------------------------------------------------------------

static void _HostTraceOutputVT (void)
{
	// void HostTraceOutputVT (unsigned short, const char*, char*)

	struct StackFrame
	{
		unsigned short	module;
		const char*		fmt;
		unsigned long	va_addr;
	};

	// Get the caller's parameters.

	unsigned short	module = GET_PARAMETER (module);
	emuptr			fmt = GET_PARAMETER (fmt);
	unsigned long	va_addr =  GET_PARAMETER (va_addr);

	// Check the parameters.

	if (!fmt)
	{
		errno = hostErrInvalidParameter;
		return;
	}

	// Make a copy of the format string.

	string	fmt2 (ToString (fmt));

	// Collect the specified parameters. We need to make copies of everything
	// so that it's in the right endian order and to reverse any effects
	// of wordswapping.  The values specified on the stack (the integers,
	// chars, doubles, pointers, etc.) get converted and placed in stackData.
	// The data pointed to by the pointers gets converted and placed in stringData.

	ByteList	stackData;
	StringList stringData;

	if (!::CollectParameters (va_addr - m68k_areg (regs, 7), fmt2, stackData, stringData))
	{
		m68k_dreg (regs, 0) = (uint32) hostErrInvalidParameter;
		return;
	}

	// Write everything out

	gTracer.OutputVT( module, fmt2.c_str (), (va_list) &stackData[0]);
}


// ---------------------------------------------------------------------------
//		¥ _HostOutputVTL
// ---------------------------------------------------------------------------

static void _HostTraceOutputVTL (void)
{
	// void HostTraceOutputVTL (unsigned short, const char*, char*)

	struct StackFrame
	{
		unsigned short	module;
		const char*		fmt;
		unsigned long	va_addr;
	};

	// Get the caller's parameters.

	unsigned short	module = GET_PARAMETER (module);
	emuptr			fmt = GET_PARAMETER (fmt);
	unsigned long	va_addr =  GET_PARAMETER (va_addr);

	// Check the parameters.

	if (!fmt)
	{
		errno = hostErrInvalidParameter;
		return;
	}

	// Make a copy of the format string.

	string	fmt2 (ToString (fmt));

	// Collect the specified parameters. We need to make copies of everything
	// so that it's in the right endian order and to reverse any effects
	// of wordswapping.  The values specified on the stack (the integers,
	// chars, doubles, pointers, etc.) get converted and placed in stackData.
	// The data pointed to by the pointers gets converted and placed in stringData.

	ByteList	stackData;
	StringList stringData;

	if (!::CollectParameters (va_addr - m68k_areg (regs, 7), fmt2, stackData, stringData))
	{
		m68k_dreg (regs, 0) = (uint32) hostErrInvalidParameter;
		return;
	}

	// Write everything out

	gTracer.OutputVTL( module, fmt2.c_str (), (va_list) &stackData[0]);
}


// ---------------------------------------------------------------------------
//		¥ _HostOutputB
// ---------------------------------------------------------------------------

static void _HostTraceOutputB (void)
{
	// void HostTraceOutputB (unsigned short, const void*, HostSizeType)

	struct StackFrame
	{
		unsigned short	module;
		const void*		buff;
		HostSizeType	length;
	};
	
	// Get the caller's parameters.

	unsigned short	module	= GET_PARAMETER (module);
	emuptr			buff	= GET_PARAMETER (buff);
	HostSizeType	length 	= GET_PARAMETER (length);

	// Check the parameters.

	if (!buff || !length)
	{
		errno = hostErrInvalidParameter;
		return;
	}

	void*	tempBuff = malloc (length);

	if (tempBuff)
	{	
		uae_memcpy (tempBuff, buff, length);
		gTracer.OutputB (module, tempBuff, length);
		free (tempBuff);
	}
	else
	{
		errno = hostErrInvalidParameter;
	}
}

#endif	// HAS_TRACER


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostSlotMax
// ---------------------------------------------------------------------------

static void _HostSlotMax (void)
{
	int	maxSlotNo = 0;

	Preference<SlotInfoList>	slots (kPrefKeySlotList);

	SlotInfoList::const_iterator	iter = (*slots).begin ();
	while (iter != (*slots).end ())
	{
		if (maxSlotNo < iter->fSlotNumber)
		{
			maxSlotNo = iter->fSlotNumber;
		}

		++iter;
	}

	m68k_dreg (regs, 0) = maxSlotNo;
}


// ---------------------------------------------------------------------------
//		¥ _HostSlotRoot
// ---------------------------------------------------------------------------

static void _HostSlotRoot (void)
{
	struct StackFrame
	{
		long	slotNo;
	};

	long	slotNo = GET_PARAMETER (slotNo);

	m68k_areg (regs, 0) = EmMemNULL;

	Preference<SlotInfoList>	slots (kPrefKeySlotList);

	SlotInfoList::const_iterator	iter = (*slots).begin ();
	while (iter != (*slots).end ())
	{
		if (slotNo == iter->fSlotNumber)
		{
			if (iter->fSlotOccupied)
			{
				::PrvReturnString (iter->fSlotRoot.GetFullPath ());
			}

			break;
		}

		++iter;
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostSlotHasCard
// ---------------------------------------------------------------------------

static void _HostSlotHasCard (void)
{
	struct StackFrame
	{
		long	slotNo;
	};

	long	slotNo = GET_PARAMETER (slotNo);

	m68k_dreg (regs, 0) = 0;

	Preference<SlotInfoList>	slots (kPrefKeySlotList);

	SlotInfoList::const_iterator	iter = (*slots).begin ();
	while (iter != (*slots).end ())
	{
		if (slotNo == iter->fSlotNumber)
		{
			m68k_dreg (regs, 0) = iter->fSlotOccupied;
			break;
		}

		++iter;
	}
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ _HostGetFile
// ---------------------------------------------------------------------------

static void _HostGetFile (void)
{
	struct StackFrame
	{
		const char* prompt;
		const char* defaultDir;
	};

	m68k_areg (regs, 0) = EmMemNULL;

	string 			promptStr		= ToString (GET_PARAMETER (prompt));
	string 			defaultDirStr	= ToString (GET_PARAMETER (defaultDir));
	EmDirRef		defaultDir (defaultDirStr);

	EmFileRef		result;
	EmFileTypeList	filterList;

	if (EmDlg::DoGetFile (result, promptStr, defaultDir, filterList) == kDlgItemOK)
	{
		::PrvReturnString (result.GetFullPath ());
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostPutFile
// ---------------------------------------------------------------------------

static void _HostPutFile (void)
{
	struct StackFrame
	{
		const char* prompt;
		const char* defaultDir;
		const char* defaultName;
	};

	m68k_areg (regs, 0) = EmMemNULL;

	string 			promptStr		= ToString (GET_PARAMETER (prompt));
	string 			defaultDirStr	= ToString (GET_PARAMETER (defaultDir));
	string 			defaultNameStr	= ToString (GET_PARAMETER (defaultName));

	EmDirRef		defaultDir (defaultDirStr);

	EmFileRef		result;
	EmFileTypeList	filterList;

	if (EmDlg::DoPutFile (result, promptStr, defaultDir,
							filterList, defaultNameStr) == kDlgItemOK)
	{
		::PrvReturnString (result.GetFullPath ());
	}
}


// ---------------------------------------------------------------------------
//		¥ _HostGetDirectory
// ---------------------------------------------------------------------------

static void _HostGetDirectory (void)
{
	struct StackFrame
	{
		const char* prompt;
		const char* defaultDir;
	};

	m68k_areg (regs, 0) = EmMemNULL;

	string 			promptStr		= ToString (GET_PARAMETER (prompt));
	string 			defaultDirStr	= ToString (GET_PARAMETER (defaultDir));
	EmDirRef		defaultDir (defaultDirStr);

	EmDirRef		result;
	EmFileTypeList	filterList;

	if (EmDlg::DoGetDirectory (result, promptStr, defaultDir) == kDlgItemOK)
	{
		::PrvReturnString (result.GetFullPath ());
	}
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ HostGetHandler
// ---------------------------------------------------------------------------

HostHandler HostGetHandler (HostControlSelectorType selector)
{
	HostHandler fn = NULL;

	// Hack for GremlinIsOn; see comments at head of HostTraps.h.

	if ((selector & 0xFF00) == 0)
	{
		selector = hostSelectorGremlinIsRunning;
	}

	if (selector < hostSelectorLastTrapNumber)
	{
		fn = gHandlerTable [selector];
	}

	return fn;
}


// ---------------------------------------------------------------------------
//		¥ CollectParameters
// ---------------------------------------------------------------------------

Bool CollectParameters (int stackOffset, const string& fmt, ByteList& stackData, StringList& stringData)
{
	// Skip past the first few items on the stack.

	int callerStackOffset = stackOffset;

	// Start parsing up the format string.

	string::const_iterator	iter;

	for (iter = fmt.begin (); iter != fmt.end ();)
	{
		char	ch = *iter++;

		/*
			Format specification:

				% ? 12 .4 h d
				| |  |  | | |
				| |  |  | | +-- conversion letter
				| |  |  | +---- size modifier (l, L, or h)
				| |  |  +------ precision
				| |  +--------- minimum width field
				| +------------ flags (one or more of +, -, #, 0, or space)
				+-------------- start of specification
		*/

		if (ch == '%')
		{
			if (iter == fmt.end ())
				return false;

			ch = *iter++;

			// Skip over any flags.

			while (ch == '+' || ch == '-' || ch == ' ' || ch == '#' || ch == '0')
			{
				if (iter == fmt.end ())
					return false;

				ch = *iter++;
			}

			// Skip over any minimum width field.

			if (ch == '*')
			{
				if (iter == fmt.end ())
					return false;

				ch = *iter++;
			}
			else
			{
				while (ch >= '0' && ch <= '9')
				{
					if (iter == fmt.end ())
						return false;

					ch = *iter++;
				}
			}

			// Skip over any precision.

			if (ch == '.')
			{
				if (iter == fmt.end ())
					return false;

				ch = *iter++;

				while (ch >= '0' && ch <= '9')
				{
					if (iter == fmt.end ())
						return false;

					ch = *iter++;
				}
			}

			// Get any size modifier.

			enum { kSizeNone, kSizeLongInt, kSizeLongDouble, kSizeShortInt };

			int mod = kSizeNone;

			if (ch == 'l')
				mod = kSizeLongInt;
			else if (ch == 'L')
				mod = kSizeLongDouble;
			else if (ch == 'h')
				mod = kSizeShortInt;

			// If there was a modifier, it's been handled,
			// so skip over it.

			if (mod != kSizeNone)
			{
				if (iter == fmt.end ())
					return false;

				ch = *iter++;
			}

			switch (ch)
			{
				case 'd':
				case 'i':
				case 'u':
				case 'o':
				case 'x':
				case 'X':
					// int, short, or long
					if (mod == kSizeNone || mod == kSizeShortInt)
						PushShort (callerStackOffset, stackData);
					else
						PushLong (callerStackOffset, stackData);
					break;

				case 'f':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
					// double or long double
					if (mod == kSizeNone)
						PushDouble (callerStackOffset, stackData);
					else
						PushLongDouble (callerStackOffset, stackData);
					break;

				case 'c':
					// int or wint_t
					if (mod == kSizeNone)
						PushShort (callerStackOffset, stackData);
#if defined (_MSC_VER)
					else if (sizeof (wint_t) == 2)
						PushShort (callerStackOffset, stackData);
#endif
					else
						PushLong (callerStackOffset, stackData);
					break;

				case 's':
					PushString (callerStackOffset, stackData, stringData);
					break;

				case 'p':
					PushLong (callerStackOffset, stackData);
					break;

				case 'n':
					// pointer
					// !!! Not supported for now...
					return false;

				case '%':
					// none
					break;

				default:
					// Bad conversion...
					return false;
			}
		}
	}

	return true;
}


// ---------------------------------------------------------------------------
//		¥ PushShort
// ---------------------------------------------------------------------------

void PushShort (int& callerStackOffset, ByteList& stackData)
{
	// Read a 2-byte int from the caller's stack, and push it
	// onto our stack as a 4-byte int.

	uint16 value = EmMemGet16 (m68k_areg (regs, 7) + callerStackOffset);
	callerStackOffset += sizeof (value);

	ByteList::size_type	oldSize = stackData.size ();
	stackData.insert (stackData.end (), sizeof (int), 0);	// Make space for 4 more bytes

	*(int*) &stackData[oldSize] = value;
}


// ---------------------------------------------------------------------------
//		¥ PushLong
// ---------------------------------------------------------------------------

void PushLong (int& callerStackOffset, ByteList& stackData)
{
	// Read a 4-byte long int from the caller's stack, and push it
	// onto our stack as a 4-byte long int.

	uint32 value = EmMemGet32 (m68k_areg (regs, 7) + callerStackOffset);
	callerStackOffset += sizeof (value);

	ByteList::size_type	oldSize = stackData.size ();
	stackData.insert (stackData.end (), sizeof (long), 0);	// Make space for 4 more bytes

	*(long*) &stackData[oldSize] = value;
}


// ---------------------------------------------------------------------------
//		¥ PushDouble
// ---------------------------------------------------------------------------

void PushDouble (int& callerStackOffset, ByteList& stackData)
{
	UNUSED_PARAM(callerStackOffset)
	UNUSED_PARAM(stackData)
}


// ---------------------------------------------------------------------------
//		¥ PushLongDouble
// ---------------------------------------------------------------------------

void PushLongDouble (int& callerStackOffset, ByteList& stackData)
{
	UNUSED_PARAM(callerStackOffset)
	UNUSED_PARAM(stackData)
}


// ---------------------------------------------------------------------------
//		¥ PushString
// ---------------------------------------------------------------------------

void PushString (int& callerStackOffset, ByteList& stackData, StringList& stringData)
{
	// Get the string pointer and clone the string into a new string object.

	emuptr	stringPtr = EmMemGet32 (m68k_areg (regs, 7) + callerStackOffset);
	string	strCopy;
	size_t	strLen = uae_strlen (stringPtr);

	if (strLen > 0)
	{
		strCopy.resize (strLen);
		uae_strcpy (&strCopy[0], stringPtr);
	}

	// Add this string to the string array.

	stringData.push_back (strCopy);

	// In the stack data byte array, add a pointer to the string. 

	ByteList::size_type	oldSize = stackData.size ();
	stackData.insert (stackData.end (), sizeof (char*), 0); // Make space for 4 more bytes
	*(uint32*) &stackData[oldSize] = (uint32) (*(stringData.end () - 1)).c_str ();

	// Bump the caller's stack pointer by the size of a string pointer.

	callerStackOffset += sizeof (char*);
}


// ---------------------------------------------------------------------------
//		¥ ToString
// ---------------------------------------------------------------------------

string ToString (emuptr s)
{
	string	result;

	size_t	sLen = uae_strlen (s);
	if (sLen > 0)
	{
		result.resize (sLen);
		uae_strcpy (&result[0], s);
	}

	return result;
}


// ---------------------------------------------------------------------------
//		¥ ToFILE
// ---------------------------------------------------------------------------

FILE* ToFILE (emuptr f)
{
	if ((HostFILEType*) f == hostLogFile)
	{
		return hostLogFILE;
	}

	return (FILE*) f;
}


// ---------------------------------------------------------------------------
//		¥ GetHostTmType
// ---------------------------------------------------------------------------

void GetHostTmType (HostTmType& dest, emuptr src)
{
	dest.tm_sec_	= EmMemGet32 (src + offsetof (HostTmType, tm_sec_));
	dest.tm_min_	= EmMemGet32 (src + offsetof (HostTmType, tm_min_));
	dest.tm_hour_	= EmMemGet32 (src + offsetof (HostTmType, tm_hour_));
	dest.tm_mday_	= EmMemGet32 (src + offsetof (HostTmType, tm_mday_));
	dest.tm_mon_	= EmMemGet32 (src + offsetof (HostTmType, tm_mon_));
	dest.tm_year_	= EmMemGet32 (src + offsetof (HostTmType, tm_year_));
	dest.tm_wday_	= EmMemGet32 (src + offsetof (HostTmType, tm_wday_));
	dest.tm_yday_	= EmMemGet32 (src + offsetof (HostTmType, tm_yday_));
	dest.tm_isdst_	= EmMemGet32 (src + offsetof (HostTmType, tm_isdst_));
}


// ---------------------------------------------------------------------------
//		¥ PutHostTmType
// ---------------------------------------------------------------------------

void PutHostTmType (emuptr dest, const HostTmType& src)
{
	EmMemPut32 (dest + offsetof (HostTmType, tm_sec_),	src.tm_sec_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_min_),	src.tm_min_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_hour_),	src.tm_hour_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_mday_),	src.tm_mday_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_mon_),	src.tm_mon_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_year_),	src.tm_year_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_wday_),	src.tm_wday_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_yday_),	src.tm_yday_);
	EmMemPut32 (dest + offsetof (HostTmType, tm_isdst_),	src.tm_isdst_);
}


// ---------------------------------------------------------------------------
//		¥ TmFromHostTm
// ---------------------------------------------------------------------------

void TmFromHostTm (struct tm& dest, const HostTmType& src)
{
	dest.tm_sec		= src.tm_sec_;
	dest.tm_min		= src.tm_min_;
	dest.tm_hour	= src.tm_hour_;
	dest.tm_mday	= src.tm_mday_;
	dest.tm_mon		= src.tm_mon_;
	dest.tm_year	= src.tm_year_;
	dest.tm_wday	= src.tm_wday_;
	dest.tm_yday	= src.tm_yday_;
	dest.tm_isdst	= src.tm_isdst_;
}


// ---------------------------------------------------------------------------
//		¥ HostTmFromTm
// ---------------------------------------------------------------------------

void HostTmFromTm (HostTmType& dest, const struct tm& src)
{
	dest.tm_sec_	= src.tm_sec;
	dest.tm_min_	= src.tm_min;
	dest.tm_hour_	= src.tm_hour;
	dest.tm_mday_	= src.tm_mday;
	dest.tm_mon_	= src.tm_mon;
	dest.tm_year_	= src.tm_year;
	dest.tm_wday_	= src.tm_wday;
	dest.tm_yday_	= src.tm_yday;
	dest.tm_isdst_	= src.tm_isdst;
}


// ---------------------------------------------------------------------------
//		¥ MapAndReturn
// ---------------------------------------------------------------------------

void MapAndReturn (const void* p, long size)
{
	emuptr	result = EmBankMapped::GetEmulatedAddress (p);

	if (result == EmMemNULL)
	{
		EmBankMapped::MapPhysicalMemory (p, size);
		result = EmBankMapped::GetEmulatedAddress (p);
	}

	m68k_areg (regs, 0) = result;
}


// ---------------------------------------------------------------------------
//		¥ MapAndReturn
// ---------------------------------------------------------------------------

void MapAndReturn (string& s)
{
	MapAndReturn (s.c_str (), s.size () + 1);
}


// ---------------------------------------------------------------------------
//		¥ PrvReturnString
// ---------------------------------------------------------------------------

void PrvReturnString (const char* p)
{
	EmBankMapped::UnmapPhysicalMemory (gResultString.c_str ());

	if (p)
	{
		gResultString = p;
		::MapAndReturn (gResultString);
	}
	else
	{
		gResultString.erase ();
		m68k_areg (regs, 0) = EmMemNULL;
	}
}


// ---------------------------------------------------------------------------
//		¥ PrvReturnString
// ---------------------------------------------------------------------------

void PrvReturnString (const string& s)
{
	EmBankMapped::UnmapPhysicalMemory (gResultString.c_str ());

	gResultString = s;

	::MapAndReturn (gResultString);
}


#pragma mark -

// ---------------------------------------------------------------------------
//		¥ Host::Initialize
// ---------------------------------------------------------------------------

void Host::Initialize	(void)
{
	memset (gHandlerTable, 0, sizeof (gHandlerTable));

	gHandlerTable [hostSelectorGetHostVersion]			= _HostGetHostVersion;
	gHandlerTable [hostSelectorGetHostID]				= _HostGetHostID;
	gHandlerTable [hostSelectorGetHostPlatform]			= _HostGetHostPlatform;
	gHandlerTable [hostSelectorIsSelectorImplemented]	= _HostIsSelectorImplemented;
	gHandlerTable [hostSelectorGestalt] 				= _HostGestalt;
	gHandlerTable [hostSelectorIsCallingTrap]			= _HostIsCallingTrap;

#if HAS_PROFILING
	gHandlerTable [hostSelectorProfileInit]				= _HostProfileInit;
	gHandlerTable [hostSelectorProfileStart]			= _HostProfileStart;
	gHandlerTable [hostSelectorProfileStop]				= _HostProfileStop;
	gHandlerTable [hostSelectorProfileDump]				= _HostProfileDump;
	gHandlerTable [hostSelectorProfileCleanup]			= _HostProfileCleanup;
	gHandlerTable [hostSelectorProfileDetailFn]			= _HostProfileDetailFn;
	gHandlerTable [hostSelectorProfileGetCycles]		= _HostProfileGetCycles;
#endif

	gHandlerTable [hostSelectorErrNo]					= _HostErrNo;

	gHandlerTable [hostSelectorFClose]					= _HostFClose;
	gHandlerTable [hostSelectorFEOF]					= _HostFEOF;
	gHandlerTable [hostSelectorFError]					= _HostFError;
	gHandlerTable [hostSelectorFFlush]					= _HostFFlush;
	gHandlerTable [hostSelectorFGetC]					= _HostFGetC;
	gHandlerTable [hostSelectorFGetPos]					= _HostFGetPos;
	gHandlerTable [hostSelectorFGetS]					= _HostFGetS;
	gHandlerTable [hostSelectorFOpen]					= _HostFOpen;
	gHandlerTable [hostSelectorFPrintF]					= _HostFPrintF;
	gHandlerTable [hostSelectorFPutC]					= _HostFPutC;
	gHandlerTable [hostSelectorFPutS]					= _HostFPutS;
	gHandlerTable [hostSelectorFRead]					= _HostFRead;
	gHandlerTable [hostSelectorRemove]					= _HostRemove;
	gHandlerTable [hostSelectorRename]					= _HostRename;
//	gHandlerTable [hostSelectorFReopen]					= _HostFReopen;
//	gHandlerTable [hostSelectorFScanF]					= _HostFScanF;
	gHandlerTable [hostSelectorFSeek]					= _HostFSeek;
	gHandlerTable [hostSelectorFSetPos]					= _HostFSetPos;
	gHandlerTable [hostSelectorFTell]					= _HostFTell;
	gHandlerTable [hostSelectorFWrite]					= _HostFWrite;
	gHandlerTable [hostSelectorTmpFile]					= _HostTmpFile;
	gHandlerTable [hostSelectorTmpNam]					= _HostTmpNam;
	gHandlerTable [hostSelectorGetEnv]					= _HostGetEnv;

	gHandlerTable [hostSelectorMalloc]					= _HostMalloc;
	gHandlerTable [hostSelectorRealloc]					= _HostRealloc;
	gHandlerTable [hostSelectorFree]					= _HostFree;

	gHandlerTable [hostSelectorAscTime]					= _HostAscTime;
	gHandlerTable [hostSelectorClock]					= _HostClock;
	gHandlerTable [hostSelectorCTime]					= _HostCTime;
//	gHandlerTable [hostSelectorDiffTime]				= _HostDiffTime;
	gHandlerTable [hostSelectorGMTime]					= _HostGMTime;
	gHandlerTable [hostSelectorLocalTime]				= _HostLocalTime;
	gHandlerTable [hostSelectorMkTime]					= _HostMkTime;
	gHandlerTable [hostSelectorStrFTime]				= _HostStrFTime;
	gHandlerTable [hostSelectorTime]					= _HostTime;

	gHandlerTable [hostSelectorMkDir]					= _HostMkDir;
	gHandlerTable [hostSelectorRmDir]					= _HostRmDir;
	gHandlerTable [hostSelectorOpenDir]					= _HostOpenDir;
	gHandlerTable [hostSelectorReadDir]					= _HostReadDir;
	gHandlerTable [hostSelectorCloseDir]				= _HostCloseDir;

//	gHandlerTable [hostSelectorFStat]					= _HostFStat;
	gHandlerTable [hostSelectorStat]					= _HostStat;
	
	gHandlerTable [hostSelectorGetFileAttr]				= _HostGetFileAttr;
	gHandlerTable [hostSelectorSetFileAttr]				= _HostSetFileAttr;

//	gHandlerTable [hostSelectorFTruncate]				= _HostFTruncate;
	gHandlerTable [hostSelectorTruncate]				= _HostTruncate;

	gHandlerTable [hostSelectorUTime]					= _HostUTime;

	gHandlerTable [hostSelectorGremlinIsRunning]		= _HostGremlinIsRunning;
	gHandlerTable [hostSelectorGremlinNumber]			= _HostGremlinNumber;
	gHandlerTable [hostSelectorGremlinCounter]			= _HostGremlinCounter;
	gHandlerTable [hostSelectorGremlinLimit]			= _HostGremlinLimit;
	gHandlerTable [hostSelectorGremlinNew]				= _HostGremlinNew;

	gHandlerTable [hostSelectorImportFile]				= _HostImportFile;
	gHandlerTable [hostSelectorExportFile]				= _HostExportFile;
	gHandlerTable [hostSelectorSaveScreen]				= _HostSaveScreen;

	gHandlerTable [hostSelectorExgLibOpen]				= _HostExgLibOpen;
	gHandlerTable [hostSelectorExgLibClose]				= _HostExgLibClose;
	gHandlerTable [hostSelectorExgLibSleep]				= _HostExgLibSleep;
	gHandlerTable [hostSelectorExgLibWake]				= _HostExgLibWake;
	gHandlerTable [hostSelectorExgLibHandleEvent]		= _HostExgLibHandleEvent;
	gHandlerTable [hostSelectorExgLibConnect]			= _HostExgLibConnect;
	gHandlerTable [hostSelectorExgLibAccept]			= _HostExgLibAccept;
	gHandlerTable [hostSelectorExgLibDisconnect]		= _HostExgLibDisconnect;
	gHandlerTable [hostSelectorExgLibPut]				= _HostExgLibPut;
	gHandlerTable [hostSelectorExgLibGet]				= _HostExgLibGet;
	gHandlerTable [hostSelectorExgLibSend]				= _HostExgLibSend;
	gHandlerTable [hostSelectorExgLibReceive]			= _HostExgLibReceive;
	gHandlerTable [hostSelectorExgLibControl]			= _HostExgLibControl;
	gHandlerTable [hostSelectorExgLibRequest]			= _HostExgLibRequest;

	gHandlerTable [hostSelectorGetPreference]			= _HostGetPreference;
	gHandlerTable [hostSelectorSetPreference]			= _HostSetPreference;

	gHandlerTable [hostSelectorLogFile]					= _HostLogFile;
	gHandlerTable [hostSelectorSetLogFileSize]			= _HostSetLogFileSize;

//	gHandlerTable [hostSelectorSessionCreate]			= _HostSessionCreate;
//	gHandlerTable [hostSelectorSessionOpen]				= _HostSessionOpen;
	gHandlerTable [hostSelectorSessionSave]				= _HostSessionSave;
	gHandlerTable [hostSelectorSessionClose]			= _HostSessionClose;
	gHandlerTable [hostSelectorSessionQuit]				= _HostSessionQuit;
	gHandlerTable [hostSelectorSignalSend]				= _HostSignalSend;
	gHandlerTable [hostSelectorSignalWait]				= _HostSignalWait;
	gHandlerTable [hostSelectorSignalResume]			= _HostSignalResume;

#if HAS_TRACER
	gHandlerTable [hostSelectorTraceInit]				= _HostTraceInit;
	gHandlerTable [hostSelectorTraceClose]				= _HostTraceClose;
	gHandlerTable [hostSelectorTraceOutputT]			= _HostTraceOutputT;
	gHandlerTable [hostSelectorTraceOutputTL]			= _HostTraceOutputTL;
	gHandlerTable [hostSelectorTraceOutputVT]			= _HostTraceOutputVT;
	gHandlerTable [hostSelectorTraceOutputVTL]			= _HostTraceOutputVTL;
	gHandlerTable [hostSelectorTraceOutputB]			= _HostTraceOutputB;
#endif

	gHandlerTable [hostSelectorSlotMax]					= _HostSlotMax;
	gHandlerTable [hostSelectorSlotRoot]				= _HostSlotRoot;
	gHandlerTable [hostSelectorSlotHasCard]				= _HostSlotHasCard;

	gHandlerTable [hostSelectorGetFile]					= _HostGetFile;
	gHandlerTable [hostSelectorPutFile]					= _HostPutFile;
	gHandlerTable [hostSelectorGetDirectory]			= _HostGetDirectory;
}


// ---------------------------------------------------------------------------
//		¥ Host::Reset
// ---------------------------------------------------------------------------

void Host::Reset		(void)
{
	::PrvReleaseAllResources ();
}


// ---------------------------------------------------------------------------
//		¥ Host::Save
// ---------------------------------------------------------------------------

void Host::Save		(SessionFile&)
{
}


// ---------------------------------------------------------------------------
//		¥ Host::Load
// ---------------------------------------------------------------------------

void Host::Load		(SessionFile&)
{
}


// ---------------------------------------------------------------------------
//		¥ Host::Dispose
// ---------------------------------------------------------------------------

void Host::Dispose	(void)
{
	::PrvReleaseAllResources ();
}


// ---------------------------------------------------------------------------
//		¥ PrvReleaseAllResources
// ---------------------------------------------------------------------------

void PrvReleaseAllResources (void)
{
	// Close all open files.

	{
		vector<FILE*>::iterator	iter = gOpenFiles.begin ();
		while (iter != gOpenFiles.end ())
		{
			fclose (*iter);
			++iter;
		}

		gOpenFiles.clear ();
	}

	// Close all open directories.

	{
		vector<MyDIR*>::iterator	iter = gOpenDirs.begin ();
		while (iter != gOpenDirs.end ())
		{
			delete *iter;
			++iter;
		}

		gOpenDirs.clear ();

		// Unmap any mapped gHostDirEnt.

		EmBankMapped::UnmapPhysicalMemory (&gHostDirEnt);
	}

	// Release all allocated memory.

	{
		vector<void*>::iterator	iter = gAllocatedBlocks.begin ();
		while (iter != gAllocatedBlocks.end ())
		{
			EmBankMapped::UnmapPhysicalMemory (*iter);
			Platform::DisposeMemory (*iter);
			++iter;
		}

		gAllocatedBlocks.clear ();
	}

	// Unmap misc memory.

	EmBankMapped::UnmapPhysicalMemory (gResultString.c_str ());
	EmBankMapped::UnmapPhysicalMemory (&gGMTime);
	EmBankMapped::UnmapPhysicalMemory (&gLocalTime);
}


// ---------------------------------------------------------------------------
//		¥ PrvMalloc
// ---------------------------------------------------------------------------

emuptr PrvMalloc (long size)
{
	// Prepare the return value.

	emuptr	result = EmMemNULL;

	// Call the function.

	void*	newPtr = NULL;
	
	try
	{
		newPtr = Platform::AllocateMemory (size);
	}
	catch (...)
	{
	}

	// If the call worked, remember the pointer, map
	// it into emulated space, and return the mapped
	// pointer to the caller.

	if (newPtr)
	{
		gAllocatedBlocks.push_back (newPtr);
		EmBankMapped::MapPhysicalMemory (newPtr, size);

		result = EmBankMapped::GetEmulatedAddress (newPtr);
	}

	return result;
}


// ---------------------------------------------------------------------------
//		¥ PrvRealloc
// ---------------------------------------------------------------------------

emuptr PrvRealloc (emuptr p, long size)
{
	// Prepare the return value.

	emuptr	result = EmMemNULL;

	// Recover the real pointer

	void*	oldPtr = EmBankMapped::GetRealAddress (p);
	if (oldPtr)
	{
		// Find the saved pointer in our list so that we can
		// remove it.

		vector<void*>::iterator	iter = gAllocatedBlocks.begin ();
		while (iter != gAllocatedBlocks.end ())
		{
			if (*iter == oldPtr)
			{
				// Found the saved pointer.  Now try the realloc.

				void*	newPtr = NULL;
				
				try
				{
					newPtr = Platform::ReallocMemory (oldPtr, size);
				}
				catch (...)
				{
				}

				// If that worked, then do the bookkeeping.

				if (newPtr)
				{
					gAllocatedBlocks.erase (iter);
					gAllocatedBlocks.push_back (newPtr);

					EmBankMapped::UnmapPhysicalMemory (oldPtr);
					EmBankMapped::MapPhysicalMemory (newPtr, size);

					result = EmBankMapped::GetEmulatedAddress (newPtr);
				}
				else
				{
					// Could not realloc!  Just return NULL
					// (already stored in "result").
				}

				return result;
			}

			++iter;
		}

		EmAssert (false);
		// !!! Handle not finding saved pointer
	}
	else
	{
		EmAssert (false);
		// !!! Handle not finding real address.
	}

	return result;
}


// ---------------------------------------------------------------------------
//		¥ PrvFree
// ---------------------------------------------------------------------------

void PrvFree (emuptr p)
{
	// Check the parameters.

	if (p)
	{
		// Recover the real pointer

		void*	oldPtr = EmBankMapped::GetRealAddress (p);
		if (oldPtr)
		{
			// Find the saved pointer in our list so that we can
			// remove it.

			vector<void*>::iterator	iter = gAllocatedBlocks.begin ();
			while (iter != gAllocatedBlocks.end ())
			{
				if (*iter == oldPtr)
				{
					Platform::DisposeMemory (oldPtr);

					EmBankMapped::UnmapPhysicalMemory (*iter);
					gAllocatedBlocks.erase (iter);

					return;
				}

				++iter;
			}

			EmAssert (false);
			// !!! Handle not finding saved pointer
		}
		else
		{
			EmAssert (false);
			// !!! Handle not finding real address.
		}
	}
}


#include "PalmPackPop.h"
