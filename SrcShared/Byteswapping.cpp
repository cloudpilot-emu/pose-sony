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

#include "EmCommon.h"
#include "Byteswapping.h"

#if BYTESWAP

#include "EmRPC.h"				// sysPktRPC2Cmd
#include "UAE.h"				// regstruct

#if defined (_MSC_VER) || defined (__GNUC__)

unsigned long	flipBits[] =
{
	/* 0000 -> 0000 */	0x00,
	/* 0001 -> 1000 */	0x08,
	/* 0010 -> 0100 */	0x04,
	/* 0011 -> 1100 */	0x0C,
	/* 0100 -> 0010 */	0x02,
	/* 0101 -> 1010 */	0x0A,
	/* 0110 -> 0110 */	0x06,
	/* 0111 -> 1110 */	0x0E,
	/* 1000 -> 0001 */	0x01,
	/* 1001 -> 1001 */	0x09,
	/* 1010 -> 0101 */	0x05,
	/* 1011 -> 1101 */	0x0D,
	/* 1100 -> 0011 */	0x03,
	/* 1101 -> 1011 */	0x0B,
	/* 1110 -> 0111 */	0x07,
	/* 1111 -> 1111 */	0x0F
};

#endif

void Byteswap (EventType& event)
{
	Byteswap (event.eType);
	Byteswap (event.penDown);
	Byteswap (event.screenX);
	Byteswap (event.screenY);

	switch(event.eType)
	{
		case nilEvent:
			break;

		case penDownEvent:
			Byteswap (event.data.penUp.start);
			Byteswap (event.data.penUp.end);
			break;

		case penUpEvent:
			Byteswap (event.data.penUp.start);
			Byteswap (event.data.penUp.end);
			break;

		case penMoveEvent:
			Byteswap (event.data.penUp.start);
			Byteswap (event.data.penUp.end);
			break;

		case keyDownEvent:
			Byteswap (event.data.keyDown.chr);
			Byteswap (event.data.keyDown.keyCode);
			Byteswap (event.data.keyDown.modifiers);
			break;

		case winEnterEvent:
			Byteswap (event.data.winEnter.enterWindow);
			Byteswap (event.data.winEnter.exitWindow);
			break;

		case winExitEvent:
			Byteswap (event.data.winExit.enterWindow);
			Byteswap (event.data.winExit.exitWindow);
			break;

		case ctlEnterEvent:
			Byteswap (event.data.ctlEnter.controlID);
			Byteswap (event.data.ctlEnter.pControl);
			break;

		case ctlExitEvent:
			break;

		case ctlSelectEvent:
			Byteswap (event.data.ctlSelect.controlID);
			Byteswap (event.data.ctlSelect.pControl);
			Byteswap (event.data.ctlSelect.on);
			break;

		case ctlRepeatEvent:
			Byteswap (event.data.ctlRepeat.controlID);
			Byteswap (event.data.ctlRepeat.pControl);
			Byteswap (event.data.ctlRepeat.time);
			break;

		case lstEnterEvent:
			Byteswap (event.data.lstEnter.listID);
			Byteswap (event.data.lstEnter.pList);
			Byteswap (event.data.lstEnter.selection);
			break;

		case lstSelectEvent:
			Byteswap (event.data.lstSelect.listID);
			Byteswap (event.data.lstSelect.pList);
			Byteswap (event.data.lstSelect.selection);
			break;

		case lstExitEvent:
			Byteswap (event.data.lstExit.listID);
			Byteswap (event.data.lstExit.pList);
			break;

		case popSelectEvent:
			Byteswap (event.data.popSelect.controlID);
			Byteswap (event.data.popSelect.controlP);
			Byteswap (event.data.popSelect.listID);
			Byteswap (event.data.popSelect.listP);
			Byteswap (event.data.popSelect.selection);
			Byteswap (event.data.popSelect.priorSelection);
			break;

		case fldEnterEvent:
			Byteswap (event.data.fldEnter.fieldID);
			Byteswap (event.data.fldEnter.pField);
			break;

		case fldHeightChangedEvent:
			Byteswap (event.data.fldHeightChanged.fieldID);
			Byteswap (event.data.fldHeightChanged.pField);
			Byteswap (event.data.fldHeightChanged.newHeight);
			Byteswap (event.data.fldHeightChanged.currentPos);
			break;

		case fldChangedEvent:
			Byteswap (event.data.fldChanged.fieldID);
			Byteswap (event.data.fldChanged.pField);
			break;

		case tblEnterEvent:
			Byteswap (event.data.tblEnter.tableID);
			Byteswap (event.data.tblEnter.pTable);
			Byteswap (event.data.tblEnter.row);
			Byteswap (event.data.tblEnter.column);
			break;

		case tblSelectEvent:
			Byteswap (event.data.tblEnter.tableID);
			Byteswap (event.data.tblEnter.pTable);
			Byteswap (event.data.tblEnter.row);
			Byteswap (event.data.tblEnter.column);
			break;

		case daySelectEvent:
			Byteswap (event.data.daySelect.pSelector);
			Byteswap (event.data.daySelect.selection);
			Byteswap (event.data.daySelect.useThisDate);
			break;

		case menuEvent:
			Byteswap (event.data.menu.itemID);
			break;

		case appStopEvent:
			break;

		case frmLoadEvent:
			Byteswap (event.data.frmLoad.formID);
			break;

		case frmOpenEvent:
			Byteswap (event.data.frmOpen.formID);
			break;

		case frmGotoEvent:
			Byteswap (event.data.frmGoto.formID);
			Byteswap (event.data.frmGoto.recordNum);
			Byteswap (event.data.frmGoto.matchPos);
			Byteswap (event.data.frmGoto.matchLen);
			Byteswap (event.data.frmGoto.matchFieldNum);
			Byteswap (event.data.frmGoto.matchCustom);
			break;

		case frmUpdateEvent:
			Byteswap (event.data.frmUpdate.formID);
			Byteswap (event.data.frmUpdate.updateCode);
			break;

		case frmSaveEvent:
			break;

		case frmCloseEvent:
			Byteswap (event.data.frmClose.formID);
			break;

		case frmTitleEnterEvent:
			Byteswap (event.data.frmTitleEnter.formID);
			break;

		case frmTitleSelectEvent:
			Byteswap (event.data.frmTitleSelect.formID);
			break;

		case tblExitEvent:
			Byteswap (event.data.tblExit.tableID);
			Byteswap (event.data.tblExit.pTable);
			Byteswap (event.data.tblExit.row);
			Byteswap (event.data.tblExit.column);
			break;

		case sclEnterEvent:
			Byteswap (event.data.sclEnter.scrollBarID);
			Byteswap (event.data.sclEnter.pScrollBar);
			break;

		case sclExitEvent:
			Byteswap (event.data.sclExit.scrollBarID);
			Byteswap (event.data.sclExit.pScrollBar);
			Byteswap (event.data.sclExit.value);
			Byteswap (event.data.sclExit.newValue);
			break;

		case sclRepeatEvent:
			Byteswap (event.data.sclRepeat.scrollBarID);
			Byteswap (event.data.sclRepeat.pScrollBar);
			Byteswap (event.data.sclRepeat.value);
			Byteswap (event.data.sclRepeat.newValue);
			Byteswap (event.data.sclRepeat.time);
			break;

		case tsmConfirmEvent:
			Byteswap (event.data.tsmConfirm.yomiText);
			Byteswap (event.data.tsmConfirm.formID);
			break;

		case tsmFepButtonEvent:
			Byteswap (event.data.tsmFepButton.buttonID);
			break;

		case tsmFepModeEvent:
			Byteswap (event.data.tsmFepMode.mode);
			break;

		case menuCmdBarOpenEvent:
			Byteswap (event.data.menuCmdBarOpen.preventFieldButtons);
			Byteswap (event.data.menuCmdBarOpen.reserved);
			break;

		case menuOpenEvent:
			Byteswap (event.data.menuOpen.menuRscID);
			Byteswap (event.data.menuOpen.cause);
			break;

		case menuCloseEvent:
			// Doesn't appear to be used.
			break;

		case frmGadgetEnterEvent:
			Byteswap (event.data.gadgetEnter.gadgetID);
			Byteswap (event.data.gadgetEnter.gadgetP);
			break;

		case frmGadgetMiscEvent:
			Byteswap (event.data.gadgetMisc.gadgetID);
			Byteswap (event.data.gadgetMisc.gadgetP);
			Byteswap (event.data.gadgetMisc.selector);
			Byteswap (event.data.gadgetMisc.dataP);
			break;
	}
}


