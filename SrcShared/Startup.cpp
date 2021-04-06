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
#include "Startup.h"

#include "Byteswapping.h"		// Canonical
#include "EmPalmStructs.h"		// EmProxyDatabaseHdrType
#include "EmStreamFile.h"		// EmStreamFile
#include "ErrorHandling.h"		// Errors::kOK
#include "Hordes.h"				// Hordes::New
#include "Miscellaneous.h"		// IsExecutable, GetLoadableFileList
#include "Platform.h"			// ForceStartupScreen, QueryNewDocument
#include "PreferenceMgr.h"		// Preference
#include "Strings.r.h"			// kStr_Autoload, etc.

#include <algorithm>			// find()
#include <string>
#include <vector>

// Startup actions.
//static Bool				gAskWhatToDo;
static Bool				gCreateSession;
static Bool				gOpenSession;

// Post-startup actions.
static Bool				gStartNewHorde;
static Bool				gHordeQuitWhenDone;
// Quit actions.
static Bool				gClose;
static Bool				gQuitOnExit;
static Bool				gQuit;

// Action-specific data.
static Configuration	gCfg;				// For CreateSession.
static EmFileRef		gSessionOpenRef;	// For OpenSession.
static HordeInfo		gHorde;				// For StartNewGremlin
static StringList		gHordeApps;			// For StartNewGremlin
static EmFileRef		gSessionCloseRef;	// For CloseSession

	// These are the files listed on the command line.
static string			gAutoRunApp;
static EmFileRefList	gAutoLoadFiles1;
//static EmFileRefList		gAutoRunFiles1;
//static EmFileRefList		gAutoRunAndQuitFiles1;

	// These are the files found in the Auto* directories.
static EmFileRefList	gAutoLoadFiles2;
static EmFileRefList	gAutoRunFiles2;
static EmFileRefList	gAutoRunAndQuitFiles2;


// These are the keys we use when tracking what the user has specified.

static const char		kOptPSF[]				= "psf";
static const char		kOptROM[]				= "rom";
static const char		kOptRAM[]				= "ram";
static const char		kOptDevice[]			= "device";
static const char		kOptSkin[]				= "skin";
static const char		kOptLoad[]				= "load";
static const char		kOptRun[]				= "run";
static const char		kOptQuitOnExit[]		= "quit_on_exit";
static const char		kOptLog[]				= "log_dir";
static const char		kOptHordeFirst[]		= "horde_first";
static const char		kOptHordeLast[]			= "horde_last";
static const char		kOptHordeApps[]			= "horde_apps";
static const char		kOptHordeSaveDir[]		= "horde_save_dir";
static const char		kOptHordeSaveFreq[]		= "horde_save_freq";
static const char		kOptHordeDepthMax[]		= "horde_depth_max";
static const char		kOptHordeDepthSwitch[]	= "horde_depth_switch";
static const char		kOptHordeQuitWhenDone[]	= "horde_quit_when_done";


// These are the options the user can specify on the command line.

static const struct
{
	const char*	option;
	const char*	optKey;
	int			optType;	// 0 == no parameter, 1 = has parameter
}
kOptionMap [] =
{
	{ "-psf",					kOptPSF,				1 },
	{ "-rom",					kOptROM,				1 },
	{ "-ram",					kOptRAM,				1 },
	{ "-ram_size",				kOptRAM,				1 },
	{ "-device",				kOptDevice,				1 },
	{ "-skin",					kOptSkin,				1 },
	{ "-silkscreen",			kOptSkin,				1 },
	{ "-load_apps",				kOptLoad,				1 },
	{ "-run_app",				kOptRun,				1 },
	{ "-quit_on_exit",			kOptQuitOnExit,			0 },
	{ "-log_save_dir",			kOptLog,				1 },
	{ "-horde",					kOptHordeFirst,			1 },
	{ "-horde_first",			kOptHordeFirst,			1 },
	{ "-horde_last",			kOptHordeLast,			1 },
	{ "-horde_apps",			kOptHordeApps,			1 },
	{ "-horde_save_dir",		kOptHordeSaveDir,		1 },
	{ "-horde_save_freq",		kOptHordeSaveFreq,		1 },
	{ "-horde_depth_max",		kOptHordeDepthMax,		1 },
	{ "-horde_depth_switch",	kOptHordeDepthSwitch,	1 },
	{ "-horde_quit_when_done",	kOptHordeQuitWhenDone,	0 }
};


typedef StringStringMap	OptionList;


// Handy macro for helping us find and access options in the OptionList.
// For the option with the base name "name", this macro defines:
//
//		"iter<name>"	An iterator pointing to the named OptionList entry.
//		"have<name>"	A bool that says whether the iterator points to
//						something valid.
//		"opt<name>"		A string reference that refers to the option value
//						if "have<name>" is true.

