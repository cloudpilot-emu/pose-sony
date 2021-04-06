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

#ifndef _MARSHAL_H_
#define _MARSHAL_H_

#include "EmMemory.h"			// EmMemGet32, EmMemGet16, EmMemGet8
#include "Platform.h"			// Platform::AllocateMemory
#include "UAE.h"				// m68k_areg
#include "UAE_Utils.h"			// uae_memcpy

#ifdef SONY_ROM
#include "SonyShared\ExpansionMgr.h"
#include "SonyShared\VFSMgr.h"
#include "SonyShared\SlotDrvLib.h"	// for Sony & SlotDriver Lib, CardMetricsType
#endif


/* ===========================================================================

	Functions and macros for helping with marshaling and unmarshaling
	parameters from and to the emulated process's stack.  First, the stack
	is described with a struct called StackFrame which lists the function's
	parameters in order. For example, the StackFrame definition for
	NetLibDmReceive is as follows:

		struct StackFrame
		{
			Word			libRefNum;
			NetSocketRef	socketRef;
			VoidPtr			recordP;
			DWord			recordOffset;
			Word			rcvLen;
			Word			flags;
			VoidPtr			fromAddrP;
			WordPtr			fromLenP;
			SDWord			timeout;
			Err*			errP;
		};

	This struct describes the format of the parameters on the stack. You
	then use one of several macros to fetch and possibly return values from
	and to those parameters.

	Use PARAM_VAL to manage pass-by-value parameter, such as a Long or a
	Word. Example:

		PARAM_VAL(Word, libRefNum);

	This defines a local variable called libRefNum that contains the value
	passed on the stack.  The variable is an instance of a class that can
	produce a Word, and so can be passed to functions that take a Word.

	Use PARAM_REF to manage a pass-by-reference parameter, such as a Long*
	or a Word*.  Example:

		PARAM_REF(Word, fromLenP, kInput);

	This defines a local variable called fromLenP that contains the pointer
	passed on the stack as well as a copy of the value that it points to. 
	The variable is an instance of a class that can produce a Word*, and so
	can be passed to functions that take a Word*.  If the parameter
	reference was NULL, then the class produces NULL as well.  The third
	parameter to the macro describes whether or not the value passed in is
	an input parameter (kInput), output parameter (kOutput) or both
	(kInOut). If kInput or kInOut, the referenced value is fetched and
	converted to host format.  Otherwise, it is assumed that the parameter
	references uninitialized contents.  If kOutput or kInOut, the value is
	copied back into the memory specified by the parameter reference. 
	Values are explicitly copied back to emulated memory by calling
	fromLenP's Put() method.

	Use PARAM_PTR to manage an arbitrary block of data pointed to by a
	function parameter, such as a void* or BytePtr.  Example:

		PARAM_PTR(void, recordP, len, kInOut);

	This defines a local variable called recordP that points to a block of
	data "len" bytes long.  If the fourth parameter is kInput or kInOut, the
	memory pointed to by the function parameter is copied into this block of
	data.  If the fourth parameter is kOutput or kInOut, the contents of the
	buffer are copied back to the emulated memory when the Put() method is
	called.  NULL pointers are handled.

	Use PARAM_STR as a special kind of PARAM_PTR where the length is not
	explicitly known, but the referenced data is NULL terminated.  Example:

		PARAM_STR(Char, nameP);

	This defines a local variable called nameP that contains a pointer to a
	string, the contents of which are copied from emulated memory.  These
	variables are actually like PARAM_STRs with kInput as the third
	parameter, but there's no real need for that restriction.

	The above macros expand to variable definitions.  The name of the
	variable is given by the second macro parameter, and the type of the
	variable is determined by the first parameter.  The type of variable is
	actually created from a class template.  There are four class templates:
	ParamVal, ParamRef, ParamPtr, and ParamStr.

	The constructors for these classes fetch the the appropriate values from
	memory, allocating any necessary buffers.  Their destructors do nothing
	(which really saves in terms of exception handling overhead).

	When the fetched values have been used and it's time to return any
	changed values back to the emulated stack/heap, all the classes (except
	ParamVal) have Put methods.  You must call this method when you are done
	with the values.  The Put method writes back any altered values and
	releases any memory allocated in the constructor.

=========================================================================== */

#define PARAM_VAL(type, name)	\
	ParamVal<type, offsetof(StackFrame, name)>	name

#define PARAM_REF(type, name, io)	\
	ParamRef<type, offsetof(StackFrame, name), io>	name

#define PARAM_PTR(type, name, len, io)	\
	ParamPtr<type, offsetof(StackFrame, name), io>	name(len)

#define PARAM_STR(type, name)	\
	ParamStr<type, offsetof(StackFrame, name)>	name

