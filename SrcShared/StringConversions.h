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

#ifndef _STRING_CONVERSIONS_H_
#define _STRING_CONVERSIONS_H_

#include "EmDevice.h"			// EmDevice

#include <string>

class EmDirRef;
class EmFileRef;

// ----------------------------------------------------------------------
//	Our preferences routines need to convert our preference data to and
//	from string format (the format in which the data is stored on disk).
//	Following are a bunch of overloaded routines for converting strings
//	into all the data types we use for preferences.
// ----------------------------------------------------------------------

bool FromString (const string& s, bool& value);
bool FromString (const string& s, char& value);
bool FromString (const string& s, signed char& value);
bool FromString (const string& s, unsigned char& value);
bool FromString (const string& s, signed short& value);
bool FromString (const string& s, unsigned short& value);
bool FromString (const string& s, signed int& value);
bool FromString (const string& s, unsigned int& value);
bool FromString (const string& s, signed long& value);
bool FromString (const string& s, unsigned long& value);
bool FromString (const string& s, string& value);
bool FromString (const string& s, char* value);
bool FromString (const string& s, CloseActionType& value);
bool FromString (const string& s, EmDevice& value);
bool FromString (const string& s, EmDirRef& value);
bool FromString (const string& s, EmFileRef& value);

// ----------------------------------------------------------------------
//	Our preferences routines need to convert our preference data to and
//	from string format (the format in which the data is stored on disk).
//	Following are a bunch of overloaded routines for converting data
//	(in all the types we use for preferences) into strings.
// ----------------------------------------------------------------------

string ToString (bool value);
string ToString (char value);
string ToString (signed char value);
string ToString (unsigned char value);
string ToString (signed short value);
string ToString (unsigned short value);
string ToString (signed int value);
string ToString (unsigned int value);
string ToString (signed long value);
string ToString (unsigned long value);
string ToString (const string& value);
string ToString (const char* value);
string ToString (CloseActionType value);
string ToString (const EmDevice& value);
string ToString (const EmDirRef& value);
string ToString (const EmFileRef& value);

#endif	/* _STRING_CONVERSIONS_H_ */
