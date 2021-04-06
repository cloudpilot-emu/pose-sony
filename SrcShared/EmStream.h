/* -*- mode: C++; tab-width: 4 -*- */
// ===========================================================================
//	EmStream.h				   ©1993-1998 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Abstract class for reading/writing an ordered sequence of bytes

#ifndef EmStream_h
#define EmStream_h

#include "Byteswapping.h"		// Canonical
#include "EmTypes.h"			// ErrCode

#include <deque>
#include <list>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------

enum StreamFromType
{
	kStreamFromStart = 1,
	kStreamFromEnd,
	kStreamFromMarker
};

// ---------------------------------------------------------------------------

class EmStream
{
	public:
								EmStream		(void);
		virtual					~EmStream		(void);

		virtual void			SetMarker		(int32			inOffset,
												 StreamFromType	inFromWhere);
		virtual int32			GetMarker		(void) const;

		virtual void			SetLength		(int32			inLength);
		virtual int32			GetLength		(void) const;

		Bool					AtEnd			(void) const
								{
									return GetMarker () >= GetLength ();
								}

						// Write Operations

		virtual ErrCode			PutBytes		(const void*	inBuffer,
												 int32			ioByteCount);

		EmStream&				operator <<		(const char* inString)
								{
									WriteCString (inString);
									return (*this);
								}

		EmStream&				operator <<		(const string& inString)
								{
									WriteString (inString);
									return (*this);
								}

