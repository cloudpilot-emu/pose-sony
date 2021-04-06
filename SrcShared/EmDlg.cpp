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
#include "EmDlg.h"

#include "DebugMgr.h"			// gDebuggerGlobals
#include "EmBankDRAM.h"			// EmBankDRAM::ValidAddress
#include "EmBankROM.h"			// EmBankROM::ValidAddress
#include "EmBankSRAM.h"			// EmBankSRAM::ValidAddress
#include "EmFileImport.h"		// EmFileImport
#include "EmROMTransfer.h"		// EmROMTransfer
#include "EmROMReader.h"		// EmROMReader
#include "EmSession.h"			// gSession
#include "EmStreamFile.h"		// EmStreamFile
#include "EmTransportSerial.h"	// PortNameList, GetSerialPortNameList, etc.
#include "EmTransportSocket.h"	// PortNameList, GetPortNameList, etc.
#include "EmTransportUSB.h"		// EmTransportUSB
#include "ErrorHandling.h"		// Errors::SetParameter
#include "Hordes.h"				// Hordes::New
#include "LoadApplication.h"	// SavePalmFile
#include "Logging.h"			// FOR_EACH_LOG_PREF
#include "Miscellaneous.h"		// MemoryTextList, GetMemoryTextList
#include "Platform.h"			// Platform
#include "PreferenceMgr.h"		// Preference, gEmuPrefs
#include "ROMStubs.h"			// FSCustomControl, SysLibFind
#include "Strings.r.h"			// kStr_AppName
#include "Skins.h"				// SkinNameList
#include "TracerPlatform.h"		// gTracer

#include "algorithm"			// find, remove_if, unary_function<>

#ifdef SONY_ROM
#include "SonyShared\MiscellaneousSony.h"		// MemoryTextList, GetMSSizeTextList
#endif

static void PrvConvertBaudListToStrings (const EmTransportSerial::BaudList& baudList,
									StringList& baudStrList);
static void	PrvFilterMemorySizes (MemoryTextList& sizes, const Configuration& cfg);

