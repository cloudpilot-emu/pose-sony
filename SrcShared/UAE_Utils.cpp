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
#include "UAE_Utils.h"

#include "EmMemory.h"			// EmMemGet8, EmMemPut8, etc.


// ======================================================================
//	Private functions
// ======================================================================

// ---------------------------------------------------------------------------
// The functions in this file work with two kinds of "pointers": emuptr's
// and void*'s.  Accessing memory for each kind of pointer is different.
// Since the functions are template functions, we can't know at the time we
// write the function what kind of pointer we'll be dealing with, so we can't
// hard-code the memory-accessing method.  Instead, we provide all of these
// inline accessors and let the compiler sort it out.
// ---------------------------------------------------------------------------

inline char _get_byte(const void* p)
	{ return *(char*) p; }

inline char _get_byte(emuptr p)
	{ return (char) EmMemGet8(p); }

inline void _put_byte(void* p, const uint8 v)
	{ *(uint8*) p = v; }

inline void _put_byte(emuptr p, const uint8 v)
	{ EmMemPut8(p, v); }

inline uint16	_get_word(const void* p)
	{ return *(uint16*) p; }

inline uint16	_get_word(emuptr p)
	{ return (uint16) EmMemGet16(p); }

inline void _put_word(void* p, const uint16 v)
	{ *(uint16*) p = v; }

inline void _put_word(emuptr p, const uint16 v)
	{ EmMemPut16(p, v); }

inline uint32	_get_long(const void* p)
	{ return *(uint32*) p; }

inline uint32	_get_long(emuptr p)
	{ return (uint32) EmMemGet32(p); }

inline void _put_long(void* p, const uint32 v)
	{ *(uint32*) p = v; }

inline void _put_long(emuptr p, const uint32 v)
	{ EmMemPut32(p, v); }

inline const void* _get_real_address(const void* p)
	{ return p; }

inline const void* _get_real_address(emuptr p)
	{ return EmMemGetRealAddress(p); }

template <class T>
inline void _add_delta (T*& v, long delta)
{
	v = (T*) (((char*) v) + delta);
}

inline void _add_delta (emuptr& v, long delta)
{
	v += delta;
}

template <class T>
inline void _increment (T& v)
{
	_add_delta (v, 1);
}