void Byteswap (FieldAttrType& attr)
{
	Byteswap (*(unsigned short*) &attr);

#if defined (_MSC_VER) || defined (__GNUC__)
	unsigned short	v = *(unsigned short*) &attr;

	/*
		======== BITFIELD LAYOUT CHEAT-SHEET ========

		typedef struct {
			UInt16 usable			:1;
			UInt16 visible		:1;
			UInt16 editable		:1;
			UInt16 singleLine		:1;
			UInt16 hasFocus		:1;
			UInt16 dynamicSize	:1;
			UInt16 insPtVisible	:1;
			UInt16 dirty			:1;
			UInt16 underlined		:2;
			UInt16 justification	:2;
			UInt16 autoShift		:1;
			UInt16 hasScrollBar	:1;
			UInt16 numeric		:1;
		} FieldAttrType;

		// On the Mac:

		|---------- high byte ----------|---------- low byte -----------|

		 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0  
		  7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0
		+---+---+---+---+---+---+---+---+-------+-------+---+---+---+---+
		| u | v | e | s | h | d | i | d |   u   |   j   | a | h | n | * |
		+---+---+---+---+---+---+---+---+-------+-------+---+---+---+---+


		// On Windows (in-register representation):

		|---------- high byte ----------|---------- low byte -----------|

		 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0  
		  7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0
		+---+---+---+---+-------+-------+---+---+---+---+---+---+---+---+
		| * | n | h | a |   j   |   u   | d | i | d | h | s | e | v | u |
		+---+---+---+---+-------+-------+---+---+---+---+---+---+---+---+
	*/

	v = (unsigned short) (
		(flipBits[ (v >>  0) & 0x0F ] << 12) |
		(flipBits[ (v >>  4) & 0x0F ] <<  8) |
		(flipBits[ (v >>  8) & 0x0F ] <<  4) |
		(flipBits[ (v >> 12) & 0x0F ] <<  0));

	*(unsigned short*) &attr = v;
#endif
}


