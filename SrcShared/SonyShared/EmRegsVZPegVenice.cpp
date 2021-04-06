// EmRegsVzPegVenice.cpp: EmRegsVzPegVenice クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "EmCommon.h"
#include "EmRegsVzPegVenice.h"
#include "EmRegsVZPrv.h"
#include "EmMemory.h"
#include "EmBankRegs.h"			// EmBankRegs::InvalidAccess
#include "EmScreen.h"			// EmScreenUpdateInfo
#include "UAE_Utils.h"			// uae_memcpy


#include "EmSPISlaveADS784x.h"	// EmSPISlaveADS784x

#include "PalmPack.h"
#define NON_PORTABLE
	#include "EZAustin/IncsPrv/HardwareAustin.h"	// hwrEZPortCKbdRow0, hwrEZPortBRS232Enable, etc.
#undef NON_PORTABLE
#include "PalmPackPop.h"

#include "SonyShared\ExpansionMgr.h"
#include "SonyWin\Platform_MsfsLib.h"



////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

#define hwrVZVenicePortEAlermLED	0x08			// PE3  AlermLED

// Port B Bit settings for Venice

// Port D Bit settings for Venice
#define	hwrVZVenicePortDKbdCol0		0x01			// PD0 /
#define	hwrVZVenicePortDKbdCol1		0x02			// PD1 /
#define	hwrVZVenicePortDKbdCol2		0x04			// PD2 /
#define hwrVZVenicePortDMS_IF_Intl	0x08			// PD3 / メモリスティック挿入検出割り込み

#define	hwrVZVenicePortDDockButton	0x10			// PD4 / HotSync割り込み
#define hwrVZVenicePortDPowerFail	0x80			// PD7 / (L) Power Fail Interrupt	(aka IRQ6) (level, low)

// Port F Bit settings for Venice
#define	hwrVZVenicePortFLCDPowered	0x40			// PF6 / LCD電源制御信号

// Port G Bit settings for Venice
#define hwrVZVenicePortGADC_CS		0x08			// PG3 / ADC chip select
#define hwrVZVenicePortGRS232Enable	0x10			// PG4 / Enable the RS232 Transceiver


// Port K Bit settings for VENICE
#define hwrVZVenicePortKVibrate		0x04			// PK2  Vibrate
#define	hwrVZVenicePortKBacklightOn	0x10			// PK4 / (H) Backlight ON Enable
#define	hwrVZVenicePortKKbdRow0		0x20			// PK5 / 
#define	hwrVZVenicePortKKbdRow1		0x40			// PK6
#define	hwrVZVenicePortKKbdRow2		0x80			// PK7

// Port J Bit settings for Venice
#define	hwrVZVenicePortJDC_IN		0x04			// PJ2 : DC_IN / DCｼﾞｬｯｸ電源供給あり
#define	hwrVZVenicePortJLCDEnableOn	0x08			// PJ3 :(H) LCD Enable

// 以下未定
const int		kNumButtonRows = 3;
const int		kNumButtonCols = 3;

const uint16	kButtonMap[kNumButtonRows][kNumButtonCols] =
{
	{ keyBitHard1,	keyBitHard2,	keyBitHard3},
	{ keyBitPageUp,	keyBitPageDown,	0 },
	{ keyBitPower,	keyBitHard4,	keyBitJogBack }
};

// ---------------------------------------------------------------------------
//		･ EmRegsEZPalmIIIc::EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsVzPegVenice::EmRegsVzPegVenice (void) :
	EmRegsVZ (),
	fSPISlaveADC (new EmSPISlaveADS784x (kChannelSet2))
{
}


