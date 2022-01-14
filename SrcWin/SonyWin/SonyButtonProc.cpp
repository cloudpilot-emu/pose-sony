#include "EmCommon.h"
#include "EmWindow.h"
#include "EmWindowWin.h"
#include "EmRegion.h"			// EmRegion
#include "EmApplicationWin.h"
#include "EmSession.h"			// EmSessionStopper

#include "EmSubroutine.h"

#include <ROMStubs.h>
#include "resource.h"			// IDI_EMULATOR
#include "SonyShared\SonyChars.h"			// for Sony & JogDial
#include "SonyWin\Platform_MsfsLib.h"
#include "SonyWin\Platform_ExpMgrLib.h"
#include "SonyButtonProc.h"

static	CPrvBrush		gbrsHatch;
static	UINT			gidJogKeyRepeat = NULL;		// タイマーID
static  Bool			gButtonTracking = false;
static	SkinElementType	gCurrentButton;

#ifdef SONY_ROM
static	UINT			gIdPublicKeyUp = NULL;		// タイマーID
static	UINT			gIdPublicKeyDown = NULL;	// タイマーID
#endif //SONY_ROM

JogButtonState	gJogButtonState[BUTTON_NUM] = 
{
	// nButton,				ReleaseBitmap		PressBitmap			DisableBitmap		bPress,		bEnabled,	
	kElement_JogPush,		IDB_PUSH_RELEASE,	IDB_PUSH_PRESS,		NULL,				false,		true,		
	kElement_JogRepeat,		IDB_REPEAT_RELEASE,	IDB_REPEAT_PRESS,	IDB_REPEAT_DISABLE, false,		false,		
	kElement_JogUp,			IDB_UP_RELEASE,		IDB_UP_PRESS,		NULL,				false,		true,		
	kElement_JogDown,		IDB_DOWN_RELEASE,	IDB_DOWN_PRESS,		NULL,				false,		true,		
	kElement_MS_InOut,		IDB_MS_RELEASE,		IDB_MS_PRESS,		IDB_MS_DISABLE,		true,		true,		
	kElement_JogESC,		IDB_ESC_RELEASE,	IDB_ESC_PRESS,		NULL,				false,		true,		
	kElement_PowerButton,	IDB_POWER_RELEASE,	IDB_POWER_PRESS,	NULL,				true,		true,		
};

void	LCD_DrawButtonForPEG(HDC hDC, SkinElementType witch)
{
	HDC		dstDC = hDC;
	HWND	hwnd = NULL;
	if (!hDC)
	{

		hwnd = gHostWindow->GetHwnd();
		if (!hwnd)
			return ;

		dstDC = ::GetDC(hwnd);
	}

	JogButtonState	*pTarget = NULL;
	for (int i=0; i<BUTTON_NUM; i++)
	{
		if (witch == gJogButtonState[i].nButton)
			pTarget = &gJogButtonState[i];
	}
	if (!pTarget)
		return;

	RECT	rc;
	if (!SkinGetElementRect(witch, &rc))
		return;

	WORD			bmpID = NULL;
	if (pTarget->nReleaseBmpID && !pTarget->bPress && pTarget->bEnabled)
		bmpID = pTarget->nReleaseBmpID;
	if (pTarget->nPressBmpID && pTarget->bPress && pTarget->bEnabled)
		bmpID = pTarget->nPressBmpID;
	else if (pTarget->nDisableBmpID && !pTarget->bEnabled)
		bmpID = pTarget->nDisableBmpID;

	if (bmpID)
	{
		HBITMAP	bmpButton = ::LoadBitmap(gInstance, MAKEINTRESOURCE(bmpID));
		HDC 	srcDC = ::CreateCompatibleDC (dstDC);
		HBITMAP	oldBitmap = SelectBitmap (srcDC, bmpButton);
		BITMAP	bmp;
		::GetObject(bmpButton, sizeof(BITMAP), &bmp);
		::StretchBlt(dstDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
					 srcDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
		if (oldBitmap)
			SelectBitmap(srcDC, oldBitmap);
		DeleteBitmap(bmpButton);
		::DeleteDC(srcDC);
	}

	if (!hDC && hwnd)
		::ReleaseDC(hwnd, dstDC);
}

void	LCD_SetStateJogButton(SkinElementType witch, Bool bPress, Bool bEnabled)
{
	JogButtonState	*pTarget = NULL;
	for (int i=0; i<BUTTON_NUM; i++)
	{
		if (witch == gJogButtonState[i].nButton)
			pTarget = &gJogButtonState[i];
	}
	if (!pTarget)
		return;
	pTarget->bPress   = bPress;
	pTarget->bEnabled = bEnabled;
}

Bool LCD_IsPressJogButton(SkinElementType witch)
{
	JogButtonState	*pTarget = NULL;
	for (int i=0; i<BUTTON_NUM; i++)
	{
		if (witch == gJogButtonState[i].nButton)
			pTarget = &gJogButtonState[i];
	}
	if (!pTarget)
		return false;

	return (Bool)(pTarget->bPress);
}

Bool LCD_IsEnabledJogButton(SkinElementType witch)
{
	JogButtonState	*pTarget = NULL;
	for (int i=0; i<BUTTON_NUM; i++)
	{
		if (witch == gJogButtonState[i].nButton)
			pTarget = &gJogButtonState[i];
	}
	if (!pTarget)
		return false;

	return (Bool)(pTarget->bEnabled);
}

void	LCD_InitStateJogButton()
{
	for (int i=0; i<BUTTON_NUM; i++)
	{
		gJogButtonState[i].bPress   = (gJogButtonState[i].nButton != kElement_MS_InOut && gJogButtonState[i].nButton != kElement_PowerButton) ? false : true;
		gJogButtonState[i].bEnabled = (gJogButtonState[i].nButton == kElement_JogRepeat) ? false : true;
	}
}

/////////////////////////////////////////////////////////////////
//  vchrJogPressRepeatの繰り返し発行
static void CALLBACK JogKeyRepeatProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	EmSessionStopper	stopper (gSession, kStopOnSysCall);
	if (stopper.Stopped ()) {
			::EvtEnqueueKey(vchrJogPressRepeat, 0, commandKeyMask);	
	}
}

void CALLBACK JogKeyRepeatStarterProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	::KillTimer(hwnd, gidJogKeyRepeat);
	gidJogKeyRepeat = ::SetTimer(hwnd, TIMERID_JOGKEY_REPAETER, JOGKEY_REPAET_STEPTIME, JogKeyRepeatProc);
}

#ifdef SONY_ROM
/////////////////////////////////////////////////////////////////
//  pageUpChr&pageDownChrの繰り返し発行
static void CALLBACK PublicKeyUpProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	EmSessionStopper	stopper (gSession, kStopOnSysCall);
	if (stopper.Stopped ()) {
			::EvtEnqueueKey(pageUpChr, 0, commandKeyMask);	
	}
}

void CALLBACK PublicKeyUpStarterProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	::KillTimer(hwnd, gIdPublicKeyUp);
	gIdPublicKeyUp = ::SetTimer(hwnd, TIMERID_JOGKEY_REPAETER, JOGKEY_REPAET_STEPTIME, PublicKeyUpProc);
}

static void CALLBACK PublicKeyDownProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	EmSessionStopper	stopper (gSession, kStopOnSysCall);
	if (stopper.Stopped ()) {
			::EvtEnqueueKey(pageDownChr, 0, commandKeyMask);	
	}
}

void CALLBACK PublicKeyDownStarterProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	::KillTimer(hwnd, gIdPublicKeyDown);
	gIdPublicKeyDown = ::SetTimer(hwnd, TIMERID_JOGKEY_REPAETER, JOGKEY_REPAET_STEPTIME, PublicKeyDownProc);
}
#endif //SONY_ROM


