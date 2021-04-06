#ifndef EmRegsLCDCtrl_H
#define EmRegsLCDCtrl_H

#include "EmHAL.h"				// EmHALHandler
#include "EmRegs.h"				// EmRegs

#define	MQ_GraphicController_LCDOnBit		0x00000088
#define	MQ_GraphicController_Enabled		0x00000001
#define	MQ_GraphicController_LowRezBit		0x0000C000
#define	MQ_GraphicController_ColorDepthMask	0x000000F0

// Color Depth
#define	MQ_GraphicController_1BPP_ColorPalette			0x00000000
#define	MQ_GraphicController_2BPP_ColorPalette			0x00000010
#define	MQ_GraphicController_4BPP_ColorPalette			0x00000020
#define	MQ_GraphicController_8BPP_ColorPalette			0x00000030
#define	MQ_GraphicController_1BPP_mono_NoColorPalette	0x00000080	//  1-bpp monochrome graphics. Color palette is bypassed.
#define	MQ_GraphicController_2BPP_mono_NoColorPalette	0x00000090	//  2-bpp monochrome graphics. Color palette is bypassed.
#define	MQ_GraphicController_4BPP_mono_NoColorPalette	0x000000A0	//  4-bpp monochrome graphics. Color palette is bypassed.	
#define	MQ_GraphicController_8BPP_mono_NoColorPalette	0x000000B0	//  8-bpp monochrome graphics. Color palette is bypassed.
#define	MQ_GraphicController_16BPP_NoColorPalette		0x000000C0	// 16-bpp (RGB565)graphics with color palette bypassed.

