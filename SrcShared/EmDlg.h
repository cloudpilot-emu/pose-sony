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

#ifndef EmDlg_h
#define EmDlg_h

#include "EmFileImport.h"		// EmFileImportMethod
#include "EmFileRef.h"			// EmFileRefList
#include "EmTransport.h"		// EmTransport
#include "EmTypes.h"			// EmResetType
#include "EmStructs.h"			// DatabaseInfoList

#include "Logging.h"			// FOR_EACH_LOG_PREF

class EmROMTransfer;

class LAS;
template <class A> class EmROMReader;


enum EmDlgCmdID
{
	kDlgCmdNone					= 0,

	kDlgCmdInit					= 1,
	kDlgCmdIdle					= 2,
	kDlgCmdItemSelected			= 3,
	kDlgCmdPanelEnter			= 4,
	kDlgCmdPanelExit			= 5
};

enum EmDlgFnResult
{
	kDlgResultNone				= 0,

	kDlgResultContinue			= 1,
	kDlgResultClose				= 2
};

enum EmDlgID
{
	kDlgNone					= 0,

	kDlgAboutBox				= 1,
	kDlgSessionNew				= 2,
	kDlgSessionSave				= 3,
	kDlgHordeNew				= 4,
	kDlgDatabaseImport			= 5,
	kDlgDatabaseExport			= 6,
	kDlgROMTransferQuery		= 7,
	kDlgROMTransferProgress		= 8,
	kDlgGremlinControl			= 9,
	kDlgEditPreferences			= 10,
	kDlgEditLogging				= 11,
	kDlgEditDebugging			= 12,
	kDlgEditSkins				= 13,
	kDlgCommonDialog			= 14,
	kDlgSaveBound				= 15,
	kDlgEditCards				= 16,
	kDlgEditBreakpoints			= 17,
	kDlgEditCodeBreakpoint		= 18,
	kDlgEditTracingOptions		= 19,
	kDlgEditPreferencesFullyBound	= 20,
	kDlgReset					= 21
};

enum EmDlgPanelID
{
	kDlgPanelNone				= 0,

	kDlgPanelAbtMain			= 1,
	kDlgPanelAbtWindows			= 2,
	kDlgPanelAbtMac				= 3,
	kDlgPanelAbtUAE				= 4,
	kDlgPanelAbtQNX				= 5
};

#define DLG_BASE(dlgID)	((dlgID) * 100)
enum EmDlgItemID
{
	kDlgItemNone				= 0,

	kDlgItemOK					= 0x01,
	kDlgItemCancel				= 0x02,
	kDlgItemYes					= 0x03,
	kDlgItemNo					= 0x04,
	kDlgItemContinue			= 0x05,
	kDlgItemDebug				= 0x06,
	kDlgItemReset				= 0x07,
	kDlgItemNextGremlin			= 0x08,

	kDlgItemAbtAppName			= DLG_BASE(kDlgAboutBox) + 0,
	kDlgItemAbtURLPalmWeb		= DLG_BASE(kDlgAboutBox) + 1,
	kDlgItemAbtURLPalmMail		= DLG_BASE(kDlgAboutBox) + 2,
	kDlgItemAbtURLWindowsWeb	= DLG_BASE(kDlgAboutBox) + 3,
	kDlgItemAbtURLWindowsMail	= DLG_BASE(kDlgAboutBox) + 4,
	kDlgItemAbtURLMacWeb		= DLG_BASE(kDlgAboutBox) + 5,
	kDlgItemAbtURLMacMail		= DLG_BASE(kDlgAboutBox) + 6,
	kDlgItemAbtURLUAEWeb		= DLG_BASE(kDlgAboutBox) + 7,
	kDlgItemAbtURLUAEMail		= DLG_BASE(kDlgAboutBox) + 8,
	kDlgItemAbtURLQNXWeb		= DLG_BASE(kDlgAboutBox) + 9,
	kDlgItemAbtURLQNXMail		= DLG_BASE(kDlgAboutBox) + 10,

	kDlgItemNewDevice			= DLG_BASE(kDlgSessionNew) + 0,
	kDlgItemNewSkin				= DLG_BASE(kDlgSessionNew) + 1,
	kDlgItemNewMemorySize		= DLG_BASE(kDlgSessionNew) + 2,
	kDlgItemNewROM				= DLG_BASE(kDlgSessionNew) + 3,
	kDlgItemNewROMBrowse		= DLG_BASE(kDlgSessionNew) + 4,

#ifdef SONY_ROM
	kDlgItemNewMSSize			= DLG_BASE(kDlgSessionNew) + 5,
#endif