void Byteswap (HwrM68328Type& regs)
{
	Byteswap (regs.csAGroupBase);
	Byteswap (regs.csBGroupBase);
	Byteswap (regs.csCGroupBase);
	Byteswap (regs.csDGroupBase);

	Byteswap (regs.csAGroupMask);
	Byteswap (regs.csBGroupMask);
	Byteswap (regs.csCGroupMask);
	Byteswap (regs.csDGroupMask);

	Byteswap (regs.csASelect0);
	Byteswap (regs.csASelect1);
	Byteswap (regs.csASelect2);
	Byteswap (regs.csASelect3);

	Byteswap (regs.csBSelect0);
	Byteswap (regs.csBSelect1);
	Byteswap (regs.csBSelect2);
	Byteswap (regs.csBSelect3);

	Byteswap (regs.csCSelect0);
	Byteswap (regs.csCSelect1);
	Byteswap (regs.csCSelect2);
	Byteswap (regs.csCSelect3);

	Byteswap (regs.csDSelect0);
	Byteswap (regs.csDSelect1);
	Byteswap (regs.csDSelect2);
	Byteswap (regs.csDSelect3);

	Byteswap (regs.csDebug);

	Byteswap (regs.pllControl);
	Byteswap (regs.pllFreqSel);
	Byteswap (regs.pllTest);

	Byteswap (regs.intControl);
	Byteswap (regs.intMaskHi);
	Byteswap (regs.intMaskLo);
	Byteswap (regs.intWakeupEnHi);
	Byteswap (regs.intWakeupEnLo);
	Byteswap (regs.intStatusHi);
	Byteswap (regs.intStatusLo);
	Byteswap (regs.intPendingHi);
	Byteswap (regs.intPendingLo);

	Byteswap (regs.pwmControl);
	Byteswap (regs.pwmPeriod);
	Byteswap (regs.pwmWidth);
	Byteswap (regs.pwmCounter);

	Byteswap (regs.tmr1Control);
	Byteswap (regs.tmr1Prescaler);
	Byteswap (regs.tmr1Compare);
	Byteswap (regs.tmr1Capture);
	Byteswap (regs.tmr1Counter);
	Byteswap (regs.tmr1Status);
	
	Byteswap (regs.tmr2Control);
	Byteswap (regs.tmr2Prescaler);
	Byteswap (regs.tmr2Compare);
	Byteswap (regs.tmr2Capture);
	Byteswap (regs.tmr2Counter);
	Byteswap (regs.tmr2Status);
	
	Byteswap (regs.wdControl);
	Byteswap (regs.wdReference);
	Byteswap (regs.wdCounter);

	Byteswap (regs.spiSlave);

	Byteswap (regs.spiMasterData);
	Byteswap (regs.spiMasterControl);

	Byteswap (regs.uControl);
	Byteswap (regs.uBaud);
	Byteswap (regs.uReceive);
	Byteswap (regs.uTransmit);
	Byteswap (regs.uMisc);

	Byteswap (regs.lcdStartAddr);
	Byteswap (regs.lcdScreenWidth);
	Byteswap (regs.lcdScreenHeight);
	Byteswap (regs.lcdCursorXPos);
	Byteswap (regs.lcdCursorYPos);
	Byteswap (regs.lcdCursorWidthHeight);
	Byteswap (regs.lcdGrayPalette);

	Byteswap (regs.rtcHourMinSec);
	Byteswap (regs.rtcAlarm);
	Byteswap (regs.rtcReserved);
	Byteswap (regs.rtcControl);
	Byteswap (regs.rtcIntStatus);
	Byteswap (regs.rtcIntEnable);
	Byteswap (regs.stopWatch);
}


