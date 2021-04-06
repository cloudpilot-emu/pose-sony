/******************************************************************************
 *                                                                            *
 *                 (C) Copyright 2000, Sony Corporation                       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *                                                                            *
 *    <IDENTIFICATION>                                                        *
 *       file name    : $Workfile: SonyChars.h $
 *                                                                            *
 *    <PROFILE>                                                               *
 *       Sony-specific Virtual character definition                           *
 *                                                                            *
 *    <HISTORY>                                                               *
 *       Started on   : 00/11/28                                              *
 *       Last Modified: $Date: 01/01/05 9:52 $
 *                                                                            *
 ******************************************************************************/
/* this file is best viewed by setting TAB-stop as 3 */

#ifndef __SONYCHARS_H__
#define __SONYCHARS_H__

#define vchrSonyMin				(0x1700)
#define vchrSonyMax				(0x170F)			/* 16 command keys */

/***** Jog *****/
	/* Developers are encouraged to use those chars */
#define vchrJogUp					(0x1700)
#define vchrJogDown				(0x1701)
#define vchrJogPushRepeat		(0x1702)
#define vchrJogPushedUp			(0x1703)
#define vchrJogPushedDown		(0x1704)
#define vchrJogPush				(0x1705)
#define vchrJogRelease			(0x1706)
#define vchrJogBack				(0x1707) 		/* added @ 2001 */

/*** re-define old key names ***/
	/* Developpers are encouraged not to use those obsolete chars */
#define vchrJogPressRepeat		vchrJogPushRepeat
#define vchrJogPageUp			vchrJogPushedUp
#define vchrJogPageDown			vchrJogPushedDown
#define vchrJogPress				vchrJogPush	

/* movement & chars */
/*
        Up     PushedUp
        A        A
        |        |
        |      --
        |    --
           --
         ---------> Push/PushRepeat
Release <---------
           --
        |    --
        |      --
        |        |
        V        V
       Down    PushedDown


        -----------> Back
*/

/***** Rmc *****/
#define vchrRmcKeyPush			(0x170A)
#define vchrRmcKeyRelease		(0x170B)
	/* each remocon-key is identified with keyCode field */
	/* autoRepeat modifier is set when KeyPush is repeated */

/***** Notification Trigger *****/
#define vchrSonySysNotify		(0x170F)

/*** Access macros ***/
	/* eP must be EventPtr */
#define SonyKeyIsJog(eP)	\
	(((eP->data.keyDown.chr >= vchrJogUp)	\
		 && (eP->data.keyDown.chr <= vchrJogBack))? true: false) 

#endif // __SONYCHARS_H__

