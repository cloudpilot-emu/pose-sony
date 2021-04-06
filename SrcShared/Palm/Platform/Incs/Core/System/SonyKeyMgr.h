/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2000, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonyKeyMgr.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       KeyBit definitions and KeyManger for Sony System                     *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 99/04/28                                              *
 *       Last Modified: $Date: 00/12/10 11:49 $
 *                                                                            *
 ******************************************************************************/
/* this file is best viewed by setting TAB-stop as 3 */

#ifndef __SONYKEYMGR_H__
#define __SONYKEYMGR_H__

/******************************************************************************
 *    Definition of bit field returned from KeyCurrentState                   *
 ******************************************************************************/
#define keyBitJogSelect		(0x0400)		// JogDial: Push key
#define keyBitJogBack		(0x0800)		// JogDial: Back key

/******************************************************************************
 *    Key manager Routines                                                    *
 ******************************************************************************/
/* We can add some initialization for Sony Original Keys

#ifdef __cplusplus
extern "C" {
#endif

// Set/Get the auto-key repeat rate
Err 		KeyRates(Boolean set, UInt16 *initDelayP, UInt16 *periodP, 
						UInt16 *doubleTapDelayP, Boolean *queueAheadP)
							SYS_TRAP(sysTrapKeyRates);
							
// Get the current state of the hardware keys
// This is now updated every tick, even when more than 1 key is held down.
UInt32	KeyCurrentState(void)
							SYS_TRAP(sysTrapKeyCurrentState);
							
// Set the state of the hardware key mask which controls if the key
// generates a keyDownEvent
UInt32	KeySetMask(UInt32 keyMask)
							SYS_TRAP(sysTrapKeySetMask);
							
#ifdef __cplusplus
}
#endif

*/	
#endif	//__SONYKEYMGR_H__