void Byteswap (HwrM68EZ328Type& regs)
{
	Byteswap (regs.swID);

	Byteswap (regs.csAGroupBase);
	Byteswap (regs.csBGroupBase);
	Byteswap (regs.csCGroupBase);
	Byteswap (regs.csDGroupBase);

	Byteswap (regs.csASelect);
	Byteswap (regs.csBSelect);
	Byteswap (regs.csCSelect);
	Byteswap (regs.csDSelect);

	Byteswap (regs.emuCS);

	Byteswap (regs.pllControl);
	Byteswap (regs.pllFreqSel);
	Byteswap (regs.pllTest);

	Byteswap (regs.intControl);
	Byteswap (regs.intMaskHi);
	Byteswap (regs.intMaskLo);
	Byteswap (regs.intStatusHi);
	Byteswap (regs.intStatusLo);
	Byteswap (regs.intPendingHi);
	Byteswap (regs.intPendingLo);

	Byteswap (regs.pwmControl);

	Byteswap (regs.tmr1Control);
	Byteswap (regs.tmr1Prescaler);
	Byteswap (regs.tmr1Compare);
	Byteswap (regs.tmr1Capture);
	Byteswap (regs.tmr1Counter);
	Byteswap (regs.tmr1Status);

	Byteswap (regs.spiMasterData);
	Byteswap (regs.spiMasterControl);

	Byteswap (regs.uControl);
	Byteswap (regs.uBaud);
	Byteswap (regs.uReceive);
	Byteswap (regs.uTransmit);
	Byteswap (regs.uMisc);
	Byteswap (regs.uNonIntPresc);

	Byteswap (regs.lcdStartAddr);
	Byteswap (regs.lcdScreenWidth);
	Byteswap (regs.lcdScreenHeight);
	Byteswap (regs.lcdCursorXPos);
	Byteswap (regs.lcdCursorYPos);
	Byteswap (regs.lcdCursorWidthHeight);
	Byteswap (regs.lcdContrastControlPWM);

	Byteswap (regs.rtcHourMinSec);
	Byteswap (regs.rtcAlarm);
	Byteswap (regs.rtcWatchDog);
	Byteswap (regs.rtcControl);
	Byteswap (regs.rtcIntStatus);
	Byteswap (regs.rtcIntEnable);
	Byteswap (regs.stopWatch);
	Byteswap (regs.rtcDay);
	Byteswap (regs.rtcDayAlarm);

	Byteswap (regs.dramConfig);
	Byteswap (regs.dramControl);

	Byteswap (regs.emuAddrCompare);   
	Byteswap (regs.emuAddrMask);
	Byteswap (regs.emuControlCompare);
	Byteswap (regs.emuControlMask);
	Byteswap (regs.emuControl);
	Byteswap (regs.emuStatus);
}


