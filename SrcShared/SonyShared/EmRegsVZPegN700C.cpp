// EmRegsVzPegN700C.cpp: EmRegsVzPegN700C クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "EmCommon.h"
#include "EmRegsVzPegN700C.h"
#include "EmRegsVZPrv.h"
#include "EmMemory.h"
#include "EmBankRegs.h"			// EmBankRegs::InvalidAccess


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

// Port B Bit settings for HiRez_CLIE

// Port C Bit settings for HiRez_CLIE
#define	hwrVZPEG700PortCBacklightOn	0x10			// (H) Backlight ON Enable
#define	hwrVZPEG700PortCKbdRow0		0x01
#define	hwrVZPEG700PortCKbdRow1		0x02
#define	hwrVZPEG700PortCKbdRow2		0x04

// Port D Bit settings for HiRez_CLIE
#define	hwrVZPEG700PortDKbdCol0		0x01
#define	hwrVZPEG700PortDKbdCol1		0x02
#define	hwrVZPEG700PortDKbdCol2		0x04
#define hwrVZPEG700PortDMS_IF_Intl	0x08			// MemoryStick Interface IC
#define	hwrVZPEG700PortDDockButton	0x10
#define hwrVZPEG700PortDPowerFail	0x80			// (L) Power Fail Interrupt	(aka IRQ6) (level, low)

// Port F Bit settings for HiRez_CLIE

// Port E Bit settings for HiRez_CLIE
#define hwrVZPEG700PortEHold		0x08			// Hold select

// Port G Bit settings for HiRez_CLIE
#define hwrVZPEG700PortMADC_CS		0x20		// ADC chip select

// Port J Bit settings for HiRez_CLIE
#define	hwrVZPEG700PortJHSYN_IRQ	0x01
#define hwrVZPEG700PortJRS232Enable	0x10		// Enable the RS232 Transceiver
#define	hwrVZPEG700PortJDC_IN		0x20		// DC_IN

// 以下未定
#define	hwrVZPEG700PortKLCDPowered	0x10

const int		kNumButtonRows = 3;
const int		kNumButtonCols = 3;

const uint16	kButtonMap[kNumButtonRows][kNumButtonCols] =
{
	{ keyBitHard1,	keyBitHard2,	keyBitHard3},
	{ keyBitPageUp,	keyBitPageDown,	0 },
	{ keyBitPower,	keyBitHard4,	keyBitJogBack}
};

// ---------------------------------------------------------------------------
//		･ EmRegsEZPalmIIIc::EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsVzPegN700C::EmRegsVzPegN700C (void) :
	EmRegsVZ (),
	fSPISlaveADC (new EmSPISlaveADS784x (kChannelSet2))
{
}