#define DEFINE_VARS(name)												\
	OptionList::iterator	iter##name	= options.find(kOpt##name);		\
	Bool					have##name	= iter##name != options.end();	\
	string&					opt##name	= iter##name->second


/***********************************************************************
 *
 * FUNCTION:    PrvGetDatabaseInfosFromAppNames
 *
 * DESCRIPTION: Given a list of application names (as determined either
 *				by their 'tAIN' resources or their database names) and
 *				return AppInfos for all applications installed in the
 *				system with those names.
 *
 * PARAMETERS:  names - the StringList with the names of the applications
 *					to search for.  *All* executables are considered,
 *					includes .prc's and .pqa's.
 *
 *				results - the DatabaseInfoList to receive the DatabaseInfos of
 *					the executables with a name that appears in the
 *					"names" collection.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

static void PrvGetDatabaseInfosFromAppNames (const StringList& names, DatabaseInfoList& results)
{
	StringList	namesCopy (names);

	if (!namesCopy.empty())
	{
		// If the string list begins with a "-", then the list is
		// a list of files to be *skipped*.

		Bool				mustBeFound = true;
		string				firstName = namesCopy[0];
		if (firstName[0] == '-')
		{
			mustBeFound = false;
			firstName.erase (firstName.begin ());
			namesCopy[0] = firstName;
		}

		// Get a list of installed applications.

		DatabaseInfoList	dbInfos;
		::GetDatabases (dbInfos, kApplicationsOnly);

		// For each application in the list we just got, see if it was
		// specified in the user's name list.

		DatabaseInfoList::iterator	infoIter = dbInfos.begin();
		while (infoIter != dbInfos.end())
		{
			string						appInfoName(infoIter->name);
			StringList::const_iterator	nameIter = find (namesCopy.begin(), namesCopy.end(), appInfoName);
			Bool						found = nameIter != namesCopy.end();

			// Check to see if one of two things are true:
			//
			//	* The installed application is on the include list.
			//	* The installed application in not on the exclude list.
			//
			// If either case is true, push it onto our "results" list.

			if (found == mustBeFound)
			{
				results.push_back (*infoIter);
			}

			++infoIter;
		}
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvDontUnderstand
 * FUNCTION:    PrvMissingArgument
 * FUNCTION:    PrvInvalidRAMSize
 * FUNCTION:    PrvInvalidSkin
 *
 * DESCRIPTION: Display error messages concerning poor, misunderstood
 *				switches and their parameters.
 *
 * PARAMETERS:  The switch ("-foo") guy who caused the problem.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

static void PrvError (const char* msg)
{
#if PLATFORM_UNIX
	printf ("%s\n", msg);
#else
	EmDlg::DoCommonDialog (msg, kDlgFlags_OK);
#endif
}
static void PrvDontUnderstand (const char* arg)
{
	char	buffer[200];
	sprintf (buffer, "Don't understand the command line parameter \"%s\".", arg);
	::PrvError (buffer);
}

static void PrvMissingArgument (const char* arg)
{
	char	buffer[200];
	sprintf (buffer, "The command line parameter \"%s\" needs to be followed by an "
		"argument (\"No, it doesn't.\" \"Yes, it does.\" \"No, it doesn't...\").", arg);
	::PrvError (buffer);
}

static void PrvInvalidRAMSize (const char* arg)
{
	char	buffer[200];
	sprintf (buffer, "\"%s\" is an invalid RAM size. Specify 128, 256, 512, 1024, "
		"2048, 4096, or 8192.", arg);
	::PrvError (buffer);
}

static void PrvInvalidSkin (const char* argSkin, const char* argDevice)
{
	char	buffer[200];
	sprintf (buffer, "The skin \"%s\" cannot be used with the device \"%s\".", argSkin, argDevice);
	::PrvError (buffer);
}

static void PrvPrintHelp (void)
{
	printf ("Options are:\n");
	printf (" -psf <file>          Session file to open at startup\n");
	printf (" -rom <file>          ROM file to use at startup\n");
	printf (" -ram_size <size>     Amount of RAM (in K) to emulate at startup\n");
	printf (" -ram <size>          Synonym for -ram_size\n");
	printf (" -device <name>       Device type to use at startup\n");
	printf (" -skin <name>         Name of skin to use for the device\n");
	printf (" -silkscreen <name>   Synonym for -skin\n");
	printf (" -load_apps <name(s)> Comma-seperated list of names of .prc files to load at startup\n");
	printf (" -run <name>          Name of file to automatically run at startup\n");
	printf (" -quit_on_exit        Cause Poser to quit after -run application exits\n");
	printf ("\n");

	Platform::PrintHelp ();

	printf ("\n");
	printf ("Specifying the -psf option on the command line causes Poser to attempt\n");
	printf ("to load and run the specified session file at startup.\n");
	printf ("\n");
	printf ("Specifying the -rom, -ram_size, and -device options on the command line\n");
	printf ("causes Poser to create a new session based on those specifications.  If\n");
	printf ("only some of those three options are specified, then Poser will present\n");
	printf ("a New Session dialog with the specified items already filled in.\n");
	printf ("\n");
	printf ("The parameter passed with the -skin (or -silkscreen) option is the name\n");
	printf ("of the silkscreen as defined in the .skin file.\n");
	printf ("\n");
	printf ("The parameter passed with the -load_apps option is a comma-seperated list\n");
	printf ("of file names.  The parameter passed with the -run_app option is the name\n");
	printf ("of the Palm OS application itself, not the name of the file it came from.\n");
	printf ("This is generally the name that appears in the Palm OS Launcher.\n");
}


/***********************************************************************
 *
 * FUNCTION:    PrvCollectOptions
 *
 * DESCRIPTION: Breaks up the command line text into pairs of switches
 *				(the "-foo" part) and their optional parameters (anything
 *				that comes after the "-foo" part).  Switches are
 *				validated, and they and any parameter are added to
 *				the given OptionList.
 *
 * PARAMETERS:  argc, argv - standard C parameters describing the
 *					command line.
 *
 *				options - the OptionList to received the parsed and
 *					validated parameters.
 *
 * RETURNED:    True if everything went swimmingly.  If an error occured
 *				(for instance, an invalid switch was specified or a
 *				switch that needed a parameter didn't have one), the
 *				error is reported and the function returns false.
 *
 ***********************************************************************/

