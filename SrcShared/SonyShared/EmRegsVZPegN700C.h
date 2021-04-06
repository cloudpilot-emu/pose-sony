// EmRegsVzPegN700C.h: EmRegsVzPegN700C クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EmRegsVzPegN700C_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
#define AFX_EmRegsVzPegN700C_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EmRegsVZ.h"
#include "EmRegsSED1376.h"

class EmScreenUpdateInfo;

class EmRegsVzPegN700C : public EmRegsVZ  
{
public:
							EmRegsVzPegN700C();
	virtual					~EmRegsVzPegN700C();

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

#endif // !defined(AFX_EmRegsVzPegN700C_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