// ---------------------------------------------------------------------------
//		･ EmRegsEZPalmIIIc::~EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsVzPegVenice::~EmRegsVzPegVenice (void)
{
	delete fSPISlaveADC;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetLCDHasFrame
// ---------------------------------------------------------------------------

Bool EmRegsVzPegVenice::GetLCDHasFrame (void)
{
	// Override the Dragonball version and let the SED 1375 handle it.

	return EmRegsVZ::GetLCDHasFrame ();
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetLCDBeginEnd
// ---------------------------------------------------------------------------

void EmRegsVzPegVenice::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	EmRegsVZ::GetLCDBeginEnd (begin, end);
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetLCDScanlines
// ---------------------------------------------------------------------------

void EmRegsVzPegVenice::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	int32	bpp			= 1 << (READ_REGISTER (lcdPanelControl) & 0x03);
	int32	width		= READ_REGISTER (lcdScreenWidth);
	int32	height		= READ_REGISTER (lcdScreenHeight) + 1;
	int32	rowBytes	= READ_REGISTER (lcdPageWidth) * 2;
	emuptr	baseAddr	= READ_REGISTER (lcdStartAddr);

	if (width != height) height = width;

	info.fLeftMargin	= READ_REGISTER (lcdPanningOffset) & 0x0F;

	EmPixMapFormat	format	=	bpp == 1 ? kPixMapFormat1 :
								bpp == 2 ? kPixMapFormat2 :
								bpp == 4 ? kPixMapFormat4 :
								kPixMapFormat8;

	RGBList	colorTable;
	this->PrvGetPalette (colorTable);

	// Set format, size, and color table of EmPixMap.

	info.fImage.SetSize			(EmPoint (width, height));
	info.fImage.SetFormat		(format);
	info.fImage.SetRowBytes		(rowBytes);
	info.fImage.SetColorTable	(colorTable);

	// Determine first and last scanlines to fetch, and fetch them.

	info.fFirstLine		= (info.fScreenLow - baseAddr) / rowBytes;
	info.fLastLine		= (info.fScreenHigh - baseAddr - 1) / rowBytes + 1;

	if (info.fLastLine > height)
		info.fLastLine = height;

	long	firstLineOffset	= info.fFirstLine * rowBytes;
	long	lastLineOffset	= info.fLastLine * rowBytes;

	uae_memcpy (
		(void*) ((uint8*) info.fImage.GetBits () + firstLineOffset),
		baseAddr + firstLineOffset,
		lastLineOffset - firstLineOffset);
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegVenice::GetLCDScreenOn (void)
{
	UInt8	portJData = READ_REGISTER (portJData);
	return (portJData & hwrVZVenicePortJLCDEnableOn) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetBacklightOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegVenice::GetLCDBacklightOn (void)
{
	return (READ_REGISTER (portKData) & hwrVZVenicePortKBacklightOn) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegVenice::GetSerialPortOn (int /*uartNum*/)
{
	return (READ_REGISTER (portGData) & hwrVZVenicePortGRS232Enable) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetPortInputValue
// ---------------------------------------------------------------------------

uint8 EmRegsVzPegVenice::GetPortInputValue (int port)
{
	uint8	result = EmRegsVZ::GetPortInputValue (port);

	if (port == 'M')
	{
		result |= 0x00;
	}

	if (port == 'J')
	{
		result |= hwrVZVenicePortJDC_IN;
	}

	if (port == 'F')
	{
		// Make sure this is always set, or HwrDisplayWake will hang
		result |= hwrVZVenicePortFLCDPowered;
	}
	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetPortInternalValue
// ---------------------------------------------------------------------------

uint8 EmRegsVzPegVenice::GetPortInternalValue (int port)
{
	uint8	result = EmRegsVZ::GetPortInternalValue (port);
	
	if (port == 'C') {
		result |= 0x10;
	}

	if (port == 'D')
	{
		result = GetKeyBits ();

		// Ensure that bit hwrEZPortDDockButton is set.  If it's clear, HotSync
		// will sync via the modem instead of the serial port.
		//
		// Also make sure that hwrEZPortDPowerFail is set.  If it's clear,
		// the battery code will make the device go to sleep immediately.

		result |= hwrVZVenicePortDDockButton | hwrVZVenicePortDPowerFail;
	}

	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetKeyInfo
// ---------------------------------------------------------------------------

void EmRegsVzPegVenice::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	*numRows = kNumButtonRows;
	*numCols = kNumButtonCols;

	memcpy (keyMap, kButtonMap, sizeof (kButtonMap));

	// Determine what row is being asked for.
	UInt8	portKDir	= READ_REGISTER (portKDir);
	UInt8	portKData	= READ_REGISTER (portKData);

	rows[0]	= (portKDir & hwrVZVenicePortKKbdRow0) != 0 && (portKData & hwrVZVenicePortKKbdRow0) == 0;
	rows[1]	= (portKDir & hwrVZVenicePortKKbdRow1) != 0 && (portKData & hwrVZVenicePortKKbdRow1) == 0;
	rows[2]	= (portKDir & hwrVZVenicePortKKbdRow2) != 0 && (portKData & hwrVZVenicePortKKbdRow2) == 0;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetSPISlave
// ---------------------------------------------------------------------------

EmSPISlave* EmRegsVzPegVenice::GetSPISlave (void)
{
	if ((READ_REGISTER (portGData) & hwrVZVenicePortGADC_CS) == 0)
	{
		return fSPISlaveADC;
	}

	return NULL;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetLEDState
// ---------------------------------------------------------------------------

uint16 EmRegsVzPegVenice::GetLEDState (void)
{
	uint16	result		= kLEDOff;
	UInt8	portEData	= READ_REGISTER (portEData);

	if (portEData & hwrVZVenicePortEAlermLED)
		result |= (kLEDRed | kLEDGreen);
	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegVenice::GetVibrateOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegVenice::GetVibrateOn (void)
{
	UInt8	portKData	= READ_REGISTER (portKData);
	return (portKData & hwrVZVenicePortKVibrate);
}


#if 0
void EmRegsVzPegVenice::MSCardDetectEvent (Bool buttonIsDown)
{
	// If the button changes state, set or clear the MS-Card detect interrupt.
#if 1
	if (buttonIsDown)
	{
		ExpCardInserted(1);
	}
	else
	{
		ExpCardRemoved(1);
	}
#else
	fgPortDEdge = 8;
	UInt8	portDData	 = READ_REGISTER (portDData);
	uint16	intPendingLo = READ_REGISTER (intPendingLo);
	intPendingLo |= hwrEZ328IntLoInt3;

	if (buttonIsDown)
	{
		portDData &= ~hwrVZHzCliePortDMsIns;
	}
	else
	{
		portDData |= hwrVZHzCliePortDMsIns;
	}
	WRITE_REGISTER (portDData, portDData);
	WRITE_REGISTER (intPendingLo, intPendingLo);

	EmRegsEZ::UpdateInterrupts ();
#endif
}

#endif


