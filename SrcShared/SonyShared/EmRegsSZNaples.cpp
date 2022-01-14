// EmRegsSzNaples.cpp: EmRegsSzNaples �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "EmCommon.h"
#include "EmRegsSZNaples.h"
#include "EmRegsSZPrv.h"
#include "EmMemory.h"
#include "EmBankRegs.h"			// EmBankRegs::InvalidAccess


#include "EmSPISlaveADS784x.h"	// EmSPISlaveADS784x

#include "PalmPack.h"
#include "PalmPackPop.h"

#include "SonyShared\SonyKeyMgr.h"
#include "SonyShared\ExpansionMgr.h"
#include "SonyWin\Platform_MsfsLib.h"

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

// Port C Bit settings for Naples
#define	hwrSZNaplesPortCKbdRow0			0x01		// PC0: KB_ROW0
#define	hwrSZNaplesPortCKbdRow1			0x02		// PC1: KB_ROW1
#define	hwrSZNaplesPortCKbdRow2			0x04		// PC2: KB_ROW2

// Port D Bit settings for Naples
#define	hwrSZNaplesPortDKbdCol0			0x01		// PD0: KB_COL0
#define	hwrSZNaplesPortDKbdCol1			0x02		// PD1: KB_COL1
#define	hwrSZNaplesPortDKbdCol2			0x04		// PD2: KB_COL2

#define	hwrSZNaplesPortDDockButton		0x10		// PD4: HSYN_IRQ
#define hwrSZNaplesPortDPowerFail		0x80		// PD7: LOWB_IRQ (L) Power Fail Interrupt	(aka IRQ6) (level, low)

// Port F Bit settings for Naples
#define hwrSZNaplesPortFRS232Enable		0x01		// PF0: RS232C_SD Enable the RS232 Transceiver

// Port G Bit settings for Naples
#define hwrSZNaplesPortGADC_CNVST		0x20		// PG5: ADC Conversion Start

// Port K Bit settings for Naples
#define	hwrSZNaplesPortKBacklightOn		0x40		// PK6: LIGHT_ON Backlight ON Enable

// Port J Bit settings for Naples

// Port N Bit settings for Naples
#define hwrSZNaplesPortNMS_INS			0x10		// PN4: MS_INS �������X�e�B�b�N�}�����o���荞��(IRQ3)
#define hwrSZNaplesPortNAlermLED		0x20		// PN5: ATNLED_ON AlermLED
#define	hwrSZNaplesPortNVibrate			0x80		// PN7: VIBRATE �U�����[�^�[

// Port P Bit settings for Naples
#define	hwrSZNaplesPortPDC_IN			0x80		// PP7 : DC_IN / DC�ެ���d����������
//#define hwrSZNaplesPortPADC_CNVST		0x40		// PP6 : ADC Conversion Start

// �ȉ�����
const int		kNumButtonRows = 3;
const int		kNumButtonCols = 3;

const uint16	kButtonMap[kNumButtonRows][kNumButtonCols] =
{
	{ keyBitHard1,	keyBitHard2,	keyBitHard3},
	{ keyBitPageUp,	keyBitPageDown,	0 },
	{ keyBitPower,	keyBitHard4,	keyBitJogBack }
};


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::EmRegsSzNaples
// ---------------------------------------------------------------------------

EmRegsSzNaples::EmRegsSzNaples (void) :
	EmRegsSZ (),
	fSPISlaveADC (new EmSPISlaveADS784x (kChannelSet2))
{
}


// ---------------------------------------------------------------------------
//		� EmRegsEZPalmIIIc::~EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsSzNaples::~EmRegsSzNaples (void)
{
	delete fSPISlaveADC;
}

// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetLCDHasFrame
// ---------------------------------------------------------------------------

Bool EmRegsSzNaples::GetLCDHasFrame (void)
{
	// Override the Dragonball version and let the SED 1375 handle it.
	return EmRegsSZ::GetLCDHasFrame ();
}

// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetLCDBeginEnd
// ---------------------------------------------------------------------------

void EmRegsSzNaples::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	EmRegsSZ::GetLCDBeginEnd (begin, end);
}

// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetLCDScanlines
// ---------------------------------------------------------------------------

