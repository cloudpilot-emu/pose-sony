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
#include "Sounds.h"

#include "EmWindow.h"			// EmWindow::GetWindow
#include "EmWindowWin.h"		// GetHostWindow
#include "Platform.h"

#include <math.h>				// sin, atan

WinSounds*	gSounds;

#define MUSIC_FREQ 22050 // this can be adjusted for quality

WinSounds::WinSounds (void) :
//	m_Critical (),
	m_Que (),
	m_Active (FALSE),
	m_SoundOpen (FALSE),
	m_SoundDevice (NULL),
	m_SoundBufferSize (0),
	m_SoundBuffer (NULL),
//	m_SoundHeader (),
	m_HeaderPrep (FALSE),
	m_WaitEvent (INVALID_HANDLE_VALUE)
{
	::InitializeCriticalSection (&m_Critical);
}

WinSounds::~WinSounds(void)
{
	PrvStopPlaying();

	if (m_SoundOpen)
	{
		waveOutClose (m_SoundDevice);
		m_SoundOpen = FALSE;
	}

	Platform::DeleteMemory (m_SoundBuffer);

	if (m_WaitEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle (m_WaitEvent);
		m_WaitEvent = INVALID_HANDLE_VALUE;
	}

	::DeleteCriticalSection (&m_Critical);
}

void WinSounds::QueueNote (int frequency, int duration, int amplitude)
{
	::EnterCriticalSection (&m_Critical);

	_sound_element	elem;

	elem.frequency = frequency;
	elem.duration = duration;
	elem.amplitude = amplitude;

	m_Que.push_back (elem);

	if (!m_Active)		// need to start playing, just send message to main window
	{
		::PostMessage (EmWindow::GetWindow()->GetHostData()->GetHostWindow(), MM_WOM_DONE, 0, 0);
		m_Active = TRUE;
	}

	::LeaveCriticalSection(&m_Critical);
}

void WinSounds::StopSound()
{
	::EnterCriticalSection (&m_Critical);
	PrvStopSound ();
	::LeaveCriticalSection (&m_Critical);
}

void WinSounds::PrvStopSound()
{
	PrvStopPlaying ();
	m_Que.clear ();
}

void WinSounds::PrvStopPlaying()
{
	if (m_SoundOpen)
	{
		if (m_Active)
		{
			waveOutReset (m_SoundDevice); // in case we are actually playing
		}

		if (m_HeaderPrep)
		{
			waveOutUnprepareHeader (m_SoundDevice, &m_SoundHeader, sizeof (WAVEHDR));
			m_HeaderPrep = FALSE;
		}

		waveOutClose (m_SoundDevice);
		m_SoundOpen = FALSE;
	}

	m_Active = FALSE;
}

void WinSounds::NextSound(void)
{
	::EnterCriticalSection (&m_Critical);

	m_Active = FALSE; // avoid device reset, must have been idle or got done message

	if (!PrvNextSound())
		PrvStopSound();

	::LeaveCriticalSection (&m_Critical);
}

// seperate function so it can return false if anything goes wrong

BOOL WinSounds::PrvNextSound(void)
{
	_sound_element	elem;
	int				numSamples;
	WAVEFORMATEX	waveformat;

	if (m_HeaderPrep)
	{
		waveOutUnprepareHeader (m_SoundDevice, &m_SoundHeader, sizeof (WAVEHDR));
		m_HeaderPrep = FALSE;
	}

	if (m_Que.size () == 0)
		return FALSE;	// all done

	elem = *m_Que.begin ();
	m_Que.pop_front ();

	if (elem.frequency == 0)	// this is just a delay
	{
		PauseSound (elem.duration);
		return TRUE;
	}

	waveformat.wFormatTag		= WAVE_FORMAT_PCM;
	waveformat.nChannels		= 1;
	waveformat.nSamplesPerSec	= MUSIC_FREQ;
	waveformat.nAvgBytesPerSec	= MUSIC_FREQ;
	waveformat.nBlockAlign		= 1;
	waveformat.wBitsPerSample	= 8;

	if (!m_SoundOpen)
	{
		if (waveOutOpen (&m_SoundDevice, (UINT) WAVE_MAPPER, &waveformat,
		   (DWORD) EmWindow::GetWindow()->GetHostData()->GetHostWindow(),
		   NULL, (DWORD) CALLBACK_WINDOW) != 0)
		{
			return FALSE; // no sound available
		}

		m_SoundOpen = TRUE;
	}

	// (samp/sec) * msecs * (secs/msec) = samps
	numSamples = MUSIC_FREQ * elem.duration / 1000;

	if (m_SoundBuffer == NULL || m_SoundBufferSize < numSamples)
	{
		Platform::DeleteMemory (m_SoundBuffer);
		m_SoundBuffer = (BYTE*) Platform::AllocateMemory (numSamples);

		if (m_SoundBuffer == NULL)
			return FALSE;

		m_SoundBufferSize = numSamples;
	}

	m_SoundHeader.lpData = (LPSTR) m_SoundBuffer; // may have moved
	m_SoundHeader.dwBufferLength = numSamples;
	m_SoundHeader.dwBytesRecorded = 0;
	m_SoundHeader.dwUser = 0;
	m_SoundHeader.dwFlags = 0;
	m_SoundHeader.dwLoops = 0;

	// Generate nice sign waves for the sample buffer.

	BYTE*	pData = m_SoundBuffer;
	double	f = elem.frequency;
	double	pi = 4.0 * atan(1.0);
	double	w = 2.0 * pi * f;
	int		scale = 127 * elem.amplitude / 64;	// Scale 0-64 to 0-127
	for (int dw = 0; dw < numSamples; dw++)
	{
		double t = (double) dw / (double) MUSIC_FREQ; 
		*pData++ = 128 + (scale * sin(w * t));
//		*pData++ = 128 + (scale * (sin(w * t) > 0 ? 1 : -1));	// Square wave
	}

	if (waveOutPrepareHeader (m_SoundDevice, &m_SoundHeader, sizeof (WAVEHDR)) != 0)
	{
		return FALSE;
	}

	m_HeaderPrep = TRUE;

	if (waveOutWrite (m_SoundDevice, &m_SoundHeader, sizeof (WAVEHDR)) != 0)
	{
		return FALSE;
	}

	m_Active = TRUE;
	return TRUE;
}

void WinSounds::PauseSound (int duration)
{
	if (m_WaitEvent == INVALID_HANDLE_VALUE)
	{
		m_WaitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	}

	if (m_WaitEvent != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject (m_WaitEvent, duration);
	}

	::PostMessage (EmWindow::GetWindow()->GetHostData()->GetHostWindow(),
		MM_WOM_DONE, 0, 0L);
}