// ---------------------------------------------------------------------------
//		･ EmRegsEZPalmIIIc::~EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsVzPegN700C::~EmRegsVzPegN700C (void)
{
	delete fSPISlaveADC;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDDepth
// ---------------------------------------------------------------------------
/*
int32 EmRegsVzPegN700C::GetLCDDepth (void)
{
	return EmBankSED1375::GetLCDDepth ();
}
*/

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDRowBytes
// ---------------------------------------------------------------------------
/*
int32 EmRegsVzPegN700C::GetLCDRowBytes (void)
{
	return EmBankSED1375::GetLCDRowBytes ();
}
*/

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDWidth
// ---------------------------------------------------------------------------
/*
int32 EmRegsVzPegN700C::GetLCDWidth (void)
{
	return EmBankSED1375::GetLCDWidth ();
}
*/

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDHeight
// ---------------------------------------------------------------------------
/*
int32 EmRegsVzPegN700C::GetLCDHeight (void)
{
	return EmBankSED1375::GetLCDHeight ();
}
*/

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDStartAddr
// ---------------------------------------------------------------------------
/*
emuptr EmRegsVzPegN700C::GetLCDStartAddr (void)
{
	return EmBankSED1375::GetLCDStartAddr ();
}
*/

Bool EmRegsVzPegN700C::GetLCDHasFrame (void)
{
	// Override the Dragonball version and let the SED 1375 handle it.

	return EmHALHandler::GetLCDHasFrame ();
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDBeginEnd
// ---------------------------------------------------------------------------

void EmRegsVzPegN700C::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	// Override the Dragonball version and let the SED 1375 handle it.

	EmHALHandler::GetLCDBeginEnd (begin, end);
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDScanlines
// ---------------------------------------------------------------------------

void EmRegsVzPegN700C::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	EmHALHandler::GetLCDScanlines (info);
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegN700C::GetLCDScreenOn (void)
{
	UInt8	portKData = READ_REGISTER (portKData);
	return (portKData & hwrVZPEG700PortKLCDPowered) != 0;
//	return true;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetBacklightOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegN700C::GetLCDBacklightOn (void)
{
	return (READ_REGISTER (portCData) & hwrVZPEG700PortCBacklightOn) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegN700C::GetSerialPortOn (int /*uartNum*/)
{
	return (READ_REGISTER (portJData) & hwrVZPEG700PortJRS232Enable) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetPortInputValue
// ---------------------------------------------------------------------------

uint8 EmRegsVzPegN700C::GetPortInputValue (int port)
{
	uint8	result = EmRegsVZ::GetPortInputValue (port);

	if (port == 'K')
	{
		// Make sure this is always set, or HwrDisplayWake will hang
		result |= 0x02;
	}

	if (port == 'E')
	{
		result |= hwrVZPEG700PortEHold;
	}

	if (port == 'J')
	{
		// Make sure this is always set, or HwrDisplayWake will hang
		result |= hwrVZPEG700PortJDC_IN | hwrVZPEG700PortJHSYN_IRQ;
	}
	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetPortInternalValue
// ---------------------------------------------------------------------------

uint8 EmRegsVzPegN700C::GetPortInternalValue (int port)
{
	uint8	result = EmRegsVZ::GetPortInternalValue (port);
	
	if (port == 'D')
	{
		result = GetKeyBits ();

		// Ensure that bit hwrEZPortDDockButton is set.  If it's clear, HotSync
		// will sync via the modem instead of the serial port.
		//
		// Also make sure that hwrEZPortDPowerFail is set.  If it's clear,
		// the battery code will make the device go to sleep immediately.

		result |= hwrVZPEG700PortDDockButton | hwrVZPEG700PortDPowerFail;

		result |= hwrVZPEG700PortDMS_IF_Intl;
	}

	if (port == 'K')
	{
		// Make sure this is always set, or HwrDisplayWake will hang
		result |= 0x02;
	}

	if (port == 'E')
	{
		result |= hwrVZPEG700PortEHold;
	}

	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetKeyInfo
// ---------------------------------------------------------------------------

void EmRegsVzPegN700C::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	*numRows = kNumButtonRows;
	*numCols = kNumButtonCols;

	memcpy (keyMap, kButtonMap, sizeof (kButtonMap));

	// Determine what row is being asked for.
	UInt8	portCDir	= READ_REGISTER (portCDir);
	UInt8	portCData	= READ_REGISTER (portCData);

	rows[0]	= (portCDir & hwrVZPEG700PortCKbdRow0) != 0 && (portCData & hwrVZPEG700PortCKbdRow0) == 0;
	rows[1]	= (portCDir & hwrVZPEG700PortCKbdRow1) != 0 && (portCData & hwrVZPEG700PortCKbdRow1) == 0;
	rows[2]	= (portCDir & hwrVZPEG700PortCKbdRow2) != 0 && (portCData & hwrVZPEG700PortCKbdRow2) == 0;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetSPISlave
// ---------------------------------------------------------------------------

EmSPISlave* EmRegsVzPegN700C::GetSPISlave (void)
{
	if ((READ_REGISTER (portMData) & hwrVZPEG700PortMADC_CS) == 0)
	{
		return fSPISlaveADC;
	}

	return NULL;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetLEDState
// ---------------------------------------------------------------------------

uint16 EmRegsVzPegN700C::GetLEDState (void)
{
	uint16	result		= kLEDOff;
	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegN700C::GetVibrateOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegN700C::GetVibrateOn (void)
{
	return false;
}



#if 0
void EmRegsVzPegN700C::MSCardDetectEvent (Bool buttonIsDown)
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