static OptionList* gOptions;

static int PrvParseOneOption (int argc, char** argv, int& argIndex)
{
	const char*	arg = argv[argIndex];

	// For this argument, see if it is a recognized switch.

	for (unsigned jj = 0; jj < countof (kOptionMap); ++jj)
	{
		if (_stricmp (arg, kOptionMap[jj].option) == 0)
		{
			// It's recognized; see if we need to also collect a parameter.

			if (kOptionMap[jj].optType == 0)
			{
				// No parameter, just add the switch to our collection.

				(*gOptions)[kOptionMap[jj].optKey] = "";
				argIndex += 1;
				return 1;
			}
			else if (kOptionMap[jj].optType == 1)
			{
				if ((argIndex + 1) < argc)
				{
					// Add the switch and parameter to our collection.

					(*gOptions)[kOptionMap[jj].optKey] = argv[argIndex + 1];
					argIndex += 2;
					return 2;
				}
				else
				{
					// Needed a parameter, but there wasn't one.
					::PrvMissingArgument (arg);
					return 0;
				}
			}
		}
	}

	::PrvDontUnderstand (arg);

	return 0;
}

static Bool PrvCollectOptions (int argc, char** argv, OptionList& options)
{
	// Iterate over the command line arguments.

	gOptions = &options;

	if (!Platform::CollectOptions (argc, argv, &::PrvParseOneOption))
	{
		::PrvPrintHelp ();
		return false;
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvConvertRAM
 *
 * DESCRIPTION: Convert a -ram parameter into a number and validate it.
 *
 * PARAMETERS:  str - the switch parameter.
 *
 *				ramSize - the RAMSizeType to receive the converted value.
 *
 * RETURNED:    True if everything went well.  If the resulting value
 *				was not a valid RAMSizeType value, then returns false.
 *
 ***********************************************************************/

static Bool PrvConvertRAM(const string& str, RAMSizeType& ramSize)
{
	ramSize = atol (str.c_str ());

	MemoryTextList	sizes;
	::GetMemoryTextList (sizes);

	MemoryTextList::iterator	iter = sizes.begin ();
	while (iter != sizes.end ())
	{
		if (ramSize == iter->first)
		{
			return true;
		}

		++iter;
	}

	return false;
}


/***********************************************************************
 *
 * FUNCTION:    PrvParseFileList
 *
 * DESCRIPTION: Break up a comma-delimited list of files, returning the
 *				pieces in a EmFileRefList.
 *
 * PARAMETERS:  fileList - the EmFileRefList to receive the files from the
 *					comma-delimited list.  This collection is *not* first
 *					cleared out, so it's possible to add to the
 *					collection with this function.
 *
 *				option - the string containing the comma-delimited files.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

static void PrvParseFileList (EmFileRefList& fileList, string option)
{
	StringList	items;
	::SeparateList (items, option, ',');

	StringList::iterator	iter = items.begin ();
	while (iter != items.end ())
	{
		fileList.push_back (EmFileRef (*iter));
		++iter;
	}
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleOpenSessionParameters
 *
 * DESCRIPTION: Handle the following command line options:
 *
 *					kOptPSF
 *
 * PARAMETERS:  options - the OptionList containing the complete set
 *					of parsed switches and parameters.
 *
 * RETURNED:    True if everything when OK.  If there's something wrong
 *				with the specifications, this function displays an
 *				error message and return false.
 *
 ***********************************************************************/

static Bool PrvHandleOpenSessionParameters (OptionList& options)
{
	if (::IsBoundFully ())
		return true;

	DEFINE_VARS(PSF);

	if (havePSF)
	{
		// Clear out any previous action

		Startup::ScheduleAskWhatToDo ();

		// Schedule the new action

		EmFileRef	psfRef = EmFileRef (optPSF);
		Startup::ScheduleOpenSession (psfRef);
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleCreateSessionParameters
 *
 * DESCRIPTION: Handle the following command line options:
 *
 *					kOptROM
 *					kOptRAM
 *					kOptDevice
 *
 * PARAMETERS:  options - the OptionList containing the complete set
 *					of parsed switches and parameters.
 *
 * RETURNED:    True if everything when OK.  If there's something wrong
 *				with the specifications, this function displays an
 *				error message and return false.
 *
 ***********************************************************************/

static Bool PrvHandleCreateSessionParameters (OptionList& options)
{
	if (::IsBound ())
		return true;

	DEFINE_VARS(ROM);
	DEFINE_VARS(RAM);
	DEFINE_VARS(Device);

	Bool					haveSome	= haveROM || haveRAM || haveDevice;
	Bool					haveAll		= haveROM && haveRAM && haveDevice;

	if (haveSome)
	{
		Preference<Configuration>	prefConfig (kPrefKeyLastConfiguration);
		Configuration				cfg = *prefConfig;

		// Check for a specified ROM file.

		if (haveROM)
		{
			cfg.fROMFile = EmFileRef (optROM);
		}

		// Check for a specified RAM size.

		if (haveRAM)
		{
			if (!::PrvConvertRAM (optRAM, cfg.fRAMSize))
			{
				::PrvInvalidRAMSize (optRAM.c_str ());
				return false;
			}
		}

		// Check for a specified device type.

		if (haveDevice)
		{
			EmDevice	device (optDevice);
			if (device.Supported ())
			{
				cfg.fDevice = device;
			}
			else
			{
				::PrvDontUnderstand (optDevice.c_str ());
				return false;
			}
		}

		// Try to start up with the specified parameters.
		// If the command line didn't specify all the required values, ask for them.

		if (!haveAll && !Platform::QueryNewDocument (cfg))
		{
			// User cancelled the "New Configuration" dialog.
			// Bring up the dialog with the New/Open/Download/Exit buttons.

			Startup::ScheduleAskWhatToDo ();
		}
		else
		{
			// Clear out any previous action

			Startup::ScheduleAskWhatToDo ();

			// Schedule the new action

			Startup::ScheduleCreateSession (cfg);
		}
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleNewHordeParameters
 *
 * DESCRIPTION: Handle the following command line options:
 *
 *					kOptHordeFirst
 *					kOptHordeLast
 *					kOptHordeApps
 *					kOptHordeSaveDir
 *					kOptHordeSaveFreq
 *					kOptHordeDepthMax
 *					kOptHordeDepthSwitch
 *					kOptHordeQuitWhenDone
 *
 * PARAMETERS:  options - the OptionList containing the complete set
 *					of parsed switches and parameters.
 *
 * RETURNED:    True if everything when OK.  If there's something wrong
 *				with the specifications, this function displays an
 *				error message and return false.
 *
 ***********************************************************************/

static Bool PrvHandleNewHordeParameters (OptionList& options)
{
	DEFINE_VARS(HordeFirst);
	DEFINE_VARS(HordeLast);
	DEFINE_VARS(HordeApps);
	DEFINE_VARS(HordeSaveDir);
	DEFINE_VARS(HordeSaveFreq);
	DEFINE_VARS(HordeDepthMax);
	DEFINE_VARS(HordeDepthSwitch);
	DEFINE_VARS(HordeQuitWhenDone);

	if (haveHordeFirst ||
		haveHordeLast ||
		haveHordeApps ||
		haveHordeSaveDir ||
		haveHordeSaveFreq ||
		haveHordeDepthMax ||
		haveHordeDepthSwitch)
	{
		HordeInfo	info;
		StringList	appNames;

		if (haveHordeFirst)
		{
			info.fStartNumber = atoi (optHordeFirst.c_str ());

			if (!haveHordeLast)
			{
				info.fStopNumber = info.fStartNumber;
			}
		}

		if (haveHordeLast)
		{
			info.fStopNumber = atoi (optHordeLast.c_str ());

			if (!haveHordeFirst)
			{
				info.fStartNumber = info.fStopNumber;
			}
		}

		if (!haveHordeFirst && !haveHordeLast)
		{
			info.fStartNumber = 0;
			info.fStopNumber = 0;
		}

		if (haveHordeApps)
		{
			// Get the list of user-specified names.
			// We can't do much more with them now -- like look up
			// the applications on the device -- as the device may
			// not be up and running at this time.  We have to defer
			// doing that until we're ready to start the Gremlin.

			::SeparateList (appNames, optHordeApps, ',');

		}

		if (haveHordeSaveDir)
		{
			Hordes::SetGremlinsHome (optHordeSaveDir);
		}
		else
		{
			Hordes::SetGremlinsHomeToDefault ();
		}

		if (haveHordeSaveFreq)
		{
			info.fSaveFrequency = atoi (optHordeSaveFreq.c_str ());
		}

		if (haveHordeDepthMax)
		{
			info.fMaxDepth = atoi (optHordeDepthMax.c_str ());
		}

		if (haveHordeDepthSwitch)
		{
			info.fSwitchDepth = atoi (optHordeDepthSwitch.c_str ());
		}

		if (haveHordeFirst || haveHordeLast)
		{
			info.OldToNew ();	// Transfer old fields to new.

			Startup::ScheduleNewHorde (info, appNames);
		}
	}

	gHordeQuitWhenDone = haveHordeQuitWhenDone;

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleAutoLoadParameters
 *
 * DESCRIPTION: Handle the following command line options:
 *
 *					kOptLoad
 *					kOptRun
 *					kOptQuitOnExit
 *
 * PARAMETERS:  options - the OptionList containing the complete set
 *					of parsed switches and parameters.
 *
 * RETURNED:    True if everything when OK.  If there's something wrong
 *				with the specifications, this function displays an
 *				error message and return false.
 *
 ***********************************************************************/

static Bool PrvHandleAutoLoadParameters (OptionList& options)
{
	DEFINE_VARS(Load);
	DEFINE_VARS(Run);
	DEFINE_VARS(QuitOnExit);

	UNUSED_PARAM (optQuitOnExit)

	if (haveLoad)
	{
		::PrvParseFileList (gAutoLoadFiles1, optLoad);
	}

	if (haveRun)
	{
		gAutoRunApp = optRun;
	}

	if (haveQuitOnExit)
	{
		Startup::ScheduleQuitOnExit ();
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvHandleSkinParameters
 *
 * DESCRIPTION: Handle the following command line options:
 *
 *					kOptSkin
 *					kOptSilkscreen (synonym)
 *
 * PARAMETERS:  options - the OptionList containing the complete set
 *					of parsed switches and parameters.
 *
 * RETURNED:    True if everything when OK.  If there's something wrong
 *				with the specifications, this function displays an
 *				error message and return false.
 *
 ***********************************************************************/

static Bool PrvHandleSkinParameters (OptionList& options)
{
	DEFINE_VARS(Device);
	DEFINE_VARS(Skin);

	if (haveSkin && haveDevice)
	{
		EmDevice	device (optDevice);

		if (!device.Supported ())
		{
			::PrvDontUnderstand (optDevice.c_str ());
			return false;
		}

		if (!::SkinValidSkin (device, optSkin))
		{
			::PrvInvalidSkin (optSkin.c_str (), optDevice.c_str ());
			return false;
		}

		::SkinSetSkinName (device, optSkin);
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    PrvParseCommandLine
 *
 * DESCRIPTION: Parse up the command line into its consituent parts,
 *				validate the parts, and act on the specifications.
 *
 * PARAMETERS:  argc, argv - standard C parameters describing the
 *					command line.
 *
 * RETURNED:    Nothing.  All actions to be taken based on the users
 *				specifications are recorded in global variables.
 *
 ***********************************************************************/

static void PrvParseCommandLine (int argc, char** argv)
{
	OptionList		options;

	// Convert the command line into a map of switch/parameter pairs.

	if (!::PrvCollectOptions (argc, argv, options))
		goto BadParameter;

	// Handle kOptPSF.

	if (!::PrvHandleOpenSessionParameters (options))
		goto BadParameter;

	// Handle kOptROM, kOptRAM, and kOptDevice.

	if (!::PrvHandleCreateSessionParameters (options))
		goto BadParameter;

	// Handle kOptHordeFirst, kOptHordeLast, kOptHordeApps, kOptHordeSaveDir,
	// kOptHordeSaveFreq, kOptHordeDepthMax, kOptHordeDepthSwitch, and
	// kOptHordeQuitWhenDone.

	if (!::PrvHandleNewHordeParameters (options))
		goto BadParameter;

	// Handle kOptLoad, kOptRun, and kOptQuitOnExit.

	if (!::PrvHandleAutoLoadParameters (options))
		goto BadParameter;

	// Handle kOptSkin
	if (!::PrvHandleSkinParameters (options))
		goto BadParameter;

	// Handle kOptLog
	// !!! TBD

	return;

BadParameter:
	// All bets are off.  Bring up the dialog with the
	// New/Open/Download/Exit buttons.

	Startup::ScheduleAskWhatToDo ();
	return;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::DetermineStartupActions
 *
 * DESCRIPTION: Determine what to do at startup time, based on any
 *				command-line options, what the user was doing when they
 *				last ran Poser, and whether or not the CapsLock key
 *				is toggled.
 *
 *				In general, the startup rules are as follows:
 *
 *			 	1 If the Caps Lock key is toggled in the ON position, always bring
 *				  up the New/Open/... dialog.
 *
 *				2 Scan the command line for startup parameters.  If an error occurs
 *				  trying to scan the command line, the error is reported and the user
 *				  is presented with the New/Open/... dialog.
 *
 *				3 Use the .psf file if one is specified.  If an error occurs trying
 *				  to load the file, the error is reported and the user is presented
 *				  with the New/Open/... dialog.
 *
 *				4 If any of -rom, -ram, -device, or -silkscreen are specified, try
 *				  to start a new session based on those values.  If all are specified,
 *				  the new session is automatically created.  If any of those four
 *				  values are missing, the "New Configuration" dialog is displayed.
 *				  If the user cancels the dialog, or if there is an error creating
 *				  the new session, any error is reported and the user is presented
 *				  with the New/Open/... dialog.
 *
 *				5 If no command line options are specified, try re-opening the last
 *				  saved .psf file (this step is skipped if the user last created a
 *				  new session, but did NOT save that session to a file).  If an error
 *				  occurs trying to load the file, the error is reported and the user
 *				  is presented with the New/Open/... dialog.
 *
 *				6 Try creating a new session based on the settings the user last
 *				  specified when creating a session.  If there is an error creating
 *				  the new session, the error is reported and the user is presented
 *				  with the New/Open/... dialog.
 *
 *				7 Finally, if all else fails, present the user with the New/Open/...
 *				  dialog.
 *
 * PARAMETERS:  argc, argv - standard C parameters describing the
 *					command line.
 *
 * RETURNED:    True if we were able to determine a startup course of
 *				action.  If false is returned, something horrible happened.
 *				The caller should assume that an error was reported,
 *				and that the application should quit.
 *
 ***********************************************************************/

Bool Startup::DetermineStartupActions (int argc, char** argv)
{
	// By default, throw up our hands.

	Startup::ScheduleAskWhatToDo ();

	// See if the user wants to force the appearance of the Startup
	// screen (by holding down the ShiftLock key) and skip the auto-
	// loading of the previous session file.

	if (::IsBound () || !Platform::ForceStartupScreen ())
	{
		Preference<Configuration>	pref1 (kPrefKeyLastConfiguration);
		Preference<EmFileRef>		pref2 (kPrefKeyLastPSF);

		Configuration	cfg = *pref1;
		EmFileRef		ramFileRef = *pref2;

		// See if there is embedded PSF resource. If so, use it.

		if (::IsBoundFully ())
		{			
			Startup::ScheduleOpenSession (ramFileRef);	// Ref ignored...
		}
		
		// Else, see if there is an embedded ROM. If so, we will be creating a
		// new document based on it.

		else if (::IsBoundPartially ())
		{
			// Get the configuration settings from the embedded configuration.
			// If we can get them, create a new document based on them.

			cfg.fDevice		= Platform::GetBoundDevice ();
			cfg.fRAMSize	= Platform::GetBoundRAMSize ();

			Startup::ScheduleCreateSession (cfg);	// ROM image comes from resource...
		}

		// Else, see if there was a previously saved RAM file.	If so, open it.

		else if (ramFileRef.IsSpecified ())
		{
			Startup::ScheduleOpenSession (ramFileRef);
		}


		// Else, see if there was a previously created document.  If so,
		// create a new document based on its settings.

		else if (cfg.fDevice.Supported () &&
				cfg.fRAMSize != 0 &&
				cfg.fROMFile.IsSpecified () )
		{
			Startup::ScheduleCreateSession (cfg);
		}

		// Now that default actions have been established, let's see if
		// there's anything interesting on the command line.

		::PrvParseCommandLine (argc, argv);
	}

	return true;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::GetAutoLoads
 *
 * DESCRIPTION: Return the list of applications that should automatically
 *				be loaded at startup time, regardless of whether a new
 *				session was created or an old one loaded.  The list of
 *				applications is collected from the command line and from
 *				the specially-named AutoLoad, AutoRun, and AutoRunAndQuit
 *				directories.
 *
 * PARAMETERS:  fileList - the EmFileRefList to receive the files to load.
 *					The collection is cleared out before the new items
 *					are added to it.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

static void PrvLookForAutoloads (void)
{
	gAutoLoadFiles2.clear();
	gAutoRunFiles2.clear();
	gAutoRunAndQuitFiles2.clear();

	::GetLoadableFileList (Platform::GetString (kStr_Autoload), gAutoLoadFiles2);
	::GetLoadableFileList (Platform::GetString (kStr_Autorun), gAutoRunFiles2);
	::GetLoadableFileList (Platform::GetString (kStr_AutorunAndQuit), gAutoRunAndQuitFiles2);
}

static void PrvAppendFiles (EmFileRefList& list1, const EmFileRefList& list2)
{
	list1.insert(list1.end(), list2.begin(), list2.end());
}

void Startup::GetAutoLoads (EmFileRefList& fileList)
{
	fileList.clear();

	::PrvLookForAutoloads();

	::PrvAppendFiles (fileList, gAutoLoadFiles1);
//	::PrvAppendFiles (fileList, gAutoRunFiles1);
//	::PrvAppendFiles (fileList, gAutoRunAndQuitFiles1);

	::PrvAppendFiles (fileList, gAutoLoadFiles2);
	::PrvAppendFiles (fileList, gAutoRunFiles2);
	::PrvAppendFiles (fileList, gAutoRunAndQuitFiles2);

	if (/*gAutoRunAndQuitFiles1.size() +*/ gAutoRunAndQuitFiles2.size() > 0)
	{
		Startup::ScheduleQuitOnExit ();
	}
}


/***********************************************************************
 *
 * FUNCTION:    Startup::GetAutoRunApp
 *
 * DESCRIPTION: Returns the *database name* of the application to
 *				switch to at startup time.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    The name of the database of the executable.
 *
 ***********************************************************************/

static string PrvTryGetApp (const EmFileRefList& fileList)
{
	string	result;

	EmFileRefList::const_iterator	iter = fileList.begin();
	while (iter != fileList.end())
	{
		if ((*iter).IsType (kFileTypePalmApp))	// egcs can't handle iter->IsType()!!!
		{
			try
			{
				EmStreamFile	stream (*iter, kOpenExistingForRead);

				EmProxyDatabaseHdrType	header;
				stream.GetBytes (header.GetPtr (), header.GetSize ());

				if (::IsExecutable (header.type, header.creator, header.attributes))
				{
					result = (char*) header.name.GetPtr ();
					break;
				}
			}
			catch (...)
			{
			}
		}

		++iter;
	}

	return result;
}

string Startup::GetAutoRunApp (void)
{
	string	result;

	// Get the name the user specified on the command line (if any)
	// and get the DatabaseInfo for it, so that we can get the database name.

	if (!gAutoRunApp.empty())
	{
		StringList	appNames;
		appNames.push_back (gAutoRunApp);

		DatabaseInfoList	dbInfos;
		::PrvGetDatabaseInfosFromAppNames (appNames, dbInfos);

		if (dbInfos.size() > 0)
		{
			result = dbInfos.begin()->dbName;
		}
	}

	// If the user didn't specify an executable, or we couldn't find
	// that executable, then work from the files found in the AutoRun
	// and AutoRunAndQuit directories.

//	if (result.empty())
//		result = ::PrvTryGetApp (gAutoRunFiles1);

	if (result.empty())
		result = ::PrvTryGetApp (gAutoRunFiles2);

//	if (result.empty())
//		result = ::PrvTryGetApp (gAutoRunAndQuitFiles1);

	if (result.empty())
		result = ::PrvTryGetApp (gAutoRunAndQuitFiles2);

	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::AskWhatToDo
 *
 * DESCRIPTION: Return whether or not we are supposed to ask the user
 *				what to do (that is, display the dialog box with the
 *				New, Open, Download, and Quit buttons).  In general,
 *				this function returns true if there's nothing else
 *				scheduled to be done (create a session, open a session,
 *				or quit the emulator).
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::AskWhatToDo (void)
{
//	Bool	result = gAskWhatToDo;
//	gAskWhatToDo = false;
//	return result;

	return !gCreateSession && !gOpenSession && !gQuit && !gClose;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::CreateSession
 *
 * DESCRIPTION: Return whether or not we are supposed to create a new
 *				session based on previously determined criteria.  This
 *				is a one-shot function: subsequent calls will return
 *				false until ScheduleCreateSession is called again.
 *
 * PARAMETERS:  cfg - the Configuration to receive the information to
 *					be used when creating the new session.
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::CreateSession (Configuration& cfg)
{
	Bool	result = gCreateSession;
	if (result)
		cfg = gCfg;
	gCreateSession = false;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::OpenSession
 *
 * DESCRIPTION: Return whether or not we are supposed to open an old
 *				session based on previously determined criteria.  This
 *				is a one-shot function: subsequent calls will return
 *				false until ScheduleOpenSession is called again.
 *
 * PARAMETERS:  ref - the EmFileRef to receive the reference to
 *					the .psf file to be opened.
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::OpenSession (EmFileRef& ref)
{
	Bool	result = gOpenSession;
	if (result)
		ref = gSessionOpenRef;
	gOpenSession = false;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::NewHorde
 *
 * DESCRIPTION: Return whether or not we are supposed to start a horde
 *				session based on previously determined criteria.  This
 *				is a one-shot function: subsequent calls will return
 *				false until ScheduleNewHorde is called again.
 *
 * PARAMETERS:  info - the HordeInfo to receive the information used to
 *					create the new horde.
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::NewHorde (HordeInfo* info)
{
	Bool	result = gStartNewHorde;

	if (info != NULL)
	{
		if (result)
		{
			*info = gHorde;

			// Find the AppInfos for the user-specified applications.

			::PrvGetDatabaseInfosFromAppNames (gHordeApps, info->fAppList);
		}

		gStartNewHorde = false;
	}

	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::HordeQuitWhenDone
 *
 * DESCRIPTION: Return whether or not we are supposed to quit the
 *				emulator session when the Horde has completed.
 *
 * PARAMETERS:  none.
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::HordeQuitWhenDone (void)
{
	return gHordeQuitWhenDone;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::CloseSession
 *
 * DESCRIPTION: Return whether or not Poser is supposed to close the
 *				current session right now.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::CloseSession (EmFileRef& f)
{
	Bool	result = gClose;
	f = gSessionCloseRef;
	gClose = false;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::QuitOnExit
 *
 * DESCRIPTION: Return whether or not Poser is supposed to quit when
 *				a particular application exits.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::QuitOnExit (void)
{
	Bool	result = gQuitOnExit;
//	gQuitOnExit = false;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::Quit
 *
 * DESCRIPTION: Return whether or not it's time to quit.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool Startup::Quit (void)
{
	Bool	result = gQuit;
	gQuit = false;
	return result;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::Clear
 *
 * DESCRIPTION: Clear all settings.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::Clear (void)
{
	gCreateSession = false;
	gOpenSession = false;
	gStartNewHorde = false;
	gQuitOnExit = false;
	gQuit = false;

	gAutoRunApp = "";
	gAutoLoadFiles1.clear();
	gAutoLoadFiles2.clear();
	gAutoRunFiles2.clear();
	gAutoRunAndQuitFiles2.clear();
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleAskWhatToDo
 *
 * DESCRIPTION: Schedule our "state machine" so that AskWhatToDo will
 *				return True.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleAskWhatToDo (void)
{
	Startup::Clear ();
//	gAskWhatToDo = true;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleCreateSession
 *
 * DESCRIPTION: Schedule our "state machine" so that CreateSession will
 *				return True.
 *
 * PARAMETERS:  cfg - the Configuration returned to the caller of
 *					CreatSession.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleCreateSession (const Configuration& cfg)
{
	gCreateSession = true;
//	gAskWhatToDo = false;
	gCfg = cfg;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleOpenSession
 *
 * DESCRIPTION: Schedule our "state machine" so that OpenSession will
 *				return True.
 *
 * PARAMETERS:  ref - the EmFileRef returned to the caller of
 *					OpenSession.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleOpenSession (const EmFileRef& ref)
{
	gOpenSession = true;
//	gAskWhatToDo = false;
	gSessionOpenRef = ref;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleNewHorde
 *
 * DESCRIPTION: Schedule our "state machine" so that NewHorde will
 *				return True.
 *
 * PARAMETERS:  info - the HordeInfo returned to the caller of NewHorde.
 *
 *				appNames - names of the applications to inflict the
 *					Gremlins on.  Normally, this information is
 *					specified in the fAppList field of the HordeInfo.
 *					However, at the time this function is called,
 *					we may not be able to generate the DatabaseInfoList
 *					(the emulator may be just starting up and not
 *					emulating a Palm OS environment, yet).  So we
 *					remember the names here.  When NewHorde is called,
 *					the appNames list is converted into an DatabaseInfoList.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleNewHorde (const HordeInfo& info, const StringList& appNames)
{
	gStartNewHorde = true;
	gHorde = info;
	gHordeApps = appNames;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleCloseSession
 *
 * DESCRIPTION: Schedule our "state machine" so that CloseSession will
 *				return True.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleCloseSession (const EmFileRef& f)
{
	gSessionCloseRef = f;
	gClose = true;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleQuitOnExit
 *
 * DESCRIPTION: Schedule our "state machine" so that QuitOnExit will
 *				return True.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleQuitOnExit (void)
{
	gQuitOnExit = true;
}


/***********************************************************************
 *
 * FUNCTION:    Startup::ScheduleQuit
 *
 * DESCRIPTION: Schedule our "state machine" so that Quit will
 *				return True.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void Startup::ScheduleQuit (void)
{
//	Startup::Clear();
	gQuit = true;
//	gAskWhatToDo = false;
}