/***********************************************************************
 *
 * FUNCTION:	DoGetFile
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgItemID EmDlg::DoGetFile (	EmFileRef& result,
								const string& prompt,
								const EmDirRef& defaultPath,
								const EmFileTypeList& filterList)
{
	return EmDlg::HostRunGetFile (result, prompt, defaultPath, filterList);
}


/***********************************************************************
 *
 * FUNCTION:	DoGetFileList
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgItemID EmDlg::DoGetFileList (	EmFileRefList& results,
									const string& prompt,
									const EmDirRef& defaultPath,
									const EmFileTypeList& filterList)
{
	return EmDlg::HostRunGetFileList (results, prompt, defaultPath, filterList);
}


/***********************************************************************
 *
 * FUNCTION:	DoPutFile
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgItemID EmDlg::DoPutFile (	EmFileRef& result,
								const string& prompt,
								const EmDirRef& defaultPath,
								const EmFileTypeList& filterList,
								const string& defaultName)
{
	return EmDlg::HostRunPutFile (result, prompt, defaultPath, filterList, defaultName);
}


/***********************************************************************
 *
 * FUNCTION:	DoGetDirectory
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgItemID EmDlg::DoGetDirectory (	EmDirRef& result,
									const string& prompt,
									const EmDirRef& defaultPath)
{
	return EmDlg::HostRunGetDirectory (result, prompt, defaultPath);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoAboutBox
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgFnResult EmDlg::PrvAboutBox (EmDlgContext& context)
{
	EmDlgRef	dlg = context.fDlg;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			if (context.fPanelID == kDlgPanelAbtMain)
			{
				Errors::SetParameter ("%application", Platform::GetString (kStr_AppName));
				Errors::SetParameter ("%version", Platform::GetShortVersionString ());

				string	appAndVersion = Errors::ReplaceParameters (kStr_AppAndVers);

				EmDlg::SetItemText (dlg, kDlgItemAbtAppName, appAndVersion);
			}
			
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}
				
				case kDlgItemAbtURLPalmWeb:
				case kDlgItemAbtURLPalmMail:
				case kDlgItemAbtURLWindowsWeb:
				case kDlgItemAbtURLWindowsMail:
				case kDlgItemAbtURLMacWeb:
				case kDlgItemAbtURLMacMail:
				case kDlgItemAbtURLUAEWeb:
				case kDlgItemAbtURLUAEMail:
				case kDlgItemAbtURLQNXWeb:
				case kDlgItemAbtURLQNXMail:
				{
					string	URL = EmDlg::GetItemText (dlg, context.fItemID);
#if 0
					::LaunchURL (URL);
#endif
					break;
				}
				
				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoAboutBox (void)
{
	return EmDlg::RunDialog (EmDlg::PrvAboutBox, NULL, kDlgAboutBox);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoSessionNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct SessionNewData
{
	Configuration*			cfg;
	Configuration			fWorkingCfg;
	EmDeviceList			fDevices;
};


EmDlgFnResult EmDlg::PrvSessionNew (EmDlgContext& context)
{
	SessionNewData*	data = (SessionNewData*) context.fUserData;
	EmDlgRef		dlg = context.fDlg;
	Configuration&	cfg = data->fWorkingCfg;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Make a copy of the configuration that we update as the
			// user makes dialog selections.

			cfg = *(data->cfg);

			// If a file is specified, make sure it is in the MRU list
			if (cfg.fROMFile.IsSpecified())
				gEmuPrefs->UpdateROMMRU (cfg.fROMFile);

			// Build our menus.

			EmDlg::PrvBuildROMMenu (context);
			EmDlg::PrvBuildDeviceMenu (context);
			EmDlg::PrvBuildSkinMenu (context);
			EmDlg::PrvBuildMemorySizeMenu (context);
			EmDlg::PrvSetROMName (context);
#ifdef SONY_ROM
			EmDlg::PrvBuildMSSizeMenu (context);
#endif
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					*(data->cfg) = cfg;

					SkinNameList	skins;
					::SkinGetSkinNames (cfg.fDevice, skins);
					long	menuID = EmDlg::GetItemValue (dlg, kDlgItemNewSkin);

					::SkinSetSkinName (cfg.fDevice, skins[menuID]);

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemNewDevice:
				{
					EmDeviceList::size_type	index	= EmDlg::GetItemValue (dlg, kDlgItemNewDevice);

					cfg.fDevice = data->fDevices [index];

					EmDlg::PrvBuildSkinMenu (context);
					EmDlg::PrvBuildMemorySizeMenu (context);
					break;
				}

				case kDlgItemNewSkin:
				{
					break;
				}

				case kDlgItemNewMemorySize:
				{
					MemoryTextList	sizes;
					::GetMemoryTextList (sizes);
					::PrvFilterMemorySizes (sizes, cfg);

					MemoryTextList::size_type	index = EmDlg::GetItemValue (dlg, kDlgItemNewMemorySize);

					cfg.fRAMSize = sizes [index].first;
					break;
				}

#ifdef SONY_ROM
				case kDlgItemNewMSSize:
				{
					MemoryTextList	sizes;
					::GetMSSizeTextList (sizes);

					MemoryTextList::size_type	index = EmDlg::GetItemValue (dlg, kDlgItemNewMSSize);

					cfg.fMSSize = sizes [index].first;
					break;
				}
#endif // SONY_ROM

				case kDlgItemNewROMBrowse:
					break;

				case kDlgItemNewROM:
				{
					EmFileTypeList	typeList;
					typeList.push_back (kFileTypeROM);

					EmFileRef	romFileRef;
					EmDirRef	romDirRef (cfg.fROMFile.GetParent ());

					int		index = EmDlg::GetItemValue (dlg, kDlgItemNewROM);

					if (index >= 2) 
					{
						EmFileRef	fileRef = gEmuPrefs->GetIndROMMRU (index - 2);

						cfg.fROMFile = fileRef;
					}
					else if (EmDlg::DoGetFile (romFileRef, "Choose ROM file:",
										romDirRef, typeList) == kDlgItemOK)
					{
						EmDeviceList devices = EmDevice::GetDeviceList ();
						EmDeviceList::iterator	devices_end;

						devices_end = PrvFilterDeviceList(romFileRef, devices);

						if (devices_end == devices.begin ())
						{
							// No devices matched

							Errors::DoDialog (kStr_UnknownDevice, kDlgFlags_OK);
						}

						cfg.fROMFile = romFileRef;
					}

					if (cfg.fROMFile.IsSpecified ())
						gEmuPrefs->UpdateROMMRU (cfg.fROMFile);

					EmDlg::PrvSetROMName (context);

					EmDlg::PrvBuildROMMenu (context);
					EmDlg::PrvBuildDeviceMenu (context);
					EmDlg::PrvBuildSkinMenu (context);
					EmDlg::PrvBuildMemorySizeMenu (context);
#ifdef SONY_ROM
//					EmDlg::PrvBuildMemorySizeMenu (context);
					EmDlg::PrvBuildMSSizeMenu (context);
#endif

					break;
				}

				default:
					EmAssert (false);
			}	// switch (context.fItemID)

			break;
		}	// case kDlgCmdItemSelected

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}	// switch (command)

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoSessionNew (Configuration& cfg)
{
	SessionNewData	data;
	data.cfg = &cfg;

	return EmDlg::RunDialog (EmDlg::PrvSessionNew, &data, kDlgSessionNew);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoSessionSave
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgItemID	EmDlg::DoSessionSave (const string& docName, Bool quitting)
{
	string	appName = Platform::GetString (kStr_AppName);
	string	docName2 (docName);

	if (docName2.empty ())
		docName2 = Platform::GetString (kStr_Untitled);

	return EmDlg::HostRunSessionSave (appName, docName2, quitting);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoHordeNew
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct HordeNewData
{
	HordeInfo		info;
	EmDatabaseList	appList;
};


EmDlgFnResult EmDlg::PrvHordeNew (EmDlgContext& context)
{
	HordeNewData*	data = (HordeNewData*) context.fUserData;
	EmDlgRef		dlg = context.fDlg;
	HordeInfo&		info = data->info;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Get Horde preference settings.

			Preference<HordeInfo>	pref (kPrefKeyHordeInfo);
			info = *pref;

			// Install the settings into the dialog box items.

			EmDlg::SetItemValue (dlg, kDlgItemHrdStart,			info.fStartNumber);
			EmDlg::SetItemValue (dlg, kDlgItemHrdStop,			info.fStopNumber);
			EmDlg::SetItemValue (dlg, kDlgItemHrdDepthSwitch,	info.fDepthSwitch);
			EmDlg::SetItemValue (dlg, kDlgItemHrdDepthSave,		info.fDepthSave);
			EmDlg::SetItemValue (dlg, kDlgItemHrdDepthStop,		info.fDepthStop);
			EmDlg::SetItemValue (dlg, kDlgItemHrdCheckSwitch,	info.fCanSwitch);
			EmDlg::SetItemValue (dlg, kDlgItemHrdCheckSave,		info.fCanSave);
			EmDlg::SetItemValue (dlg, kDlgItemHrdCheckStop,		info.fCanStop);

			// Get the list of applications.

			::GetDatabases (data->appList, kApplicationsOnly);

			// Insert the names of the applications into the list.

			Bool	selected = false;
			EmDatabaseList::iterator	iter = data->appList.begin();
			while (iter != data->appList.end())
			{
				DatabaseInfo&	thisDB = *iter;

				// Add the current item.

				EmDlg::AppendToList (dlg, kDlgItemHrdAppList, thisDB.name);

				// If the item we just added is in our "selected" list,
				// then select it.

				EmDatabaseList::iterator	begin = info.fAppList.begin ();
				EmDatabaseList::iterator	end = info.fAppList.end ();

				if (find (begin, end, thisDB) != end)
				{
					selected = true;
					int index = iter - data->appList.begin();
					EmDlg::SelectListItem (dlg, kDlgItemHrdAppList, index);
				}

				++iter;
			}

			// If nothing's selected, then select something (the first item).

			if (!selected)
			{
				EmDlg::SelectListItem (dlg, kDlgItemHrdAppList, 0);
			}

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					info.fStartNumber	= EmDlg::GetItemValue (dlg, kDlgItemHrdStart);
					info.fStopNumber	= EmDlg::GetItemValue (dlg, kDlgItemHrdStop);
					info.fDepthSwitch	= EmDlg::GetItemValue (dlg, kDlgItemHrdDepthSwitch);
					info.fDepthSave		= EmDlg::GetItemValue (dlg, kDlgItemHrdDepthSave);
					info.fDepthStop		= EmDlg::GetItemValue (dlg, kDlgItemHrdDepthStop);
					info.fCanSwitch		= EmDlg::GetItemValue (dlg, kDlgItemHrdCheckSwitch);
					info.fCanSave		= EmDlg::GetItemValue (dlg, kDlgItemHrdCheckSave);
					info.fCanStop		= EmDlg::GetItemValue (dlg, kDlgItemHrdCheckStop);

					// Get the indexes for the selected items.

					EmDlgListIndexList	items;
					EmDlg::GetSelectedItems (dlg, kDlgItemHrdAppList, items);

					// Use the indexes to get the actual items, and add those
					// items to the "result" container.

					info.fAppList.clear ();
					EmDlgListIndexList::iterator	iter = items.begin();
					while (iter != items.end())
					{
						DatabaseInfo	dbInfo = data->appList[*iter];
						info.fAppList.push_back (dbInfo);
						++iter;
					}

					// Transfer the new fields to the old fields for Hordes to use.

					info.NewToOld ();

					// Save the preferences

					Preference<HordeInfo>	pref (kPrefKeyHordeInfo);
					pref = info;

					// Startup up the gremlin sub-system.

					Hordes::New (info);

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemHrdAppList:
				case kDlgItemHrdStart:
				case kDlgItemHrdStop:
				case kDlgItemHrdCheckSwitch:
				case kDlgItemHrdCheckSave:
				case kDlgItemHrdCheckStop:
				case kDlgItemHrdDepthSwitch:
				case kDlgItemHrdDepthSave:
				case kDlgItemHrdDepthStop:
					break;

				case kDlgItemHrdLogging:
					EmDlg::DoEditLoggingOptions (kGremlinLogging);
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoHordeNew (void)
{
	HordeNewData	data;

	return EmDlg::RunDialog (EmDlg::PrvHordeNew, &data, kDlgHordeNew);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoDatabaseImport
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct DatabaseImportData
{
	EmFileRefList		fFiles;
	EmFileImportMethod	fMethod;
	EmStreamFile*		fStream;
	EmFileImport*		fImporter;
	long				fProgressCurrent;	// Progress into the current file
	long				fProgressBase;		// Progress accumulated by previous files
	long				fProgressMax;		// Progress max value.
	long				fCurrentFile;		// Current file we're installing.
	long				fDoneStart;
};


EmDlgFnResult EmDlg::PrvDatabaseImport (EmDlgContext& context)
{
	DatabaseImportData*	data = (DatabaseImportData*) context.fUserData;
	EmDlgRef			dlg = context.fDlg;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			data->fStream		= NULL;
			data->fImporter		= NULL;

			try
			{
				data->fCurrentFile		= 0;
				data->fStream			= new EmStreamFile (data->fFiles[data->fCurrentFile], kOpenExistingForRead);
				data->fImporter			= new EmFileImport (*data->fStream, data->fMethod);
				data->fProgressCurrent	= -1;
				data->fProgressBase		= 0;
				data->fProgressMax		= EmFileImport::CalculateProgressMax (data->fFiles, data->fMethod);
				data->fDoneStart		= 0;

				long	remainingFiles	= data->fFiles.size () - data->fCurrentFile;
				string	curAppName		= data->fFiles[data->fCurrentFile].GetName ();

				EmDlg::SetItemValue	(dlg, kDlgItemImpNumFiles,		remainingFiles);
//				EmDlg::SetItemText	(dlg, kDlgItemImpCurAppName,	curAppName);
			}
			catch (...)
			{
				delete data->fImporter;
				delete data->fStream;

				data->fStream		= NULL;
				data->fImporter		= NULL;
			}

			context.fNeedsIdle = true;
			break;
		}

		case kDlgCmdIdle:
		{
			if (data->fImporter)
			{
				// Prevent against recursion (can occur if we need to display an error dialog).

				static Bool	inIdle;
				if (inIdle)
					break;

				inIdle = true;

				// Install a little bit of the file.

				ErrCode	err = data->fImporter->Continue ();

				// If an error occured, display a dialog.

				if (err)
				{
					string	curAppName		= data->fFiles[data->fCurrentFile].GetName ();
					Errors::SetParameter ("%filename", curAppName);
					Errors::ReportIfError (kStr_CmdInstall, err, 0, false);
				}

				// Everything went OK.  Get the current progress amount.
				// If it changed, update the dialog.
				else
				{
					long	progressCurrent	= data->fImporter->GetProgress ();
					long	progressMax		= data->fProgressMax;

					if (data->fProgressCurrent != progressCurrent)
					{
						data->fProgressCurrent = progressCurrent;

						progressCurrent += data->fProgressBase;

						// Scale progressMax to < 32K (progress control on Windows
						// prefers it that way).

						long	divider = progressMax / (32 * 1024L) + 1;

						progressMax /= divider;
						progressCurrent /= divider;

						long	remainingFiles	= data->fFiles.size () - data->fCurrentFile;
						string	curAppName		= data->fFiles[data->fCurrentFile].GetName ();

						EmDlg::SetItemValue	(dlg, kDlgItemImpNumFiles,		remainingFiles);
//						EmDlg::SetItemText	(dlg, kDlgItemImpCurAppName,	curAppName);
						EmDlg::SetItemMax	(dlg, kDlgItemImpProgress,		progressMax);
						EmDlg::SetItemValue	(dlg, kDlgItemImpProgress,		progressCurrent);
					}
				}

				// If we're done with this file, clean up, and prepare for any
				// subsequent files.

				if (data->fImporter->Done ())
				{
					gEmuPrefs->UpdatePRCMRU (data->fFiles[data->fCurrentFile]);

					data->fProgressCurrent = -1;
					data->fProgressBase += data->fImporter->GetProgress ();

					delete data->fImporter;
					data->fImporter = NULL;

					delete data->fStream;
					data->fStream = NULL;

					data->fCurrentFile++;
					if (data->fCurrentFile < (long) data->fFiles.size ())
					{
						try
						{
							data->fStream	= new EmStreamFile (data->fFiles[data->fCurrentFile], kOpenExistingForRead);
							data->fImporter	= new EmFileImport (*data->fStream, data->fMethod);
						}
						catch (...)
						{
							delete data->fImporter;
							delete data->fStream;

							data->fStream		= NULL;
							data->fImporter		= NULL;
						}
					}
					else
					{
						// There are no more files to install.  Initialize our
						// counter used to delay the closing of the dialog.

						data->fDoneStart = Platform::GetMilliseconds ();
					}
				}

				inIdle = false;
			}

			// After we're done installing all files, wait a little bit in order to
			// see the completed progress dialog.

			else if (Platform::GetMilliseconds () - data->fDoneStart > 500)
			{
				return kDlgResultClose;
			}

			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// Fall thru...
				}

				case kDlgItemCancel:
				{
					if (context.fItemID == kDlgItemCancel)
						data->fImporter->Cancel ();

					delete data->fImporter;
					data->fImporter = NULL;

					return kDlgResultClose;
				}

				case kDlgItemImpNumFiles:
//				case kDlgItemImpCurAppName:
				case kDlgItemImpProgress:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoDatabaseImport (const EmFileRefList& files, EmFileImportMethod method)
{
	// Validate the list of files.

	EmFileRefList					newFileList;
	EmFileRefList::const_iterator	iter = files.begin ();

	while (iter != files.end ())
	{
		if (iter->IsSpecified () && iter->Exists ())
		{
			newFileList.push_back (*iter);
		}
		else
		{
			// !!! Report that the file doesn't exist.
		}

		++iter;
	}

	if (newFileList.size () <= 0)
	{
		return kDlgItemOK;
	}

	// Stop the session so that the files can be safely loaded.
	EmSessionStopper	stopper (gSession, kStopOnIdle);
	if (!stopper.Stopped ())
		return kDlgItemCancel;

	DatabaseImportData	data;

	data.fFiles		= newFileList;
	data.fMethod	= method;

	return EmDlg::RunDialog (EmDlg::PrvDatabaseImport, &data, kDlgDatabaseImport);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoDatabaseExport
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct ExportDatbaseData
{
	EmDatabaseList	fAllDatabases;
};


static Bool PrvExportFile (const DatabaseInfo& db)
{
	UInt16			dbAttributes;
	UInt32			dbType;
	UInt32			dbCreator;
	ErrCode			err = ::DmDatabaseInfo (db.cardNo, db.dbID, NULL,
											&dbAttributes, NULL, NULL,
											NULL, NULL, NULL, NULL,
											NULL, &dbType, &dbCreator);
	Errors::ThrowIfPalmError (err);

	// Figure out the file type.

	EmFileType		type = kFileTypeNone;

	if ((dbAttributes & dmHdrAttrResDB) != 0)
	{
		type = kFileTypePalmApp;
	}
	else
	{
		if (dbCreator == sysFileCClipper)
			type = kFileTypePalmQA;
		else
			type = kFileTypePalmDB;
	}

	EmFileRef		result;
	string			prompt ("Save as...");
	EmDirRef		defaultPath;
	EmFileTypeList	filterList;
	string			defaultName (db.dbName);

	string			extension = type == kFileTypePalmApp ? ".prc" :
								type == kFileTypePalmQA ? ".pqa" : ".pdb";

	if (!::EndsWith (defaultName.c_str (), extension.c_str ()))
	{
		defaultName += extension;
	}

	filterList.push_back (type);
	filterList.push_back (kFileTypePalmAll);
	filterList.push_back (kFileTypeAll);

	EmDlgItemID	item = EmDlg::DoPutFile (	result,
											prompt,
											defaultPath,
											filterList,
											defaultName);

	if (item == kDlgItemOK)
	{
		EmStreamFile	stream (result, kCreateOrEraseForUpdate,
								kFileCreatorInstaller, type);

		::SavePalmFile (stream, db.cardNo, db.dbName);
	}

	return (item == kDlgItemOK);
}


EmDlgFnResult EmDlg::PrvDatabaseExport (EmDlgContext& context)
{
	ExportDatbaseData*	data = (ExportDatbaseData*) context.fUserData;
	EmDlgRef			dlg = context.fDlg;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Get the list of installed databases.

			::GetDatabases (data->fAllDatabases, kAllDatabases);

			// Add then names of the databases to the list.

			EmDatabaseList::iterator	iter = data->fAllDatabases.begin();
			while (iter != data->fAllDatabases.end())
			{
				string	itemText (iter->dbName);

				// If the database name is different from any user-visible
				// name, then use both of them.

				if (strcmp (iter->dbName, iter->name) != 0)
				{
					itemText += " (";
					itemText += iter->name;
					itemText += ")";
				}

				EmDlg::AppendToList (dlg, kDlgItemExpDbList, itemText.c_str ());
				++iter;
			}
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// Get the indexes for the selected items.

					EmDlgListIndexList	items;
					EmDlg::GetSelectedItems(dlg, kDlgItemExpDbList, items);

					// Export each item.

					Bool	cancel = false;
					EmDlgListIndexList::iterator	iter = items.begin ();
					while (!cancel && iter != items.end ())
					{
						DatabaseInfo&	db = data->fAllDatabases[*iter];

						cancel = !::PrvExportFile (db);

						++iter;
					}
					
					if (cancel)
						break;

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemExpDbList:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoDatabaseExport (void)
{
	ExportDatbaseData	data;

	return EmDlg::RunDialog (EmDlg::PrvDatabaseExport, &data, kDlgDatabaseExport);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoReset
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgFnResult EmDlg::PrvReset (EmDlgContext& context)
{
	EmResetType*	choice = (EmResetType*) context.fUserData;
	EmDlgRef		dlg = context.fDlg;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			EmDlg::SetItemValue (dlg, kDlgItemRstSoft, 1);
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					if (EmDlg::GetItemValue (dlg, kDlgItemRstSoft))
					{
						*choice = kResetSoft;
					}
					else if (EmDlg::GetItemValue (dlg, kDlgItemRstHard))
					{
						*choice = kResetHard;
					}
					else
					{
						EmAssert (EmDlg::GetItemValue (dlg, kDlgItemRstDebug));
						*choice = kResetDebug;
					}

					if (EmDlg::GetItemValue (dlg, kDlgItemRstNoExt) != 0)
					{
						*choice = (EmResetType) ((int) *choice | kResetNoExt);
					}
					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemRstSoft:
				case kDlgItemRstHard:
				case kDlgItemRstDebug:
				case kDlgItemRstNoExt:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoReset (EmResetType& type)
{
	return EmDlg::RunDialog (EmDlg::PrvReset, &type, kDlgReset);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoROMTransferQuery
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct ROMTransferQueryData
{
	StringList				fPortNameList;
	EmTransport::Config**	fConfig;
};


static void PrvGetDlqPortItemList (StringList& menuItems)
{
	menuItems.clear ();

	// Add the serial ports.

	{
		EmTransportSerial::PortNameList	names;
		EmTransportSerial::GetPortNameList (names);

		EmTransportSerial::PortNameList::iterator	iter = names.begin ();
		while (iter != names.end ())
		{
			menuItems.push_back (string (*iter));
			++iter;
		}
	}

#if 0 && !PLATFORM_UNIX
	// Add the USB ports.

	{
		EmTransportUSB::PortNameList	names;
		EmTransportUSB::GetPortNameList (names);

		if (names.size () > 0)
		{
			menuItems.push_back ("");
		}

		EmTransportUSB::PortNameList::iterator	iter = names.begin ();
		while (iter != names.end ())
		{
			menuItems.push_back (string (*iter));
			++iter;
		}
	}
#endif
}


EmDlgFnResult EmDlg::PrvROMTransferQuery (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	ROMTransferQueryData&	data = *(ROMTransferQueryData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// On Unix, the port is specified in an edit text item.
			// On other platforms, we present a list of ports in a menu.

			// !!! TBD: restore port setting from a preference.

#if PLATFORM_UNIX
			EmDlg::SetItemText (dlg, kDlgItemDlqPortList, "/dev/ttyS0");
#else
			EmDlg::AppendToMenu (dlg, kDlgItemDlqPortList, data.fPortNameList);
			EmDlg::SetItemValue (dlg, kDlgItemDlqPortList, 0);
#endif

			EmTransportSerial::BaudList		baudList;
			EmTransportSerial::GetSerialBaudList (baudList);

			StringList		baudStrList;
			::PrvConvertBaudListToStrings (baudList, baudStrList);

			EmDlg::AppendToMenu (dlg, kDlgItemDlqBaudList, baudStrList);
			EmDlg::SetItemValue (dlg, kDlgItemDlqBaudList, 0);		// 115K baud

			// Load the instructions string.  Load it in 9 parts,
			// as the entire string is greater than:
			//
			//	Rez can handle on the Mac.
			//	MSDev's string editor on Windows.
			//	The ANSI-allowed string length (for Unix).

			string	ins;
			for (int ii = 0; ii < 9; ++ii)
			{
				string	temp = Platform::GetString (kStr_ROMTransferInstructions + ii);
				ins += temp;
			}

			EmDlg::SetItemText (dlg, kDlgItemDlqInstructions, ins);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// Get the selected settings.

#if PLATFORM_UNIX
					string	portName = EmDlg::GetItemText (dlg, kDlgItemDlqPortList);
#else
					long	portNum		= EmDlg::GetItemValue (dlg, kDlgItemDlqPortList);
					string	portName	= data.fPortNameList [portNum];
#endif

					EmTransportType transportType;	
					transportType = EmTransport::GetTransportTypeFromPortName (portName.c_str ());

					switch (transportType)
					{
						case kTransportSerial:
						{
							long	baudNum = EmDlg::GetItemValue (dlg, kDlgItemDlqBaudList);

							EmTransportSerial::BaudList		baudList;
							EmTransportSerial::GetSerialBaudList (baudList);

							EmTransportSerial::ConfigSerial*	serConfig = new EmTransportSerial::ConfigSerial;

							serConfig->fPort			= portName;
							serConfig->fBaud			= baudList [baudNum];
							serConfig->fParity			= EmTransportSerial::kNoParity;
							serConfig->fStopBits		= 1;
							serConfig->fDataBits		= 8;
							serConfig->fHwrHandshake	= true;

							*data.fConfig = serConfig;

							break;
						}

						case kTransportUSB:
						{
							EmTransportUSB::ConfigUSB*	usbConfig = new EmTransportUSB::ConfigUSB;

							*data.fConfig = usbConfig;

							break;
						}

						case kTransportSocket:
						case kTransportNone:
							break;
					}

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemDlqInstructions:
				case kDlgItemDlqPortList:
				case kDlgItemDlqBaudList:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoROMTransferQuery (EmTransport::Config*& config)
{
	config = NULL;

	ROMTransferQueryData	data;
	data.fConfig = &config;

	// Generate the list here, and do it just once.  It's important to
	// do it just once, as the list of ports could possibly change between
	// the time they're added to the menu and the time one is chosen to
	// be returned.  This could happen, for example, on the Mac where
	// opening a USB connection creates a new "virtual" serial port.

	::PrvGetDlqPortItemList (data.fPortNameList);

	return EmDlg::RunDialog (EmDlg::PrvROMTransferQuery, &data, kDlgROMTransferQuery);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoROMTransferProgress
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgFnResult EmDlg::PrvROMTransferProgress (EmDlgContext& context)
{
	EmDlgRef		dlg = context.fDlg;
	EmROMTransfer*	transferer = (EmROMTransfer*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			context.fNeedsIdle = true;
			break;
		}

		case kDlgCmdIdle:
		{
			try
			{
				if (!transferer->Continue (dlg))
					return kDlgResultClose;
			}
			catch (ErrCode /*err*/)
			{
				// !!! Should report error code?
				return kDlgResultClose;
			}

			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// Fall thru...
				}

				case kDlgItemCancel:
				{
					transferer->Abort (dlg);
					return kDlgResultClose;
				}

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoROMTransferProgress (EmROMTransfer& transferer)
{
	return EmDlg::RunDialog (EmDlg::PrvROMTransferProgress, &transferer, kDlgROMTransferProgress);
}


