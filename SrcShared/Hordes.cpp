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

#include "EmCommon.h"
#include "Hordes.h"				// class Hordes

#include "CGremlins.h"			// Gremlins
#include "CGremlinsStubs.h"		// StubAppGremlinsOff
#include "EmMapFile.h"			// EmMapFile::Write, etc.
#include "EmSession.h"			// gSession, ScheduleResumeHordesFromFile
#include "ErrorHandling.h"		// Errors::ThrowIfPalmError
#include "Logging.h"			// LogStartNew, etc.
#include "Platform.h"			// Platform::GetMilliseconds
#include "PreferenceMgr.h"		// Preference, gEmuPrefs
#include "ROMStubs.h"			// EvtWakeup
#include "Startup.h"			// HordeQuitWhenDone, ScheduleQuit
#include "StringConversions.h"	// ToString, FromString;
#include "Strings.r.h"			// kStr_CmdOpen, etc.


////////////////////////////////////////////////////////////////////////////////////////
// HORDES CONSTANTS

static const int	MAXGREMLINS						= 999;
static const char	kStrRootStateFilename[]			= "Gremlin_Root_State.psf";
static const char	kStrSearchProgressFilename[]	= "Gremlin_Search_Progress.dat";

// Defining the following as constants caused me a lot of pain. On the one hand,
// it's a good idea to keep these filenames in one place. But on the other hand,
// they contain printf format codes that won't be obvious when you're using
// these contants-- and failing to provide the arguments for the corresponding
// format codes will not produce any compiler errors. I've decided to define
// these as constants, and have left comments where they are used alerting to
// the presence of the formatting codes.

static const char	kStrSuspendedStateFilename[]	= "Gremlin_%03ld_Suspended.psf";
static const char	kStrAutoSaveFilename[]			= "Gremlin_%03ld_Event_%08ld.psf";


////////////////////////////////////////////////////////////////////////////////////////
// HORDES STATIC DATA

static	Gremlins	gTheGremlin;
static	int32		gGremlinStartNumber;
static	int32		gGremlinStopNumber;
static	int32		gSwitchDepth;
static	int32		gMaxDepth;
		int32		gGremlinSaveFrequency;
DatabaseInfoList	gGremlinAppList;
static	int32		gCurrentGremlin;
static	int32		gCurrentDepth;
static	bool		gIsOn;
static	uint32		gStartTime;
static	uint32		gStopTime;
static	bool		gGremlinHaltedInError[MAXGREMLINS + 1];
static	EmDirRef	gHomeForHordesFiles;

static Bool			gForceNewHordesDirectory;
static EmDirRef		gGremlinDir;

Bool				gWarningHappened;
Bool				gErrorHappened;


////////////////////////////////////////////////////////////////////////////////////////
// HORDES METHODS

