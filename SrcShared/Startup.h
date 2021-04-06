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

#ifndef _STARTUP_H_
#define _STARTUP_H_

#include "EmFileRef.h"			// EmFileRefList
#include "EmStructs.h"			// StringList

struct Configuration;
struct HordeInfo;
class EmFileRef;

class Startup
{
	public:
		static Bool				DetermineStartupActions	(int argc, char** argv);

		static void				GetAutoLoads			(EmFileRefList&);
		static string			GetAutoRunApp			(void);

		// Getters...

		static Bool				AskWhatToDo				(void);
		static Bool				CreateSession			(Configuration&);
		static Bool				OpenSession				(EmFileRef&);
		static Bool				NewHorde				(HordeInfo*);
		static Bool				HordeQuitWhenDone		(void);
		static Bool				CloseSession			(EmFileRef&);
		static Bool				QuitOnExit				(void);
		static Bool				Quit					(void);

		// Setters...

		static void				Clear					(void);
		static void				ScheduleAskWhatToDo		(void);
		static void				ScheduleCreateSession	(const Configuration&);
		static void				ScheduleOpenSession		(const EmFileRef&);
		static void				ScheduleNewHorde		(const HordeInfo&, const StringList&);
		static void				ScheduleCloseSession	(const EmFileRef&);
		static void				ScheduleQuitOnExit		(void);
		static void				ScheduleQuit			(void);
};

#endif	/* _STARTUP_H_ */
