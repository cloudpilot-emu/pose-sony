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

#pragma once

#include "EmApplication.h"		// EmApplication

/*
	EmApplicationWin is a Windows-specific sub-class of EmApplication.  It
	is responsible for translating platform-specific application-related
	actions into cross-platform actions, making use of the the cross-
	platform EmApplication implementations.

	EmApplicationWin is also responsible for initializing, running, and
	shutting down the application in a "Windows" way.  That is, it
	initializes any Windows-specific services such as COM and WinSock,
	runs the main event loop by calling GetMessage, et al., and shuts
	things down similarly.
*/

class EmApplicationWin : public EmApplication
{
	public:
								EmApplicationWin		(void);
		virtual					~EmApplicationWin		(void);

	public:

		virtual Bool			Startup					(int argc, char** argv);
		void					Run						(void);
		virtual void			Shutdown				(void);

	public:

		HWND					GetWindow				(void) { return fAppWindow; }

	private:

		void					HandleStartupDialog		(void);
		void					HandleMainEventLoop		(void);

	public:

		virtual void			BindPoser				(Bool fullSave,
														 const EmFileRef& dest);

		virtual Bool 			ROMResourcePresent		(void);
		virtual Bool	 		GetROMResource			(Chunk&);

		virtual Bool	 		PSFResourcePresent		(void);
		virtual Bool 			GetPSFResource			(Chunk&);

		virtual Bool	 		ConfigResourcePresent	(void);
		virtual Bool 			GetConfigResource		(Chunk&);

		virtual Bool 			SkinfoResourcePresent	(void);
		virtual Bool 			GetSkinfoResource		(Chunk&);

		virtual Bool	 		Skin1xResourcePresent	(void);
		virtual Bool 			GetSkin1xResource		(Chunk&);

		virtual Bool 			Skin2xResourcePresent	(void);
		virtual Bool 			GetSkin2xResource		(Chunk&);

		virtual EmDevice		GetBoundDevice			(void);
		virtual RAMSizeType		GetBoundRAMSize			(void);

	private:	// Main window routines.

		void					PrvCreateWindow			(void);
		static LRESULT CALLBACK	PrvWndProc				(HWND hWnd,
														 UINT msg,
														 WPARAM wParam,
														 LPARAM lParam);
		void					PrvOnCommand			(HWND hWnd,
														 int id,
														 HWND hwndCtl,
														 UINT codeNotify);
		void					PrvOnDropFiles			(HWND hWnd, HDROP hdrop);
		void					PrvOnTimer				(HWND hWnd, UINT id);

	private:	// Start Menu routines.

		void					PrvProcessStartMenu		(void);
		Bool					PrvAskAboutStartMenu	(void);
		static BOOL CALLBACK	PrvStartMenuDlgProc		(HWND hWnd,
														 UINT uMsg,
														 WPARAM wParam,
														 LPARAM lParam);

	private:	// Startup Dialog routines.

		int						PrvStartupScreen		(void);
		static BOOL CALLBACK	PrvStartupDlgProc		(HWND hwnd,
														 UINT msg,
														 WPARAM wparam,
														 LPARAM lparam);

	private:	// Startup/shutdown utilities.

		void					PrvRegisterTypes		(void);
		Bool					PrvSetRegKey			(LPCTSTR lpszKey,
														 LPCTSTR lpszValue,
														 LPCTSTR lpszValueName = NULL);
		void					PrvWinSockStartup		(void);

	private:
		HWND					fAppWindow;
};

extern EmApplicationWin*		gHostApplication;
extern HINSTANCE				gInstance;
extern WORD						gWinSockVersion;

//#define TRACE_MSG
#if defined (_DEBUG) && defined (TRACE_MSG)
void _AfxTraceMsg (LPCTSTR lpszPrefix, const MSG* pMsg);
#endif