		EmStream&				operator <<		(int8 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(uint8 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(char inChar)
								{
									Canonical (inChar);
									PutBytes (&inChar, sizeof (inChar));
									return (*this);
								}

		EmStream&				operator <<		(int16 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(uint16 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(int32 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(uint32 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(int64 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(uint64 inNum)
								{
									Canonical (inNum);
									PutBytes (&inNum, sizeof (inNum));
									return (*this);
								}

		EmStream&				operator <<		(bool inBool)
								{
									Canonical (inBool);
									PutBytes (&inBool, sizeof (inBool));
									return (*this);
								}

						// Read Operations

		virtual ErrCode			GetBytes		(void*	outBuffer,
												 int32	ioByteCount);
		int32					PeekData		(void*	outButter,
												 int32	inByteCount);

		EmStream&				operator >>		(char* outString)
								{
									ReadCString (outString);
									return (*this);
								}

		EmStream&				operator >>		(string& outString)
								{
									ReadString (outString);
									return (*this);
								}

		EmStream&				operator >>		(int8 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(uint8 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(char &outChar)
								{
									GetBytes (&outChar, sizeof (outChar));
									Canonical (outChar);
									return (*this);
								}

		EmStream&				operator >>		(int16 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(uint16 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(int32 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(uint32 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(int64 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(uint64 &outNum)
								{
									GetBytes (&outNum, sizeof (outNum));
									Canonical (outNum);
									return (*this);
								}

		EmStream&				operator >>		(bool &outBool)
								{
									GetBytes (&outBool, sizeof( outBool));
									Canonical (outBool);
									return (*this);
								}

		template <class T>
		EmStream&				operator >>		(deque<T>& container)
								{
									Int32	numElements;
									*this >> numElements;

									container.resize (numElements);

									deque<T>::iterator	iter = container.begin ();
									while (iter != container.end ())
									{
										*this >> *iter;
										++iter;
									}

									return *this;
								}

		template <class T>
		EmStream&				operator >>		(list<T>& container)
								{
									Int32	numElements;
									*this >> numElements;

									container.resize (numElements);

									list<T>::iterator	iter = container.begin ();
									while (iter != container.end ())
									{
										*this >> *iter;
										++iter;
									}

									return *this;
								}

		template <class T>
		EmStream&				operator >>		(vector<T>& container)
								{
									Int32	numElements;
									*this >> numElements;

									container.resize (numElements);

									vector<T>::iterator	iter = container.begin ();
									while (iter != container.end ())
									{
										*this >> *iter;
										++iter;
									}

									return *this;
								}

		template <class T>
		EmStream&				operator <<		(const deque<T>& container)
								{
									Int32	numElements = container.size ();

									*this << numElements;

									deque<T>::const_iterator	iter = container.begin ();
									while (iter != container.end ())
									{
										*this << *iter;
										++iter;
									}

									return *this;
								}

		template <class T>
		EmStream&				operator <<		(const list<T>& container)
								{
									Int32	numElements = container.size ();

									*this << numElements;

									list<T>::const_iterator	iter = container.begin ();
									while (iter != container.end ())
									{
										*this << *iter;
										++iter;
									}

									return *this;
								}

		template <class T>
		EmStream&				operator <<		(const vector<T>& container)
								{
									Int32	numElements = container.size ();

									*this << numElements;

									vector<T>::const_iterator	iter = container.begin ();
									while (iter != container.end ())
									{
										*this << *iter;
										++iter;
									}

									return *this;
								}

		// Data-specific read/write functions
		//   There is an equivalent Shift operator for each one
		//	 except WritePtr/ReadPtr (since Ptr is really a char*,
		//	 which is the same as a C string).

		int32					WriteCString	(const char*	inString);
		int32					ReadCString		(char*			outString);

		int32					WriteString		(const string&	inString);
		int32					ReadString		(string&		outString);

		int						PrintF			(const char* fmt, ...);
		int						ScanF			(const char* fmt, ...);

		int						PutC			(int);
		int						GetC			(void);

		int						PutS			(const char*);
		char*					GetS			(char*, int n);

	protected:
		int32					mMarker;
		int32					mLength;
};


#if 0
	// Although the MSL headers indicate that it can support member template
	// functions (see _MSL_MUST_INLINE_MEMBER_TEMPLATE in mslconfig), the
	// following out-of-line definitions result in a compilation error.  So,
	// for now, the definitions are in-line.

template <class T>
EmStream& EmStream::operator>> (deque<T>& container)
{
	Int32	numElements;
	*this >> numElements;

	container.clear ();

	for (Int32 ii = 0; ii < numElements; ++ii)
	{
		T	object;
		*this >> object;
		container.push_back (object);
	}

	return *this;
}


template <class T>
EmStream& EmStream::operator<T> >> (list<T>& container)
{
	Int32	numElements;
	*this >> numElements;

	container.clear ();

	for (Int32 ii = 0; ii < numElements; ++ii)
	{
		T	object;
		*this >> object;
		container.push_back (object);
	}

	return *this;
}


template <class T>
EmStream& EmStream::operator<T> >> (vector<T>& container)
{
	Int32	numElements;
	*this >> numElements;

	container.clear ();

	for (Int32 ii = 0; ii < numElements; ++ii)
	{
		T	object;
		*this >> object;
		container.push_back (object);
	}

	return *this;
}


template <class T>
EmStream& EmStream::operator<T> << (const deque<T>& container)
{
	Int32	numElements = container.size ();

	*this << numElements;

	const deque<T>::const_iterator	iter = container.begin ();
	while (iter != container.end ())
	{
		*this << *iter;
	}

	return *this;
}


template <class T>
EmStream& EmStream::operator<T> << (const list<T>& container)
{
	Int32	numElements = container.size ();

	*this << numElements;

	const list<T>::const_iterator	iter = container.begin ();
	while (iter != container.end ())
	{
		*this << *iter;
	}

	return *this;
}


template <class T>
EmStream& EmStream::operator<T> << (const vector<T>& container)
{
	Int32	numElements = container.size ();

	*this << numElements;

	const vector<T>::const_iterator	iter = container.begin ();
	while (iter != container.end ())
	{
		*this << *iter;
	}

	return *this;
}
#endif	/* if 0 */

class EmStreamBlock : public EmStream
{
	public:
								EmStreamBlock	(void*, int32);
		virtual					~EmStreamBlock	(void);

		virtual void			SetLength		(int32			inLength);

		virtual ErrCode			PutBytes		(const void*	inBuffer,
												 int32			ioByteCount);
		virtual ErrCode			GetBytes		(void*			outBuffer,
												 int32			ioByteCount);

	private:
		void*					fData;
};



#endif	/* EmStream_h */