	kDlgItemDontSave			= DLG_BASE(kDlgSessionSave) + 0,

	kDlgItemHrdAppList			= DLG_BASE(kDlgHordeNew) + 0,
	kDlgItemHrdStart			= DLG_BASE(kDlgHordeNew) + 1,
	kDlgItemHrdStop				= DLG_BASE(kDlgHordeNew) + 2,
	kDlgItemHrdCheckSwitch		= DLG_BASE(kDlgHordeNew) + 3,
	kDlgItemHrdCheckSave		= DLG_BASE(kDlgHordeNew) + 4,
	kDlgItemHrdCheckStop		= DLG_BASE(kDlgHordeNew) + 5,
	kDlgItemHrdDepthSwitch		= DLG_BASE(kDlgHordeNew) + 6,
	kDlgItemHrdDepthSave		= DLG_BASE(kDlgHordeNew) + 7,
	kDlgItemHrdDepthStop		= DLG_BASE(kDlgHordeNew) + 8,
	kDlgItemHrdLogging			= DLG_BASE(kDlgHordeNew) + 9,

	kDlgItemImpNumFiles			= DLG_BASE(kDlgDatabaseImport) + 0,
	kDlgItemImpProgress			= DLG_BASE(kDlgDatabaseImport) + 1,

	kDlgItemExpDbList			= DLG_BASE(kDlgDatabaseExport) + 0,

	kDlgItemDlqInstructions		= DLG_BASE(kDlgROMTransferQuery) + 0,
	kDlgItemDlqPortList			= DLG_BASE(kDlgROMTransferQuery) + 1,
	kDlgItemDlqBaudList			= DLG_BASE(kDlgROMTransferQuery) + 2,

	kDlgItemDlpMessage			= DLG_BASE(kDlgROMTransferProgress) + 0,
	kDlgItemDlpProgress			= DLG_BASE(kDlgROMTransferProgress) + 1,

	kDlgItemPrfPort				= DLG_BASE(kDlgEditPreferences) + 0,
	kDlgItemPrfTarget			= DLG_BASE(kDlgEditPreferences) + 1,
	kDlgItemPrfRedirect			= DLG_BASE(kDlgEditPreferences) + 2,
	kDlgItemPrfEnableSound		= DLG_BASE(kDlgEditPreferences) + 3,
	kDlgItemPrfSaveAlways		= DLG_BASE(kDlgEditPreferences) + 4,
	kDlgItemPrfSaveAsk			= DLG_BASE(kDlgEditPreferences) + 5,
	kDlgItemPrfSaveNever		= DLG_BASE(kDlgEditPreferences) + 6,
	kDlgItemPrfUserName			= DLG_BASE(kDlgEditPreferences) + 7,

	kDlgItemLogNormal			= DLG_BASE(kDlgEditLogging) + 0,
	kDlgItemLogGremlins			= DLG_BASE(kDlgEditLogging) + 1,
	kDlgItemLogRadioBase		= DLG_BASE(kDlgEditLogging) + 10,
#undef DEFINE_BUTTON_ID
#define DEFINE_BUTTON_ID(name)	kDlgItemLog##name,
	FOR_EACH_LOG_PREF(DEFINE_BUTTON_ID)

	kDlgItemDbgRadioBase		= DLG_BASE(kDlgEditDebugging) + 10,
#undef DEFINE_BUTTON_ID
#define DEFINE_BUTTON_ID(name)	kDlgItemDbg##name,
	FOR_EACH_REPORT_PREF(DEFINE_BUTTON_ID)

	kDlgItemSknSkinList			= DLG_BASE(kDlgEditSkins) + 0,
	kDlgItemSknDoubleScale		= DLG_BASE(kDlgEditSkins) + 1,
	kDlgItemSknWhiteBackground	= DLG_BASE(kDlgEditSkins) + 2,