/***********************************************************************
 *
 * FUNCTION:	GremlinControlOpen
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:	GremlinControlUpdate
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/


/***********************************************************************
 *
 * FUNCTION:	GremlinControlClose
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditPreferences
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

#if !PLATFORM_UNIX
// Returns the list of menu items to be installed in the kDlgItemPrfPort
// popup menu.  However, on Unix, this dialog item is an edit text item,
// not a menu.  Therefore, this function is not called and not needed.

static void PrvGetPrefPortItemList (StringList& menuItems)
{
	menuItems.clear ();

	// Start off with the "No Port" item.

	menuItems.push_back (Platform::GetString (kStr_NoPort));

	// Add the serial ports.

	{
		EmTransportSerial::PortNameList	names;
		EmTransportSerial::GetPortNameList (names);

		if (names.size () > 0)
		{
			menuItems.push_back ("");
		}

		EmTransportSerial::PortNameList::iterator	iter = names.begin ();
		while (iter != names.end ())
		{
			menuItems.push_back (string (*iter));
			++iter;
		}
	}

	// Add the TCP ports.

	{
		EmTransportSocket::PortNameList	names;
		EmTransportSocket::GetPortNameList (names);

		if (names.size () > 0)
		{
			menuItems.push_back ("");
		}

		EmTransportSocket::PortNameList::iterator	iter = names.begin ();
		while (iter != names.end ())
		{
			menuItems.push_back (string (*iter));
			++iter;
		}
	}
}
#endif


#if !PLATFORM_UNIX
// This function is responsible for updating kDlgItemPrfTarget (which
// is an edit-text item) when the user selects TCP in the kDlgItemPrfPort
// item (which is a menu).
//
// However, on Unix, kDlgPrfPort is an edit-text item, and there is no
// kDlgItemPrfTarget.  Therefore, this routine should not be used on Unix.

static void PrvUpdateSocketEdit (EmDlgContext& context)
{
	EmDlgRef		dlg = context.fDlg;

	long			selectedItem = EmDlg::GetItemValue (dlg, kDlgItemPrfPort);

	StringList		menuItems;
	::PrvGetPrefPortItemList (menuItems);

	EmAssert (selectedItem >= 0 && selectedItem < (long) menuItems.size ());

	string			selectedItemText = menuItems[selectedItem];

	EmAssert (!selectedItemText.empty ());

	EmTransportType	type = EmTransport::GetTransportTypeFromPortName (selectedItemText.c_str ());
	Bool			tcpSelected = type == kTransportSocket;

	EmDlg::EnableDisableItem (dlg, kDlgItemPrfTarget, tcpSelected);
}
#endif


static void PrvEditPreferencesInit (EmDlgContext& context)
{
	EmDlgRef	dlg = context.fDlg;

#if !PLATFORM_UNIX
	// ------------------------------------------------------------
	// Set up the port menu.
	// ------------------------------------------------------------

	Preference<string>		prefCommPort (kPrefKeyCommPort);

	// Populate the menu.

	StringList	menuItems;
	::PrvGetPrefPortItemList (menuItems);
	EmDlg::AppendToMenu (dlg, kDlgItemPrfPort, menuItems);

	// Select the correct menu item.

	StringList::iterator	iter = menuItems.begin ();
	while (iter != menuItems.end ())
	{
		if (!iter->empty () && *iter == *prefCommPort)
		{
			EmDlg::SetItemValue (dlg, kDlgItemPrfPort,
									iter - menuItems.begin ());

			break;
		}

		++iter;
	}

	// If we couldn't find a menu item to select,
	// pick a default.

	if (iter == menuItems.end ())
	{
		EmDlg::SetItemValue (dlg, kDlgItemPrfPort, 0);
	}


	// ------------------------------------------------------------
	// Set up the TCP target edit text item.  Enable or disable
	// it as appropriate.
	// ------------------------------------------------------------

	Preference<string>			prefSerialTargetHost (kPrefKeySerialTargetHost);
	Preference<string>			prefSerialTargetPort (kPrefKeySerialTargetPort);

	string	serialTarget (*prefSerialTargetHost + ":" + *prefSerialTargetPort);
	EmDlg::SetItemText (dlg, kDlgItemPrfTarget, serialTarget);
	::PrvUpdateSocketEdit (context);
#else
	// ------------------------------------------------------------
	// Set up the port text item.
	// ------------------------------------------------------------

	Preference<string>		prefCommPort (kPrefKeyCommPort);

	if (strcmp (prefCommPort->c_str (), "TCP") != 0)
	{
		EmDlg::SetItemText (dlg, kDlgItemPrfPort, *prefCommPort);
	}
	else
	{
		Preference<string>			prefSerialTargetHost (kPrefKeySerialTargetHost);
		Preference<string>			prefSerialTargetPort (kPrefKeySerialTargetPort);

		string	serialTarget (*prefSerialTargetHost + ":" + *prefSerialTargetPort);
		EmDlg::SetItemText (dlg, kDlgItemPrfPort, serialTarget);
	}
#endif

	// ------------------------------------------------------------
	// Set up the NetLib Redirect and Enable Sounds checkboxes.
	// ------------------------------------------------------------

	Preference<bool>			prefRedirectNetLib (kPrefKeyRedirectNetLib);
	Preference<bool>			prefEnableSounds (kPrefKeyEnableSounds);

	EmDlg::SetItemValue (dlg, kDlgItemPrfRedirect, *prefRedirectNetLib);
	EmDlg::SetItemValue (dlg, kDlgItemPrfEnableSound, *prefEnableSounds);


	// ------------------------------------------------------------
	// Set up the Close Action radio buttons.
	// ------------------------------------------------------------

	Preference<CloseActionType>	prefCloseAction (kPrefKeyCloseAction);

	if (*prefCloseAction == kSaveAlways)
	{
		EmDlg::SetItemValue (dlg, kDlgItemPrfSaveAlways, 1);
	}
	else if (*prefCloseAction == kSaveAsk)
	{
		EmDlg::SetItemValue (dlg, kDlgItemPrfSaveAsk, 1);
	}
	else
	{
		EmDlg::SetItemValue (dlg, kDlgItemPrfSaveNever, 1);
	}


	// ------------------------------------------------------------
	// Set up the HotSync User Name edit text item.
	// ------------------------------------------------------------

	Preference<string>			prefUserName (kPrefKeyUserName);

	EmDlg::SetItemText (dlg, kDlgItemPrfUserName, *prefUserName);
}


static Bool PrvEditPreferencesValidate (EmDlgContext& context)
{
	EmDlgRef	dlg = context.fDlg;
	string		userName = EmDlg::GetItemText (dlg, kDlgItemPrfUserName);

	if (userName.size () > dlkMaxUserNameLength)
	{
		EmDlg::DoCommonDialog (kStr_UserNameTooLong, kDlgFlags_OK);
		return false;
	}

	return true;
}


static Bool PrvEditPreferencesCommit (EmDlgContext& context)
{
	EmDlgRef	dlg = context.fDlg;

	// Make sure all the preferences are something we can live with.

	if (!::PrvEditPreferencesValidate (context))
		return false;

#if !PLATFORM_UNIX
	// ------------------------------------------------------------
	// Save the serial port settings.
	// ------------------------------------------------------------
	
	long	selectedPort = EmDlg::GetItemValue (dlg, kDlgItemPrfPort);

	StringList	menuItems;
	::PrvGetPrefPortItemList (menuItems);

	Preference<string>			prefCommPort (kPrefKeyCommPort);
	prefCommPort = menuItems[selectedPort];

	// Get the (host, port) target for serial over tcp redirection

	Preference<string>			prefSerialTargetHost (kPrefKeySerialTargetHost);
	Preference<string>			prefSerialTargetPort (kPrefKeySerialTargetPort);

	string						target = EmDlg::GetItemText (dlg, kDlgItemPrfTarget);
	string::size_type			pos = target.find (':');

	if (pos != string::npos)
	{
		prefSerialTargetHost = target.substr (0, pos);
		prefSerialTargetPort = target.substr (pos + 1);
	}
	else
	{
		// !!! Target is invalid?
	}
#else
	// ------------------------------------------------------------
	// Save the serial port settings.
	// ------------------------------------------------------------

	Preference<string>			prefCommPort (kPrefKeyCommPort);
	string						port = EmDlg::GetItemText (dlg, kDlgItemPrfPort);

	EmTransportType transportType;	
	transportType = EmTransport::GetTransportTypeFromPortName (port.c_str ());

	if (transportType == kTransportSerial)
	{
		prefCommPort = port;
	}
	else
	{
		prefCommPort = string ("TCP");

		Preference<string>			prefSerialTargetHost (kPrefKeySerialTargetHost);
		Preference<string>			prefSerialTargetPort (kPrefKeySerialTargetPort);

		string						target = port;
		string::size_type			pos = target.find (':');

		if (pos != string::npos)
		{
			prefSerialTargetHost = target.substr (0, pos);
			prefSerialTargetPort = target.substr (pos + 1);
		}
		else
		{
			// !!! Target is invalid?
		}
	}
#endif

	// ------------------------------------------------------------
	// Save the NetLib Redirect and Enable Sounds settings.
	// ------------------------------------------------------------

	Preference<bool>			prefRedirectNetLib (kPrefKeyRedirectNetLib);
	Preference<bool>			prefEnableSounds (kPrefKeyEnableSounds);

	prefRedirectNetLib	= EmDlg::GetItemValue (dlg, kDlgItemPrfRedirect) != 0;
	prefEnableSounds	= EmDlg::GetItemValue (dlg, kDlgItemPrfEnableSound) != 0;


	// ------------------------------------------------------------
	// Save the Close Action settings.
	// ------------------------------------------------------------

	Preference<CloseActionType>	prefCloseAction (kPrefKeyCloseAction);

	if (EmDlg::GetItemValue (dlg, kDlgItemPrfSaveAlways) != 0)
	{
		prefCloseAction = kSaveAlways;
	}
	else if (EmDlg::GetItemValue (dlg, kDlgItemPrfSaveAsk) != 0)
	{
		prefCloseAction = kSaveAsk;
	}
	else
	{
		prefCloseAction = kSaveNever;
	}


	// ------------------------------------------------------------
	// Save the HotSync User Name setting.
	// ------------------------------------------------------------

	Preference<string>			prefUserName (kPrefKeyUserName);

	prefUserName = EmDlg::GetItemText (dlg, kDlgItemPrfUserName);


	// Say that everything was valid and the dialog box can be closed.

	return true;
}


EmDlgFnResult EmDlg::PrvEditPreferences (EmDlgContext& context)
{
	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			::PrvEditPreferencesInit (context);
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					if (!::PrvEditPreferencesCommit (context))
					{
						break;
					}

					// Update the emulated environment with the
					// new preferences.

					if (gSession && Patches::UIInitialized ())
					{
						EmSessionStopper stopper (gSession, kStopNow);
						if (stopper.Stopped ())
						{
							Preference<string>	prefUserName (kPrefKeyUserName);
							::SetHotSyncUserName (prefUserName->c_str ());

							// Update the transports used for UART emulation.

							gEmuPrefs->SetTransports ();
						}
					}

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemPrfPort:
#if !PLATFORM_UNIX
					::PrvUpdateSocketEdit (context);
#endif
					break;

				case kDlgItemPrfTarget:
				case kDlgItemPrfRedirect:
				case kDlgItemPrfEnableSound:
				case kDlgItemPrfSaveAlways:
				case kDlgItemPrfSaveAsk:
				case kDlgItemPrfSaveNever:
				case kDlgItemPrfUserName:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditPreferences (void)
{
	EmDlgID	dlgID = ::IsBoundFully () ? kDlgEditPreferencesFullyBound : kDlgEditPreferences;

	return EmDlg::RunDialog (EmDlg::PrvEditPreferences, NULL, dlgID);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditLoggingOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditLoggingOptionsData
{
	LoggingType		fActiveSet;

	// Define a "uint8" variable for each of the logging options we
	// support.  The name of each variable is of the form "f<name>",
	// where "<name>" is what appear in the FOR_EACH_LOG_PREF macro.

#undef DEFINE_STORAGE
#define DEFINE_STORAGE(name)	uint8	f##name;
	FOR_EACH_LOG_PREF (DEFINE_STORAGE)
};


static void PrvFetchLoggingPrefs (EmDlgContext& context)
{
	// Transfer all of the logging values from the preferences
	// to our local storage.

	EditLoggingOptionsData&	data = *(EditLoggingOptionsData*) context.fUserData;

	#undef GET_PREF
	#define GET_PREF(name)							\
		{											\
			Preference<uint8> pref(kPrefKey##name);	\
			data.f##name = *pref;					\
		}

	FOR_EACH_LOG_PREF (GET_PREF)
}


static void PrvInstallLoggingPrefs (EmDlgContext& context)
{
	// Transfer all of the logging values from our local
	// storages to the preferences system.

	EditLoggingOptionsData&	data = *(EditLoggingOptionsData*) context.fUserData;

	#undef SET_PREF
	#define SET_PREF(name)							\
		{											\
			Preference<uint8> pref(kPrefKey##name);	\
			pref = data.f##name;					\
		}

	FOR_EACH_LOG_PREF (SET_PREF)
}


static void PrvLoggingPrefsToButtons (EmDlgContext& context)
{
	// Set the buttons in the current panel.

	EditLoggingOptionsData&	data = *(EditLoggingOptionsData*) context.fUserData;
	int	bitmask = data.fActiveSet;

	#undef TO_BUTTON
	#define TO_BUTTON(name)	\
		EmDlg::SetItemValue (context.fDlg, kDlgItemLog##name, (data.f##name & bitmask) != 0);

	FOR_EACH_LOG_PREF (TO_BUTTON)
}


static void PrvLoggingPrefsFromButtons (EmDlgContext& context)
{
	// Update our local preference values from the buttons in the current panel.

	EditLoggingOptionsData&	data = *(EditLoggingOptionsData*) context.fUserData;
	int	bitmask = data.fActiveSet;

	#undef FROM_BUTTON
	#define FROM_BUTTON(name)											\
		if (EmDlg::GetItemValue (context.fDlg, kDlgItemLog##name) != 0)	\
			data.f##name |= bitmask;									\
		else															\
			data.f##name &= ~bitmask;

	FOR_EACH_LOG_PREF (FROM_BUTTON)
}


EmDlgFnResult EmDlg::PrvEditLoggingOptions (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditLoggingOptionsData&	data = *(EditLoggingOptionsData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			::PrvFetchLoggingPrefs (context);
			::PrvLoggingPrefsToButtons (context);

			if (data.fActiveSet == kNormalLogging)
				EmDlg::SetItemValue (dlg, kDlgItemLogNormal, 1);
			else
				EmDlg::SetItemValue (dlg, kDlgItemLogGremlins, 1);

			// Disable unsupported options.

			EmDlg::DisableItem (dlg, kDlgItemLogLogErrorMessages);
			EmDlg::DisableItem (dlg, kDlgItemLogLogWarningMessages);
			EmDlg::DisableItem (dlg, kDlgItemLogLogCPUOpcodes);
			EmDlg::DisableItem (dlg, kDlgItemLogLogApplicationCalls);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					::PrvLoggingPrefsFromButtons (context);
					::PrvInstallLoggingPrefs (context);

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemLogNormal:
				case kDlgItemLogGremlins:
					::PrvLoggingPrefsFromButtons (context);
					if (context.fItemID == kDlgItemLogNormal)
						data.fActiveSet = kNormalLogging;
					else
						data.fActiveSet = kGremlinLogging;
					::PrvLoggingPrefsToButtons (context);
					break;

				#undef DUMMY_CASE
				#define DUMMY_CASE(name)	case kDlgItemLog##name:
				FOR_EACH_LOG_PREF (DUMMY_CASE)
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditLoggingOptions (LoggingType initialSet)
{
	EditLoggingOptionsData	data;

	data.fActiveSet	= initialSet;

	return EmDlg::RunDialog (EmDlg::PrvEditLoggingOptions, &data, kDlgEditLogging);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditDebuggingOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditDebuggingOptionsData
{
	// Define a "Bool" variable for each of the debugging options we
	// support.  The name of each variable is of the form "f<name>",
	// where "<name>" is what appear in the FOR_EACH_REPORT_PREF macro.

#undef DEFINE_STORAGE
#define DEFINE_STORAGE(name)	Bool	f##name;
	FOR_EACH_REPORT_PREF (DEFINE_STORAGE)
};


static void PrvFetchDebuggingPrefs (EmDlgContext& context)
{
	// Transfer all of the debugging values from the preferences
	// to our local storage.

	EditDebuggingOptionsData&	data = *(EditDebuggingOptionsData*) context.fUserData;

	#undef GET_PREF
	#define GET_PREF(name)							\
		{											\
			Preference<Bool> pref(kPrefKey##name);	\
			data.f##name = *pref;					\
		}

	FOR_EACH_REPORT_PREF (GET_PREF)
}


static void PrvInstallDebuggingPrefs (EmDlgContext& context)
{
	// Transfer all of the debugging values from our local
	// storages to the preferences system.

	EditDebuggingOptionsData&	data = *(EditDebuggingOptionsData*) context.fUserData;

	#undef SET_PREF
	#define SET_PREF(name)							\
		{											\
			Preference<Bool> pref(kPrefKey##name);	\
			pref = data.f##name;					\
		}

	FOR_EACH_REPORT_PREF (SET_PREF)
}


static void PrvDebuggingPrefsToButtons (EmDlgContext& context)
{
	// Set the buttons in the current panel.

	EditDebuggingOptionsData&	data = *(EditDebuggingOptionsData*) context.fUserData;

	#undef TO_BUTTON
	#define TO_BUTTON(name)	\
		EmDlg::SetItemValue (context.fDlg, kDlgItemDbg##name, data.f##name);

	FOR_EACH_REPORT_PREF (TO_BUTTON)
}


static void PrvDebuggingPrefsFromButtons (EmDlgContext& context)
{
	// Update our local preference values from the buttons in the current panel.

	EditDebuggingOptionsData&	data = *(EditDebuggingOptionsData*) context.fUserData;

	#undef FROM_BUTTON
	#define FROM_BUTTON(name)	\
		data.f##name = EmDlg::GetItemValue (context.fDlg, kDlgItemDbg##name) != 0;

	FOR_EACH_REPORT_PREF (FROM_BUTTON)
}


EmDlgFnResult EmDlg::PrvEditDebuggingOptions (EmDlgContext& context)
{
//	EmDlgRef					dlg = context.fDlg;
//	EditDebuggingOptionsData&	data = *(EditDebuggingOptionsData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			::PrvFetchDebuggingPrefs (context);
			::PrvDebuggingPrefsToButtons (context);
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					::PrvDebuggingPrefsFromButtons (context);
					::PrvInstallDebuggingPrefs (context);

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				#undef DUMMY_CASE
				#define DUMMY_CASE(name)	case kDlgItemDbg##name:
				FOR_EACH_REPORT_PREF (DUMMY_CASE)
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditDebuggingOptions (void)
{
	EditDebuggingOptionsData	data;

	return EmDlg::RunDialog (EmDlg::PrvEditDebuggingOptions, &data, kDlgEditDebugging);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditSkins
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditSkinsData
{
	EmDevice		fDevice;
	SkinNameList	fSkins;
};


EmDlgFnResult EmDlg::PrvEditSkins (EmDlgContext& context)
{
	EmDlgRef		dlg = context.fDlg;
	EditSkinsData&	data = *(EditSkinsData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Get the skin the user has chosen.

			SkinName	prefSkin = ::SkinGetSkinName (data.fDevice);

			// Add each skin in the list to the UI element.

			Bool		selected = false;
			int			index = 0;
			SkinNameList::iterator	skinIter = data.fSkins.begin();

			while (skinIter != data.fSkins.end())
			{
				EmDlg::AppendToList (dlg, kDlgItemSknSkinList, *skinIter);

				// Select the currently-used skin.

				if (*skinIter == prefSkin)
				{
					EmDlg::SelectListItem (dlg, kDlgItemSknSkinList, index);
					selected = true;
				}

				++index;
				++skinIter;
			}

			// Ensure that *something* was selected.

			if (!selected)
			{
				EmDlg::SelectListItem (dlg, kDlgItemSknSkinList, 0);
			}

			// Set up the checkboxes

			Preference<ScaleType>	prefScale (kPrefKeyScale);
			EmDlg::SetItemValue (dlg, kDlgItemSknDoubleScale, (*prefScale == 2) ? 1 : 0);

			Preference<RGBType>		prefBackgroundColor (kPrefKeyBackgroundColor);
			EmDlg::SetItemValue (dlg, kDlgItemSknWhiteBackground, prefBackgroundColor.Loaded () ? 1 : 0);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// Update the skin choice.

					// Get the selected skin.

					EmDlgListIndex	index = EmDlg::GetSelectedItem (dlg, kDlgItemSknSkinList);
					EmAssert (index != kDlgItemListNone);

					// Update the preferences.

					::SkinSetSkinName (data.fDevice, data.fSkins[index]);

					// Update the scale.

					Preference<ScaleType>	prefScale (kPrefKeyScale);
					prefScale = (GetItemValue (dlg, kDlgItemSknDoubleScale) != 0) ? 2 : 1;

					// Update the background color.

					if (EmDlg::GetItemValue (dlg, kDlgItemSknWhiteBackground) != 0)
					{
						RGBType				color (0xFF, 0xFF, 0xFF);
						Preference<RGBType>	prefBackgroundColor (kPrefKeyBackgroundColor);
						prefBackgroundColor = color;
					}
					else
					{
						gPrefs->DeletePref (kPrefKeyBackgroundColor);
					}
					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemSknSkinList:
				case kDlgItemSknDoubleScale:
				case kDlgItemSknWhiteBackground:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditSkins (void)
{
	Preference<Configuration>	prefConfiguration (kPrefKeyLastConfiguration);
	Configuration				cfg = *prefConfiguration;

	EditSkinsData	data;
	data.fDevice = cfg.fDevice;

	::SkinGetSkinNames (data.fDevice, data.fSkins);

	return EmDlg::RunDialog (EmDlg::PrvEditSkins, &data, kDlgEditSkins);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditCardOptions
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditCardOptionsData
{
	SlotInfoList	fWorkingInfo;
};

const int kMaxVolumes = 8;

static void  PrvEditCardOptionsOK (EmDlgContext& context)
{
	EditCardOptionsData&		data = *(EditCardOptionsData*) context.fUserData;

	Preference<SlotInfoList>	pref (kPrefKeySlotList);
	SlotInfoList				origList = *pref;
	pref = data.fWorkingInfo;

	// For any that changed, mount/unmount the cards.

	UInt16	refNum;
	Err		err = ::SysLibFind ("HostFS Library", &refNum);

	if (err != errNone || refNum == sysInvalidRefNum)
	{
		EmDlg::DoCommonDialog (kStr_NeedHostFS, kDlgFlags_OK);
		return;
	}

	// We've made sure that we have the HostFS library,
	// so it's OK to try to notify it about changes.

	// Iterate over all the items before the user changed
	// them in the Options dialog.

	SlotInfoList::iterator	iter1 = origList.begin ();
	while (iter1 != origList.end ())
	{
		// Iterator over all the items *after* the user changed
		// them in the dialog.

		SlotInfoList::iterator	iter2 = data.fWorkingInfo.begin ();
		while (iter2 != data.fWorkingInfo.end ())
		{
			// Compare one item from the "before" list to one item
			// in the "after" list.  If they're for the same volume,
			// we will want to compare them further.

			if (iter1->fSlotNumber == iter2->fSlotNumber)
			{
				// The volume in this slot either used to be mounted
				// and now is not, or didn't used to be mounted but
				// now is.  We'll need to tell the HostFS library.

				if (iter1->fSlotOccupied != iter2->fSlotOccupied)
				{
					const UInt32	kCreator	= 'pose';
					const UInt16	kMounted	= 1;
					const UInt16	kUnmounted	= 0;

					UInt16	selector = iter2->fSlotOccupied ? kMounted : kUnmounted;
					void*	cardNum = (void*) iter1->fSlotNumber;

					// Note, in order to make this call, the CPU should be stopped
					// at an idle event (kStopOnIdle).  That's because mounting
					// and unmounting can send out notification.  If the
					// notification is sent out in the background (that is, while
					// the current Palm OS task is not the UI task), then the
					// notification manager calls SysTaskWait.  This will switch
					// to another task if it can, and prime a timer to re-awake
					// the background task if not.  However, this timer is based
					// on an interrupt going off, and while we're calling into the
					// ROM, interrupts are turned off, leading to a hang.
					// Therefore, is is imperative that we call this function
					// while the UI task is the current task.

					::FSCustomControl (refNum, kCreator, selector, cardNum, NULL);
				}

				break;
			}

			++iter2;
		}

		++iter1;
	}
}


EmDlgFnResult EmDlg::PrvEditCardOptions (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditCardOptionsData&	data = *(EditCardOptionsData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Get the current preferences.

			Preference<SlotInfoList>	pref (kPrefKeySlotList);
			data.fWorkingInfo = *pref;

			// Make sure there are at least kMaxVolume entries.  If not, then
			// fill in the missing ones.

			long	curSize = data.fWorkingInfo.size ();
			while (curSize < kMaxVolumes)
			{
				SlotInfoType	info;
				info.fSlotNumber = curSize + 1;
				info.fSlotOccupied = false;
				data.fWorkingInfo.push_back (info);

				curSize++;
			}

			// Install the volumes into the list.

			EmDlg::ClearList (dlg, kDlgItemCrdList);

			for (int ii = 0; ii < kMaxVolumes; ++ii)
			{
				EmDlg::AppendToList (dlg, kDlgItemCrdList, string (1, (char) ('1' + ii)));
			}

			// Select one of the volumes.

			EmDlg::SelectListItem (dlg, kDlgItemCrdList, 0);

			// Call ourselves to make sure that the other dialog
			// items are properly initialized in light of the
			// currently selected item.

			EmDlgContext	subContext (context);
			subContext.fCommandID = kDlgCmdItemSelected;
			subContext.fItemID = kDlgItemCrdList;
			EmDlg::PrvEditCardOptions (subContext);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// User pressed OK, save the changes.

					::PrvEditCardOptionsOK (context);

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}
				
				case kDlgItemCrdList:
				{
					// User changed the currently selected card.

					EmDlgListIndex	item = EmDlg::GetSelectedItem (dlg, kDlgItemCrdList);
					if (item >= 0)
					{
						SlotInfoType&	info = data.fWorkingInfo[item];

						// Get the path to the card's root.  If empty, display
						// a default message so that the user doesn't just see
						// a blank pane.

						string	fullPath (info.fSlotRoot.GetFullPath ());
						if (fullPath.empty ())
							fullPath = "<Not selected>";

						// Set the root path text and the button that says if
						// it's mounted or not.

						EmDlg::SetItemText (dlg, kDlgItemCrdPath, fullPath);
						EmDlg::SetItemValue (dlg, kDlgItemCrdMounted, info.fSlotOccupied);
					}
					break;
				}

				case kDlgItemCrdPath:
					break;

				case kDlgItemCrdMounted:
				{
					// User toggled the checkbox that says whether or not the
					// card is mounted.  Save the new setting.

					EmDlgListIndex	item = EmDlg::GetSelectedItem (dlg, kDlgItemCrdList);
					if (item >= 0)
					{
						SlotInfoType&	info = data.fWorkingInfo[item];

						info.fSlotOccupied = EmDlg::GetItemValue (dlg, kDlgItemCrdMounted);
					}
					break;
				}

				case kDlgItemCrdBrowse:
				{
					// User clicked on the Browse button.  Bring up a
					// "Get Directory" dialog.

					EmDlgListIndex	item = EmDlg::GetSelectedItem (dlg, kDlgItemCrdList);
					if (item >= 0)
					{
						SlotInfoType&	info = data.fWorkingInfo[item];

						EmDirRef	result;
						string		prompt ("Select a directory:");
						EmDirRef	defaultPath (info.fSlotRoot);

						if (EmDlg::DoGetDirectory (result, prompt, defaultPath) == kDlgItemOK)
						{
							info.fSlotRoot = result;
							EmDlg::SetItemText (dlg, kDlgItemCrdPath, result.GetFullPath ());
						}
					}
					break;
				}

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditCardOptions (void)
{
	EditCardOptionsData	data;

	return EmDlg::RunDialog (EmDlg::PrvEditCardOptions, &data, kDlgEditCards);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditBreakpoints
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditCodeBreakpointData
{
	Bool	fEnabled;
	emuptr	fAddress;
	string	fCondition;
};


struct EditBreakpointsData
{
	EditCodeBreakpointData	fCodeBreakpoints[dbgTotalBreakpoints];
};


// Enable or disable the Edit and Clear buttons.

static void PrvEnableCodeBreakpointControls (EmDlgContext& context, bool enable)
{
	EmDlgRef	dlg = context.fDlg;

	EmDlg::EnableDisableItem (dlg, kDlgItemBrkButtonEdit, enable);
	EmDlg::EnableDisableItem (dlg, kDlgItemBrkButtonClear, enable);
}


// Enable or disable the Start Address and Number Of Bytes edit items.

static void PrvEnableDataBreakpointControls (EmDlgContext& context, bool enable)
{
	EmDlgRef	dlg = context.fDlg;

	EmDlg::EnableDisableItem (dlg, kDlgItemBrkStartAddress, enable);
	EmDlg::EnableDisableItem (dlg, kDlgItemBrkNumberOfBytes, enable);
}


// Install the current set of code breakpoints into the list item.

static void PrvRefreshCodeBreakpointList (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditBreakpointsData&	data = *(EditBreakpointsData*) context.fUserData;

	EmDlgListIndex	selected = EmDlg::GetSelectedItem (dlg, kDlgItemBrkList);

	EmDlg::ClearList (dlg, kDlgItemBrkList);

	for (int ii = 0; ii < dbgTotalBreakpoints; ++ii)
	{
		char text[256];

		if (data.fCodeBreakpoints[ii].fEnabled)
		{
			sprintf (text, "at 0x%08lX", data.fCodeBreakpoints[ii].fAddress);
			string	condition = data.fCodeBreakpoints[ii].fCondition;
			if (!condition.empty ())
			{
				strcat (text, " when ");
				strcat (text, condition.c_str ());
			}
		}
		else
		{
			sprintf (text, "(#%d - not set)", ii);
		}

		EmDlg::AppendToList (dlg, kDlgItemBrkList, text);
	}

	if (selected != kDlgItemListNone)
	{
		EmDlg::SelectListItem (dlg, kDlgItemBrkList, selected);
	}
}


// Copy the breakpoint information from the Debugger globals.

static void PrvGetCodeBreakpoints (EmDlgContext& context)
{
	EditBreakpointsData&	data = *(EditBreakpointsData*) context.fUserData;

	for (int ii = 0; ii < dbgTotalBreakpoints; ++ii)
	{
		data.fCodeBreakpoints[ii].fEnabled		= gDebuggerGlobals.bp[ii].enabled;
		data.fCodeBreakpoints[ii].fAddress		= (emuptr) gDebuggerGlobals.bp[ii].addr;
		data.fCodeBreakpoints[ii].fCondition	= "";

		BreakpointCondition*	c = gDebuggerGlobals.bpCondition[ii];
		if (c)
		{
			data.fCodeBreakpoints[ii].fCondition = c->source;
		}
	}
}


// Install the breakpoint information into the Debugger globals.

static void PrvSetCodeBreakpoints (EmDlgContext& context)
{
	EditBreakpointsData&	data = *(EditBreakpointsData*) context.fUserData;

	for (int ii = 0; ii < dbgTotalBreakpoints; ++ii)
	{
		if (data.fCodeBreakpoints[ii].fEnabled)
		{
			BreakpointCondition*	c = NULL;

			string	condition = data.fCodeBreakpoints[ii].fCondition;
			if (!condition.empty ())
			{
				c = Debug::NewBreakpointCondition (condition.c_str ());
			}

			Debug::SetBreakpoint (ii, data.fCodeBreakpoints[ii].fAddress, c);
		}
		else
		{
			Debug::ClearBreakpoint (ii);
		}
	}
}


EmDlgFnResult EmDlg::PrvEditBreakpoints (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditBreakpointsData&	data = *(EditBreakpointsData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Set up the list of code breakpoints.

			::PrvGetCodeBreakpoints (context);
			::PrvRefreshCodeBreakpointList (context);

			// Initially no breakpoint is selected in the list, so the "edit" and
			// "clear" buttons should be disabled.

			::PrvEnableCodeBreakpointControls (context, false);

			// Set up the data breakpoint items

			char	text[256];

			EmDlg::SetItemValue (dlg, kDlgItemBrkCheckEnabled,
				gDebuggerGlobals.watchEnabled ? 1 : 0);

			sprintf (text, "0x%08lX", gDebuggerGlobals.watchAddr);
			EmDlg::SetItemText (dlg, kDlgItemBrkStartAddress, text);

			sprintf (text, "0x%08lX", gDebuggerGlobals.watchBytes);
			EmDlg::SetItemText (dlg, kDlgItemBrkNumberOfBytes, text);

			::PrvEnableDataBreakpointControls (context, gDebuggerGlobals.watchEnabled);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// User pressed OK, save the changes.

					// Set the code breakpoints

					::PrvSetCodeBreakpoints (context);


					// Set data breakpoint

					gDebuggerGlobals.watchEnabled = EmDlg::GetItemValue (dlg, kDlgItemBrkCheckEnabled) != 0;

					if (gDebuggerGlobals.watchEnabled)
					{
						gDebuggerGlobals.watchAddr = EmDlg::GetItemValue (dlg, kDlgItemBrkStartAddress);
						gDebuggerGlobals.watchBytes = EmDlg::GetItemValue (dlg, kDlgItemBrkNumberOfBytes);
					}

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}
				
				case kDlgItemBrkList:
				{
					EmDlgListIndex	selected = EmDlg::GetSelectedItem (dlg, kDlgItemBrkList);
					::PrvEnableCodeBreakpointControls (context, selected != kDlgItemListNone);
					break;
				}

				case kDlgItemBrkButtonEdit:
				{
					EmDlgListIndex	selected = EmDlg::GetSelectedItem (dlg, kDlgItemBrkList);
					if (selected != kDlgItemListNone)
					{
						EmDlg::DoEditCodeBreakpoint (data.fCodeBreakpoints[selected]);
						::PrvRefreshCodeBreakpointList (context);
					}
					break;
				}

				case kDlgItemBrkButtonClear:
				{
					EmDlgListIndex	selected = EmDlg::GetSelectedItem (dlg, kDlgItemBrkList);
					if (selected != kDlgItemListNone)
					{
						data.fCodeBreakpoints[selected].fEnabled = false;
						::PrvRefreshCodeBreakpointList (context);
					}
					break;
				}

				case kDlgItemBrkCheckEnabled:
				{
					long	enabled = EmDlg::GetItemValue (dlg, kDlgItemBrkCheckEnabled);
					::PrvEnableDataBreakpointControls (context, enabled != 0);
					break;
				}

				case kDlgItemBrkStartAddress:
				{
					break;
				}

				case kDlgItemBrkNumberOfBytes:
				{
					break;
				}

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditBreakpoints (void)
{
	EditBreakpointsData	data;

	return EmDlg::RunDialog (EmDlg::PrvEditBreakpoints, &data, kDlgEditBreakpoints);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditCodeBreakpoint
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgFnResult EmDlg::PrvEditCodeBreakpoint (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditCodeBreakpointData&	data = *(EditCodeBreakpointData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			char	text[256];

			sprintf (text, "0x%08lX", data.fAddress);
			EmDlg::SetItemText (dlg, kDlgItemBrkAddress, text);

			EmDlg::SetItemText (dlg, kDlgItemBrkCondition, data.fCondition);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// User pressed OK, save the changes.
					char	text[256];

					data.fAddress	= EmDlg::GetItemValue (dlg, kDlgItemBrkAddress);
					data.fCondition	= EmDlg::GetItemText (dlg, kDlgItemBrkCondition);

					if (data.fAddress & 1)
					{
						string	formatStr = Platform::GetString (kStr_InvalidAddressNotEven);
						string	addressStr = EmDlg::GetItemText (dlg, kDlgItemBrkAddress);
						sprintf (text, formatStr.c_str (), addressStr.c_str ());
						EmDlg::DoCommonDialog (text, kDlgFlags_OK);
						break;
					}

					if (!EmBankROM::ValidAddress (data.fAddress, 2) &&
						!EmBankSRAM::ValidAddress (data.fAddress, 2) &&
						!EmBankDRAM::ValidAddress (data.fAddress, 2))
					{
						string	formatStr = Platform::GetString (kStr_InvalidAddressNotInROMOrRAM);
						string	addressStr = EmDlg::GetItemText (dlg, kDlgItemBrkAddress);
						sprintf (text, formatStr.c_str (), addressStr.c_str ());
						EmDlg::DoCommonDialog (text, kDlgFlags_OK);
						break;
					}

					if (!data.fCondition.empty ())
					{
						BreakpointCondition* c = NULL;

						c = Debug::NewBreakpointCondition (data.fCondition.c_str ());
						if (!c)
						{
							string	formatStr = Platform::GetString (kStr_CannotParseCondition);
							sprintf (text, formatStr.c_str (), data.fCondition.c_str ());
							EmDlg::DoCommonDialog (text, kDlgFlags_OK);
							break;
						}

						delete c;
					}

					data.fEnabled = true;

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}
				
				case kDlgItemBrkAddress:
				case kDlgItemBrkCondition:
				{
					break;
				}

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditCodeBreakpoint (EditCodeBreakpointData& data)
{
	return EmDlg::RunDialog (EmDlg::PrvEditCodeBreakpoint, &data, kDlgEditCodeBreakpoint);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoEditCodeBreakpoint
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditTracingOptionsData
{
	unsigned short	fTracerType;
};


#if !PLATFORM_UNIX	// No gTracer on Unix, yet.
void static PrvPopTracerSettings (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditTracingOptionsData&	data = *(EditTracingOptionsData*) context.fUserData;

	// Propagate settings from controls to internal table

	if (data.fTracerType)
	{
		TracerTypeInfo*	info = gTracer.GetTracerTypeInfo (data.fTracerType);
		EmAssert (info);

		string	value = EmDlg::GetItemText (dlg, kDlgItemTrcTargetValue);
		value = value.substr (0, sizeof (info->paramTmpVal) - 1);
		strcpy (info->paramTmpVal, value.c_str ());

		info->autoConnectTmpState = EmDlg::GetItemValue (dlg, kDlgItemTrcAutoConnect) != 0;
	}
}
#endif


#if !PLATFORM_UNIX	// No gTracer on Unix, yet.
void static PrvPushTracerSettings (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditTracingOptionsData&	data = *(EditTracingOptionsData*) context.fUserData;

	// Propagate settings from internal table to controls
	if (data.fTracerType == 0)
	{
		EmDlg::SetItemText (dlg, kDlgItemTrcTargetValue, "");
		EmDlg::SetItemText (dlg, kDlgItemTrcTargetText, "");

		EmDlg::DisableItem (dlg, kDlgItemTrcTargetValue);
		EmDlg::DisableItem (dlg, kDlgItemTrcTargetText);

		EmDlg::SetItemValue (dlg, kDlgItemTrcAutoConnect, 0);
		EmDlg::DisableItem (dlg, kDlgItemTrcAutoConnect);
	}
	else
	{
		TracerTypeInfo*	info = gTracer.GetTracerTypeInfo (data.fTracerType);

		EmDlg::SetItemText (dlg, kDlgItemTrcTargetValue, info->paramTmpVal);
		EmDlg::SetItemText (dlg, kDlgItemTrcTargetText, info->paramDescr);

		EmDlg::EnableItem (dlg, kDlgItemTrcTargetValue);
		EmDlg::EnableItem (dlg, kDlgItemTrcTargetText);

		if (info->autoConnectSupport)
		{
			EmDlg::SetItemValue (dlg, kDlgItemTrcAutoConnect, info->autoConnectTmpState);
			EmDlg::EnableItem (dlg, kDlgItemTrcAutoConnect);
		}
		else
		{
			EmDlg::SetItemValue (dlg, kDlgItemTrcAutoConnect, 0);
			EmDlg::DisableItem (dlg, kDlgItemTrcAutoConnect);
		}
	}
}
#endif


EmDlgFnResult EmDlg::PrvEditTracingOptions (EmDlgContext& context)
{
#if !PLATFORM_UNIX	// No gTracer on Unix, yet.
	EmDlgRef				dlg = context.fDlg;
	EditTracingOptionsData&	data = *(EditTracingOptionsData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Copy prefs states to internal states

			unsigned short ii;

			for (ii = 1; ii <= gTracer.GetTracerTypeCount (); ++ii)
			{
				TracerTypeInfo*	info = gTracer.GetTracerTypeInfo (ii);
				EmAssert(info);

				strcpy (info->paramTmpVal, info->paramCurVal);
				info->autoConnectTmpState = info->autoConnectCurState;
			}

			// Get the currently selected tracer type.

			data.fTracerType = gTracer.GetCurrentTracerTypeIndex ();

			// Build up the menu of tracer types and select the current one.

			EmDlg::AppendToMenu (dlg, kDlgItemTrcOutput, "None (discards traces)");

			for (ii = 1; ii <= gTracer.GetTracerTypeCount (); ++ii)
			{
				TracerTypeInfo*	info = gTracer.GetTracerTypeInfo (ii);
				EmAssert (info);

				EmDlg::AppendToMenu (dlg, kDlgItemTrcOutput, info->friendlyName);
			}

			EmDlg::SetItemValue (dlg, kDlgItemTrcOutput, data.fTracerType);

			// Install settings for this tracer type.

			::PrvPushTracerSettings (context);

			// Show the version information.

			char	version[100];
			gTracer.GetLibraryVersionString(version, 100);
			EmDlg::SetItemText (dlg, kDlgItemTrcInfo, version);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					Bool	parameterValueChanged = false;

					// If the user made a selection...

					if (data.fTracerType)
					{
						// Commit the current settings for the current tracer type.

						::PrvPopTracerSettings (context);

						// Get the pointer to the current tracer info.

						TracerTypeInfo*	info = gTracer.GetTracerTypeInfo (data.fTracerType);
						EmAssert (info);

						// If the tracer data changed, make it permanent.

						if (strcmp (info->paramCurVal, info->paramTmpVal) != 0)
						{
							parameterValueChanged = true;
							strcpy (info->paramCurVal, info->paramTmpVal);
						}

						info->autoConnectCurState = info->autoConnectTmpState;
					}

					// Make the selected tracer the current one.

					gTracer.SetCurrentTracerTypeIndex (data.fTracerType, parameterValueChanged);

					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemTrcOutput:
				{
					// Save the current settings for the old tracer type.
					::PrvPopTracerSettings (context);

					// Get the new tracer type.
					data.fTracerType = EmDlg::GetItemValue (dlg, kDlgItemTrcOutput);

					// Install the settings for the new tracer type.
					::PrvPushTracerSettings (context);

					break;
				}

				case kDlgItemTrcTargetText:
				case kDlgItemTrcTargetValue:
				case kDlgItemTrcInfo:
				{
					break;
				}

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}
#endif

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoEditTracingOptions (void)
{
	EditTracingOptionsData	data;

	return EmDlg::RunDialog (EmDlg::PrvEditTracingOptions, &data, kDlgEditTracingOptions);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoCommonDialog
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

struct EditCommonDialogData
{
	const char*			fMessage;
	EmCommonDialogFlags	fFlags;
};


struct ButtonState
{
	ButtonState (void) :
		fTextID (0),
		fDefault (false),
		fCancel (false),
		fEnabled (false)
	{
	}

	ButtonState (StrCode textID, Bool def, Bool cancel, Bool enabled) :
		fTextID (textID),
		fDefault (def),
		fCancel (cancel),
		fEnabled (enabled)
	{
	}

	StrCode		fTextID;
	Bool		fDefault;
	Bool		fCancel;
	Bool		fEnabled;
};


EmDlgFnResult EmDlg::PrvCommonDialog (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	EditCommonDialogData&	data = *(EditCommonDialogData*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			// Get the right icon.  They all start out as invisible, so
			// show the right one.

			EmDlgItemID	iconID = kDlgItemCmnIconCaution;

#if 0
#pragma message ("Fix me")
			{
				switch (data.fFlags & kAlertMask)
				{
					case kNoteAlert:	iconID = kDlgItemCmnIconNote;		break;
					case kCautionAlert: iconID = kDlgItemCmnIconCaution;	break;
					case kErrorAlert:	iconID = kDlgItemCmnIconStop;		break;
				}
			}
#endif

			EmDlg::ShowItem (dlg, iconID);

			// Sort out the button situation.

			// Determine what the buttons should say and how they should look.

			ButtonState	buttons[3];
			{
				for (int ii = 0, buttonIndex = 0; ii < 3; ++ii)
				{
					int	flags = GET_BUTTON (ii, data.fFlags);
					if (flags & kButtonVisible)
					{
						StrCode	kStringCodes[] =
						{
							0,
							kStr_OK,
							kStr_Cancel,
							kStr_Yes,
							kStr_No,
							kStr_Continue,
							kStr_Debug,
							kStr_Reset,
							0, 0, 0, 0, 0, 0, 0, 0
						};

						ButtonState	newButton (
							kStringCodes[flags & kButtonMask],
							(flags & kButtonDefault) != 0,
							(flags & kButtonEscape) != 0,
							(flags & kButtonEnabled) != 0);

						buttons[buttonIndex] = newButton;

						++buttonIndex;
					}
				}
			}

			// Make the UI elements match what we determined above.

			for (int ii = 0; ii < 3; ++ii)
			{
				EmDlgItemID	itemID = (EmDlgItemID) (kDlgItemCmnButton1 + ii);

				if (buttons[ii].fTextID)
				{
					// The button is visible.  Set its text and enable/disable it.

					EmDlg::SetItemText (dlg, itemID, buttons[ii].fTextID);
					EmDlg::EnableDisableItem (dlg, itemID, buttons[ii].fEnabled);

					if (buttons[ii].fDefault)
					{
						EmDlg::SetDlgDefaultButton (context, itemID);
					}

					if (buttons[ii].fCancel)
					{
						EmDlg::SetDlgCancelButton (context, itemID);
					}
				}
				else
				{
					// The button is invisible.  Hide it, and move the others over.

					EmDlg::HideItem (dlg, itemID);
				}
			}

			// Measure the text, resize the box it goes into, and set the text.

			EmRect	r		= EmDlg::GetItemBounds (dlg, kDlgItemCmnText);
			int		height	= EmDlg::GetTextHeight (dlg, kDlgItemCmnText, data.fMessage);
			int		delta	= height - r.Height ();

			if (delta > 0)
			{
#if PLATFORM_WINDOWS
				// With PowerPlant and FLTK, resizing the window will resize
				// and move around the elements for us.

				r.fBottom += delta;
				EmDlg::SetItemBounds (dlg, kDlgItemCmnText, r);
#endif

				r = EmDlg::GetDlgBounds (dlg);
				r.fBottom += delta;
				EmDlg::SetDlgBounds (dlg, r);
			}

			EmDlg::SetItemText (dlg, kDlgItemCmnText, data.fMessage);

			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemCmnButton1:
				case kDlgItemCmnButton2:
				case kDlgItemCmnButton3:
				{
					return kDlgResultClose;
				}

				case kDlgItemCmnButtonCopy:
				{
					string	text = EmDlg::GetItemText (context.fDlg, kDlgItemCmnText);
					Platform::CopyToClipboard (text.c_str (), text.size ());
					break;
				}

				case kDlgItemCmnText:
				case kDlgItemCmnIconStop:
				case kDlgItemCmnIconCaution:
				case kDlgItemCmnIconNote:
					break;

				case kDlgItemOK:
					EmAssert (false);
					break;

				case kDlgItemCancel:
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoCommonDialog (StrCode msg,
								   EmCommonDialogFlags flags)
{
	string	str (Platform::GetString (msg));
	return EmDlg::DoCommonDialog (str, flags);
}


EmDlgItemID EmDlg::DoCommonDialog (const char* msg,
								   EmCommonDialogFlags flags)
{
	EditCommonDialogData	data;

	data.fMessage	= msg;
	data.fFlags		= flags;

	EmDlgItemID	result;
	EmDlgItemID	button = EmDlg::RunDialog (EmDlg::PrvCommonDialog, &data, kDlgCommonDialog);

	switch (button)
	{
		case kDlgItemCmnButton1:
			result = (EmDlgItemID) (GET_BUTTON (0, flags) & kButtonMask);
			break;

		case kDlgItemCmnButton2:
			result = (EmDlgItemID) (GET_BUTTON (1, flags) & kButtonMask);
			break;

		case kDlgItemCmnButton3:
			result = (EmDlgItemID) (GET_BUTTON (2, flags) & kButtonMask);
			break;

		default:
			EmAssert (false);
	}

	return result;
}


EmDlgItemID EmDlg::DoCommonDialog (const string& msg,
								   EmCommonDialogFlags flags)
{
	return EmDlg::DoCommonDialog (msg.c_str (), flags);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	DoSaveBound
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgFnResult EmDlg::PrvSaveBound (EmDlgContext& context)
{
	EmDlgRef	dlg = context.fDlg;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			EmDlg::DisableItem (dlg, kDlgItemOK);
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					Bool			fullSave = EmDlg::GetItemValue (dlg, kDlgItemBndSaveRAM) != 0;
					EmFileRef		destFile;
					EmFileTypeList	filterList (1, kFileTypeApplication);

#if PLATFORM_MAC
					string			newName ("Emulator_Bound");
#elif PLATFORM_UNIX
					string			newName ("pose_bound");
#elif PLATFORM_WINDOWS
					string			newName ("Emulator_Bound.exe");
#else
#error "Unsupported platform"
#endif

					if (EmDlg::DoPutFile (destFile, "Save new emulator",
						EmDirRef::GetEmulatorDirectory (), filterList, newName) == kDlgItemOK)
					{
						Platform::BindPoser (fullSave, destFile);
					}
					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				case kDlgItemBndSaveROM:
				case kDlgItemBndSaveRAM:
					EmDlg::EnableItem (dlg, kDlgItemOK);
					break;

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}


EmDlgItemID EmDlg::DoSaveBound (void)
{
	return EmDlg::RunDialog (EmDlg::PrvSaveBound, NULL, kDlgSaveBound);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	RunDialog
 *
 * DESCRIPTION:	Common routine that all dialog clients go through.
 *				This function checks to see if we are in the UI thread
 *				or not.  If not, it forwards the call to the session
 *				so that it can block and wait for the UI thread to
 *				pick up and handle the call.  If we are already in the
 *				UI thread, then call the Host routine that displays
 *				and drives the dialog.
 *
 * PARAMETERS:	fn - the custom dialog handler
 *				userData - custom data passed back to the dialog handler.
 *				dlgID - ID of dialog to create.
 *
 * RETURNED:	ID of dialog item that dismissed the dialog.
 *
 ***********************************************************************/
 