BOOL ControlButtonForCLIE(const EmPoint& where, Bool down)
{
	SkinElementType	what;

	what = ::SkinTestPoint (where);

	if ((down && IsLocalControlButton(what)) || (!down && IsLocalControlButton(gCurrentButton)))
	{
		if (gButtonTracking)
		{
			gButtonTracking = down;
			if (!gButtonTracking)
			{
				::ReleaseCapture ();
				if (IsJogButton(gCurrentButton) && gCurrentButton != kElement_JogPush)	// for Sony & JogDial
				{
					if (gCurrentButton == kElement_JogRepeat) 
					{
						if (gidJogKeyRepeat) 
						{
							::KillTimer(gHostWindow->GetHwnd(), gidJogKeyRepeat);
							gidJogKeyRepeat = NULL;
						}
						::LCD_SetStateJogButton(gCurrentButton, false, true);
						::LCD_DrawButtonForPEG(NULL, gCurrentButton);
					}
					else
					{
						::LCD_SetStateJogButton(gCurrentButton, false, true);
						::LCD_DrawButtonForPEG(NULL, gCurrentButton);
					}
				}
			}
			return true;
		}
		else if (down)
		{
			gButtonTracking = down;
			// press JogDial button.	
			if (IsJogButton(what))				// for Sony & JogDial
			{
				// Power On/Off状態のチェック
				Bool lcdOn = EmHAL::GetLCDScreenOn ();
				if (!lcdOn)
					return true;		// Power Off 状態

				if (what == kElement_JogRepeat && !::LCD_IsEnabledJogButton(what))
					return true;

				::SetCapture (gHostWindow->GetHwnd());
				if (what == kElement_JogRepeat)
				{
					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped ())
						::EvtEnqueueKey(vchrJogPressRepeat, 0, commandKeyMask);	// for Sony & JogDial

					gidJogKeyRepeat = ::SetTimer(gHostWindow->GetHwnd(), TIMERID_JOGKEY_REPAET_STARTER, JOGKEY_REPAET_STARTTIME, JogKeyRepeatStarterProc);
					::LCD_SetStateJogButton(what, true, true);
				}
				else if (what == kElement_JogPush)
				{
					if (::LCD_IsPressJogButton(what))
					{
						::LCD_SetStateJogButton(kElement_JogPush, false, true);
						::LCD_DrawButtonForPEG(NULL, kElement_JogPush);
	
						::LCD_SetStateJogButton(kElement_JogRepeat, false, false);
						::LCD_DrawButtonForPEG(NULL, kElement_JogRepeat);

						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							::EvtEnqueueKey(vchrJogRelease, 0, commandKeyMask);	// for Sony & JogDial
					}
					else
					{
						::LCD_SetStateJogButton(kElement_JogRepeat, false, true);
						::LCD_DrawButtonForPEG(NULL, kElement_JogRepeat);

						::LCD_SetStateJogButton(kElement_JogPush, true, true);

						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							::EvtEnqueueKey(vchrJogPress, 0, commandKeyMask);	// for Sony & JogDial
					}
				}
				else
				{
					UInt16	nKey = 0;
					switch (what)
					{
					case kElement_JogUp: nKey = (::LCD_IsPressJogButton(kElement_JogPush)) ? vchrJogPageUp : vchrJogUp; break;
					case kElement_JogDown: nKey = (::LCD_IsPressJogButton(kElement_JogPush)) ? vchrJogPageDown : vchrJogDown; break;
					}

					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped ())
						::EvtEnqueueKey(nKey, 0, commandKeyMask);	// for Sony & JogDial

					::LCD_SetStateJogButton(what, true, true);
				}
				gCurrentButton = what;	// for Sony & JogDial
				gButtonTracking = TRUE;	// for Sony & JogDial

				::LCD_DrawButtonForPEG(NULL, gCurrentButton);
				return true;
			}

			// Press MS-Card[ON/OFF] button on PEG's skin
			else if (what == kElement_MS_InOut) 
			{
				// Power On/Off状態のチェック
				Bool lcdOn = EmHAL::GetLCDScreenOn ();

				::SetCapture (gHostWindow->GetHwnd());	// for Sony & MemoryStick

				gCurrentButton = what;	// for Sony & MemoryStick
				gButtonTracking = TRUE;	// for Sony & MemoryStick

				if (g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE)
				{
					if (!lcdOn || !Platform_ExpMgrLib::IsUsable())		// PowerOffまたはReset処理中の状態
						g_nCardInserted = MSSTATE_REQ_INSERT;			// ExpCardInserted 要求
					else
					{
						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							ExpCardInserted(1);
					}
				}
				else
				{
					if (!lcdOn || !Platform_ExpMgrLib::IsUsable())		// PowerOffまたはReset処理中の状態
						g_nCardInserted = MSSTATE_REQ_REMOVE;			// ExpCardRemoved 要求
					else
					{
						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							::ExpCardRemoved(1);
					}
				}

				::LCD_SetStateJogButton(kElement_MS_InOut, (g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE) ? false : true, true);
				::LCD_DrawButtonForPEG(NULL, kElement_MS_InOut);

				return true;
			}
		}
	}
	else if ((down && what == kElement_JogESC) || (!down && gCurrentButton == kElement_JogESC))
	{
		gCurrentButton = what;	// for Sony & JogDial
		gButtonTracking = down;	// for Sony & JogDial/
		::LCD_SetStateJogButton(kElement_JogESC, down, true);
		::LCD_DrawButtonForPEG(NULL, kElement_JogESC);
	}
	return false;
}

