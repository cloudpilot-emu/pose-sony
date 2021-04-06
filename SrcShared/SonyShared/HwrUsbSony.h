/******************************************************************************
 *
 * Copyright (c) 2000 Sony
 * All rights reserved.
 *
 * File: HwrUsb.c
 *
 * Description:
 *		
 *		
 *		
 *
 *****************************************************************************/

typedef struct HwrUsbType {
	UInt8	dataWrite;
	UInt8	dataRead;
	UInt8	cmdWrite;
 	} HwrUsbType;
typedef volatile struct HwrUsbType*	HwrUsbPtr;

#define HwrUsbBase4M	0x10600000L /* 4M ROM, CSA1 base address */
#define HwrUsbBase2M	0x10300000L /* 2M ROM, CSA1 base address */

//#define HwrMyUsbType HwrUsbType
//#define HwrMyUsbPtr HwrUsbPtr

/* HwrMyUsbPtr	baseUsbP = (HwrMyUsbPtr)HwrUsbBase; */


