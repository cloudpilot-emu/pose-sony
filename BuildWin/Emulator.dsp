# Microsoft Developer Studio Project File - Name="Emulator" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Emulator - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "Emulator.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "Emulator.mak" CFG="Emulator - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "Emulator - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "Emulator - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE "Emulator - Win32 Release_Internal" ("Win32 (x86) Application" 用)
!MESSAGE "Emulator - Win32 Release_Profile" ("Win32 (x86) Application" 用)
!MESSAGE "Emulator - Win32 Release_Internal_Profile" ("Win32 (x86) Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Emulator - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D INCLUDE_SECRET_STUFF=0 /D HAS_PROFILING=0 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /D "SONY_ROM" /D "BUILDING_AGAINST_PALMOS35" /D "NDEBUG" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=0 /D PLATFORM_WINDOWS=1 /FR /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i ".." /i "..\SrcWin" /i "..\SrcWin\Res" /i "..\SrcShared" /i "..\..\SrcShared" /d "NDEBUG" /d INCLUDE_SECRET_STUFF=0 /d HAS_PROFILING=0 /d PLATFORM_WINDOWS=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib winmm.lib /nologo /subsystem:windows /incremental:yes /machine:I386
# SUBTRACT LINK32 /profile /map /debug

!ELSEIF  "$(CFG)" == "Emulator - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=1 /FR /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GR /GX /ZI /Od /D "SONY_ROM" /D "BUILDING_AGAINST_PALMOS35" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=1 /D PLATFORM_WINDOWS=1 /FR /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i ".." /i "..\SrcWin" /i "..\SrcWin\Res" /i "..\SrcShared" /i "..\..\SrcShared" /d "_DEBUG" /d INCLUDE_SECRET_STUFF=1 /d HAS_PROFILING=1 /d PLATFORM_WINDOWS=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/Emulator_Debug.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Emulator - Win32 Release_Internal"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_Internal"
# PROP BASE Intermediate_Dir "Release_Internal"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Internal"
# PROP Intermediate_Dir "Release_Internal"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=0 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /D "SONY_ROM" /D "BUILDING_AGAINST_PALMOS35" /D "NDEBUG" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=0 /D PLATFORM_WINDOWS=1 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i ".." /i "..\SrcWin" /i "..\SrcWin\Res" /i "..\SrcShared" /i "..\..\SrcShared" /d "NDEBUG" /d INCLUDE_SECRET_STUFF=1 /d HAS_PROFILING=0 /d PLATFORM_WINDOWS=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /map /machine:I386
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib winmm.lib /nologo /subsystem:windows /map /machine:I386 /out:"Release_Internal/Emulator_Internal.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Emulator - Win32 Release_Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Emulator"
# PROP BASE Intermediate_Dir "Emulator"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Profile"
# PROP Intermediate_Dir "Release_Profile"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D INCLUDE_SECRET_STUFF=0 /D HAS_PROFILING=1 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /D "SONY_ROM" /D "BUILDING_AGAINST_PALMOS35" /D "NDEBUG" /D INCLUDE_SECRET_STUFF=0 /D HAS_PROFILING=1 /D PLATFORM_WINDOWS=1 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i ".." /i "..\SrcWin" /i "..\SrcWin\Res" /i "..\SrcShared" /i "..\..\SrcShared" /d "NDEBUG" /d INCLUDE_SECRET_STUFF=0 /d HAS_PROFILING=1 /d PLATFORM_WINDOWS=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib /nologo /subsystem:windows /map /machine:I386
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib winmm.lib /nologo /subsystem:windows /map /machine:I386 /out:"Release_Profile/Emulator_Profile.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "Emulator - Win32 Release_Internal_Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Emulator___Win32_Release_Internal_Profile"
# PROP BASE Intermediate_Dir "Emulator___Win32_Release_Internal_Profile"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Internal_Profile"
# PROP Intermediate_Dir "Release_Internal_Profile"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=1 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /D "SONY_ROM" /D "BUILDING_AGAINST_PALMOS35" /D "NDEBUG" /D INCLUDE_SECRET_STUFF=1 /D HAS_PROFILING=1 /D PLATFORM_WINDOWS=1 /Yu"EmCommon.h" /FD @CppOptions.txt /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i ".." /i "..\SrcWin" /i "..\SrcWin\Res" /i "..\SrcShared" /i "..\..\SrcShared" /d "NDEBUG" /d INCLUDE_SECRET_STUFF=1
# ADD RSC /l 0x409 /i ".." /i "..\SrcWin" /i "..\SrcWin\Res" /i "..\SrcShared" /i "..\..\SrcShared" /d "NDEBUG" /d INCLUDE_SECRET_STUFF=1 /d HAS_PROFILING=1 /d PLATFORM_WINDOWS=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib /nologo /subsystem:windows /map /machine:I386 /out:"Release_Internal/Emulator_Internal.exe"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib wsock32.lib winmm.lib /nologo /subsystem:windows /map /machine:I386 /out:"Release_Internal_Profile/Emulator_Internal_Profile.exe"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "Emulator - Win32 Release"
# Name "Emulator - Win32 Debug"
# Name "Emulator - Win32 Release_Internal"
# Name "Emulator - Win32 Release_Profile"
# Name "Emulator - Win32 Release_Internal_Profile"
# Begin Group "Emulator - Windows"

# PROP Default_Filter ""
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcWin\Res\DefaultLarge.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\DefaultSmall.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Document.ico
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Emulator.ico
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Emulator.manifest
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Emulator.rc
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Emulator2.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Finger.cur
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\resource.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Strings.r.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\Strings.rc

!IF  "$(CFG)" == "Emulator - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Emulator - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Emulator - Win32 Release_Internal"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Emulator - Win32 Release_Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Emulator - Win32 Release_Internal_Profile"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\SrcShared\Strings.txt
# End Source File
# End Group
# Begin Group "Incs - Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CppOptions.txt
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmApplicationWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmCommonWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmDirRefWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmDlgWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmDocumentWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmFileRefWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmMenusWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmPatchLoaderWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmPixMapWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmRegionWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmTransportSerialWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmTransportUSBWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmWindowWin.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\MMFMessaging.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Platform.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Platform_NetLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Sounds.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\StartMenu.h
# End Source File
# End Group
# Begin Group "Sony - Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcWin\SonyWin\LCD_PenEvent_Sony.inl
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\LCDSony.inl
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\OpenFileList.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\Platform_ExpMgrLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\Platform_ExpMgrLib_Win.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\Platform_MsfsLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\Platform_MsfsLib_Win.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\Platform_VfsLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\Platform_VfsLib_Win.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\SonyButtonProc.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\SonyWin\SonyButtonProc.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\SrcWin\EmApplicationWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmDirRefWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmDlgWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmDocumentWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmFileRefWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmMenusWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmPatchLoaderWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmPixMapWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmRegionWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmTransportSerialWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmTransportUSBWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\EmWindowWin.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\MMFMessaging.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Platform_NetLib_Sck.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Platform_Win.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Sounds.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\StartMenu.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\TracerPlatform.cpp
# End Source File
# End Group
# Begin Group "Emulator - Shared"

# PROP Default_Filter ""
# Begin Group "Incs - Shared"

# PROP Default_Filter ""
# Begin Group "Shared Includes"

# PROP Default_Filter ""
# Begin Group "Palm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\Palm\Platform\Incs\Core\UI\AttentionMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Bitmap.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\BuildDefaults.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\BuildDefines.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\CharAttr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Chars.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\CoreTraps.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Crc.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\DataMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\DataPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\DateTime.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\DebugPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\DLCommon.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\DLServer.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\ErrorBase.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\Event.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\FatalAlert.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\FeatureMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\Field.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\Find.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Font.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\Form.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\Globals.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\Hardware\HAL.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\Hardware\IncsPrv\Hardware.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\device\EZAustin\IncsPrv\HardwareAustin.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\device\EZSumo\IncsPrv\HardwareEZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\device\328Jerry\IncsPrv\HardwareTD1.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\Device\VZTrn\IncsPrv\HardwareVZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\Hardware\HwrMiscFlags.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\Hardware\IncsPrv\HwrROMToken.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\InsPoint.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\IntlMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\KeyMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Libraries\LibTraps.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\Platform\Incs\Core\System\LocaleMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Localize.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\Hardware\IncsPrv\M68328Hwr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\Hardware\IncsPrv\M68EZ328Hwr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\Hardware\M68KHwr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\Platform\Core\Hardware\IncsPrv\M68SZ328Hwr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\Platform\Core\Hardware\IncsPrv\M68VZ328Hwr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\MemoryMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\MemoryPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\NetBitUtils.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\NetMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\OverlayMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\Platform\Incs\Core\System\PalmLocale.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\PalmTypes.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\PenMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Preferences.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Rect.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\ScrollBar.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\device\EZAustin\IncsPrv\SED1375Hwr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SerialLinkMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\SerialLinkPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SerialMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\Platform\Incs\Core\System\SoundMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SysEvent.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SysEvtMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\SysEvtPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SystemMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SystemPkt.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\IncsPrv\SystemPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\SystemResources.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\Table.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\TextMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\UI\UIResources.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Incs\Core\System\Window.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\SrcShared\Palm.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\PalmOptErrorCheckLevel.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\PalmPack.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\PalmPackPop.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\String2
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Switches.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\UAE.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\xstring2
# End Source File
# End Group
# Begin Source File

SOURCE="..\SrcShared\ATraps.h"
# End Source File
# Begin Source File

SOURCE=..\SrcWin\bytesex.h
# End Source File
# Begin Source File

SOURCE="..\SrcShared\Byteswapping.h"
# End Source File
# Begin Source File

SOURCE="..\SrcShared\CGremlins.h"
# End Source File
# Begin Source File

SOURCE="..\SrcShared\CGremlinsStubs.h"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\ChunkFile.h
# End Source File
# Begin Source File

SOURCE="..\SrcShared\DebugMgr.h"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EcmIf.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EcmObject.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmAction.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmApplication.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmAssert.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmCommands.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmCommon.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDevice.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDirRef.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDlg.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDocument.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmErrCodes.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmEventOutput.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmEventPlayback.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmException.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmExgMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmFileImport.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmFileRef.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmJPEG.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmLowMem.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmMapFile.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmMenus.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmMinimize.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmFunction.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmHeap.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmOS.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmStructs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmStructs.i
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPixMap.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPoint.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmQuantizer.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRect.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRefCounted.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRegion.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmROMReader.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmROMTransfer.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRPC.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmScreen.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmSession.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmStream.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmStreamFile.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmStructs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmSubroutine.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmThreadSafeQueue.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransport.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransportSerial.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransportSocket.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransportUSB.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTypes.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmWindow.h
# End Source File
# Begin Source File

SOURCE=..\SrcWin\endian.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\ErrorHandling.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hordes.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\HostControl.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\HostControlPrv.h
# End Source File
# Begin Source File

SOURCE="..\SrcShared\LoadApplication.h"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Logging.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Marshal.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\MetaMemory.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Miscellaneous.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\PreferenceMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Profiling.h
# End Source File
# Begin Source File

SOURCE="..\SrcShared\ROMStubs.h"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SessionFile.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Skins.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SLP.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SocketMessaging.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Startup.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\StringConversions.h
# End Source File
# Begin Source File

SOURCE="..\SrcShared\StringData.h"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SystemPacket.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\TracerCommon.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\TracerPlatform.h
# End Source File
# End Group
# Begin Group "Hardware"

# PROP Default_Filter ""
# Begin Group "Incs - Hardware"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankDRAM.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankDummy.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankMapped.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankRegs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankROM.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankSRAM.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmCPU.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmCPU68K.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmCPUARM.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmHAL.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmMemory.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328PalmIII.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328PalmPilot.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328PalmVII.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328Pilot.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328Prv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328Symbol1700.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsASICSymbol1700.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmIIIc.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmIIIe.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmIIIx.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmM100.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmV.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmVII.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmREgsEZPalmVIIx.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmVx.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZTemp.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZTRGpro.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZVisor.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsFrameBuffer.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsMediaQ11xx.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsPLDPalmVIIEZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSED1375.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSED1376.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSZPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSZTemp.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsUSBPhilipsPDIUSBD12.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsUSBVisor.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZHandEra330.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZPalmM500.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZPalmM505.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZPrv.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZTemp.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZVisorEdge.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZVisorPlatinum.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZVisorPrism.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmSPISlave.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmSPISlaveADS784x.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmUAEGlue.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmUARTDragonball.h
# End Source File
# End Group
# Begin Group "TRG"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmHandEra330Defs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmHandEraCFBus.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmHandEraSDBus.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmRegs330CPLD.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmRegs330CPLD.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmSPISlave330Current.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmSPISlave330Current.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRG.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRG.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGATA.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGATA.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCF.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCF.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCFDefs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCFIO.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCFIO.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCFMem.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGCFMem.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGDiskIO.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGDiskIO.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGDiskType.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGDiskType.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGSD.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\TRG\EmTRGSD.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankDRAM.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankDummy.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankMapped.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankRegs.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankROM.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmBankSRAM.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmCPU.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmCPU68K.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmCPUARM.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmHAL.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmMemory.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328PalmPilot.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegs328Symbol1700.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsASICSymbol1700.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZ.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmIIIc.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmM100.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmV.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmVII.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZPalmVIIx.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZTemp.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZTRGpro.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsEZVisor.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsFrameBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsMediaQ11xx.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsPLDPalmVIIEZ.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSED1375.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSED1376.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSZ.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsSZTemp.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsUSBPhilipsPDIUSBD12.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsUSBVisor.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZ.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZHandEra330.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZPalmM500.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZPalmM505.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZTemp.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZVisorEdge.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZVisorPlatinum.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmRegsVZVisorPrism.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmSPISlave.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmSPISlaveADS784x.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmUAEGlue.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hardware\EmUARTDragonball.cpp
# End Source File
# End Group
# Begin Group "UAE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\UAE\compiler.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\config.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\cpudefs.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\cpuemu.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\cpustbl.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\cputbl.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\custom.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\machdep_m68k.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\machdep_maccess.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\memory_cpu.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\newcpu.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\options.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\readcpu.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\readcpu.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\sysconfig.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\sysdeps.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\UAE\target.h
# End Source File
# End Group
# Begin Group "Gzip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\Gzip\bits.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\crypt.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\deflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\gzip.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\inflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\lzw.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\revision.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\tailor.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\trees.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Gzip\util.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "omnithread"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\omnithread\nt.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\omnithread\nt.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\omnithread\omnithread.h
# End Source File
# End Group
# Begin Group "jpeg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\jpeg\cderror.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcapimin.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcapistd.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jccoefct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jccolor.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcdctmgr.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jchuff.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jchuff.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcinit.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcmainct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcmarker.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcmaster.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcomapi.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcWin\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcparam.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcphuff.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcprepct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jcsample.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jctrans.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdapimin.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdapistd.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdatadst.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdatasrc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdcoefct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdcolor.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdct.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jddctmgr.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdhuff.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdinput.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdmainct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdmarker.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdmaster.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdmerge.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdphuff.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdpostct.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdsample.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jdtrans.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jerror.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jerror.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jfdctflt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jfdctfst.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jfdctint.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jidctflt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jidctfst.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jidctint.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jidctred.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jmemmgr.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jmemnobs.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jquant1.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jquant2.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jutils.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\SrcShared\jpeg\jversion.h
# End Source File
# End Group
# Begin Group "Patches"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchIf.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchLoader.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchMgr.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModule.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModule.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleHtal.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleHtal.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleMap.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleMap.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleMemMgr.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleNetLib.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleNetLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleSys.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleSys.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchModuleTypes.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchState.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Patches\EmPatchState.h
# End Source File
# End Group
# Begin Group "Sony - Shared"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\SrcShared\SonyShared\Bank_USBSony.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\Bank_USBSony.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmDeviceSony.inl
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_ExpMgr.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_ExpMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_MsfsLib.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_MsfsLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_SlotDrvLib.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_SlotDrvLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_VfsLib.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmPatchModule_VfsLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsClockIRQCntrl.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsClockIRQCntrl.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsCommandItf.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsCommandItf.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsExpCardCLIE.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsExpCardCLIE.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsEzPegS300.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsEzPegS300.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsEzPegS500C.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsEzPegS500C.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsFMSound.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsFMSound.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsFMSoundforSZ.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsFMSoundforSZ.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsLCDCtrl.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsLCDCtrl.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsLCDCtrlT2.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsLCDCtrlT2.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsRAMforCLIE.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsRAMforCLIE.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsSharedRAMforCLIE.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsSharedRAMforCLIE.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsSZNaples.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsSZNaples.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsSZRedwood.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsSZRedwood.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegModena.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegModena.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegN700C.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegN700C.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegNasca.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegNasca.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegVenice.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegVenice.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegYellowStone.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\EmRegsVZPegYellowStone.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\ExpansionMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\Ffs.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\FSLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\HwrUsbSony.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\MarshalSony.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\MarshalSony.inl
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\MiscellaneousSony.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\MiscellaneousSony.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\ROMStubsSony.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\SessionFileSony.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\SlotDrvLib.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\SonyChars.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\SonyDevice.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\SonyKeyMgr.h
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SonyShared\VFSMgr.h
# End Source File
# End Group
# Begin Source File

SOURCE="..\SrcShared\ATraps.cpp"
# End Source File
# Begin Source File

SOURCE="..\SrcShared\Byteswapping.cpp"
# End Source File
# Begin Source File

SOURCE="..\SrcShared\CGremlins.cpp"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\CGremlinsStubs.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\ChunkFile.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Palm\platform\Core\System\Src\Crc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE="..\SrcShared\DebugMgr.cpp"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmAction.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmApplication.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmCommon.cpp
# ADD CPP /Yc"EmCommon.h"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDevice.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDirRef.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmEventOutput.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmEventPlayback.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmException.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmExgMgr.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmFileImport.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmFileRef.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmJPEG.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmLowMem.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmMapFile.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmMenus.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmMinimize.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmFunction.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmHeap.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmOS.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPalmStructs.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPixMap.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmPoint.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmQuantizer.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRect.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRefCounted.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRegion.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmROMReader.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmROMTransfer.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmRPC.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmScreen.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmSession.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmStream.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmStreamFile.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmSubroutine.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmThreadSafeQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransport.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransportSerial.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransportSocket.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmTransportUSB.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\EmWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\ErrorHandling.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Hordes.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\HostControl.cpp
# End Source File
# Begin Source File

SOURCE="..\SrcShared\LoadApplication.cpp"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Logging.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Marshal.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\MetaMemory.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Miscellaneous.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\PreferenceMgr.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Profiling.cpp
# End Source File
# Begin Source File

SOURCE="..\SrcShared\ROMStubs.cpp"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SessionFile.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Skins.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SLP.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SocketMessaging.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\Startup.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\StringConversions.cpp
# End Source File
# Begin Source File

SOURCE="..\SrcShared\StringData.cpp"
# End Source File
# Begin Source File

SOURCE=..\SrcShared\SystemPacket.cpp
# End Source File
# Begin Source File

SOURCE=..\SrcShared\TracerCommon.cpp
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter "*.txt"
# Begin Source File

SOURCE=..\Docs\_Bugs.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_Building.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_Contributing.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_Credits.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_GPL.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_News.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_OldNews.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_ReadMe.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\_ToDo.txt
# End Source File
# Begin Source File

SOURCE=..\Docs\NotesPrv.txt
# End Source File
# End Group
# Begin Group "Scripting"

# PROP Default_Filter ""
# Begin Group "Perl"

# PROP Default_Filter ".pl;.pm"
# Begin Source File

SOURCE=..\Scripting\Perl\EmFunctions.pm
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\EmRPC.pm
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\EmSysTraps.pm
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\EmUtils.pm
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\FormSpy.pl
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\ListOpenDatabases.pl
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\MakeSysTraps.pl
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\PoserRPC.pl
# End Source File
# Begin Source File

SOURCE=..\Scripting\Perl\SkipStartup.pl
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\SrcWin\Res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\down_down.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\down_release.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\esc_pres.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\esc_rele.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\ms_disab.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\ms_press.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\ms_release.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\power_press.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\power_release.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\push_down.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\push_release.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\repeat_down.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\repeat_off.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\repeat_release.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\up_down.bmp
# End Source File
# Begin Source File

SOURCE=..\SrcWin\Res\up_release.bmp
# End Source File
# End Target
# End Project