typedef struct {

	// CPU Control Register
	UInt32	CPUControl;						// 0x0000
	UInt32	GraphicEngineStatus;			// 0x0004
	UInt32	GPIOControl00;					// 0x0008	
	UInt32	GPIOControl01;					// 0x000C

	UInt32	CPUTestMode;					// 0x0010
	
	UInt8	_filler01[0x6C];				// 0x0014 - 0x007F

	// MIU Interface Control Register
	UInt32	MIUIFCtrl1;						// 0x0080 : MM00
	UInt32	MIUIFCtrl2;						// 0x0084 : MM01
	UInt32	MIUTestCtrl1;					// 0x0088 : MM02
	UInt32	MIUTestCtrl2;					// 0x008C : MM03
	UInt32	MIUTestData;					// 0x0090 : MM04

	// Unknown registers
	UInt8	filler02[0x6C];					// 0x0094 - 0x00FF

	// Global Intterrupt Control Register
	UInt32	InterruptControl;				// 0x0100
	UInt32	InterruptMask;					// 0x0104
	UInt32	InterruptStatus;				// 0x0108
	UInt32	InterruptPinRawStatus;			// 0x010C

	// Unknown registers
	UInt8	filler03[0x70];					// 0x0110 - 0x017F

	// Graphic Control Register
	UInt32	GraphicController;				// 0x0180 : GC00
	UInt32	PowerSequencing;				// 0x0184 : GC01
	UInt32	HorizontalDisplay;				// 0x0188 : GC02
	UInt32	VerticalDisplay;				// 0x018C : GC03

	UInt32	HorizontalSynchronize;			// 0x0190 : GC04
	UInt32	VerticalSynchronize;			// 0x0194 : GC05
	UInt32	HorizontalCounter;				// 0x0198 : GC06
	UInt32	VerticalCounter;				// 0x019C : GC07

	UInt32	HorizontalWindow;				// 0x01A0 : GC08
	UInt32	VerticalWindow;					// 0x01A4 : GC09
	UInt32	Reserved01A8;					// 0x01A8 : GC0A
	UInt32	LineClock;						// 0x01AC : GC0B

	UInt32	WindowStartAddress;				// 0x01B0 : GC0C
	UInt32	AlternateWindowStartAddress;	// 0x01B4 : GC0D
	UInt32	WindowStride;					// 0x01B8 : GC0E
	UInt32	Reserved01BC;					// 0x01BC : GC0F

	UInt32	HwrCursorPosition;				// 0x01C0 : GC10
	UInt32	HwrCursorStart;					// 0x01C4 : GC11
	UInt32	HwrCursorForeColor;				// 0x01C8 : GC12
	UInt32	HwrCursorBackColor;				// 0x01CC : GC13

	UInt32	Reserved01D0;					// 0x01D0 : GC14
	UInt32	Reserved01D4;					// 0x01D4 : GC15
	UInt32	Reserved01D8;					// 0x01D8 : GC16
	UInt32	Reserved01DC;					// 0x01DC : GC17

	UInt32	Reserved01E0;					// 0x01E0 : GC18
	UInt32	Reserved01E4;					// 0x01E4 : GC19
	UInt32	FrameCloock;					// 0x01E8 : GC1A
	UInt32	MiscellaneousSignal;			// 0x01EC : GC1B

	UInt32	HorizontalParameter;			// 0x01F0 : GC1C
	UInt32	VerticalParameter;				// 0x01F4 : GC1D
	UInt32	WindowLineStart;				// 0x01F8 : GC1E
	UInt32	CursorLineStart;				// 0x01FC : GC1F

	// Graphic Control Register
	UInt32	GraphicEngine0200;				// 0x0200 : GC00
	UInt32	GraphicEngine0204;				// 0x0204
	UInt32	GraphicEngine0208;				// 0x0208
	UInt32	GraphicEngine020C;				// 0x020C

	UInt32	GraphicEngine0210;				// 0x0210
	UInt32	GraphicEngine0214;				// 0x0214
	UInt32	GraphicEngine0218;				// 0x0218
	UInt32	GraphicEngine021C;				// 0x021C

	UInt32	GraphicEngine0220;				// 0x0220
	UInt32	GraphicEngine0224;				// 0x0224
	UInt32	GraphicEngine0228;				// 0x0228
	UInt32	GraphicEngine022C;				// 0x022C

	UInt32	GraphicEngine0230;				// 0x0230
	UInt32	Reserve0234;					// 0x0234
	UInt32	Reserve0238;					// 0x0238
	UInt32	GraphicEngine023C;				// 0x023C

	UInt32	GraphicEngine0240;				// 0x0240
	UInt32	GraphicEngine0244;				// 0x0244
	UInt32	GraphicEngine0248;				// 0x0248
	UInt32	GraphicEngine024C;				// 0x024C

	// Unknown registers
	UInt8	filler04[0x130];				// 0x0250 - 0x037F

	// Device Configration Register
	UInt32	DeviceConfig0380;				// 0x0380 : DC00
	UInt32	DeviceConfig0384;				// 0x0384
	UInt32	DeviceConfig0388;				// 0x0388
	UInt32	DeviceConfig038C;				// 0x038C

	UInt32	DeviceConfig0390;				// 0x0390
	UInt32	DeviceConfig0394;				// 0x0394
	UInt32	DeviceConfig0398;				// 0x0398
	UInt32	DeviceConfig039C;				// 0x039C

	UInt8	filler05[0x260];				// 0x03A0 - 0x05FF

	// Flat Panel Control Register
	UInt32	FlatPanel600;					// 0x0600 : FP00
	UInt32	FlatPanel604;					// 0x0604
	UInt32	FlatPanel608;					// 0x0608
	UInt32	FlatPanel60C;					// 0x060C

	UInt32	FlatPanel610;					// 0x0610
	UInt32	FlatPanel614;					// 0x0614
	UInt32	FlatPanel618;					// 0x0618
	UInt32	FlatPanel61C;					// 0x061C

	UInt32	FlatPanel620;					// 0x0620
	UInt32	FlatPanel624;					// 0x0624
	UInt32	FlatPanel628;					// 0x0628
	UInt32	FlatPanel62C;					// 0x062C

	UInt32	FlatPanel630;					// 0x0630
	UInt32	FlatPanel634;					// 0x0634
	UInt32	FlatPanel638;					// 0x0638
	UInt32	FlatPanel63C;					// 0x063C

	UInt32	FlatPanel640;					// 0x0640
	UInt32	FlatPanel644;					// 0x0644
	UInt32	FlatPanel648;					// 0x0648
	UInt32	FlatPanel64C;					// 0x064C

	UInt32	FlatPanel650;					// 0x0650
	UInt32	FlatPanel654;					// 0x0654
	UInt32	FlatPanel658;					// 0x0658
	UInt32	FlatPanel65C;					// 0x065C

	UInt32	FlatPanel660;					// 0x0660
	UInt32	FlatPanel664;					// 0x0664
	UInt32	FlatPanel668;					// 0x0668
	UInt32	FlatPanel66C;					// 0x066C

	UInt32	FlatPanel670;					// 0x0670
	UInt32	FlatPanel674;					// 0x0674
	UInt32	FlatPanel678;					// 0x0678
	UInt32	FlatPanel67C;					// 0x067C

	UInt32	FlatPanel680;					// 0x0680
	UInt32	FlatPanel684;					// 0x0684
	UInt32	FlatPanel688;					// 0x0688
	UInt32	FlatPanel68C;					// 0x068C

	UInt32	FlatPanel690;					// 0x0690
	UInt32	FlatPanel694;					// 0x0694
	UInt32	FlatPanel698;					// 0x0698
	UInt32	FlatPanel69C;					// 0x069C

	UInt32	FlatPanel6A0;					// 0x06A0
	UInt32	FlatPanel6A4;					// 0x06A4
	UInt32	FlatPanel6A8;					// 0x06A8
	UInt32	FlatPanel6AC;					// 0x06AC

	UInt32	FlatPanel6B0;					// 0x06B0
	UInt32	FlatPanel6B4;					// 0x06B4
	UInt32	FlatPanel6B8;					// 0x06B8
	UInt32	FlatPanel6BC;					// 0x06BC

	UInt32	FlatPanel6C0;					// 0x06C0
	UInt32	FlatPanel6C4;					// 0x06C4
	UInt32	FlatPanel6C8;					// 0x06C8
	UInt32	FlatPanel6CC;					// 0x06CC

	UInt32	FlatPanel6D0;					// 0x06D0
	UInt32	FlatPanel6D4;					// 0x06D4
	UInt32	FlatPanel6D8;					// 0x06D8
	UInt32	FlatPanel6DC;					// 0x06DC

	UInt8	filler06[0x120];				// 0x06E0 - 0x07FF

	UInt8	ColorPalette800[0x0400];		// 0x0800 - 0x0BFF

	UInt8	SourceFIFOSpaceC00[0x0400];		// 0x0800 - 0x0BFF
} HwrLCDCtrlType;