void EmRegsSzNaples::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	EmRegsSZ::GetLCDScanlines (info);
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmRegsSzNaples::GetLCDScreenOn (void)
{
/*
	UInt8	portBData = READ_REGISTER (portBData);
	return (portBData & 0x01) != 0;
*/
	return true;
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetBacklightOn
// ---------------------------------------------------------------------------

Bool EmRegsSzNaples::GetLCDBacklightOn (void)
{
	return (READ_REGISTER (portKData) & hwrSZNaplesPortKBacklightOn) != 0;
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmRegsSzNaples::GetSerialPortOn (int /*uartNum*/)
{
	return (READ_REGISTER (portFData) & hwrSZNaplesPortFRS232Enable) != 0;
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetPortInputValue
// ---------------------------------------------------------------------------

uint8 EmRegsSzNaples::GetPortInputValue (int port)
{
	uint8	result = EmRegsSZ::GetPortInputValue (port);

	if (port == 'P')
	{
		result |= hwrSZNaplesPortPDC_IN;
	}

	if (port == 'F')
	{
		// Make sure this is always set, or HwrDisplayWake will hang
//		result |= hwrVZNascaPortFLCDPowered;
	}

	return result;
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetPortInternalValue
// ---------------------------------------------------------------------------

uint8 EmRegsSzNaples::GetPortInternalValue (int port)
{
	uint8	result = EmRegsSZ::GetPortInternalValue (port);
	
	if (port == 'D')
	{
		result = GetKeyBits ();

		// Ensure that bit hwrEZPortDDockButton is set.  If it's clear, HotSync
		// will sync via the modem instead of the serial port.
		//
		// Also make sure that hwrEZPortDPowerFail is set.  If it's clear,
		// the battery code will make the device go to sleep immediately.

		result |= hwrSZNaplesPortDDockButton | hwrSZNaplesPortDPowerFail;
	}

	if (port == 'C')
	{
//		result = ~result & 0x07;
	}

	if (port == 'G') 
	{
		result |= 0x02;		// LCD_FLIP
	}	

	if (port == 'J') 
	{
		result |= 0xFF;		// KBD_COL0-6,LCD_ROTATE
	}	

	if (port == 'P')
	{
		result |= 0x08;		// AREM_PUSH
		result |= 0x02;		// HOLD
	}

	return result;
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetKeyInfo
// ---------------------------------------------------------------------------

void EmRegsSzNaples::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	*numRows = kNumButtonRows;
	*numCols = kNumButtonCols;

	memcpy (keyMap, kButtonMap, sizeof (kButtonMap));

	// Determine what row is being asked for.
	UInt8	portCDir	= READ_REGISTER (portCDir);
	UInt8	portCData	= READ_REGISTER (portCData);

	rows[0]	= (portCDir & hwrSZNaplesPortCKbdRow0) != 0 && (portCData & hwrSZNaplesPortCKbdRow0) == 0;
	rows[1]	= (portCDir & hwrSZNaplesPortCKbdRow1) != 0 && (portCData & hwrSZNaplesPortCKbdRow1) == 0;
	rows[2]	= (portCDir & hwrSZNaplesPortCKbdRow2) != 0 && (portCData & hwrSZNaplesPortCKbdRow2) == 0;
}

// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetSPISlave
// ---------------------------------------------------------------------------

EmSPISlave* EmRegsSzNaples::GetSPISlave (void)
{
	if ((READ_REGISTER (portGData) & hwrSZNaplesPortGADC_CNVST) == 0)
	{
		return fSPISlaveADC;
	}

	return NULL;
}

// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetLEDState
// ---------------------------------------------------------------------------

uint16 EmRegsSzNaples::GetLEDState (void)
{
	uint16	result		= kLEDOff;
	UInt8	portNData	= READ_REGISTER (portNData);

	if (portNData & hwrSZNaplesPortNAlermLED)
		result |= (kLEDRed | kLEDGreen);

	return result;
}


// ---------------------------------------------------------------------------
//		� EmRegsSzNaples::GetVibrateOn
// ---------------------------------------------------------------------------

Bool EmRegsSzNaples::GetVibrateOn (void)
{
	UInt8	portNData	= READ_REGISTER (portNData);
	return (portNData & hwrSZNaplesPortNVibrate);
}