	kDlgItemCmnText				= DLG_BASE(kDlgCommonDialog) + 0,
	kDlgItemCmnIconStop			= DLG_BASE(kDlgCommonDialog) + 1,
	kDlgItemCmnIconCaution		= DLG_BASE(kDlgCommonDialog) + 2,
	kDlgItemCmnIconNote			= DLG_BASE(kDlgCommonDialog) + 3,
	kDlgItemCmnButton1			= DLG_BASE(kDlgCommonDialog) + 4,
	kDlgItemCmnButton2			= DLG_BASE(kDlgCommonDialog) + 5,
	kDlgItemCmnButton3			= DLG_BASE(kDlgCommonDialog) + 6,
	kDlgItemCmnButtonCopy		= DLG_BASE(kDlgCommonDialog) + 7,

	kDlgItemBndSaveROM			= DLG_BASE(kDlgSaveBound) + 0,
	kDlgItemBndSaveRAM			= DLG_BASE(kDlgSaveBound) + 1,

	kDlgItemCrdList				= DLG_BASE(kDlgEditCards) + 0,
	kDlgItemCrdPath				= DLG_BASE(kDlgEditCards) + 1,
	kDlgItemCrdMounted			= DLG_BASE(kDlgEditCards) + 2,
	kDlgItemCrdBrowse			= DLG_BASE(kDlgEditCards) + 3,

	kDlgItemBrkList				= DLG_BASE(kDlgEditBreakpoints) + 0,
	kDlgItemBrkButtonEdit		= DLG_BASE(kDlgEditBreakpoints) + 1,
	kDlgItemBrkButtonClear		= DLG_BASE(kDlgEditBreakpoints) + 2,
	kDlgItemBrkCheckEnabled		= DLG_BASE(kDlgEditBreakpoints) + 3,
	kDlgItemBrkStartAddress		= DLG_BASE(kDlgEditBreakpoints) + 4,
	kDlgItemBrkNumberOfBytes	= DLG_BASE(kDlgEditBreakpoints) + 5,

	kDlgItemBrkAddress			= DLG_BASE(kDlgEditCodeBreakpoint) + 0,
	kDlgItemBrkCondition		= DLG_BASE(kDlgEditCodeBreakpoint) + 1,

	kDlgItemTrcOutput			= DLG_BASE(kDlgEditTracingOptions) + 0,
	kDlgItemTrcTargetText		= DLG_BASE(kDlgEditTracingOptions) + 1,
	kDlgItemTrcTargetValue		= DLG_BASE(kDlgEditTracingOptions) + 2,
	kDlgItemTrcInfo				= DLG_BASE(kDlgEditTracingOptions) + 3,
	kDlgItemTrcAutoConnect		= DLG_BASE(kDlgEditTracingOptions) + 4,

	kDlgItemRstSoft				= DLG_BASE(kDlgReset) + 0,
	kDlgItemRstHard				= DLG_BASE(kDlgReset) + 1,
	kDlgItemRstDebug			= DLG_BASE(kDlgReset) + 2,
	kDlgItemRstNoExt			= DLG_BASE(kDlgReset) + 3,

	kDlgItemLast
};


enum EmCommonDialogFlags
{
	// Each button is stored in an 8-bit field.  A button ID is
	// stored in the lower 4-bits, and the upper 4-bits are use
	// to hold flags indicating if the button is the default button,
	// if the button is enabled, or if the button is even visible.
	// There are three of these fields, filling 24 of the 32 bits of
	// the flags parameter.

	/*
		The various buttons have different positions depending on the platform
		being run on:

		Mac/Unix:

			+----------------------------------+
			| **                               |
			| **  Blah blah blah               |
			| **                               |
			|                                  |
			|                                  |
			|          Button3 Button2 Button1 |
			+----------------------------------+

		Windows:

			+----------------------------------+
			| **                       Button1 |
			| **  Blah blah blah       Button2 |
			| **                       Button3 |
			|                                  |
			|                                  |
			|                                  |
			+----------------------------------+
	*/

	kButtonMask			= 0x0F,

	kButtonDefault		= 0x10,
	kButtonEscape		= 0x20,
	kButtonVisible		= 0x40,
	kButtonEnabled		= 0x80,

	kButtonFieldShift	= 8,
	kButtonFieldMask	= 0x000000FF,