class EmRegsMQLCDControl : public EmRegs, public EmHALHandler
{
	public:
								EmRegsMQLCDControl		(emuptr baseRegsAddr,
														 emuptr baseVideoAddr);
		virtual					~EmRegsMQLCDControl		(void);

		// EmRegs overrides
		virtual void			Initialize				(void);
		virtual void			Reset					(Bool hardwareReset);
		virtual void			Save					(SessionFile&);
		virtual void			Load					(SessionFile&);
		virtual void			Dispose					(void);

		virtual void			SetSubBankHandlers		(void);
		virtual uint8*			GetRealAddress			(emuptr address);
		virtual emuptr			GetAddressStart			(void);
		virtual uint32			GetAddressRange			(void);

		// EmHAL overrides
		virtual Bool			GetLCDScreenOn			(void);
		virtual Bool			GetLCDBacklightOn		(void);
		virtual Bool			GetLCDHasFrame		(void);
		virtual void			GetLCDBeginEnd		(emuptr& begin, emuptr& end);
		virtual void			GetLCDScanlines		(EmScreenUpdateInfo& info);

		static	UInt16			GetResolutionMode	();

	private:
		uint32					ColorPaletteRead (emuptr address, int size);
		void					ColorPaletteWrite (emuptr address, int size, uint32 value);
//		uint32					powerSaveConfigurationRead	(emuptr address, int size);

		uint32					DC380Read					(emuptr address, int size);

		void 					InvalidateWrite				(emuptr address, int size, uint32 value);
		void					IgnoreWrite					(emuptr address, int size, uint32 value);
//		void 					lutWriteAddressWrite		(emuptr address, int size, uint32 value);
//		void 					lutReadAddressWrite			(emuptr address, int size, uint32 value);

	private:
		void					PrvGetPalette				(RGBList& thePalette);

	private:
		emuptr					fBaseRegsAddr;
		emuptr					fBaseVideoAddr;
		HwrLCDCtrlType			fRegs;
		RGBType					fClutData[256];

};

#define MQ_LCDController_BaseAddress			0x1F000000
#define MQ_LCDController_RegsAddr				0x1F040000
#define MQ_LCDController_RegsOffset				0x00001FFF
#define MQ_LCDController_VideoMemStart			(MQ_LCDController_BaseAddress)
#define MQ_LCDController_VideoMemSize			0x00040000	// 256K of memory for VRAM

#define MQ_LCDforModena_BaseAddress				0x11000000
#define MQ_LCDforModena_RegsAddr				0x11040000
#define MQ_LCDforModena_VideoMemStart			(MQ_LCDforModena_BaseAddress)


#endif // EmRegsLCDCtrl_H
