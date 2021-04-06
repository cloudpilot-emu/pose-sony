// EmRegsEzPegS300.h: EmRegsEzPegS300 クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EmRegsEzPegS300_H__0E459BF5_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
#define AFX_EmRegsEzPegS300_H__0E459BF5_4CBC_11D4_A71C_0090271F0780__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EmRegsEZ.h"

class EmRegsEzPegS300 : public EmRegsEZ  
{
public:
							EmRegsEzPegS300();
	virtual					~EmRegsEzPegS300();

public:
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

#endif // !defined(AFX_EmRegsEzPegS300_H__0E459BF5_4CBC_11D4_A71C_0090271F0780__INCLUDED_)