	// The following naming convention is used for the following values:
	//
	//	An all-upper name
	//		the button is the default button (and visible and enabled)
	//
	//	A first-upper name
	//		the button is visible and enabled.
	//
	//	An all-lower name
	//		the button is visible but disabled.

#define SET_BUTTON(p, x) (((long)(x) & kButtonFieldMask) << (kButtonFieldShift * (p)))
#define GET_BUTTON(p, x) (((x) >> (kButtonFieldShift * (p))) & kButtonFieldMask)

#define SET_BUTTON_DEFAULT(p, x)	SET_BUTTON(p, (x) | kButtonVisible | kButtonEnabled | kButtonDefault)
#define SET_BUTTON_ESCAPE(p, x)		SET_BUTTON(p, (x) | kButtonVisible | kButtonEnabled | kButtonEscape)
#define SET_BUTTON_STANDARD(p, x)	SET_BUTTON(p, (x) | kButtonVisible | kButtonEnabled)
#define SET_BUTTON_DISABLED(p, x)	SET_BUTTON(p, (x) | kButtonVisible)

	kDlgFlags_None					= 0,

	kDlgFlags_OK					= SET_BUTTON_DEFAULT	(0, kDlgItemOK),

	kDlgFlags_CANCEL				= SET_BUTTON_DEFAULT	(0, kDlgItemCancel),

	kDlgFlags_OK_Cancel				= SET_BUTTON_DEFAULT	(0, kDlgItemOK)
									| SET_BUTTON_ESCAPE		(1, kDlgItemCancel),

	kDlgFlags_Ok_CANCEL				= SET_BUTTON_STANDARD	(0, kDlgItemOK)
									| SET_BUTTON_DEFAULT	(1, kDlgItemCancel),

	kDlgFlags_YES_No				= SET_BUTTON_DEFAULT	(0, kDlgItemYes)
									| SET_BUTTON_ESCAPE		(1, kDlgItemNo),

	kDlgFlags_Yes_NO				= SET_BUTTON_STANDARD	(0, kDlgItemYes)
									| SET_BUTTON_DEFAULT	(1, kDlgItemNo),

	kDlgFlags_Continue_DEBUG_Reset	= SET_BUTTON_STANDARD	(0, kDlgItemContinue)
									| SET_BUTTON_DEFAULT	(1, kDlgItemDebug)
									| SET_BUTTON_STANDARD	(2, kDlgItemReset),

	kDlgFlags_continue_DEBUG_Reset	= SET_BUTTON_DISABLED	(0, kDlgItemContinue)
									| SET_BUTTON_DEFAULT	(1, kDlgItemDebug)
									| SET_BUTTON_STANDARD	(2, kDlgItemReset),

	kDlgFlags_continue_debug_RESET	= SET_BUTTON_DISABLED	(0, kDlgItemContinue)
									| SET_BUTTON_DISABLED	(1, kDlgItemDebug)
									| SET_BUTTON_DEFAULT	(2, kDlgItemReset)
};

typedef vector<EmDlgItemID>		EmDlgItemIDList;

typedef long					EmDlgItemIndex;
typedef vector<EmDlgItemIndex>	EmDlgItemIndexList;

typedef long					EmDlgListIndex;	// Zero-based
typedef vector<EmDlgListIndex>	EmDlgListIndexList;
const EmDlgListIndex			kDlgItemListNone = -1;

struct EmDlgContext;

typedef EmDlgFnResult			(*EmDlgFn)(EmDlgContext&);
typedef void*					EmDlgRef;

struct EmDlgContext
{
						EmDlgContext (void);
						EmDlgContext (const EmDlgContext&);

	EmDlgFnResult		Init ();
	EmDlgFnResult		Event (EmDlgItemID);
	EmDlgFnResult		Idle ();
	EmDlgFnResult		PanelEnter ();
	EmDlgFnResult		PanelExit ();

	EmDlgFn				fFn;
	EmDlgFnResult		fFnResult;
	EmDlgRef			fDlg;
	EmDlgID				fDlgID;
	EmDlgPanelID		fPanelID;
	EmDlgItemID			fItemID;
	EmDlgCmdID			fCommandID;
	bool				fNeedsIdle;
	void*				fUserData;

	EmDlgItemID			fDefaultItem;
	EmDlgItemID			fCancelItem;
};

// !!! Needs to be moved elsewhere
typedef DatabaseInfoList	EmDatabaseList;
struct EditCodeBreakpointData;

class EmDlg
{
	public:
		static EmDlgItemID		DoGetFile					(EmFileRef& result,
															 const string& prompt,
															 const EmDirRef& defaultPath,
															 const EmFileTypeList& filterList);
		static EmDlgItemID		DoGetFileList				(EmFileRefList& results,
															 const string& prompt,
															 const EmDirRef& defaultPath,
															 const EmFileTypeList& filterList);
		static EmDlgItemID		DoPutFile					(EmFileRef& result,
															 const string& prompt,
															 const EmDirRef& defaultPath,
															 const EmFileTypeList& filterList,
															 const string& defaultName);
		static EmDlgItemID		DoGetDirectory				(EmDirRef& result,
															 const string& prompt,
															 const EmDirRef& defaultPath);

