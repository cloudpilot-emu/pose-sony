// EmRegsVzPegNasca.cpp: EmRegsVzPegNasca クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "EmCommon.h"
#include "EmRegsVzPegNasca.h"
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

// Port B Bit settings for NASCA
#define	hwrVZNascaPortBDC_IN		0x40

// Port C Bit settings for NASCA
#define	hwrVZNascaPortCLCDEnableOn	0x80			// (H) LCD Enable

// Port D Bit settings for NASCA
#define	hwrVZNascaPortDKbdCol0		0x01
#define	hwrVZNascaPortDKbdCol1		0x02
#define	hwrVZNascaPortDKbdCol2		0x04
#define hwrVZNascaPortDMS_IF_Intl	0x08			// MemoryStick Interface IC

#define	hwrVZNascaPortDDockButton	0x10
#define hwrVZNascaPortDPowerFail	0x80			// (L) Power Fail Interrupt	(aka IRQ6) (level, low)

// Port F Bit settings for NASCA
#define	hwrVZNascaPortFLCDPowered	0x40

// Port G Bit settings for NASCA
#define hwrVZNascaPortGADC_CS		0x08			// ADC chip select
#define hwrVZNascaPortGRS232Enable	0x10			// Enable the RS232 Transceiver


// Port K Bit settings for NASCA
#define	hwrVZNascaPortKBacklightOn	0x10			// (H) Backlight ON Enable
#define	hwrVZNascaPortKKbdRow0		0x20
#define	hwrVZNascaPortKKbdRow1		0x40
#define	hwrVZNascaPortKKbdRow2		0x80

// 以下未定
const int		kNumButtonRows = 3;
const int		kNumButtonCols = 3;

const uint16	kButtonMap[kNumButtonRows][kNumButtonCols] =
{
	{ keyBitHard1,	keyBitHard2,	keyBitHard3},
	{ keyBitPageUp,	keyBitPageDown,	0 },
	{ keyBitPower,	keyBitHard4,	0 }
};

// ---------------------------------------------------------------------------
//		･ EmRegsEZPalmIIIc::EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsVzPegNasca::EmRegsVzPegNasca (void) :
	EmRegsVZ (),
	fSPISlaveADC (new EmSPISlaveADS784x (kChannelSet2))
{
}


// ---------------------------------------------------------------------------
//		･ EmRegsEZPalmIIIc::~EmRegsEZPalmIIIc
// ---------------------------------------------------------------------------

EmRegsVzPegNasca::~EmRegsVzPegNasca (void)
{
	delete fSPISlaveADC;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetLCDHasFrame
// ---------------------------------------------------------------------------

Bool EmRegsVzPegNasca::GetLCDHasFrame (void)
{
	// Override the Dragonball version and let the SED 1375 handle it.

	return EmRegsVZ::GetLCDHasFrame ();
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetLCDBeginEnd
// ---------------------------------------------------------------------------

void EmRegsVzPegNasca::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	EmRegsVZ::GetLCDBeginEnd (begin, end);
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetLCDScanlines
// ---------------------------------------------------------------------------

void EmRegsVzPegNasca::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	EmRegsVZ::GetLCDScanlines (info);
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegNasca::GetLCDScreenOn (void)
{
	UInt8	portCData = READ_REGISTER (portCData);
	return (portCData & hwrVZNascaPortCLCDEnableOn) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetBacklightOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegNasca::GetLCDBacklightOn (void)
{
	return (READ_REGISTER (portKData) & hwrVZNascaPortKBacklightOn) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetSerialPortOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegNasca::GetSerialPortOn (int /*uartNum*/)
{
	return (READ_REGISTER (portGData) & hwrVZNascaPortGRS232Enable) != 0;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetPortInputValue
// ---------------------------------------------------------------------------

uint8 EmRegsVzPegNasca::GetPortInputValue (int port)
{
	uint8	result = EmRegsVZ::GetPortInputValue (port);

	if (port == 'B')
	{
		result |= hwrVZNascaPortBDC_IN;
	}

	if (port == 'F')
	{
		// Make sure this is always set, or HwrDisplayWake will hang
		result |= hwrVZNascaPortFLCDPowered;
	}
return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetPortInternalValue
// ---------------------------------------------------------------------------

uint8 EmRegsVzPegNasca::GetPortInternalValue (int port)
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

		result |= hwrVZNascaPortDDockButton | hwrVZNascaPortDPowerFail;
	}

	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetKeyInfo
// ---------------------------------------------------------------------------

void EmRegsVzPegNasca::GetKeyInfo (int* numRows, int* numCols,
								uint16* keyMap, Bool* rows)
{
	*numRows = kNumButtonRows;
	*numCols = kNumButtonCols;

	memcpy (keyMap, kButtonMap, sizeof (kButtonMap));

	// Determine what row is being asked for.
	UInt8	portKDir	= READ_REGISTER (portKDir);
	UInt8	portKData	= READ_REGISTER (portKData);

	rows[0]	= (portKDir & hwrVZNascaPortKKbdRow0) != 0 && (portKData & hwrVZNascaPortKKbdRow0) == 0;
	rows[1]	= (portKDir & hwrVZNascaPortKKbdRow1) != 0 && (portKData & hwrVZNascaPortKKbdRow1) == 0;
	rows[2]	= (portKDir & hwrVZNascaPortKKbdRow2) != 0 && (portKData & hwrVZNascaPortKKbdRow2) == 0;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetSPISlave
// ---------------------------------------------------------------------------

EmSPISlave* EmRegsVzPegNasca::GetSPISlave (void)
{
	if ((READ_REGISTER (portGData) & hwrVZNascaPortGADC_CS) == 0)
	{
		return fSPISlaveADC;
	}

	return NULL;
}

// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetLEDState
// ---------------------------------------------------------------------------

uint16 EmRegsVzPegNasca::GetLEDState (void)
{
	uint16	result		= kLEDOff;
	return result;
}


// ---------------------------------------------------------------------------
//		･ EmRegsVzPegNasca::GetVibrateOn
// ---------------------------------------------------------------------------

Bool EmRegsVzPegNasca::GetVibrateOn (void)
{
	return false;
}



#if 0
void EmRegsVzPegNasca::MSCardDetectEvent (Bool buttonIsDown)
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


