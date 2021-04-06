// EmRegsEzPegS500C.h: EmRegsEzPegS500C クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EmRegsEzPegS500C_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
#define AFX_EmRegsEzPegS500C_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EmRegsEZ.h"

class EmScreenUpdateInfo;

class EmRegsEzPegS500C : public EmRegsEZ  
{
public:
							EmRegsEzPegS500C();
	virtual					~EmRegsEzPegS500C();

public:
	virtual void			GetLCDScanlines			(EmScreenUpdateInfo& info);
	virtual Bool			GetLCDScreenOn			(void);
	virtual Bool			GetLCDBacklightOn		(void);
	virtual Bool			GetSerialPortOn			(int uartNum);

	virtual uint8			GetPortInputValue		(int);
	virtual uint8			GetPortInternalValue	(int);
	virtual void			GetKeyInfo				(int* numRows, int* numCols,
														 uint16* keyMap, Bool* rows);
	protected:
		virtual EmSPISlave*		GetSPISlave				(void);

	private:
		EmSPISlave*				fSPISlaveADC;
};

#endif // !defined(AFX_EmRegsEzPegS500C_H__0E459BF3_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