		static EmDlgItemID		DoAboutBox					(void);

		static EmDlgItemID		DoSessionNew				(Configuration&);
		static EmDlgItemID		DoSessionSave				(const string& docName,
															 Bool quitting);

		static EmDlgItemID		DoHordeNew					(void);

		static EmDlgItemID		DoDatabaseImport			(const EmFileRefList&,
															 EmFileImportMethod method);
		static EmDlgItemID		DoDatabaseExport			(void);
		static EmDlgItemID		DoReset						(EmResetType&);
		static EmDlgItemID		DoROMTransferQuery			(EmTransport::Config*&);
		static EmDlgItemID		DoROMTransferProgress		(EmROMTransfer&);

		static EmDlgItemID		GremlinControlOpen			(void);
		static EmDlgItemID		GremlinControlUpdate		(void);
		static EmDlgItemID		GremlinControlClose			(void);

		static EmDlgItemID		DoEditPreferences			(void);
		static EmDlgItemID		DoEditLoggingOptions		(LoggingType);
		static EmDlgItemID		DoEditDebuggingOptions		(void);
		static EmDlgItemID		DoEditSkins					(void);
		static EmDlgItemID		DoEditCardOptions			(void);
		static EmDlgItemID		DoEditBreakpoints			(void);
		static EmDlgItemID		DoEditCodeBreakpoint		(EditCodeBreakpointData&);
		static EmDlgItemID		DoEditTracingOptions		(void);

		static EmDlgItemID		DoCommonDialog				(StrCode msg,
															 EmCommonDialogFlags);
		static EmDlgItemID		DoCommonDialog				(const char* msg,
															 EmCommonDialogFlags);
		static EmDlgItemID		DoCommonDialog				(const string& msg,
															 EmCommonDialogFlags);

		static EmDlgItemID		DoSaveBound					(void);

		static EmDlgItemID		RunDialog					(EmDlgFn, void*, EmDlgID);

	private:
		static EmDlgItemID		HostRunGetFile				(EmFileRef& result,
															 const string& prompt,
															 const EmDirRef& defaultPath,
															 const EmFileTypeList& filterList);
		static EmDlgItemID		HostRunGetFileList			(EmFileRefList& results,
															 const string& prompt,
															 const EmDirRef& defaultPath,
															 const EmFileTypeList& filterList);
		static EmDlgItemID		HostRunPutFile				(EmFileRef& result,
															 const string& prompt,
															 const EmDirRef& defaultPath,
															 const EmFileTypeList& filterList,
															 const string& defaultName);
		static EmDlgItemID		HostRunGetDirectory			(EmDirRef& result,
															 const string& prompt,
															 const EmDirRef& defaultPath);

		static EmDlgItemID		HostRunSessionSave			(const string& appName,
															 const string& docName,
															 Bool quitting);

		static EmDlgItemID		HostRunDialog				(EmDlgFn, void*, EmDlgID);

	public:		// public so that PrvIdleCallback can get to it (hack?)
		static void				HostStartIdling				(EmDlgContext&);
		static void				HostStopIdling				(EmDlgContext&);

	private:
		static EmDlgFnResult	PrvAboutBox					(EmDlgContext&);
		static EmDlgFnResult	PrvSessionNew				(EmDlgContext&);
		static EmDlgFnResult	PrvHordeNew					(EmDlgContext&);
		static EmDlgFnResult	PrvDatabaseImport			(EmDlgContext&);
		static EmDlgFnResult	PrvDatabaseExport			(EmDlgContext&);
		static EmDlgFnResult	PrvReset					(EmDlgContext&);
		static EmDlgFnResult	PrvROMTransferQuery			(EmDlgContext&);
		static EmDlgFnResult	PrvROMTransferProgress		(EmDlgContext&);
		static EmDlgFnResult	PrvEditPreferences			(EmDlgContext&);
		static EmDlgFnResult	PrvEditLoggingOptions		(EmDlgContext&);
		static EmDlgFnResult	PrvEditDebuggingOptions		(EmDlgContext&);
		static EmDlgFnResult	PrvEditSkins				(EmDlgContext&);
		static EmDlgFnResult	PrvEditCardOptions			(EmDlgContext&);
		static EmDlgFnResult	PrvEditBreakpoints			(EmDlgContext&);
		static EmDlgFnResult	PrvEditCodeBreakpoint		(EmDlgContext&);
		static EmDlgFnResult	PrvEditTracingOptions		(EmDlgContext&);
		static EmDlgFnResult	PrvCommonDialog				(EmDlgContext&);
		static EmDlgFnResult	PrvSaveBound				(EmDlgContext&);