class Marshal
{
	public:
			// VC++ is a bit medieval here...
//		static const int	kInput	= 0x01;
//		static const int	kOutput	= 0x02;
//		static const int	kInOut	= kInput | kOutput;
		enum {
			kInput	= 0x01,
			kOutput	= 0x02,
			kInOut	= kInput | kOutput
		};

		#define INPUT(io) (((io) & Marshal::kInput) != 0)
		#define OUTPUT(io) (((io) & Marshal::kOutput) != 0)

		static void*				GetBuffer (emuptr p, long len);
#if (__GNUC__ == 2)
		static void					PutBuffer (emuptr p, unsigned char*& buf, long len)
									{
										if (p)
										{
											uae_memcpy (p, (void*) buf, len);
											Platform::DisposeMemory (buf);
											buf = NULL;
										}
									}
#else
		template <class T>
		static void					PutBuffer (emuptr p, T*& buf, long len)
									{
										if (p)
										{
											uae_memcpy (p, (void*) buf, len);
											Platform::DisposeMemory (buf);
											buf = NULL;
										}
									}
#endif
		static EventType			GetEventType (emuptr p);
		static void					PutEventType (emuptr p, EventType&);

		static NetSocketAddrType	GetNetSocketAddrType (emuptr p);
		static void					PutNetSocketAddrType (emuptr p, NetSocketAddrType&);

		static NetIOParamType		GetNetIOParamType (emuptr p);
		static void					PutNetIOParamType (emuptr p, NetIOParamType&);

		static NetHostInfoBufType	GetNetHostInfoBufType (emuptr p);
		static void					PutNetHostInfoBufType (emuptr p, NetHostInfoBufType&);

		static NetServInfoBufType	GetNetServInfoBufType (emuptr p);
		static void					PutNetServInfoBufType (emuptr p, NetServInfoBufType&);


	/* ===========================================================================
		The ParamFoo classes are templatized on the type of values they are to
		fetch.  In order to perform the actual fetching, they call GetParamVal
		(and put the value back with PutParamVal).  In order for those calls to
		succeed, we have to have overloaded versions of that function for each
		type we ever fetch.  Those overloaded functions appear below.
	=========================================================================== */

		inline static void GetParamVal (long loc, int8& v)
			{ v = (int8) EmMemGet8 (loc); }

		inline static void GetParamVal (long loc, uint8& v)
			{ v = EmMemGet8 (loc); }

		inline static void GetParamVal (long loc, int16& v)
			{ v = (int16) EmMemGet16 (loc); }

		inline static void GetParamVal (long loc, uint16& v)
			{ v = EmMemGet16 (loc); }

		inline static void GetParamVal (long loc, int32& v)
			{ v = (int32) EmMemGet32 (loc); }

		inline static void GetParamVal (long loc, uint32& v)
			{ v = EmMemGet32 (loc); }

		inline static void GetParamVal (long loc, EventType& v)
			{ v = GetEventType (loc); }

		inline static void GetParamVal (long loc, NetSocketAddrType& v)
			{ v = GetNetSocketAddrType (loc); }

		inline static void GetParamVal (long loc, NetIOParamType& v)
			{ v = GetNetIOParamType (loc); }

		inline static void GetParamVal (long loc, NetHostInfoBufType& v)
			{ v = GetNetHostInfoBufType (loc); }

		inline static void GetParamVal (long loc, NetServInfoBufType& v)
			{ v = GetNetServInfoBufType (loc); }


		inline static void PutParamVal (long loc, const int8& v)
			{ EmMemPut8 (loc, v); }

		inline static void PutParamVal (long loc, const uint8& v)
			{ EmMemPut8 (loc, v); }

		inline static void PutParamVal (long loc, const int16& v)
			{ EmMemPut16 (loc, v); }

		inline static void PutParamVal (long loc, const uint16& v)
			{ EmMemPut16 (loc, v); }

		inline static void PutParamVal (long loc, const int32& v)
			{ EmMemPut32 (loc, v); }

		inline static void PutParamVal (long loc, const uint32& v)
			{ EmMemPut32 (loc, v); }

		inline static void PutParamVal (long loc, EventType& v)
			{ PutEventType (loc, v); }

		inline static void PutParamVal (long loc, NetSocketAddrType& v)
			{ PutNetSocketAddrType (loc, v); }

		inline static void PutParamVal (long loc, NetIOParamType& v)
			{ PutNetIOParamType (loc, v); }

		inline static void PutParamVal (long loc, NetHostInfoBufType& v)
			{ PutNetHostInfoBufType (loc, v); }

		inline static void PutParamVal (long loc, NetServInfoBufType& v)
			{ PutNetServInfoBufType (loc, v); }

#ifdef SONY_ROM
#include	"SonyShared\MarshalSony.h"
#endif
};