/***********************************************************************
 *
 * FUNCTION:	Hordes::Initialize
 *
 * DESCRIPTION: Standard initialization function.  Responsible for
 *				initializing this sub-system when a new session is
 *				created.  Will be followed by at least one call to
 *				Reset or Load.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Hordes::Initialize (void)
{
	gTheGremlin.Reset ();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Reset
 *
 * DESCRIPTION:	Standard reset function.  Sets the sub-system to a
 *				default state.  This occurs not only on a Reset (as
 *				from the menu item), but also when the sub-system
 *				is first initialized (Reset is called after Initialize)
 *				as well as when the system is re-loaded from an
 *				insufficient session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Hordes::Reset (void)
{
	Hordes::Stop ();
	gTheGremlin.Reset ();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Save
 *
 * DESCRIPTION:	Standard save function.  Saves any sub-system state to
 *				the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Hordes::Save (SessionFile& f)
{
	gTheGremlin.Save(f);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Load
 *
 * DESCRIPTION:	Standard load function.  Loads any sub-system state
 *				from the given session file.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Hordes::Load (SessionFile& f)
{
	Bool fHordesOn = gTheGremlin.Load(f);
	Hordes::TurnOn(fHordesOn);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Dispose
 *
 * DESCRIPTION:	Standard dispose function.  Completely release any
 *				resources acquired or allocated in Initialize and/or
 *				Load.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void Hordes::Dispose (void)
{
	gTheGremlin.Reset ();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::New
 *
 * DESCRIPTION: Starts a new Horde of Gremlins
 *
 * PARAMETERS:	HordeInfo& info - Horde initialization info
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::New(const HordeInfo& info)
{
	gGremlinStartNumber = min(info.fStartNumber, info.fStopNumber);
	gGremlinStopNumber = max(info.fStartNumber, info.fStopNumber);
	
	if (info.fSwitchDepth == -1 || gGremlinStartNumber == gGremlinStopNumber)
		gSwitchDepth = info.fMaxDepth;

	else
		gSwitchDepth = info.fSwitchDepth;
	
	gMaxDepth = info.fMaxDepth;
	gGremlinSaveFrequency = info.fSaveFrequency;
	gGremlinAppList = info.fAppList;
	gCurrentDepth = 0;
	gCurrentGremlin = gGremlinStartNumber;

	if (gSwitchDepth == 0)
		gSwitchDepth = -1;

	if (gMaxDepth == 0)
		gMaxDepth = -1;

	for (int i = 0; i <= MAXGREMLINS; i++)
		gGremlinHaltedInError[i] = false;
	
	GremlinInfo gremInfo;
	
	gremInfo.fNumber		= gCurrentGremlin;
	gremInfo.fSaveFrequency	= gGremlinSaveFrequency;
	gremInfo.fSteps			= (gMaxDepth == -1) ? gSwitchDepth : min (gSwitchDepth, gMaxDepth);
	gremInfo.fAppList		= gGremlinAppList;

	gStartTime = Platform::GetMilliseconds();

	gTheGremlin.New(gremInfo);

	Platform::GCW_Open ();

	Hordes::UseNewAutoSaveDirectory ();

	if (!InSingleGremlinMode ())
	{
		// When we save our root state, we want it to be saved with all
		// the correct GremlinInfo (per the 2.0 file format) but with
		// Gremlins turned OFF.

		Hordes::TurnOn(false);

		Hordes::SaveRootState();

		Hordes::TurnOn(true);
	}

	Hordes::StartLog();

	gWarningHappened = false;
	gErrorHappened = false;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::NewGremlin
 *
 * DESCRIPTION: Starts a new Horde of just one Gremlin --
 *				"classic Gremlins"
 *
 * PARAMETERS:	GremlinInfo& info - Gremlin initialization info
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::NewGremlin(const GremlinInfo &info)
{
	HordeInfo newHorde;

	newHorde.fStartNumber	= info.fNumber;
	newHorde.fStopNumber	= info.fNumber;
	newHorde.fSwitchDepth	= info.fSteps;
	newHorde.fMaxDepth		= info.fSteps;
	newHorde.fSaveFrequency	= info.fSaveFrequency;
	newHorde.fAppList		= info.fAppList;
	
	Hordes::New(newHorde);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Status
 *
 * DESCRIPTION: Returns several pieces of status information about the
 *				currently running Gremlin in the Horde.
 *
 * PARAMETERS:	currentNumber - returns the current Gremlin number.
 *				currentStep - returns the current event number of the
 *					currently running Gremlin
 *				currentUntil - returns the current upper event bound of
 *					currently running Gremlin
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::Status(unsigned short *currentNumber, unsigned long *currentStep,
				unsigned long *currentUntil)
{
	gTheGremlin.Status(currentNumber, currentStep, currentUntil);
}


string
Hordes::GremlinsFlagsToString()
{
	string output;

	for (int i = 0; i < MAXGREMLINS; i++)
		gGremlinHaltedInError[i] ? output += "1" : output += "0";

	return output;
}

void
Hordes::GremlinsFlagsFromString(string& inFlags)
{
	for (int i = 0; i < MAXGREMLINS; i++)
		gGremlinHaltedInError[i] = (inFlags.c_str()[i] == '1');
}

void
Hordes::SaveSearchProgress()
{
	StringStringMap	searchProgress;

	searchProgress["gGremlinStartNumber"]	= ::ToString (gGremlinStartNumber);
	searchProgress["gGremlinStopNumber"]	= ::ToString (gGremlinStopNumber);
	searchProgress["gSwitchDepth"]			= ::ToString (gSwitchDepth);
	searchProgress["gMaxDepth"]				= ::ToString (gMaxDepth);
	searchProgress["gGremlinSaveFrequency"]	= ::ToString (gGremlinSaveFrequency);
	searchProgress["gCurrentGremlin"]		= ::ToString (gCurrentGremlin);
	searchProgress["gCurrentDepth"]			= ::ToString (gCurrentDepth);
	searchProgress["gGremlinHaltedInError"]	= Hordes::GremlinsFlagsToString ();
	searchProgress["gStartTime"]			= ::ToString (gStartTime);
	searchProgress["gStopTime"]				= ::ToString (gStopTime);

	EmDirRef	gremlinDir (Hordes::GetGremlinDirectory ());
	EmFileRef	searchFile (gremlinDir, kStrSearchProgressFilename);
	EmMapFile::Write (searchFile, searchProgress);
}


void
Hordes::ResumeSearchProgress (const EmFileRef& f)
{
	StringStringMap	searchProgress;

	EmMapFile::Read (f, searchProgress);

	::FromString (searchProgress["gGremlinStartNumber"],	gGremlinStartNumber);
	::FromString (searchProgress["gGremlinStopNumber"],		gGremlinStopNumber);
	::FromString (searchProgress["gSwitchDepth"],			gSwitchDepth);
	::FromString (searchProgress["gMaxDepth"],				gMaxDepth);
	::FromString (searchProgress["gGremlinSaveFrequency"],	gGremlinSaveFrequency);
	::FromString (searchProgress["gCurrentGremlin"],		gCurrentGremlin);
	::FromString (searchProgress["gCurrentDepth"],			gCurrentDepth);

	Hordes::GremlinsFlagsFromString (searchProgress["gGremlinHaltedInError"]);

	// Get, then patch up start and stop times.

	::FromString (searchProgress["gStartTime"],				gStartTime);
	::FromString (searchProgress["gStopTime"],				gStopTime);

	uint32	delta = gStopTime - gStartTime;
	gStopTime	= Platform::GetMilliseconds ();
	gStartTime	= gStopTime - delta;

//	gSession->ScheduleResumeHordesFromFile ();
}

Bool
Hordes::IsOn()
{
	return gIsOn;
}

Bool
Hordes::InSingleGremlinMode()
{
	return gGremlinStartNumber == gGremlinStopNumber;
}


Bool
Hordes::SilentRunning()
{
	Preference<Bool>	pref (kPrefKeySilentRunning);
	return *pref;
}


Bool
Hordes::CanNew()
{
	return true;
}

Bool
Hordes::CanStep()
{
	return gTheGremlin.IsInitialized();
}

Bool
Hordes::CanResume()
{
	return gTheGremlin.IsInitialized() && !gIsOn;
}

Bool
Hordes::CanStop()
{
	return gIsOn;
}

int32
Hordes::GremlinNumber()
{
	return gCurrentGremlin;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::TurnOn
 *
 * DESCRIPTION: Turns Hordes on or off.
 *
 * PARAMETERS:	fHordesOn - specifies if Hordes should be on or off.
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::TurnOn(Bool hordesOn)
{
	gIsOn = (hordesOn != 0);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::EventCounter
 *
 * DESCRIPTION: Returns the current event count of the currently running
 *				Gremlin
 *
 * PARAMETERS:	none
 *
 * RETURNED:	event count
 *
 ***********************************************************************/