		static void				PrvBuildROMMenu				(const EmDlgContext&);
		static void				PrvBuildDeviceMenu			(const EmDlgContext&);
		static void				PrvBuildSkinMenu			(const EmDlgContext&);
		static void				PrvBuildMemorySizeMenu		(const EmDlgContext&);
		static void				PrvSetROMName				(const EmDlgContext&);

#ifdef SONY_ROM
		static void				PrvBuildMSSizeMenu			(const EmDlgContext&);
#endif

		static EmDeviceList::iterator
								PrvFilterDeviceList			(const EmFileRef& romFile, EmDeviceList& devices);

	public:	// Most of these are really "Host" functions
		static void				SetDlgBounds				(EmDlgRef, const EmRect&);
		static EmRect			GetDlgBounds				(EmDlgRef);

		static void				SetDlgDefaultButton			(EmDlgContext&, EmDlgItemID);
		static void				SetDlgCancelButton			(EmDlgContext&, EmDlgItemID);

		static void				SetItemMin					(EmDlgRef, EmDlgItemID, long max);
		static void				SetItemValue				(EmDlgRef, EmDlgItemID, long value);
		static void				SetItemMax					(EmDlgRef, EmDlgItemID, long max);
		static void				SetItemBounds				(EmDlgRef, EmDlgItemID, const EmRect&);

		static void				SetItemText					(EmDlgRef, EmDlgItemID, StrCode);
		static void				SetItemText					(EmDlgRef, EmDlgItemID, const char*);
		static void				SetItemText					(EmDlgRef, EmDlgItemID, string);

		static long				GetItemMin					(EmDlgRef, EmDlgItemID);
		static long				GetItemValue				(EmDlgRef, EmDlgItemID);
		static long				GetItemMax					(EmDlgRef, EmDlgItemID);
		static EmRect			GetItemBounds				(EmDlgRef, EmDlgItemID);
		static string			GetItemText					(EmDlgRef, EmDlgItemID);

		static void				EnableItem					(EmDlgRef, EmDlgItemID);
		static void				DisableItem					(EmDlgRef, EmDlgItemID);
		static void				EnableDisableItem			(EmDlgRef, EmDlgItemID, Bool);

		static void				ShowItem					(EmDlgRef, EmDlgItemID);
		static void				HideItem					(EmDlgRef, EmDlgItemID);
		static void				ShowHideItem				(EmDlgRef, EmDlgItemID, Bool);

		static void				ClearMenu					(EmDlgRef, EmDlgItemID);
		static void				AppendToMenu				(EmDlgRef, EmDlgItemID, const string&);
		static void				AppendToMenu				(EmDlgRef, EmDlgItemID, const StringList&);
		static void				EnableMenuItem				(EmDlgRef, EmDlgItemID, long);
		static void				DisableMenuItem				(EmDlgRef, EmDlgItemID, long);

		static void				ClearList					(EmDlgRef, EmDlgItemID);
		static void				AppendToList				(EmDlgRef, EmDlgItemID, const string&);
		static void				AppendToList				(EmDlgRef, EmDlgItemID, const StringList&);
		static void				SelectListItem				(EmDlgRef, EmDlgItemID, EmDlgListIndex);
		static void				SelectListItems				(EmDlgRef, EmDlgItemID, const EmDlgListIndexList&);
		static EmDlgListIndex	GetSelectedItem				(EmDlgRef, EmDlgItemID);
		static void				GetSelectedItems			(EmDlgRef, EmDlgItemID, EmDlgListIndexList&);

		static int				GetTextHeight				(EmDlgRef, EmDlgItemID, const string&);

		static Bool				StringToLong				(const char*, long*);

#ifdef SONY_ROM
		static void				ShowItem					(EmDlgRef, EmDlgItemID, BOOL);
#endif // SONY_ROM

};

#endif	/* EmDlg_h */
