#include "EmCommon.h"
#include "EmRegsLCDCtrl.h"

#include "EmScreen.h"			// EmScreen::InvalidateAll
#include "EmPixMap.h"			// EmPixMap::GetLCDScanlines
#include "EmMemory.h"			// EmMemGetRealAddress
#include "SessionFile.h"		// 
#include "UAE_Utils.h"			// uae_memcpy


// Given a register (specified by its field name), return its address
// in emulated space.

#define addressof(x)	(this->GetAddressStart () + offsetof(HwrLCDCtrlType, x))


// Macro to help the installation of handlers for a register.

#define INSTALL_HANDLER(read, write, reg) \
	this->SetHandler ((ReadFunction) &EmRegsMQLCDControl::read, (WriteFunction) &EmRegsMQLCDControl::write, addressof (reg), sizeof (fRegs.reg))

static	UInt16	g_highResMode = 160;

// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::EmRegsMQLCDControl
// ---------------------------------------------------------------------------

EmRegsMQLCDControl::EmRegsMQLCDControl (emuptr baseRegsAddr, emuptr baseVideoAddr) :
	fBaseRegsAddr (baseRegsAddr),
	fBaseVideoAddr (baseVideoAddr),
	fRegs ()
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::~EmRegsMQLCDControl
// ---------------------------------------------------------------------------

EmRegsMQLCDControl::~EmRegsMQLCDControl (void)
{
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::Initialize
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::Initialize (void)
{
	EmRegs::Initialize ();
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::Reset
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::Reset (Bool hardwareReset)
{
	EmRegs::Reset (hardwareReset);

	if (hardwareReset)
	{
		memset (&fRegs, 0,sizeof(fRegs));
	}

}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::Save
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::Save (SessionFile& f)
{
	EmRegs::Save (f);

	f.WriteLCDCtrlRegsType (fRegs);
	f.FixBug (SessionFile::kBugByteswappedStructs);
	f.WriteLCDCtrlPalette (fClutData);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::Load
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::Load (SessionFile& f)
{
	EmRegs::Load (f);
	if (!f.ReadLCDCtrlRegsType (fRegs))
	{
		f.SetCanReload (false);
	}

	// Read in the LCD palette, and then byteswap it.

	if (!f.ReadLCDCtrlPalette (fClutData))
	{
		f.SetCanReload (false);
	}
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::Dispose
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::Dispose (void)
{
	EmRegs::Dispose ();
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::SetSubBankHandlers
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::SetSubBankHandlers (void)
{
	// Install base handlers.

	EmRegs::SetSubBankHandlers ();

	// Now add standard/specialized handers for the defined registers.

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				CPUControl);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngineStatus);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GPIOControl00);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GPIOControl01);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				CPUTestMode);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				MIUIFCtrl1);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				MIUIFCtrl2);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				MIUTestCtrl1);
	INSTALL_HANDLER (ZeroRead,			StdWriteBE,				MIUTestCtrl2);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				MIUTestData);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				InterruptControl);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				InterruptMask);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				InterruptStatus);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				InterruptPinRawStatus);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicController);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				PowerSequencing);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HorizontalDisplay);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				VerticalDisplay);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HorizontalSynchronize);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				VerticalSynchronize);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HorizontalCounter);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				VerticalCounter);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HorizontalWindow);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				VerticalWindow);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01A8);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				LineClock);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				WindowStartAddress);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				AlternateWindowStartAddress);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				WindowStride);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01BC);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HwrCursorPosition);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HwrCursorStart);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HwrCursorForeColor);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HwrCursorBackColor);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01D0);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01D4);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01D8);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01DC);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01E0);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserved01E4);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FrameCloock);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				MiscellaneousSignal);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				HorizontalParameter);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				VerticalParameter);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				WindowLineStart);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				CursorLineStart);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0200);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0204);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0208);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine020C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0210);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0214);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0218);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine021C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0220);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0224);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0228);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine022C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine0230);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserve0234);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				Reserve0238);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				GraphicEngine023C);

	// Tenerife's 0x00000380 register is read only.
	INSTALL_HANDLER (DC380Read,			IgnoreWrite,			DeviceConfig0380);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig0384);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig0388);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig038C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig0390);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig0394);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig0398);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				DeviceConfig039C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel600);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel604);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel608);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel60C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel610);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel614);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel618);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel61C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel620);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel624);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel628);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel62C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel630);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel634);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel638);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel63C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel640);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel644);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel648);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel64C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel650);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel654);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel658);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel65C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel660);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel664);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel668);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel66C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel670);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel674);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel678);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel67C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel680);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel684);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel688);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel68C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel690);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel694);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel698);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel69C);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6A0);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6A4);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6A8);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6AC);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6B0);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6B4);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6B8);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6BC);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6C0);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6C4);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6C8);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6CC);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6D0);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6D4);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6D8);
	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				FlatPanel6DC);

	INSTALL_HANDLER (StdReadBE,			StdWriteBE,				filler05);
	INSTALL_HANDLER (ColorPaletteRead,	ColorPaletteWrite,		ColorPalette800);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetRealAddress
// ---------------------------------------------------------------------------

uint8* EmRegsMQLCDControl::GetRealAddress (emuptr address)
{
	return ((uint8*)&fRegs) + address - this->GetAddressStart ();
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetAddressStart
// ---------------------------------------------------------------------------

emuptr EmRegsMQLCDControl::GetAddressStart (void)
{
	return fBaseRegsAddr;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetAddressRange
// ---------------------------------------------------------------------------

uint32 EmRegsMQLCDControl::GetAddressRange (void)
{
	return sizeof(fRegs);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetLCDScreenOn
// ---------------------------------------------------------------------------

Bool EmRegsMQLCDControl::GetLCDScreenOn (void)
{
	return ((fRegs.GraphicController & MQ_GraphicController_Enabled) == MQ_GraphicController_Enabled);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetLCDBacklightOn
// ---------------------------------------------------------------------------

Bool EmRegsMQLCDControl::GetLCDBacklightOn (void)
{
	return true;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetLCDHasFrame
// ---------------------------------------------------------------------------

Bool EmRegsMQLCDControl::GetLCDHasFrame (void)
{
	return true;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetLCDBeginEnd
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::GetLCDBeginEnd (emuptr& begin, emuptr& end)
{
	int32	height		= ((fRegs.GraphicController & MQ_GraphicController_LowRezBit) == MQ_GraphicController_LowRezBit)
							? 160 : 320;
	int32	rowBytes	= fRegs.WindowStride & 0x0000FFFF;
	uint32	offset		= fRegs.WindowStartAddress;
	emuptr	baseAddr	= fBaseVideoAddr + offset;

	begin = baseAddr;
	end = baseAddr + rowBytes * height;
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::GetLCDScanlines
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::GetLCDScanlines (EmScreenUpdateInfo& info)
{
	// Get the screen metrics.
	int32	bpp			= (fRegs.GraphicController & MQ_GraphicController_ColorDepthMask);
	Bool	mono		= false;
	int32	rowBytes	= fRegs.WindowStride & 0x0000FFFF;
	uint32	offset		= fRegs.WindowStartAddress;
	emuptr	baseAddr	= fBaseVideoAddr + offset;
	int32	width		= ((fRegs.GraphicController & MQ_GraphicController_LowRezBit) == MQ_GraphicController_LowRezBit)
							? 160 : 320;
	int32	height		= ((fRegs.GraphicController & MQ_GraphicController_LowRezBit) == MQ_GraphicController_LowRezBit)
							? 160 : 320;
	g_highResMode		= ((fRegs.GraphicController & MQ_GraphicController_LowRezBit) == MQ_GraphicController_LowRezBit)
							? 160 : 320;

	info.fLeftMargin	= 0;

	EmPixMapFormat	format	=	bpp == MQ_GraphicController_1BPP_ColorPalette ? kPixMapFormat1 :
								bpp == MQ_GraphicController_2BPP_ColorPalette ? kPixMapFormat2 :
								bpp == MQ_GraphicController_4BPP_ColorPalette ? kPixMapFormat4 :
								bpp == MQ_GraphicController_8BPP_ColorPalette ? kPixMapFormat8 :
								bpp == MQ_GraphicController_16BPP_NoColorPalette ? kPixMapFormat16RGB565 :
								kPixMapFormat24RGB;

	if (format <= kPixMapFormat8)
	{
		RGBList	colorTable;
		this->PrvGetPalette (colorTable);

		// Set format, size, and color table of EmPixMap.
		info.fImage.SetSize			(EmPoint (width, height));
		info.fImage.SetFormat		(format);
		info.fImage.SetRowBytes		(rowBytes);
		info.fImage.SetColorTable	(colorTable);

		// Determine first and last scanlines to fetch, and fetch them.

		UInt32	maxLastLine = ((fRegs.GraphicController & MQ_GraphicController_LowRezBit) == MQ_GraphicController_LowRezBit)
								? 160 : 320;

		info.fFirstLine		= (info.fScreenLow - baseAddr) / rowBytes;
		info.fLastLine		= (info.fScreenHigh - baseAddr - 1) / rowBytes + 1;
		if (info.fLastLine > maxLastLine)
			info.fLastLine = maxLastLine;

		long	firstLineOffset	= info.fFirstLine * rowBytes;
		long	lastLineOffset	= info.fLastLine * rowBytes;

		uae_memcpy (
			(void*) ((uint8*) info.fImage.GetBits () + firstLineOffset),
			baseAddr + firstLineOffset,
			lastLineOffset - firstLineOffset);
	}
	else
	{
		Bool	byteSwapped	= true;
		// Set depth, size, and color table of EmPixMap.
		info.fImage.SetSize (EmPoint (width, height));
		info.fImage.SetFormat (kPixMapFormat24RGB);

		// Determine first and last scanlines to fetch.

		info.fFirstLine		= (info.fScreenLow - baseAddr) / rowBytes;
		info.fLastLine		= (info.fScreenHigh - baseAddr - 1) / rowBytes + 1;

		// Get location and rowBytes of source bytes.

		uint8*	srcStart		= EmMemGetRealAddress (baseAddr);
		int32	srcRowBytes		= rowBytes;
		uint8*	srcPtr			= srcStart + srcRowBytes * info.fFirstLine;
		uint8*	srcPtr0			= srcPtr;

		// Get location and rowBytes of destination bytes.

		uint8*	destStart		= (uint8*) info.fImage.GetBits ();
		int32	destRowBytes	= info.fImage.GetRowBytes ();
		uint8*	destPtr			= destStart + destRowBytes * info.fFirstLine;
		uint8*	destPtr0		= destPtr;

		// Get height of range to copy.

		int32	height = info.fLastLine - info.fFirstLine;

		// Copy the pixels from source to dest.

		for (int yy = 0; yy < height; ++yy)
		{
			for (int xx = 0; xx < width; ++xx)
			{
				uint8	p1 = EmMemDoGet8 (srcPtr++);	// GGGBBBBB
				uint8	p2 = EmMemDoGet8 (srcPtr++);	// RRRRRGGG

				// Merge the two together so that we get RRRRRGGG GGGBBBBB

				uint16	p;
				
				if (!byteSwapped)
					p = (p2 << 8) | p1;
				else
					p = (p1 << 8) | p2;

				// Shift the bits around, forming RRRRRrrr, GGGGGGgg, and
				// BBBBBbbb values, where the lower-case bits are copies of
				// the least significant bits in the upper-case bits.
				//
				// Note that all of this could also be done with three 64K
				// lookup tables.  If speed is an issue, we might want to
				// investigate that.

				if (mono)
				{
					uint8	green = ((p >> 3) & 0xFC) | ((p >>  5) & 0x03);
					*destPtr++ = green;
					*destPtr++ = green;
					*destPtr++ = green;
				}
				else
				{
					*destPtr++ = ((p >> 8) & 0xF8) | ((p >> 11) & 0x07);
					*destPtr++ = ((p >> 3) & 0xFC) | ((p >>  5) & 0x03);
					*destPtr++ = ((p << 3) & 0xF8) | ((p >>  0) & 0x07);
				}
			}

			srcPtr	= srcPtr0 += srcRowBytes;
			destPtr	= destPtr0 += destRowBytes;
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::InvalidateWrite
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::InvalidateWrite (emuptr address, int size, uint32 value)
{
	this->StdWriteBE (address, size, value);
	EmScreen::InvalidateAll ();
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::IgnoreWrite
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::IgnoreWrite (emuptr address, int size, uint32 value)
{
	// Ignore write value to address
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::ColorPaletteWrite
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::ColorPaletteWrite (emuptr address, int size, uint32 value)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	// Always set the vertical non-display status high since in the real
	// hardware, the ROM will check this flag in order to write the CLUT
	// registers.
	this->StdWriteBE (address, size, value);

	emuptr	begin, end;
	GetLCDBeginEnd (begin, end);
	EmScreen::MarkDirty (begin, end - begin);
	value &= 0x0FF;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::ColorPaletteRead
// ---------------------------------------------------------------------------

uint32 EmRegsMQLCDControl::ColorPaletteRead (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	// Always set the vertical non-display status high since in the real
	// hardware, the ROM will check this flag in order to write the CLUT
	// registers.
	return	this->StdReadBE (address, size);
}


// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::DC380Read
// ---------------------------------------------------------------------------

uint32 EmRegsMQLCDControl::DC380Read (emuptr address, int size)
{
	UNUSED_PARAM(address)
	UNUSED_PARAM(size)

	// Read Device-Configration-Register addres:0000 at always return 0.
	// Offset 0x00040380 from LCD controller register .
	return	0;
}

// ---------------------------------------------------------------------------
//		¥ EmRegsMQLCDControl::PrvGetPalette
// ---------------------------------------------------------------------------

void EmRegsMQLCDControl::PrvGetPalette (RGBList& thePalette)
{
	int32	bpp			= ((fRegs.GraphicController & 0x000000F0) >> 4);
	int32	numColors	=	bpp == 0 ? 2:
							bpp == 1 ? 4 :
							bpp == 2 ? 16 :
							256;

	thePalette.resize (numColors);

	for (int ii = 0; ii < numColors; ++ii)
	{
		thePalette[ii].fRed		= fRegs.ColorPalette800[ii*4];
		thePalette[ii].fGreen	= fRegs.ColorPalette800[ii*4+1];
		thePalette[ii].fBlue	= fRegs.ColorPalette800[ii*4+2];
	}
}

UInt16 EmRegsMQLCDControl::GetResolutionMode ()
{
	return g_highResMode;
}