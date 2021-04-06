#ifndef _SONY_DRAW_BUTTON_H_
#define _SONY_DRAW_BUTTON_H_

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

extern	Int16			g_nCardInserted;				// 
extern	JogButtonState	gJogButtonState[BUTTON_NUM];	// DrawButton.cpp

void	LCD_DrawButtonForPEG(HDC hDC, SkinElementType witch);
void	LCD_SetStateJogButton(SkinElementType witch, Bool bPress, Bool bEnabled);
void	LCD_InitStateJogButton();
Bool	LCD_IsPressJogButton(SkinElementType witch);
Bool	LCD_IsEnabledJogButton(SkinElementType witch);
void	CALLBACK JogKeyRepeatStarterProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);
BOOL	ControlButtonForCLIE(const EmPoint& where, Bool down);

#endif
