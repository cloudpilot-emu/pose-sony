Information about Palm OS(R) Emulator (POSE) for CLIÉ(TM) Handheld


[General]

-The Palm OS Emulator for Sony CLIÉ(TM) Handheld program is freeware and should be used at your own risk.
 
-This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU GENERAL PUBLIC 
LICENSE for more details.

- This program is based on Palm OS Emulator Version 3.4.

- This program emulates only the PEG-NR70 series.

- Before starting the program for the first time, delete the previous "Palm OS Emulator.ini" file in the
Windows system folder (e.g. C:\Windows for Windows XP system and C:\WINNT for Windows 2000). 

- Recommended system requirements:
   CPU: Pentium III 500MHz or above
   Memory: 128MB or more
   OS: Windows(R) 2000 or XP

- To emulate CLIÉ(TM) Handheld, start a new session and select an appropriate CLIÉ(TM) Handheld model and
ROM image.

- This program does not completely emulate the CLIÉ(TM) handheld hardware. It is recommended that the final
validation be made on an actual CLIÉ(TM) handheld device.

- This program does not support "ROM Transfer" option.


[Restrictions]

- Sending continuous key events by holding down BACK/Application/Scroll buttons is not emulated.

- Power button is not activated.

- The emulator works properly only on the "double scale" skin setting (Settings->Skin).

- "Welcome" application does not work on this version of emulator.

- When installing the CLIÉ Mail application taken from the PEG-NR70 installation CD, an error will occur.


[Memory Stick(R)]

- To emulate the insertion and removal of a Memory Stick(R), press the In/Out button located at the left
side of the emulator.

- A virtual folder of the Memory Stick(R) media will be created in the same directory as Emulator.exe where
you start the emulator. The folder name is created based on the selected size:

  Ms8:	8MB	Ms16:	16MB	Ms32:	32MB
  Ms64:	64MB	Ms128:	128MB

- To emulate the write-protect mode of the Memory Stick(R) media, set the Memory Stick(R) folder "read only".
Make sure the Memory Stick(R) media is at OUT state when you change the folder properties, or it will not
function correctly.


[Jog Dial(TM) navigator]

- Press "Push" to issue vchrJogPush event; press "Push" a second time to issue vchrJogRelease event.

- Select "Push" and then "Up" or "Down" to emulate vchrJogPushedUp and vchrJogPushedDown events respectively.

- Select "Push" and then "Repeat" to emulate vchrJogPushRepeat event.


[Miscellaneous]

- If you install an application while the emulator is at Power OFF state, the emulator may hang up.

- The "Save Bound Emulator" option works only on Windows(R) NT4.0 and Windows(R) 2000 Professional Edition.
It does not work on Windows 95, 98, 98SE or ME.

- If you select "Save", "Save As", or "Save Screen" options, you need to manually add saved file extensions
(e.g. .psf or .bmp).

- If you start a scheduling application and set an alarm with "Enable sounds" option set in the emulator's
settings, the alarm may not go off on time.

- "Install Application/Database" option: If you try to install a non-existing application using the previous
log, it will simply quit the menu without any error or warning messages.

- If you connect this emulator with Code Warrior software, your application may fail to enter into debug mode.

- If you have the Caps Lock on when you start the emulator, it will display a dialog box that allows you to
start a new session. Otherwise the emulator will try to load the last session, which may cause problems if
the previous session was for another device.

- If you have chosen the picture from Address and change the category, the error dialog may be displayed and
you may be forced to reset.

* The latest version of the emulator is available at http://www.us.sonypdadev.com/top.html
(You will need to register as a member before downloading)

* CLIÉ, Jog Dial and Memory Stick are trademarks of Sony Corporation.

* Palm OS is a trademark of Palm,Inc. Windows is a registered trademark of Microsoft Corporation.


Copyright (c) 2000-2002 Sony Corporation
