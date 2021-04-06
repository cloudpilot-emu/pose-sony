// EmRegsVzPegNasca.h: EmRegsVzPegNasca クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EmRegsVzPegNasca_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
#define AFX_EmRegsVzPegNasca_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EmRegsVZ.h"

class EmScreenUpdateInfo;

class EmRegsVzPegNasca : public EmRegsVZ  
{
public:
							EmRegsVzPegNasca			(void);
	virtual					~EmRegsVzPegNasca			(void);

	public:
		virtual Bool			GetLCDScreenOn			(void);
		virtual Bool			GetLCDBacklightOn		(void);
		virtual void			GetLCDScanlines			(EmScreenUpdateInfo& info);
		virtual Bool			GetLCDHasFrame			(void);
		virtual void			GetLCDBeginEnd			(emuptr& begin, emuptr& end);
		virtual uint16			GetLEDState				(void);

		virtual Bool			GetSerialPortOn			(int uartNum);
		virtual	Bool			GetVibrateOn			(void);

		virtual uint8			GetPortInputValue		(int);
		virtual uint8			GetPortInternalValue	(int);
		virtual void			GetKeyInfo				(int* numRows, int* numCols,
														 uint16* keyMap, Bool* rows);
	protected:
		virtual EmSPISlave*		GetSPISlave				(void);

	private:
		EmSPISlave*				fSPISlaveADC;
};

#endif // !defined(AFX_EmRegsVzPegNasca_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