template <class T>
inline void _decrement (T& v)
{
	_add_delta (v, -1);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	uae_memset
 *
 * DESCRIPTION: Same as Std C Library memset function.
 *
 * PARAMETERS:	Same as Std C Library memset function.
 *
 * RETURNED:	Same as Std C Library memset function.
 *
 ***********************************************************************/

emuptr 	uae_memset(emuptr dst, int val, size_t len)
{
	emuptr 	q = dst;

	uint32 longVal = val;
	longVal |= (longVal << 8);
	longVal |= (longVal << 16);

	EmMemPutFunc	longPutter = EmMemGetBank(dst).lput;
	EmMemPutFunc	bytePutter = EmMemGetBank(dst).bput;

	while ((q & 3) && len > 0)		// while there are leading bytes
	{
		bytePutter(q, val);
		q += sizeof (char);
		len -= sizeof (char);
	}

	while (len >= sizeof (long))	// while there are middle longs
	{
		longPutter(q, longVal);
		q += sizeof (long);
		len -= sizeof (long);
	}

	while (len > 0) 				// while there are trailing bytes
	{
		bytePutter(q, val);
		q += sizeof (char);
		len -= sizeof (char);
	}

	return dst;
}


/***********************************************************************
 *
 * FUNCTION:	uae_memchr
 *
 * DESCRIPTION: Same as Std C Library memchr function.
 *
 * PARAMETERS:	Same as Std C Library memchr function.
 *
 * RETURNED:	Same as Std C Library memchr function.
 *
 ***********************************************************************/

emuptr uae_memchr(emuptr src, int val, size_t len)
{
	emuptr p = src;

	++len;

	while (--len)
	{
		if (_get_byte (p) == val)
			return p;

		_increment (p);
	}

	return EmMemNULL;
}


/***********************************************************************
 *
 * FUNCTION:	uae_memcmp
 *
 * DESCRIPTION: Same as Std C Library memcmp function.
 *
 * PARAMETERS:	Same as Std C Library memcmp function.
 *
 * RETURNED:	Same as Std C Library memcmp function.
 *
 ***********************************************************************/

template <class T1, class T2>
int 	uae_memcmp(T1 src1, T2 src2, size_t len)
{
	T1	p1 = src1;
	T2	p2 = src2;

	++len;

	while (--len)
	{
		unsigned char	ch1 = _get_byte (p1);
		unsigned char	ch2 = _get_byte (p2);
		
		if (ch1 != ch2)
			return (ch1 < ch2) ? -1 : 1;

		_increment (p1);
		_increment (p2);
	}

	return 0;
}

	// Instantiate uae_memcmp's that work with:
	//
	//			dest		source
	//			------		------
	//			void*		emuptr
	//			const void* emuptr
	//			emuptr 	void*
	//			emuptr 	const void*
	//			emuptr 	emuptr

template int		uae_memcmp<void*, emuptr>			(void* dst, emuptr src, size_t len);
template int		uae_memcmp<const void*, emuptr>	(const void* dst, emuptr src, size_t len);
template int		uae_memcmp<emuptr, void*>			(emuptr dst, void* src, size_t len);
template int		uae_memcmp<emuptr, const void*>	(emuptr dst, const void* src, size_t len);
template int		uae_memcmp<emuptr, emuptr>		(emuptr dst, emuptr src, size_t len);


/***********************************************************************
 *
 * FUNCTION:	uae_memcpy
 *
 * DESCRIPTION: Same as Std C Library memcpy function.
 *
 * PARAMETERS:	Same as Std C Library memcpy function.
 *
 * RETURNED:	Same as Std C Library memcpy function.
 *
 ***********************************************************************/

template <class T1, class T2>
T1		uae_memcpy (T1 dst, T2 src, size_t len)
{
	T1		q = dst;
	T2		p = src;

	while (len--)
	{
		_put_byte(q, _get_byte(p));
		_increment (q);
		_increment (p);
	}

	return dst;
}

	// Instantiate uae_memcpy's that work with:
	//
	//			dest		source
	//			------		------
	//			void*		emuptr
	//			emuptr 	void*
	//			emuptr 	const void*
	//			emuptr 	emuptr

template void*		uae_memcpy<void*, emuptr>			(void* dst, emuptr src, size_t len);
template emuptr	uae_memcpy<emuptr, void*>			(emuptr dst, void* src, size_t len);
template emuptr	uae_memcpy<emuptr, const void*>	(emuptr dst, const void* src, size_t len);
template emuptr	uae_memcpy<emuptr, emuptr>		(emuptr dst, emuptr src, size_t len);


/***********************************************************************
 *
 * FUNCTION:	uae_memmove
 *
 * DESCRIPTION: Same as Std C Library memmove function.
 *
 * PARAMETERS:	Same as Std C Library memmove function.
 *
 * RETURNED:	Same as Std C Library memmove function.
 *
 ***********************************************************************/

template <class T1, class T2>
T1		uae_memmove (T1 dst, T2 src, size_t len)
{
	T1		q = dst;
	T2		p = src;

	Bool	backward = _get_real_address(dst) <= _get_real_address(src);

	if (backward)
	{
		while (len--)
		{
			_put_byte(q, _get_byte(p));
			_increment (q);
			_increment (p);
		}
	}
	else
	{
		_add_delta (q, len);
		_add_delta (p, len);

		while (len--)
		{
			_decrement (q);
			_decrement (p);
			_put_byte(q, _get_byte(p));
		}
	}

	return dst;
}

	// Instantiate uae_memmove's that work with:
	//
	//			dest		source
	//			------		------
	//			void*		emuptr
	//			emuptr 	void*
	//			emuptr 	const void*
	//			emuptr 	emuptr

template void*		uae_memmove<void*, emuptr> 		(void* dst, emuptr src, size_t len);
template emuptr	uae_memmove<emuptr, void*> 		(emuptr dst, void* src, size_t len);
template emuptr	uae_memmove<emuptr, const void*>	(emuptr dst, const void* src, size_t len);
template emuptr	uae_memmove<emuptr, emuptr>		(emuptr dst, emuptr src, size_t len);


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	uae_strlen
 *
 * DESCRIPTION: Same as Std C Library strlen function.
 *
 * PARAMETERS:	Same as Std C Library strlen function.
 *
 * RETURNED:	Same as Std C Library strlen function.
 *
 ***********************************************************************/

size_t	uae_strlen(emuptr str)
{
	emuptr eos = str;

	while (_get_byte(eos))
		_increment (eos);

	return ((size_t) (eos - str));
}


/***********************************************************************
 *
 * FUNCTION:	uae_strcpy
 *
 * DESCRIPTION: Same as Std C Library strcpy function.
 *
 * PARAMETERS:	Same as Std C Library strcpy function.
 *
 * RETURNED:	Same as Std C Library strcpy function.
 *
 ***********************************************************************/

template <class T1, class T2>
T1	uae_strcpy(T1 dst, T2 src)
{
	T1		q = dst;
	T2		p = src;
	char	ch;

	do {
		ch = _get_byte (p);
		_increment (p);
		_put_byte (q, ch);
		_increment (q);
	} while (ch);

	return dst;
}

	// Instantiate uae_strcpy's that work with:
	//
	//			dest		source
	//			------		------
	//			char*		emuptr
	//			emuptr 	char*
	//			emuptr 	const char*
	//			emuptr 	emuptr

template char*		uae_strcpy<char*, emuptr>			(char* dst, emuptr src);
template emuptr	uae_strcpy<emuptr, char*>			(emuptr dst, char* src);
template emuptr	uae_strcpy<emuptr, const char*>	(emuptr dst, const char* src);
template emuptr	uae_strcpy<emuptr, emuptr>		(emuptr dst, emuptr src);


/***********************************************************************
 *
 * FUNCTION:	uae_strncpy
 *
 * DESCRIPTION: Same as Std C Library strncpy function.
 *
 * PARAMETERS:	Same as Std C Library strncpy function.
 *
 * RETURNED:	Same as Std C Library strncpy function.
 *
 ***********************************************************************/

template <class T1, class T2>
T1	uae_strncpy(T1 dst, T2 src, size_t len)
{
	T1		q = dst;
	T2		p = src;

	++len;

	while (--len)
	{
		char	ch = _get_byte(p);
		_increment (p);

		_put_byte (q, ch);
		_increment (q);

		if (!ch)
		{
			while (--len)
			{
				_put_byte (q, 0);
				_increment (q);
			}

			break;
		}
	}

	return dst;
}

	// Instantiate uae_strncpy's that work with:
	//
	//			dest		source
	//			------		------
	//			char*		emuptr
	//			emuptr 	char*
	//			emuptr 	const char*
	//			emuptr 	emuptr

template char*		uae_strncpy<char*, emuptr> 		(char* dst, emuptr src, size_t len);
template emuptr	uae_strncpy<emuptr, char*> 		(emuptr dst, char* src, size_t len);
template emuptr	uae_strncpy<emuptr, const char*>	(emuptr dst, const char* src, size_t len);
template emuptr	uae_strncpy<emuptr, emuptr>		(emuptr dst, emuptr src, size_t len);


/***********************************************************************
 *
 * FUNCTION:	uae_strcat
 *
 * DESCRIPTION: Same as Std C Library strcat function.
 *
 * PARAMETERS:	Same as Std C Library strcat function.
 *
 * RETURNED:	Same as Std C Library strcat function.
 *
 ***********************************************************************/

template <class T1, class T2>
T1	uae_strcat(T1 dst, T2 src)
{
	T1	q = dst;
	T2	p = src;
	
	while (_get_byte (q))
		_increment(q);
	
	while (_get_byte (p))
	{
		_put_byte (q, _get_byte (p));
		_increment(q);
		_increment(p);
	}

	_put_byte (q, 0);

	return dst;
}

	// Instantiate uae_strcat's that work with:
	//
	//			dest		source
	//			------		------
	//			char*		emuptr
	//			emuptr 	char*
	//			emuptr 	const char*
	//			emuptr 	emuptr

template char*		uae_strcat<char*, emuptr>			(char* dst, emuptr src);
template emuptr	uae_strcat<emuptr, char*>			(emuptr dst, char* src);
template emuptr	uae_strcat<emuptr, const char*>	(emuptr dst, const char* src);
template emuptr	uae_strcat<emuptr, emuptr>		(emuptr dst, emuptr src);


/***********************************************************************
 *
 * FUNCTION:	uae_strncat
 *
 * DESCRIPTION: Same as Std C Library strncat function.
 *
 * PARAMETERS:	Same as Std C Library strncat function.
 *
 * RETURNED:	Same as Std C Library strncat function.
 *
 ***********************************************************************/

template <class T1, class T2>
T1	uae_strncat(T1 dst, T2 src, size_t len)
{
	T1		q = dst;
	T2		p = src;

	while (_get_byte (q))
		_increment (q);

	++len;

	while (--len)
	{
		char	ch = _get_byte(p);
		_increment (p);

		_put_byte (q, ch);
		_increment (q);

		if (!ch)
			return dst;
	}

	_put_byte (q, 0);

	return dst;
}

	// Instantiate uae_strncat's that work with:
	//
	//			dest		source
	//			------		------
	//			char*		emuptr
	//			emuptr 	char*
	//			emuptr 	const char*
	//			emuptr 	emuptr

template char*		uae_strncat<char*, emuptr> 		(char* dst, emuptr src, size_t len);
template emuptr	uae_strncat<emuptr, char*> 		(emuptr dst, char* src, size_t len);
template emuptr	uae_strncat<emuptr, const char*>	(emuptr dst, const char* src, size_t len);
template emuptr	uae_strncat<emuptr, emuptr>		(emuptr dst, emuptr src, size_t len);


/***********************************************************************
 *
 * FUNCTION:	uae_strcmp
 *
 * DESCRIPTION: Same as Std C Library strcmp function.
 *
 * PARAMETERS:	Same as Std C Library strcmp function.
 *
 * RETURNED:	Same as Std C Library strcmp function.
 *
 ***********************************************************************/

template <class T1, class T2>
int uae_strcmp(T1 dst, T2 src)
{
	T1	p1 = dst;
	T2	p2 = src;
	
	while (1)
	{
		unsigned char	c1 = _get_byte (p1);
		unsigned char	c2 = _get_byte (p2);

		if (c1 != c2)
			return (c1 - c2);
		else if (!c1)
			break;

		_increment (p1);
		_increment (p2);
	}

	return 0;
}

	// Instantiate uae_strcmp's that work with:
	//
	//			dest		source
	//			------		------
	//			char*		emuptr
	//			const char* emuptr
	//			emuptr 	char*
	//			emuptr 	const char*
	//			emuptr 	emuptr

template int		uae_strcmp<char*, emuptr>			(char* dst, emuptr src);
template int		uae_strcmp<const char*, emuptr>	(const char* dst, emuptr src);
template int		uae_strcmp<emuptr, char*>			(emuptr dst, char* src);
template int		uae_strcmp<emuptr, const char*>	(emuptr dst, const char* src);
template int		uae_strcmp<emuptr, emuptr>		(emuptr dst, emuptr src);


/***********************************************************************
 *
 * FUNCTION:	uae_strncmp
 *
 * DESCRIPTION: Same as Std C Library strncmp function.
 *
 * PARAMETERS:	Same as Std C Library strncmp function.
 *
 * RETURNED:	Same as Std C Library strncmp function.
 *
 ***********************************************************************/

template <class T1, class T2>
int uae_strncmp(T1 dst, T2 src, size_t len)
{
	T1	p1 = dst;
	T2	p2 = src;
	
	++len;
	
	while (--len)
	{
		unsigned char	c1 = _get_byte (p1);
		unsigned char	c2 = _get_byte (p2);

		if (c1 != c2)
			return (c1 - c2);
		else if (!c1)
			break;

		_increment (p1);
		_increment (p2);
	}

	return 0;
}

	// Instantiate uae_strncmp's that work with:
	//
	//			dest		source
	//			------		------
	//			char*		emuptr
	//			const char* emuptr
	//			emuptr 	char*
	//			emuptr 	const char*
	//			emuptr 	emuptr

template int		uae_strncmp<char*, emuptr> 		(char* dst, emuptr src, size_t len);
template int		uae_strncmp<const char*, emuptr>	(const char* dst, emuptr src, size_t len);
template int		uae_strncmp<emuptr, char*> 		(emuptr dst, char* src, size_t len);
template int		uae_strncmp<emuptr, const char*>	(emuptr dst, const char* src, size_t len);
template int		uae_strncmp<emuptr, emuptr>		(emuptr dst, emuptr src, size_t len);