void Byteswap (HwrM68VZ328Type& regs)
{
	Byteswap (regs.scr);

	Byteswap (regs.pcr);
	Byteswap (regs.chipID);
	Byteswap (regs.maskID);
	Byteswap (regs.swID);
	Byteswap (regs.ioDriveControl);

	Byteswap (regs.csAGroupBase);
	Byteswap (regs.csBGroupBase);
	Byteswap (regs.csCGroupBase);
	Byteswap (regs.csDGroupBase);

	Byteswap (regs.csUGroupBase);

	Byteswap (regs.csControl1);
	Byteswap (regs.csControl2);
	Byteswap (regs.csControl3);

	Byteswap (regs.csASelect);
	Byteswap (regs.csBSelect);
	Byteswap (regs.csCSelect);
	Byteswap (regs.csDSelect);

	Byteswap (regs.emuCS);

	Byteswap (regs.pllControl);
	Byteswap (regs.pllFreqSel);

	Byteswap (regs.pwrControl);

	Byteswap (regs.intVector);
	Byteswap (regs.intControl);
	Byteswap (regs.intMaskHi);
	Byteswap (regs.intMaskLo);
	Byteswap (regs.intStatusHi);
	Byteswap (regs.intStatusLo);
	Byteswap (regs.intPendingHi);
	Byteswap (regs.intPendingLo);
	Byteswap (regs.intLevelControl);

	Byteswap (regs.portADir);
	Byteswap (regs.portAData);
	Byteswap (regs.portAPullupEn);

	Byteswap (regs.portBDir);
	Byteswap (regs.portBData);
	Byteswap (regs.portBPullupEn);
	Byteswap (regs.portBSelect);

	Byteswap (regs.portCDir);
	Byteswap (regs.portCData);
	Byteswap (regs.portCPulldnEn);
	Byteswap (regs.portCSelect);

	Byteswap (regs.portDDir);
	Byteswap (regs.portDData);
	Byteswap (regs.portDPullupEn);
	Byteswap (regs.portDSelect);
	Byteswap (regs.portDPolarity);
	Byteswap (regs.portDIntReqEn);
	Byteswap (regs.portDKbdIntEn);
	Byteswap (regs.portDIntEdge);

	Byteswap (regs.portEDir);
	Byteswap (regs.portEData);
	Byteswap (regs.portEPullupEn);
	Byteswap (regs.portESelect);

	Byteswap (regs.portFDir);
	Byteswap (regs.portFData);
	Byteswap (regs.portFPullupdnEn);
	Byteswap (regs.portFSelect);

	Byteswap (regs.portGDir);
	Byteswap (regs.portGData);
	Byteswap (regs.portGPullupEn);
	Byteswap (regs.portGSelect);

	Byteswap (regs.portJDir);
	Byteswap (regs.portJData);
	Byteswap (regs.portJPullupEn);
	Byteswap (regs.portJSelect);

	Byteswap (regs.portKDir);
	Byteswap (regs.portKData);
	Byteswap (regs.portKPullupdnEn);
	Byteswap (regs.portKSelect);

	Byteswap (regs.portMDir);
	Byteswap (regs.portMData);
	Byteswap (regs.portMPullupdnEn);
	Byteswap (regs.portMSelect);

	Byteswap (regs.pwmControl);
	Byteswap (regs.pwmSampleHi);
	Byteswap (regs.pwmSampleLo);
	Byteswap (regs.pwmPeriod);
	Byteswap (regs.pwmCounter);

	Byteswap (regs.pwm2Control);
	Byteswap (regs.pwm2Period);
	Byteswap (regs.pwm2Width);
	Byteswap (regs.pwm2Counter);

	Byteswap (regs.tmr1Control);
	Byteswap (regs.tmr1Prescaler);
	Byteswap (regs.tmr1Compare);
	Byteswap (regs.tmr1Capture);
	Byteswap (regs.tmr1Counter);
	Byteswap (regs.tmr1Status);

	Byteswap (regs.tmr2Control);
	Byteswap (regs.tmr2Prescaler);
	Byteswap (regs.tmr2Compare);
	Byteswap (regs.tmr2Capture);
	Byteswap (regs.tmr2Counter);
	Byteswap (regs.tmr2Status);

	Byteswap (regs.spiRxD);
	Byteswap (regs.spiTxD);
	Byteswap (regs.spiCont1);
	Byteswap (regs.spiIntCS);
	Byteswap (regs.spiTest);
	Byteswap (regs.spiSpc);

	Byteswap (regs.spiMasterData);
	Byteswap (regs.spiMasterControl);

	Byteswap (regs.uControl);
	Byteswap (regs.uBaud);
	Byteswap (regs.uReceive);
	Byteswap (regs.uTransmit);
	Byteswap (regs.uMisc);
	Byteswap (regs.uNonIntPresc);

	Byteswap (regs.u2Control);
	Byteswap (regs.u2Baud);
	Byteswap (regs.u2Receive);
	Byteswap (regs.u2Transmit);
	Byteswap (regs.u2Misc);
	Byteswap (regs.u2NonIntPresc);
	Byteswap (regs.u2FIFOHMark);

	Byteswap (regs.lcdStartAddr);
	Byteswap (regs.lcdPageWidth);
	Byteswap (regs.lcdScreenWidth);
	Byteswap (regs.lcdScreenHeight);
	Byteswap (regs.lcdCursorXPos);
	Byteswap (regs.lcdCursorYPos);
	Byteswap (regs.lcdCursorWidthHeight);
	Byteswap (regs.lcdBlinkControl);
	Byteswap (regs.lcdPanelControl);
	Byteswap (regs.lcdPolarity);
	Byteswap (regs.lcdACDRate);
	Byteswap (regs.lcdPixelClock);
	Byteswap (regs.lcdClockControl);
	Byteswap (regs.lcdRefreshRateAdj);
	Byteswap (regs.lcdReserved1);
	Byteswap (regs.lcdPanningOffset);
	Byteswap (regs.lcdFrameRate);
	Byteswap (regs.lcdGrayPalette);
	Byteswap (regs.lcdReserved2);
	Byteswap (regs.lcdContrastControlPWM);
	Byteswap (regs.lcdRefreshModeControl);
	Byteswap (regs.lcdDMAControl);

	Byteswap (regs.rtcHourMinSec);
	Byteswap (regs.rtcAlarm);
	Byteswap (regs.rtcWatchDog);
	Byteswap (regs.rtcControl);
	Byteswap (regs.rtcIntStatus);
	Byteswap (regs.rtcIntEnable);
	Byteswap (regs.stopWatch);
	Byteswap (regs.rtcDay);
	Byteswap (regs.rtcDayAlarm);

	Byteswap (regs.dramConfig);
	Byteswap (regs.dramControl);
	Byteswap (regs.sdramControl);
	Byteswap (regs.sdramPwDn);

	Byteswap (regs.emuAddrCompare);
	Byteswap (regs.emuAddrMask);
	Byteswap (regs.emuControlCompare);
	Byteswap (regs.emuControlMask);
	Byteswap (regs.emuControl);
	Byteswap (regs.emuStatus);
}


