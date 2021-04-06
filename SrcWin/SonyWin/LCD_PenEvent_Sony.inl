#ifdef SONY_ROM
	if ((down && IsLocalControlButton(what)) || (!down && IsLocalControlButton(fCurrentButton)))
	{
		if (gButtonTracking)
		{
			gButtonTracking = down;
			if (!gButtonTracking)
			{
				::ReleaseCapture ();
				if (IsJogButton(fCurrentButton) && fCurrentButton != kElement_JogPush)	// for Sony & JogDial
				{
					if (fCurrentButton == kElement_JogRepeat) 
					{
						if (gidJogKeyRepeat) 
						{
							::KillTimer(EmWindow::GetWindow()->GetHostData()->GetHostWindow(), gidJogKeyRepeat);
							gidJogKeyRepeat = NULL;
						}
						::LCD_SetStateJogButton(fCurrentButton, false, true);
						::LCD_DrawButtonForPEG(NULL, fCurrentButton);
					}
					else
					{
						::LCD_SetStateJogButton(fCurrentButton, false, true);
						::LCD_DrawButtonForPEG(NULL, fCurrentButton);
					}
				}
			}
			return ;
		}
		else if (down)
		{
			gButtonTracking = down;
			// press JogDial button.	
			if (IsJogButton(what))				// for Sony & JogDial
			{
				// Power On/Off状態のチェック
				Bool lcdOn = EmHAL::GetLCDScreenOn ();
				if (!lcdOn)
					return;		// Power Off 状態

				if (what == kElement_JogRepeat && !::LCD_IsEnabledJogButton(what))
					return;

				::SetCapture (EmWindow::GetWindow()->GetHostData()->GetHostWindow());
				if (what == kElement_JogRepeat)
				{
					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped ())
						::EvtEnqueueKey(vchrJogPressRepeat, 0, commandKeyMask);	// for Sony & JogDial

					gidJogKeyRepeat = ::SetTimer(EmWindow::GetWindow()->GetHostData()->GetHostWindow(), TIMERID_JOGKEY_REPAET_STARTER, JOGKEY_REPAET_STARTTIME, JogKeyRepeatStarterProc);
					::LCD_SetStateJogButton(what, true, true);
				}
				else if (what == kElement_JogPush)
				{
					if (::LCD_IsPressJogButton(what))
					{
						::LCD_SetStateJogButton(kElement_JogPush, false, true);
						::LCD_DrawButtonForPEG(NULL, kElement_JogPush);
	
						::LCD_SetStateJogButton(kElement_JogRepeat, false, false);
						::LCD_DrawButtonForPEG(NULL, kElement_JogRepeat);

						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							::EvtEnqueueKey(vchrJogRelease, 0, commandKeyMask);	// for Sony & JogDial
					}
					else
					{
						::LCD_SetStateJogButton(kElement_JogRepeat, false, true);
						::LCD_DrawButtonForPEG(NULL, kElement_JogRepeat);

						::LCD_SetStateJogButton(kElement_JogPush, true, true);

						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							::EvtEnqueueKey(vchrJogPress, 0, commandKeyMask);	// for Sony & JogDial
					}
				}
				else
				{
					UInt16	nKey = 0;
					switch (what)
					{
					case kElement_JogUp: nKey = (::LCD_IsPressJogButton(kElement_JogPush)) ? vchrJogPageUp : vchrJogUp; break;
					case kElement_JogDown: nKey = (::LCD_IsPressJogButton(kElement_JogPush)) ? vchrJogPageDown : vchrJogDown; break;
					}

					EmSessionStopper	stopper (gSession, kStopOnSysCall);
					if (stopper.Stopped ())
						::EvtEnqueueKey(nKey, 0, commandKeyMask);	// for Sony & JogDial

					::LCD_SetStateJogButton(what, true, true);
				}
				fCurrentButton = what;	// for Sony & JogDial
				gButtonTracking = TRUE;	// for Sony & JogDial

				::LCD_DrawButtonForPEG(NULL, fCurrentButton);
				return ;
			}

			// Press MS-Card[ON/OFF] button on PEG's skin
			else if (what == kElement_MS_InOut) 
			{
				// Power On/Off状態のチェック
				Bool lcdOn = EmHAL::GetLCDScreenOn ();

				::SetCapture (EmWindow::GetWindow()->GetHostData()->GetHostWindow());	// for Sony & MemoryStick

				fCurrentButton = what;	// for Sony & MemoryStick
				gButtonTracking = TRUE;	// for Sony & MemoryStick

				if (g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE)
				{
					if (!lcdOn || !Platform_ExpMgrLib::IsUsable())		// PowerOffまたはReset処理中の状態
						g_nCardInserted = MSSTATE_REQ_INSERT;			// ExpCardInserted 要求
					else
					{
						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							ExpCardInserted(1);
					}
				}
				else
				{
					if (!lcdOn || !Platform_ExpMgrLib::IsUsable())		// PowerOffまたはReset処理中の状態
						g_nCardInserted = MSSTATE_REQ_REMOVE;			// ExpCardRemoved 要求
					else
					{
						EmSessionStopper	stopper (gSession, kStopOnSysCall);
						if (stopper.Stopped ())
							::ExpCardRemoved(1);
					}
				}

				::LCD_SetStateJogButton(kElement_MS_InOut, (g_nCardInserted == MSSTATE_REMOVED || g_nCardInserted == MSSTATE_REQ_REMOVE) ? false : true, true);
				::LCD_DrawButtonForPEG(NULL, kElement_MS_InOut);

				return ;
			}
		}
	}
	else if ((down && what == kElement_JogESC) || (!down && fCurrentButton == kElement_JogESC))
	{
		::LCD_SetStateJogButton(kElement_JogESC, down, true);
		::LCD_DrawButtonForPEG(NULL, kElement_JogESC);
	}
#endif	// SONY_ROM