#ifdef SONY_ROM
//
//	For Redwood & Naples
//
#include "EmCPU68K.h"
PublicControlButtonForCLIE(const EmPoint& where, Bool down)
{
	SkinElementType	what;

	what = ::SkinTestPoint (where);

	if ((down && IsPublicControlButton(what)) || (!down && IsPublicControlButton(gCurrentButton)))
	{
		if (gButtonTracking)
		{
			gButtonTracking = down;

			if (!gButtonTracking)
			{
				::ReleaseCapture ();

				if (gCurrentButton == kElement_JogESC)
				{
					::LCD_SetStateJogButton(gCurrentButton, false, true);
					::LCD_DrawButtonForPEG(NULL, gCurrentButton);

					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped ())
						::EvtEnqueueKey(alarmChr, 0, commandKeyMask);
						::EvtEnqueueKey(vchrJogBack, 0, commandKeyMask);
//					if (stopper.Stopped ())
//						::EvtEnqueueKey(alarmChr, 0, commandKeyMask);
//						::EvtEnqueueKey(launchChr, 0, commandKeyMask);
				}
				else if (gCurrentButton == kElement_UpButton)
				{
					if (gIdPublicKeyUp)
					{
						::KillTimer(gHostWindow->GetHwnd(), gIdPublicKeyUp);
						gIdPublicKeyUp = NULL;
					}
				}
				else if(gCurrentButton == kElement_DownButton)
				{
					if(gIdPublicKeyDown)
					{
						::KillTimer(gHostWindow->GetHwnd(), gIdPublicKeyDown);
						gIdPublicKeyDown = NULL;
					}
				}
			}
			return false;
		}
		else if (down)
		{
			gButtonTracking = down;

			// Power On/Off状態のチェック
			Bool lcdOn = EmHAL::GetLCDScreenOn ();
			if (!lcdOn && what != kElement_PowerButton)
				return true;		// Power Off 状態

			::SetCapture (gHostWindow->GetHwnd());

			if( what == kElement_PowerButton )
			{
				if(lcdOn)
				{
					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped ())
						::SysTaskSleep(false,false);
//						::EvtEnqueueKey(alarmChr, 0, commandKeyMask);
//						::EvtEnqueueKey(vchrAutoOff, 0, commandKeyMask);
//						::EvtEnqueueKey(hardPowerChr, 0, commandKeyMask);
//						::EvtEnqueueKey(vchrLateWakeup, 0, commandKeyMask);

					return false;
				}
				else
				{
//					gSession->Reset(kResetSoft);
//					::EvtWakeup();
//					EmSessionStopper	stopper (gSession, kStopNow);
//					if (stopper.Stopped ())
//					::EvtEnqueueKey(hardPowerChr, 0, poweredOnKeyMask);
//					UInt32 taskid = SysGetTaskID();
//					::EvtEnqueueKey(vchrLateWakeup, 0, commandKeyMask);
//					::SysTaskWakeup();
//					::EvtWakeup();
					return false;
				}
			}
			else if( what ==  kElement_UpButton )
			{
				EmSessionStopper	stopper (gSession, kStopOnSysCall);
				if (stopper.Stopped ())
					::EvtEnqueueKey(pageUpChr, 0, commandKeyMask);

				gIdPublicKeyUp = ::SetTimer(
									gHostWindow->GetHwnd()
									, TIMERID_JOGKEY_REPAET_STARTER
									, JOGKEY_REPAET_STARTTIME
									, PublicKeyUpStarterProc
								);
			}
			else if( what == kElement_DownButton )
			{
				EmSessionStopper	stopper (gSession, kStopOnSysCall);
				if (stopper.Stopped ())
					::EvtEnqueueKey(pageDownChr, 0, commandKeyMask);

				gIdPublicKeyDown = ::SetTimer(
										gHostWindow->GetHwnd()
										, TIMERID_JOGKEY_REPAET_STARTER
										, JOGKEY_REPAET_STARTTIME
										, PublicKeyDownStarterProc
									);
			}
			else if( what ==  kElement_App1Button )
			{
				EmSessionStopper	stopper (gSession, kStopOnSysCall);
				if (stopper.Stopped ())
					::EvtEnqueueKey(hard1Chr, 0, commandKeyMask);
			}
			else if( what == kElement_App2Button ) 
			{
				EmSessionStopper	stopper (gSession, kStopOnSysCall);
				if (stopper.Stopped ())
					::EvtEnqueueKey(hard2Chr, 0, commandKeyMask);
			}	
			else if( what == kElement_App3Button ) 
			{
				EmSessionStopper	stopper (gSession, kStopOnSysCall);
				if (stopper.Stopped ())
					::EvtEnqueueKey(hard3Chr, 0, commandKeyMask);
			}
			else if( what == kElement_App4Button ) 
			{
				EmSessionStopper	stopper (gSession, kStopOnSysCall);
				if (stopper.Stopped ())
					::EvtEnqueueKey(hard4Chr, 0, commandKeyMask);
			}
			else if( what ==  kElement_JogESC ) 
			{
				::LCD_SetStateJogButton(kElement_JogESC, down, true);
				::LCD_DrawButtonForPEG(NULL, kElement_JogESC);
			}
			else 
			{
				return false;
			}
			
			gCurrentButton = what;
			gButtonTracking = TRUE;

			return true;
		}
	}

	return false;
}
#endif //SONY_ROM