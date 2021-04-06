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

#ifndef __SOUNDS_H__
#define __SOUNDS_H__

// sound class definitions

struct _sound_element	// in case we want to add volume later
{
	int frequency;
	int duration;
	int amplitude;
};

typedef deque<_sound_element>	_sound_element_list;

class WinSounds
{
	public:
								WinSounds		(void);
		virtual					~WinSounds		(void);

		void					QueueNote		(int frequeny, int duration, int amplitude);
		void					StopSound		(void);
		void					NextSound		(void);

	protected:
		void					PrvStopSound	(void); // stop sound with critical section locked
		void					PrvStopPlaying	(void);
		BOOL					PrvNextSound	(void);
		void					PauseSound		(int duration);

		CRITICAL_SECTION		m_Critical;
		_sound_element_list		m_Que;
		BOOL					m_Active;
		BOOL					m_SoundOpen;
		HWAVEOUT				m_SoundDevice;
		int						m_SoundBufferSize;
		BYTE*					m_SoundBuffer;
		WAVEHDR					m_SoundHeader;
		BOOL					m_HeaderPrep;
		HANDLE					m_WaitEvent;
};

extern WinSounds*	gSounds;

#endif	/* __SOUNDS_H__ */