/* ===========================================================================
	Class that manages an immediate value from the emulated stack. This
	class can fetch the value from the stack and produce that value via a
	type operator.  The value cannot be changed, you can't take the address
	of it, nor can the value be written back to the stack.
=========================================================================== */

template <typename T, long offset>
class ParamVal
{
	public:
				ParamVal (void)
				{
					Marshal::GetParamVal (m68k_areg (regs, 7) + offset, fVal);
				}

				operator T(void) { return fVal; }

	private:
		T		fVal;
};


/* ===========================================================================
	Class that manages a value passed by reference on the emulated stack.
	This class can fetch the pointer to the value and make a copy of the
	value pointed to if the pointer was NULL.  The class can produce a
	pointer to the local copy of the value, or NULL if the parameter was
	NULL.  The class has a Put method which copies the local copy back to
	the emulated memory the original came from.  Whether or not the
	referenced value is fetched or returned is determined by the inOut
	template parameter.  If the kInput bit is set, the value is copied from
	emulated memory to a local copy.  If the kOutput bit is set, the local
	copy is copied back to emulated memory when the Put method is called.
=========================================================================== */

template <typename T, long offset, long inOut>
class ParamRef
{
	public:
				ParamRef (void)
				{
					Marshal::GetParamVal (m68k_areg (regs, 7) + offset, fPtr);
					if (fPtr && INPUT(inOut))
					{
						Marshal::GetParamVal (fPtr, fVal);
					}
				}

		void	Put (void)
				{
					if (fPtr && OUTPUT(inOut))
					{
						Marshal::PutParamVal (fPtr, fVal);
					}
				}

				operator T*(void) { return fPtr ? &fVal : NULL; }
		T		operator *(void) const { return fVal; }

		operator emuptr (void) { return fPtr; }

	private:
		emuptr	fPtr;
		T		fVal;
};


/* ===========================================================================
	Class that manages a block of memory pointed to by a pointer on the
	emulated stack.  If the pointer is non-NULL, a local block of memory is
	allocated.  If the kInput bit of the inOut template parameter is set,
	the range of emulated memory is copied into the local buffer. This class
	can produce a pointer to the local block of memory on demand (if the
	stack parameter was NULL, this class produces NULL). If the kOutput bit
	of the inOut template paraemter is set, the local block's contents are
	copied back into emulated memory when the Put method is called. 
	Regardless of inOut values, the Put method must be called in order to
	release the lock block of memory.
=========================================================================== */

template <typename T, long offset, long inOut>
class ParamPtr
{
	public:
				ParamPtr (long len) :
					fLen (len),
					fVal (NULL)
				{
					Marshal::GetParamVal (m68k_areg (regs, 7) + offset, fPtr);
					if (fPtr)
					{
						fVal = (T*) Platform::AllocateMemory (fLen);
						if (fVal && INPUT(inOut))
						{
							uae_memcpy ((void*) fVal, fPtr, fLen);
						}
					}
				}

				~ParamPtr ()	// !!! Update comments about d'tors and disposing memory
				{
					Platform::DisposeMemory (fVal);
				}

		void	Put (void)
				{
					if (fPtr && fVal && OUTPUT(inOut))
					{
						uae_memcpy (fPtr, (const void*) fVal, fLen);
					}
				}

				operator T*(void) { return fVal; }

		operator emuptr (void) { return fPtr; }

	private:
		long	fLen;
		emuptr	fPtr;
		T*		fVal;
};


/* ===========================================================================
	Class that manages a NULL-terminated range of memory pointed to by a
	pointer on the emulated stack.  The input pointer can be NULL. The
	elements of the string can be anything, but will normally be Char
	(actually, they may *have* to be Char, since I get the length of the
	string by calling uae_strlen).  The string is modifiable in-place, but
	any changes are not copied back to emulated memory (this behavior can be
	changed if needed).
=========================================================================== */

template <typename T, long offset>
class ParamStr
{
	public:
				ParamStr (void)
				{
					Marshal::GetParamVal (m68k_areg (regs, 7) + offset, fPtr);
					if (fPtr)
					{
						fVal = (T*) Platform::AllocateMemory (uae_strlen (fPtr) + 1);
						if (fVal)
						{
							uae_strcpy (fVal, fPtr);
						}
					}
				}

				~ParamStr ()	// !!! Update comments about d'tors and disposing memory
				{
					Platform::DisposeMemory (fVal);
				}

		void	Put (void)
				{
					// !!! Do something here?
				}

				operator const T*(void) { return fVal; }
				operator T*(void) { return fVal; }

		operator emuptr (void) { return fPtr; }

	private:
		emuptr	fPtr;
		T*		fVal;
};


#endif	// _MARSHAL_H_