void Byteswap (PenBtnInfoType& p)
{
	Byteswap (p.boundsR);
	Byteswap (p.asciiCode);
	Byteswap (p.keyCode);
	Byteswap (p.modifiers);
}


void Byteswap (PointType& p)
{
	Byteswap (p.x);
	Byteswap (p.y);
}


void Byteswap (RectangleType& r)
{
	Byteswap (r.topLeft);
	Byteswap (r.extent);
}


void Byteswap (SndCommandType& cmd)
{
	Byteswap (cmd.cmd);
	Byteswap (cmd.param1);
	Byteswap (cmd.param2);
	Byteswap (cmd.param3);
}


void Byteswap (SysKernelInfoType& info)
{
	COMPILE_TIME_ASSERT (sizeof (info.selector) == 1);

	Byteswap (info.selector);
	Byteswap (info.id);

	switch (info.selector)
	{
		case sysKernelInfoSelCurTaskInfo:
		case sysKernelInfoSelTaskInfo:
			Byteswap (info.param.task.id);
			Byteswap (info.param.task.nextID);
			Byteswap (info.param.task.tag);
			Byteswap (info.param.task.status);
			Byteswap (info.param.task.timer);
			Byteswap (info.param.task.timeSlice);
			Byteswap (info.param.task.priority);
			Byteswap (info.param.task.attributes);
			Byteswap (info.param.task.pendingCalls);
			Byteswap (info.param.task.senderTaskID);
			Byteswap (info.param.task.msgExchangeID);
			Byteswap (info.param.task.tcbP);
			Byteswap (info.param.task.stackP);
			Byteswap (info.param.task.stackStart);
			Byteswap (info.param.task.stackSize);
			break;

		case sysKernelInfoSelSemaphoreInfo:
			Byteswap (info.param.semaphore.id);
			Byteswap (info.param.semaphore.nextID);
			Byteswap (info.param.semaphore.tag);
			Byteswap (info.param.semaphore.initValue);
			Byteswap (info.param.semaphore.curValue);
			Byteswap (info.param.semaphore.nestLevel);
			Byteswap (info.param.semaphore.ownerID);
			break;

		case sysKernelInfoSelTimerInfo:
			Byteswap (info.param.timer.id);
			Byteswap (info.param.timer.nextID);
			Byteswap (info.param.timer.tag);
			Byteswap (info.param.timer.ticksLeft);
			Byteswap (info.param.timer.period);
			Byteswap (info.param.timer.proc);
			break;
	}
}


