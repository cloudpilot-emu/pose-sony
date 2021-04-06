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

#ifndef _UAE_UTILS_H_
#define _UAE_UTILS_H_


emuptr	uae_memset(emuptr dst, int val, size_t len);

emuptr	uae_memchr(emuptr src, int val, size_t len);

template <class T1, class T2>
int		uae_memcmp(T1 src1, T2 src2, size_t len);

template <class T1, class T2>
T1		uae_memcpy (T1 dst, T2 src, size_t len);

template <class T1, class T2>
T1		uae_memmove (T1 dst, T2 src, size_t len);

size_t	uae_strlen(emuptr str);

template <class T1, class T2>
T1	uae_strcpy(T1 dst, T2 src);

template <class T1, class T2>
T1	uae_strncpy(T1 dst, T2 src, size_t len);

template <class T1, class T2>
T1	uae_strcat(T1 dst, T2 src);

template <class T1, class T2>
T1	uae_strncat(T1 dst, T2 src, size_t len);

template <class T1, class T2>
int	uae_strcmp(T1 dst, T2 src);

template <class T1, class T2>
int	uae_strncmp(T1 dst, T2 src, size_t len);

#endif /* _UAE_UTILS_H_ */