int32
Hordes::EventCounter(void)
{
	unsigned short	number;
	unsigned long	step;
	unsigned long	until;
	Status (&number, &step, &until);

	return step;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::EventLimit
 *
 * DESCRIPTION: Returns the current event limit of the currently running
 *				Gremlin
 *
 * PARAMETERS:	none
 *
 * RETURNED:	event limit
 *
 ***********************************************************************/

int32
Hordes::EventLimit(void)
{
	unsigned short	number;
	unsigned long	step;
	unsigned long	until;
	Status (&number, &step, &until);

	return until;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::EndHordes
 *
 * DESCRIPTION: Ends Hordes, giving back control to user.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::EndHordes()
{
	Hordes::Stop();

	LogAppendMsg ("*************   Gremlin Horde ended at Gremlin #%ld", gCurrentGremlin);

	LogDump();
	LogClear();

	if (!InSingleGremlinMode())
	{
		Platform::GCW_Close();

		EmAssert (gSession);
		gSession->ScheduleLoadRootState ();
	}

	if (Startup::HordeQuitWhenDone ())
	{
		Startup::ScheduleCloseSession (EmFileRef ());
		Startup::ScheduleQuit ();
	}
	else
	{
		gWarningHappened = false;
		gErrorHappened = false;
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::ProposeNextGremlin
 *
 * DESCRIPTION: Proposes the NEXT Gremlin # and corresponding search
 *				depth, FROM the state of the Hordes run specified by the
 *				input paramaters
 *
 * PARAMETERS:	outNextGremlin - passes back suggested next Gremlin
 *				outNextDepth - passes back next depth
 *				inFromGremlin - "current" Gremlin
 *				inFromGremlin - "current" depth
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::ProposeNextGremlin(long& outNextGremlin, long& outNextDepth,
						   long inFromGremlin, long inFromDepth)
{
	outNextGremlin = inFromGremlin + 1;
	outNextDepth = inFromDepth;
	
	if (outNextGremlin == gGremlinStopNumber + 1)
	{
		outNextGremlin = gGremlinStartNumber;
		++outNextDepth;
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::StartGremlinFromLoadedRootState
 *
 * DESCRIPTION: After the CPU loads the root state during an off-cycle,
 *				it calls this to indicate that Hordes is meant to start
 *				the current Gremlin, and that the Emulator state is ready
 *				for this.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::StartGremlinFromLoadedRootState()
{
	GremlinInfo gremInfo;
	
	gremInfo.fNumber		= gCurrentGremlin;
	gremInfo.fSaveFrequency	= gGremlinSaveFrequency;
	gremInfo.fSteps			= ((gMaxDepth == -1) ? gSwitchDepth : min(gSwitchDepth, gMaxDepth));
	gremInfo.fAppList		= gGremlinAppList;

	gTheGremlin.New(gremInfo);

	LogAppendMsg("New Gremlin #%ld started from root state to %ld events",
					gCurrentGremlin, gremInfo.fSteps);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::StartGremlinFromLoadedSuspendedState
 *
 * DESCRIPTION: After the CPU loads the suspended state during an off-cycle,
 *				it calls this to indicate that Hordes is meant to resume
 *				the current Gremlin, and that the Emulator state is ready
 *				for this.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::StartGremlinFromLoadedSuspendedState()
{
	// We reset the Gremlin to go until the next occurence of the
	// depth-bound, or until gMaxDepth, whichever occurs first.

	long newUntil = gSwitchDepth * (gCurrentDepth + 1);
	if (gMaxDepth != -1)
		newUntil = min(newUntil, gMaxDepth);

	gTheGremlin.SetUntil( newUntil );

	LogAppendMsg("Resuming Gremlin #%ld to #%ld events",
		gCurrentGremlin, newUntil);

	Hordes::TurnOn(true);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::SetGremlinStatePathFromControlFile
 *
 * DESCRIPTION:	Given the file reference to a control file (actually *any*
 *				file in the Gremlins state path), set the Gremlins state
 *				path to the directory that contains this file.
 *
 * PARAMETERS:	controlFile - reference to the rootStateFile.
 *
 * RETURNED:	nothing
 *
 ***********************************************************************/

void
Hordes::SetGremlinStatePathFromControlFile (EmFileRef& controlFile)
{
	gGremlinDir = controlFile.GetParent ();
	gForceNewHordesDirectory = false;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::NextGremlin
 *
 * DESCRIPTION: Selects and runs the next Gremlin in the Horde, if there
 *				are unfinished Gremlins left. Otherwise, just loads
 *				the pre-Horde state and stops.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::NextGremlin()
{
	Hordes::Stop();

	Hordes::SaveSearchProgress();

	// Find the next Gremlin to run.

	long nextGremlin, nextDepth;
	
	// Keep looking until we find a Gremlin in the range which has
	// not halted in error.

	Hordes::ProposeNextGremlin(nextGremlin, nextDepth, gCurrentGremlin, gCurrentDepth);
	
	while (gGremlinHaltedInError[nextGremlin]) 
	{
		// All Gremlins halted in error when we are back at the current
		// Gremlin at the next depth. (We looped around without finding
		// the next Gremlin that didn't halt in error).

		if (nextGremlin == gCurrentGremlin && nextDepth >= gCurrentDepth + 1)
		{
			Hordes::EndHordes();
			return;
		}

		Hordes::ProposeNextGremlin(nextGremlin, nextDepth, nextGremlin, nextDepth);
	}

	// Update our current location in the Gremlin search tree.

	gCurrentGremlin = nextGremlin;
	gCurrentDepth = nextDepth;

	// All the Gremlins have reached gMaxDepth when the depth exceeds the
	// depth necessary to reach gMaxDepth. Special case for
	// gMaxDepth = forever.

	if ( gMaxDepth != -1 &&
		( (gCurrentDepth > gMaxDepth / gSwitchDepth) ||
		  ( (gCurrentDepth == gMaxDepth / gSwitchDepth) && (gMaxDepth % gSwitchDepth == 0) ) ) )
	{
		Hordes::EndHordes();
		return;
	}
	
	// If the current depth is 0, we start at the root state.

	if (gCurrentDepth == 0)
	{
		EmAssert (gSession);
		gSession->ScheduleNextGremlinFromRootState ();
	}

	// Otherwise, we load the suspended state, which is where we begin to
	// resume the Gremlin

	else
	{
		EmAssert (gSession);
		gSession->ScheduleNextGremlinFromSuspendedState ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::ErrorEncountered
 *
 * DESCRIPTION: Called when an error condition has been encountered.
 *				This function saves the current state and starts in
 *				motion the machinery to start the next Gremlin in the
 *				Horde.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::ErrorEncountered()
{
	long errorGremlin = Hordes::GremlinNumber();
	long errorEvent = Hordes::EventCounter() - 1;

	Hordes::AutoSaveState();

	LogAppendMsg ("=== ERROR: Gremlin #%ld terminated in error at event #%ld", gCurrentGremlin, errorEvent);

	LogDump();

	if (!InSingleGremlinMode())
	{
		gGremlinHaltedInError[errorGremlin] = true;
		Hordes::NextGremlin();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::DoDialog
 *
 * DESCRIPTION: Called when an error message needs to be displayed,
 *				either as a result of a hardware exception, and error
 *				condition detected by Poser, or the Palm application
 *				calling SysFatalAlert.  If appropriate, log the error
 *				message and tell the caller how to proceed
 *
 * PARAMETERS:	msg - the error message.
 *
 * RETURNED:	TRUE if the caller should understand that Poser is in
 *				a relatively automated Gremlins mode.  Specifically,
 *				this means that we are currently running a range of
 *				Gremlins.  FALSE if Gremlins is off, or is in single
 *				Gremlin mode.
 *
 *				TRUE if this Gremlin should be halted and we should
 *				move on to the next one.  FALSE if the user should
 *				be presented with an error dialog.
 *
 ***********************************************************************/

Bool
Hordes::DoDialog(const string& msg, int flags)
{
	if (IsOn ())
	{
		unsigned long	currentGremlin = Hordes::GremlinNumber();
		unsigned long	currentStep = Hordes::EventCounter();

		char*	typeStr = "";

		if ((GET_BUTTON (0, flags) & kButtonMask) == kDlgItemContinue)
		{
			typeStr = "WARNING";
			gWarningHappened = true;
		}
		else
		{
			typeStr = "ERROR";
			gErrorHappened = true;
		}

		LogAppendMsg ("=== %s: Gremlin #%lu Event %lu", typeStr, currentGremlin, currentStep - 1);
		LogAppendMsg ("=== %s: ********************************************************************************", typeStr);
		LogAppendMsg ("=== %s: %s", typeStr, msg.c_str ());
		LogAppendMsg ("=== %s: ********************************************************************************", typeStr);

		LogDump ();

		return !InSingleGremlinMode();
	}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::StopEventReached
 *
 * DESCRIPTION: Message to Hordes indicating that a Gremlin has
 *				completed its last event. Saves a suspended state if
 *				we intend to resume this Gremlin in the future.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::StopEventReached()
{
	long gremlinNumber = Hordes::GremlinNumber();
	long stopEventNumber = Hordes::EventLimit();

	LogAppendMsg("Gremlin #%ld finished successfully to event #%ld",
					gremlinNumber, stopEventNumber);

	if (stopEventNumber == gMaxDepth)
	{
//		LogAppendMsg("********************************************************************************");
		LogAppendMsg("*************   Gremlin Gremlin #%ld successfully completed", gremlinNumber);
//		LogAppendMsg("********************************************************************************");
	}
	
	LogDump();

	Hordes::NextGremlin();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Step
 *
 * DESCRIPTION: "Steps" the currently running Gremlin.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::Step()
{
	if (!gIsOn)
	{
		TurnOn(true);
		gTheGremlin.Step();

		// Make sure the app's awake.  Normally, we post events on a patch to
		// SysEvGroupWait.  However, if the Palm device is already waiting,
		// then that trap will never get called.  By calling EvtWakeup now,
		// we'll wake up the Palm device from its nap.

		Errors::ThrowIfPalmError (EvtWakeup ());
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Resume
 *
 * DESCRIPTION: Resumes the currently suspended Gremlin.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::Resume()
{
	if (!Hordes::IsOn())
	{
		Hordes::TurnOn(true);
		
		gStartTime = Platform::GetMilliseconds() - (gStopTime - gStartTime);

		gTheGremlin.Resume();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::Stop
 *
 * DESCRIPTION: Suspends the currently running Gremlin.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::Stop()
{
	gStopTime = Platform::GetMilliseconds();

	StubAppGremlinsOff();

	gTheGremlin.Stop();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::PostLoad
 *
 * DESCRIPTION: Initializes a load that has taken place outside the
 *				control of the Hordes subsystem. For example, if the user
 *				has opened a session file manually. This is to set the
 *				Hordes state to play the Gremlin in the file as a "horde
 *				of one."
 *
 * PARAMETERS:	f - SessionFile to load from
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::PostLoad()
{
	// We can't just call NewGremlin with the GremlinInfo because
	// gTheGremlin.Load() has already restored the state of the Gremlin,
	// so we should not call gTheGremlin.New() on it.

	Preference<GremlinInfo>	pref (kPrefKeyGremlinInfo);
	GremlinInfo info = *pref;

	gGremlinStartNumber		= info.fNumber;
	gGremlinStopNumber		= info.fNumber;
	gCurrentGremlin			= info.fNumber;
	gSwitchDepth			= info.fSteps;
	gMaxDepth				= info.fSteps;
	gGremlinSaveFrequency	= info.fSaveFrequency;
	gGremlinAppList			= info.fAppList;
	gCurrentDepth			= 0;

	gStartTime	= gTheGremlin.GetStartTime();
	gStopTime	= gTheGremlin.GetStopTime();

	if (Hordes::IsOn())
	{
		Hordes::UseNewAutoSaveDirectory ();

		EmAssert (gSession);
		gSession->ScheduleSaveRootState ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::PostFakeEvent
 *
 * DESCRIPTION: Posts a fake event through the currently running Gremlin.
 *
 * PARAMETERS:	none
 *
 * RETURNED:	TRUE if a key or point was enqueued, FALSE otherwise.
 *
 ***********************************************************************/

Bool
Hordes::PostFakeEvent()
{
	// check to see if the Gremlin has produced its max # of "events."

	if (Hordes::EventLimit() > 0 && Hordes::EventCounter() > Hordes::EventLimit())
	{
		Hordes::StopEventReached();
		return false;
	}

	Bool result = gTheGremlin.GetFakeEvent();

	Hordes::BumpCounter();

	return result;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::PostFakePenEvent
 *
 * DESCRIPTION: Posts a phony pen movement to through the currently
 *				running Gremlin
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::PostFakePenEvent()
{
	Hordes::BumpCounter();
	gTheGremlin.GetPenMovement();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::PostFakePenEvent
 *
 * DESCRIPTION: Send a char to the Emulator if any are pending for the
 *				currently running Gremlin
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

Bool
Hordes::SendCharsToType()
{
	return gTheGremlin.SendCharsToType();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::ElapsedMilliseconds
 *
 * DESCRIPTION: Returns the elapsed time of the Horde
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

uint32
Hordes::ElapsedMilliseconds()
{
	if (gIsOn)
		return Platform::GetMilliseconds () - gStartTime;

	return gStopTime - gStartTime;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::BumpCounter
 *
 * DESCRIPTION: Bumps event counter of the currently running Gremlin
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::BumpCounter (void)
{
	gTheGremlin.BumpCounter ();

	if (gGremlinSaveFrequency != 0 &&
		(Hordes::EventCounter () % gGremlinSaveFrequency) == 0)
	{
		EmAssert (gSession);
		gSession->ScheduleAutoSaveState ();
	}

	if (Hordes::EventLimit() > 0 &&
		Hordes::EventLimit() == Hordes::EventCounter())
	{	
		EmAssert (gSession);
		gSession->ScheduleSaveSuspendedState ();
	}
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::CanSwitchToApp
 *
 * DESCRIPTION: Returns whether the Horde can switch to the given Palm
 *				app
 *
 * PARAMETERS:	cardNo   \ Palm application info
 *				dbID     /
 *
 * RETURNED:	TRUE if the designated Palm app can be run by the Horde
 *				FALSE otherwise
 *
 ***********************************************************************/

Bool
Hordes::CanSwitchToApp (UInt16 cardNo, LocalID dbID)
{
	if (gGremlinAppList.size () == 0)
		return true;

	DatabaseInfoList::const_iterator	iter = gGremlinAppList.begin ();
	
	while (iter != gGremlinAppList.end ())
	{
		DatabaseInfo	dbInfo = *iter++;
		
		if (dbInfo.cardNo == cardNo && dbInfo.dbID == dbID)
			return true;
	}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::SetGremlinsHome
 *
 * DESCRIPTION: Indicates the directory in which the user would like
 *				his Horde files to collect.
 *
 * PARAMETERS:	gremlinsHome - the name of the directory
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
Hordes::SetGremlinsHome (const EmDirRef& gremlinsHome)
{
	gHomeForHordesFiles = gremlinsHome;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::SetGremlinsHomeToDefault
 *
 * DESCRIPTION: Indicates that the user would like his Horde files to
 *				collect in the default location.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
Hordes::SetGremlinsHomeToDefault (void)
{
	gHomeForHordesFiles = EmDirRef ();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::GetGremlinsHome
 *
 * DESCRIPTION: Returns the directory that the user would like to house
 *				his Horde state files.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	TRUE if gHomeForHordesFiles is defined;
 *				FALSE otherwise (use default).
 *
 ***********************************************************************/

Bool
Hordes::GetGremlinsHome (EmDirRef& outPath)
{
	// If we don't have anything, default to Poser home.

	outPath = gHomeForHordesFiles;

	return gHomeForHordesFiles.Exists ();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::AutoSaveState
 *
 * DESCRIPTION: Creates a file reference to where the auto-saved state
 *				should be saved.  Then calls a platform-specific
 *				routine to do the actual saving.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
Hordes::AutoSaveState (void)
{
	char	fileName[32];
	int32 number = Hordes::GremlinNumber();
	int32 counter = Hordes::EventCounter();

	// Please note the format codes in the constant definition:
	// first argument = Gremlin number.
	// second argument = Gremlin events elapsed.

	sprintf (fileName, kStrAutoSaveFilename, number, counter);

	EmDirRef	gremlinDir (Hordes::GetGremlinDirectory ());
	EmFileRef	fileRef (gremlinDir, fileName);

	gSession->Save (fileRef, false);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::SaveRootState
 *
 * DESCRIPTION: Creates a file reference to where the state
 *				should be saved.  Then calls a platform-specific
 *				routine to do the actual saving.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
Hordes::SaveRootState (void)
{
	EmDirRef	gremlinDir (Hordes::GetGremlinDirectory ());
	EmFileRef	fileRef (gremlinDir, kStrRootStateFilename);

	Bool hordesWasOn = Hordes::IsOn();

	if (hordesWasOn)
		Hordes::TurnOn(false);

	EmAssert (gSession);
	gSession->Save (fileRef, false);

	Hordes::TurnOn(hordesWasOn);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::LoadState
 *
 * DESCRIPTION: Does the work of loading a state while Hordes is running.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

ErrCode
Hordes::LoadState (const EmFileRef& ref)
{
	ErrCode returnedErrCode = errNone;

	try
	{
		gSession->Load (ref);
	}
	catch (ErrCode errCode)
	{
		Hordes::TurnOn(false);

		Errors::SetParameter ("%filename", ref.GetName ());
		Errors::ReportIfError (kStr_CmdOpen, errCode, 0, true);

		returnedErrCode = errCode;
	}

	return returnedErrCode;
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::LoadRootState
 *
 * DESCRIPTION: Creates a file reference to where the root state
 *				should be loaded.  Then calls a
 *				routine to do the actual loading.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

ErrCode
Hordes::LoadRootState (void)
{
	EmDirRef	gremlinDir (Hordes::GetGremlinDirectory ());
	EmFileRef	fileRef (gremlinDir, kStrRootStateFilename);

	return Hordes::LoadState(fileRef);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::SaveSuspendedState
 *
 * DESCRIPTION: Creates a file reference to where the suspended state
 *				should be saved.  Then calls a platform-specific
 *				routine to do the actual saving.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

void
Hordes::SaveSuspendedState (void)
{
	char	fileName[32];
	int32 number = Hordes::GremlinNumber();

	// Please note the format codes in the constant definition:
	// argument = Gremlin number.
	
	sprintf (fileName, kStrSuspendedStateFilename, number);

	EmDirRef	gremlinDir (Hordes::GetGremlinDirectory ());
	EmFileRef	fileRef (gremlinDir, fileName);

	gSession->Save (fileRef, false);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::LoadSuspendedState
 *
 * DESCRIPTION: Creates a file reference to where the suspended state
 *				should be loaded.  Then calls a	routine to do the actual
 *				loading.
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

ErrCode
Hordes::LoadSuspendedState (void)
{
	char	fileName[32];
	int32 number = Hordes::GremlinNumber();

	// Please note the format codes in the constant definition:
	// argument = Gremlin number.
	
	sprintf (fileName, kStrSuspendedStateFilename, number);

	EmDirRef	gremlinDir (Hordes::GetGremlinDirectory ());
	EmFileRef	fileRef (gremlinDir, fileName);

	return Hordes::LoadState(fileRef);
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::StartLog
 *
 * DESCRIPTION: Starts Hordes logging
 *
 * PARAMETERS:	none
 *
 * RETURNED:	none
 *
 ***********************************************************************/

void
Hordes::StartLog()
{
	LogClear();
	LogStartNew();

	LogAppendMsg("********************************************************************************");
	LogAppendMsg("*************   Gremlin Hordes started");
	LogAppendMsg("********************************************************************************");
	LogAppendMsg("Running Gremlins %ld to %ld", gGremlinStartNumber, gGremlinStopNumber);
	
	
	if (gSwitchDepth != -1)
		LogAppendMsg("Will run each Gremlin %ld events at a time until all Gremlins have terminated in error", gSwitchDepth);
	
	else
		LogAppendMsg("Will run each Gremlin until all Gremlins have terminated in error", gSwitchDepth);

	if (gMaxDepth != -1)
		LogAppendMsg("or have reached a maximum of %ld events", gMaxDepth);

	LogAppendMsg("********************************************************************************");

	LogDump();
}


/***********************************************************************
 *
 * FUNCTION:	Hordes::GetGremlinDirectory
 *
 * DESCRIPTION:	Return an EmDirRef for directory where information
 *				about the current Gremlin is saved.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The desired EmDirRef.
 *
 ***********************************************************************/

EmDirRef
Hordes::GetGremlinDirectory (void)
{
	// If requested, create the directory to use.

	if (gForceNewHordesDirectory)
	{
		gForceNewHordesDirectory = false;

		char	stateName[30];
		sprintf (stateName, "GremlinStates_%ld", Platform::GetMilliseconds ());

		EmDirRef	homeDir;

		if (!Hordes::GetGremlinsHome (homeDir))
		{
			homeDir = EmDirRef::GetEmulatorDirectory ();
		}

		gGremlinDir = EmDirRef (homeDir, stateName);

		if (!gGremlinDir.Exists ())
		{
			try
			{
				gGremlinDir.Create ();
			}
			catch (...)
			{
				// !!! Put up some error message

				gGremlinDir = EmDirRef ();
			}
		}
	}

	// Otherwise, return the previously specified directory.

	return gGremlinDir;
}


// ---------------------------------------------------------------------------
//		¥ Hordes::UseNewAutoSaveDirectory
// ---------------------------------------------------------------------------

void Hordes::UseNewAutoSaveDirectory (void)
{
	gForceNewHordesDirectory = true;
}