void Byteswap (SysNVParamsType& params)
{
	Byteswap (params.rtcHours);
	Byteswap (params.rtcHourMinSecCopy);
}


void Byteswap (regstruct& p)
{
	Byteswap (p.regs[0]);
	Byteswap (p.regs[1]);
	Byteswap (p.regs[2]);
	Byteswap (p.regs[3]);
	Byteswap (p.regs[4]);
	Byteswap (p.regs[5]);
	Byteswap (p.regs[6]);
	Byteswap (p.regs[7]);

	Byteswap (p.regs[8]);
	Byteswap (p.regs[9]);
	Byteswap (p.regs[10]);
	Byteswap (p.regs[11]);
	Byteswap (p.regs[12]);
	Byteswap (p.regs[13]);
	Byteswap (p.regs[14]);
	Byteswap (p.regs[15]);

	Byteswap (p.usp);
	Byteswap (p.isp);
	Byteswap (p.msp);
	Byteswap (p.sr);
	Byteswap (p.t1);
	Byteswap (p.t0);
	Byteswap (p.s);
	Byteswap (p.m);
	Byteswap (p.stopped);
	Byteswap (p.intmask);
	Byteswap (p.pc);
	Byteswap (p.pc_p);
	Byteswap (p.pc_oldp);
	Byteswap (p.pc_meta_oldp);
	Byteswap (p.vbr);
	Byteswap (p.sfc);
	Byteswap (p.dfc);

#if 0	// we don't support FP
	Byteswap (fp.regs[0]);
	Byteswap (fp.regs[1]);
	Byteswap (fp.regs[2]);
	Byteswap (fp.regs[3]);
	Byteswap (fp.regs[4]);
	Byteswap (fp.regs[5]);
	Byteswap (fp.regs[6]);
	Byteswap (fp.regs[7]);

	Byteswap (p.fpcr);
	Byteswap (p.fpsr);
	Byteswap (p.fpiar);
#endif

	Byteswap (p.spcflags);
	Byteswap (p.kick_mask);
	Byteswap (p.prefetch);
}


void Byteswap (SED1375RegsType& p)
{
	Byteswap (p.productRevisionCode);
	Byteswap (p.mode0);
	Byteswap (p.mode1);
	Byteswap (p.mode2);
	Byteswap (p.horizontalPanelSize);
	Byteswap (p.verticalPanelSizeLSB);
	Byteswap (p.verticalPanelSizeMSB);
	Byteswap (p.FPLineStartPosition);
	Byteswap (p.horizontalNonDisplayPeriod);
	Byteswap (p.FPFRAMEStartPosition);
	Byteswap (p.verticalNonDisplayPeriod);
	Byteswap (p.MODRate);
	Byteswap (p.screen1StartAddressLSB);
	Byteswap (p.screen1StartAddressMSB);
	Byteswap (p.screen1StartAddressMSBit);
	Byteswap (p.screen2StartAddressLSB);
	Byteswap (p.screen2StartAddressMSB);
	Byteswap (p.screen1StartAddressMSBit);
	Byteswap (p.memoryAddressOffset);
	Byteswap (p.screen1VerticalSizeLSB);
	Byteswap (p.screen1VerticalSizeMSB);
	Byteswap (p.lookUpTableAddress);
	Byteswap (p.unused1);
	Byteswap (p.lookUpTableData);
	Byteswap (p.GPIOConfigurationControl);
	Byteswap (p.GPIOStatusControl);
	Byteswap (p.scratchPad);
	Byteswap (p.portraitMode);
	Byteswap (p.lineByteCountRegister);
	Byteswap (p.unused2);
	Byteswap (p.unused3);
	Byteswap (p.unused4);
}

#ifdef SONY_ROM
void Byteswap (DateTimeType& p)
{
	Byteswap (p.second);
	Byteswap (p.minute);
	Byteswap (p.hour);
	Byteswap (p.day);
	Byteswap (p.month);
	Byteswap (p.year);
	Byteswap (p.weekDay);
}
#endif

#endif	// BYTESWAP