EmDlgItemID EmDlg::RunDialog (EmDlgFn fn, void* userData, EmDlgID dlgID)
{
#if HAS_OMNI_THREAD
	if (gSession && gSession->InCPUThread ())
	{
		return gSession->BlockOnDialog (fn, userData, dlgID);
	}
#else
	if (gSession && gSession->GetSessionState () == kRunning)
	{
		return gSession->BlockOnDialog (fn, userData, dlgID);
	}
#endif

	return EmDlg::HostRunDialog (fn, userData, dlgID);
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemText
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemText (EmDlgRef dlg, EmDlgItemID item, StrCode strCode)
{
	string	str = Platform::GetString (strCode);
	EmDlg::SetItemText (dlg, item, str);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SetItemText
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SetItemText (EmDlgRef dlg, EmDlgItemID item, const char* str)
{
	EmDlg::SetItemText (dlg, item, string (str));
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::EnableDisableItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::EnableDisableItem (EmDlgRef dlg, EmDlgItemID item, Bool enable)
{
	if (enable)
		EmDlg::EnableItem (dlg, item);
	else
		EmDlg::DisableItem (dlg, item);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::ShowHideItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::ShowHideItem (EmDlgRef dlg, EmDlgItemID item, Bool show)
{
	if (show)
		EmDlg::ShowItem (dlg, item);
	else
		EmDlg::HideItem (dlg, item);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::AppendToMenu
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::AppendToMenu (EmDlgRef dlg, EmDlgItemID item, const string& text)
{
	StringList	strList;
	strList.push_back (text);

	EmDlg::AppendToMenu (dlg, item, strList);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::AppendToList
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::AppendToList (EmDlgRef dlg, EmDlgItemID item, const string& text)
{
	StringList	strList;
	strList.push_back (text);

	EmDlg::AppendToList (dlg, item, strList);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::SelectListItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

void EmDlg::SelectListItem (EmDlgRef dlg, EmDlgItemID item, EmDlgListIndex listItem)
{
	EmDlgListIndexList	itemList;
	itemList.push_back (listItem);

	EmDlg::SelectListItems (dlg, item, itemList);
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::GetSelectedItem
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgListIndex EmDlg::GetSelectedItem (EmDlgRef dlg, EmDlgItemID item)
{
	EmDlgListIndexList	itemList;
	EmDlg::GetSelectedItems (dlg, item, itemList);

	if (itemList.size () > 0)
	{
		return itemList[0];
	}

	return kDlgItemListNone;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlg::StringToLong
 *
 * DESCRIPTION:	
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

Bool EmDlg::StringToLong (const char* s, long* num)
{
	if (sscanf(s, "%li", num) == 1)
		return true;

	return false;
}


#pragma mark -

/***********************************************************************
 *
 * FUNCTION:	EmDlgContext c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgContext::EmDlgContext (void) :
	fFn (NULL),
	fFnResult (kDlgResultNone),
	fDlg (NULL),
	fDlgID (kDlgNone),
	fPanelID (kDlgPanelNone),
	fItemID (kDlgItemNone),
	fCommandID (kDlgCmdNone),
	fNeedsIdle (false),
	fUserData (NULL),
	fDefaultItem (kDlgItemNone),
	fCancelItem (kDlgItemNone)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmDlgContext c'tor
 *
 * DESCRIPTION:	Constructor.  Initialize our data members.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	Nothing
 *
 ***********************************************************************/

EmDlgContext::EmDlgContext (const EmDlgContext& other) :
	fFn (other.fFn),
	fFnResult (other.fFnResult),
	fDlg (other.fDlg),
	fDlgID (other.fDlgID),
	fPanelID (other.fPanelID),
	fItemID (other.fItemID),
	fCommandID (other.fCommandID),
	fNeedsIdle (other.fNeedsIdle),
	fUserData (other.fUserData),
	fDefaultItem (other.fDefaultItem),
	fCancelItem (other.fCancelItem)
{
}


/***********************************************************************
 *
 * FUNCTION:	EmDlgContext::Init
 *
 * DESCRIPTION:	Call the custom dialog handler to initialize the dialog.
 *				If the dialog says that it wants idle time, then start
 *				idleing it.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The result returned by the dialog handler.
 *
 ***********************************************************************/

EmDlgFnResult EmDlgContext::Init (void)
{
	fItemID		= kDlgItemNone;
	fCommandID	= kDlgCmdInit;

	fFnResult	= fFn (*this);

	if (fNeedsIdle)
		EmDlg::HostStartIdling (*this);

	return fFnResult;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlgContext::Event
 *
 * DESCRIPTION:	Call the custom dialog handler to handle an event.
 *
 * PARAMETERS:	item - dialog item that was selected.
 *
 * RETURNED:	The result returned by the dialog handler.
 *
 ***********************************************************************/

EmDlgFnResult EmDlgContext::Event (EmDlgItemID itemID)
{	
	fItemID		= itemID;
	fCommandID	= kDlgCmdItemSelected;

	fFnResult	= fFn (*this);
	return fFnResult;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlgContext::Idle
 *
 * DESCRIPTION:	Call the custom dialog handler to idle.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The result returned by the dialog handler.
 *
 ***********************************************************************/

EmDlgFnResult EmDlgContext::Idle (void)
{	
	fItemID		= kDlgItemNone;
	fCommandID	= kDlgCmdIdle;

	fFnResult	= fFn (*this);
	return fFnResult;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlgContext::PanelEnter
 *
 * DESCRIPTION:	Call the custom dialog handler to idle.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The result returned by the dialog handler.
 *
 ***********************************************************************/

EmDlgFnResult EmDlgContext::PanelEnter (void)
{	
	fItemID		= kDlgItemNone;
	fCommandID	= kDlgCmdPanelEnter;

	fFnResult	= fFn (*this);
	return fFnResult;
}


/***********************************************************************
 *
 * FUNCTION:	EmDlgContext::PanelExit
 *
 * DESCRIPTION:	Call the custom dialog handler to idle.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	The result returned by the dialog handler.
 *
 ***********************************************************************/

EmDlgFnResult EmDlgContext::PanelExit (void)
{	
	fItemID		= kDlgItemNone;
	fCommandID	= kDlgCmdPanelExit;

	fFnResult	= fFn (*this);
	return fFnResult;
}


#pragma mark -

void EmDlg::PrvBuildROMMenu (const EmDlgContext& context)
{
	const EmDlgRef&	dlg		= context.fDlg;
	SessionNewData*	data	= (SessionNewData*) context.fUserData;
	Configuration&	cfg		= data->fWorkingCfg;

	int 			menuID		= 0;
	Bool			selected	= false;

	EmDlg::ClearMenu (dlg, kDlgItemNewROM);

	EmDlg::AppendToMenu (dlg, kDlgItemNewROM, Platform::GetString (kStr_OtherMRU));
	EmDlg::AppendToMenu (dlg, kDlgItemNewROM, std::string());

	menuID = 2;

	for (int ii = 0; ; ++ii)
	{
		EmFileRef	fileRef = gEmuPrefs->GetIndROMMRU (ii);
		if (!fileRef.IsSpecified())
			break;
		
		EmDlg::AppendToMenu (dlg, kDlgItemNewROM, fileRef.GetName());

		if (cfg.fROMFile.GetFullPath() == fileRef.GetFullPath())
		{
			EmDlg::SetItemValue (dlg, kDlgItemNewROM, menuID);
			selected = true;
		}

		++menuID;

	}
	
	if (menuID == 2)
	{
		EmDlg::AppendToMenu (dlg, kDlgItemNewROM, Platform::GetString (kStr_EmptyMRU));
		EmDlg::DisableMenuItem (dlg, kDlgItemNewROM, 2);
	}

	if (!selected) 
	{
		EmDlg::SetItemValue (dlg, kDlgItemNewROM, 2);
	}

}


struct PrvSupportsROM : unary_function<EmDevice&, bool>
{
	PrvSupportsROM(EmROMReader<LAS>& inROM) : ROM(inROM) {}	
	bool operator()(EmDevice& item)
	{
		return !item.SupportsROM(ROM);
	}

private:
	const EmROMReader<LAS>& ROM;
};


EmDeviceList::iterator EmDlg::PrvFilterDeviceList(const EmFileRef& romFile, EmDeviceList& devices)
{
	EmDeviceList::iterator devices_end = devices.end();
	if (romFile.IsSpecified())
	{
		try
		{
			EmStreamFile	hROM(romFile, kOpenExistingForRead);
			StMemory    	romImage (hROM.GetLength());

			hROM.GetBytes (romImage.Get (), hROM.GetLength());

			EmROMReader<LAS> ROM(romImage.Get(), hROM.GetLength());

			if (ROM.AcquireCardHeader()) {
				ROM.AcquireROMHeap();
				ROM.AcquireDatabases();
				ROM.AcquireFeatures();
				ROM.AcquireSplashDB();
			}

			devices_end = remove_if(devices.begin(), devices.end(),
				PrvSupportsROM(ROM));
		}
		catch (ErrCode)
		{
			/* On any of our errors, don't remove any devices */
		}
	}

	return devices_end;
}


void EmDlg::PrvBuildDeviceMenu (const EmDlgContext& context)
{
	const EmDlgRef&			dlg			= context.fDlg;
	SessionNewData*			data		= (SessionNewData*) context.fUserData;
	Configuration&			cfg			= data->fWorkingCfg;

	EmDeviceList			devices		= EmDevice::GetDeviceList ();

	EmDeviceList::iterator	iter		= devices.begin();
	EmDeviceList::iterator	devices_end	= devices.end();
	int 					menuID		= 0;
	Bool					selected	= false;

	devices_end = PrvFilterDeviceList(cfg.fROMFile, devices);
	iter = devices.begin();

	if (iter == devices_end)
	{
		/* We filtered out too many things, so reset to displaying all */
		devices_end = devices.end();
	}

	data->fDevices = EmDeviceList(iter, devices_end);

	EmDlg::ClearMenu (dlg, kDlgItemNewDevice);

	while (iter != devices_end)
	{
		EmDlg::AppendToMenu (dlg, kDlgItemNewDevice, iter->GetMenuString ());

		if (cfg.fDevice == *iter)
		{
			EmDlg::SetItemValue (dlg, kDlgItemNewDevice, menuID);
			selected = true;
		}

		++iter;
		++menuID;
	}

	if (!selected)
	{
		EmDlg::SetItemValue (dlg, kDlgItemNewDevice, menuID-1);
		cfg.fDevice = data->fDevices [menuID-1];

		/* Rely on caller to (always) invoke PrvBuildSkinMenu */
	}
}


void EmDlg::PrvBuildSkinMenu (const EmDlgContext& context)
{
	const EmDlgRef&			dlg			= context.fDlg;
	SessionNewData*			data		= (SessionNewData*) context.fUserData;
	Configuration&			cfg			= data->fWorkingCfg;

	SkinNameList			skins;
	::SkinGetSkinNames (cfg.fDevice, skins);
	SkinName				chosenSkin	= ::SkinGetSkinName (cfg.fDevice);

	SkinNameList::iterator	iter		= skins.begin ();
	int 					menuID		= 0;

	EmDlg::ClearMenu (dlg, kDlgItemNewSkin);

	while (iter != skins.end())
	{
		EmDlg::AppendToMenu (dlg, kDlgItemNewSkin, *iter);

		if (chosenSkin == *iter)
		{
			EmDlg::SetItemValue (dlg, kDlgItemNewSkin, menuID);
		}

		++iter;
		++menuID;
	}
}


void EmDlg::PrvBuildMemorySizeMenu (const EmDlgContext& context)
{
	const EmDlgRef&				dlg			= context.fDlg;
	SessionNewData*				data		= (SessionNewData*) context.fUserData;
	Configuration&				cfg			= data->fWorkingCfg;

	MemoryTextList				sizes;
	::GetMemoryTextList (sizes);
	::PrvFilterMemorySizes (sizes, cfg);

#ifdef SONY_ROM 
	BOOL			enabled = (cfg.fDevice.GetDeviceType() != kDevicePEGN700C
							&& cfg.fDevice.GetDeviceType() != kDevicePEGS320
							&& cfg.fDevice.GetDeviceType() != kDevicePEGS360
							&& cfg.fDevice.GetDeviceType() != kDevicePEGT400
							&& cfg.fDevice.GetDeviceType() != kDevicePEGT600
							&& cfg.fDevice.GetDeviceType() != kDevicePEGN600C
							&& cfg.fDevice.GetDeviceType() != kDevicePEGS500C
							&& cfg.fDevice.GetDeviceType() != kDevicePEGS300);

	if (cfg.fDevice.GetDeviceType() == kDevicePEGN700C
	 || cfg.fDevice.GetDeviceType() == kDevicePEGS320
	 || cfg.fDevice.GetDeviceType() == kDevicePEGT400
	 || cfg.fDevice.GetDeviceType() == kDevicePEGN600C
	 || cfg.fDevice.GetDeviceType() == kDevicePEGS500C
	 || cfg.fDevice.GetDeviceType() == kDevicePEGS300)
		cfg.fRAMSize = 8192;
	
	if (cfg.fDevice.GetDeviceType() == kDevicePEGT600
	 || cfg.fDevice.GetDeviceType() == kDevicePEGS360)
		cfg.fRAMSize = 16384;
#endif	// SONY_ROM

	MemoryTextList::iterator	iter		= sizes.begin();
	int							menuID		= 0;
	Bool						selected	= false;

	EmDlg::ClearMenu (dlg, kDlgItemNewMemorySize);

	while (iter != sizes.end())
	{
		EmDlg::AppendToMenu (dlg, kDlgItemNewMemorySize, iter->second);

		if (cfg.fRAMSize == iter->first)
		{
			EmDlg::SetItemValue (dlg, kDlgItemNewMemorySize, menuID);
			selected = true;
		}
#ifdef SONY_ROM 
		else if (!enabled)
		{
			EmDlg::DisableMenuItem (dlg, kDlgItemNewMemorySize, menuID);
		}
#endif

		++iter;
		++menuID;
	}

	if (!selected)
	{
		EmDlg::SetItemValue (dlg, kDlgItemNewMemorySize, 0);
		cfg.fRAMSize = sizes[0].first;
	}
}


void EmDlg::PrvSetROMName (const EmDlgContext& context)
{
	const EmDlgRef&	dlg		= context.fDlg;
	SessionNewData*	data	= (SessionNewData*) context.fUserData;
	Configuration&	cfg		= data->fWorkingCfg;
	EmFileRef&		rom		= cfg.fROMFile;

	EmDlg::EnableDisableItem (dlg, kDlgItemOK, rom.IsSpecified ());
}


void PrvConvertBaudListToStrings (const EmTransportSerial::BaudList& baudList,
									StringList& baudStrList)
{
	EmTransportSerial::BaudList::const_iterator	iter = baudList.begin();
	while (iter != baudList.end())
	{
		char	buffer[20];
		::FormatInteger (buffer, *iter);
		strcat (buffer, " bps");
		baudStrList.push_back (buffer);
		++iter;
	}
}

void PrvFilterMemorySizes (MemoryTextList& sizes, const Configuration& cfg)
{
	RAMSizeType					minSize	= cfg.fDevice.MinRAMSize ();
	MemoryTextList::iterator	iter	= sizes.begin();

	while (iter != sizes.end())
	{
		if (iter->first < minSize)
		{
			sizes.erase (iter);
			iter = sizes.begin ();
			continue;
		}

		++iter;
	}
}


#if 0
	// Template for callback function

EmDlgFnResult EmDlg::PrvCallback (EmDlgContext& context)
{
	EmDlgRef				dlg = context.fDlg;
	<type>*	data = (<type>*) context.fUserData;

	switch (context.fCommandID)
	{
		case kDlgCmdInit:
		{
			break;
		}

		case kDlgCmdIdle:
		{
			break;
		}

		case kDlgCmdItemSelected:
		{
			switch (context.fItemID)
			{
				case kDlgItemOK:
				{
					// Fall thru...
				}

				case kDlgItemCancel:
				{
					return kDlgResultClose;
				}

				default:
					EmAssert (false);
			}

			break;
		}

		case kDlgCmdPanelEnter:
		case kDlgCmdPanelExit:
		case kDlgCmdNone:
			EmAssert (false);
			break;
	}

	return kDlgResultContinue;
}
#endif

#ifdef SONY_ROM
#include "resource.h"
void EmDlg::PrvBuildMSSizeMenu (const EmDlgContext& context)
{
	const EmDlgRef&	dlg		= context.fDlg;
	SessionNewData*	data	= (SessionNewData*) context.fUserData;
	Configuration&	cfg		= data->fWorkingCfg;

	BOOL			enabled = (cfg.fDevice.GetDeviceType() == kDevicePEGN700C
							|| cfg.fDevice.GetDeviceType() == kDevicePEGS320
							|| cfg.fDevice.GetDeviceType() == kDevicePEGS360
							|| cfg.fDevice.GetDeviceType() == kDevicePEGT400
							|| cfg.fDevice.GetDeviceType() == kDevicePEGT600
							|| cfg.fDevice.GetDeviceType() == kDevicePEGN600C
							|| cfg.fDevice.GetDeviceType() == kDevicePEGS500C
							|| cfg.fDevice.GetDeviceType() == kDevicePEGS300);

	if (dlg)
	{
		HWND  hLabel = ::GetDlgItem((HWND)dlg, IDC_MSSIZE_LABEL);
		::ShowWindow(hLabel, (enabled) ? SW_SHOW : SW_HIDE);
		ShowItem(dlg, kDlgItemNewMSSize, enabled);
	}

	if (!enabled)
		return;
	
	MemoryTextList	sizes;
	::GetMSSizeTextList (sizes);

	MemoryTextList::iterator	iter = sizes.begin();
	int							menuID = 0;

	EmDlg::ClearMenu (dlg, kDlgItemNewMSSize);

	Bool	bFindFirst = false;
	while (iter != sizes.end())
	{
		EmDlg::AppendToMenu (dlg, kDlgItemNewMSSize, iter->second);

		if (cfg.fMSSize == iter->first)
		{
			bFindFirst = true;
			EmDlg::SetItemValue (dlg, kDlgItemNewMSSize, menuID);
		}

		++iter;
		++menuID;
	}

	if (!bFindFirst)
	{
		cfg.fMSSize = 8;
		EmDlg::SetItemValue (dlg, kDlgItemNewMSSize, 0);
	}
}
#endif // SONY_ROM
