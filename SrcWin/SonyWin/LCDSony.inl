#include <ROMStubs.h>
#include "resource.h"			// IDI_EMULATOR
#include "SonyShared\SonyChars.h"			// for Sony & JogDial
#include "SonyWin\Platform_MsfsLib.h"

static Bool		gButtonTracking;

// Disable 時のボタン描画に使用するブラシ
class CPrvBrush 
{
public:
	CPrvBrush()
	{
		// ﾊｰﾌﾄｰﾝ･ﾊｯﾁﾌﾞﾗｼ の作成
		HBITMAP	hbmp;
		short	bits[8];
		for (int i=0; i<8; i++) 
			if (i%2) bits[i] = 0xAA;
			else     bits[i] = 0x55;

		hbmp = ::CreateBitmap(8, 8, 1, 1, bits);
		brsHatch = ::CreatePatternBrush(hbmp);
		::DeleteObject(hbmp);
	};

	~CPrvBrush()
	{
		if (brsHatch)
			::DeleteObject(brsHatch);
		brsHatch = NULL;
	};

	void DrawHatch(HDC hDC, LPRECT lprc)
	{
		int mode = ::SetBkMode(hDC, TRANSPARENT);
		int rop = SetROP2(hDC, R2_MASKPEN);

		HBRUSH	brsOld = (HBRUSH)::SelectObject(hDC, brsHatch);
		HPEN	penOld = (HPEN)::SelectObject(hDC, ::GetStockObject(NULL_PEN));
		::Rectangle(hDC, lprc->left, lprc->top, lprc->right, lprc->bottom);
		::SelectObject(hDC, brsOld);
		::SelectObject(hDC, penOld);

		SetROP2(hDC, rop);
		::SetBkMode(hDC, mode);
	};

private:
	HBRUSH	brsHatch;
};

#define	IsClieButton(what) \
				(what == kElement_JogUp \
			  || what == kElement_JogDown \
			  || what == kElement_JogPush \
			  || what == kElement_JogRelease \
			  || what == kElement_JogRepeat \
			  || what == kElement_JogESC \
			  || what == kElement_MS_InOut)

#define	IsLocalControlButton(what) \
				(what == kElement_JogUp \
			  || what == kElement_JogDown \
			  || what == kElement_JogPush \
			  || what == kElement_JogRelease \
			  || what == kElement_JogRepeat \
			  || what == kElement_MS_InOut)

#define	IsJogButton(what) \
				(what == kElement_JogUp \
			  || what == kElement_JogDown \
			  || what == kElement_JogPush \
			  || what == kElement_JogRelease \
			  || what == kElement_JogRepeat)


// for Sony & JogDial
#define	TIMERID_JOGKEY_REPAET_STARTER	345		// for Sony & JogDial
#define	JOGKEY_REPAET_STARTTIME			500		// for Sony & JogDial	
#define	TIMERID_JOGKEY_REPAETER			678		// for Sony & JogDial
#define	JOGKEY_REPAET_STEPTIME			125		// for Sony & JogDial

#ifdef	SONY_ROM
extern	Int16		g_nCardInserted;
#endif //SONY_ROM

static	UINT		gidJogKeyRepeat = NULL;
static	CPrvBrush	gbrsHatch;

typedef struct _tagJogButtonState
{
	SkinElementType	nButton;
	WORD			nReleaseBmpID;
	WORD			nPressBmpID;
	WORD			nDisableBmpID;
	Bool			bPress;
	Bool			bEnabled;
} JogButtonState;

#define	BUTTON_NUM	7

static	JogButtonState	gJogButtonState[BUTTON_NUM] = 
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
		hwnd = EmWindow::GetWindow()->GetHostData()->GetHostWindow();
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

static Bool PrvIsPressJogButton(SkinElementType witch)
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

static Bool PrvIsEnabledJogButton(SkinElementType witch)
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

static void CALLBACK JogKeyRepeatStarterProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	::KillTimer(hwnd, gidJogKeyRepeat);
	gidJogKeyRepeat = ::SetTimer(hwnd, TIMERID_JOGKEY_REPAETER, JOGKEY_REPAET_STEPTIME, JogKeyRepeatProc);
}

