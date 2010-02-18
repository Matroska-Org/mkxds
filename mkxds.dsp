# Microsoft Developer Studio Project File - Name="mkxds" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mkxds - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mkxds.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mkxds.mak" CFG="mkxds - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mkxds - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mkxds - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mkxds - Win32 Release W98" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mkxds - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MKXDS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../libmatroska" /I "../libebml" /I "../libebml/src/platform/win32" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MKXDS_EXPORTS" /D "_QUEUE_STATS_" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 strmbaseu.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# Begin Custom Build - Registering DirectX Media Filters...
TargetDir=.\Release
TargetPath=.\Release\mkxds.dll
InputPath=.\Release\mkxds.dll
SOURCE="$(InputPath)"

"$(TargetDir)\null.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32.exe /s /c "$(TargetPath)" 
	echo " " > $(TargetDir)\null.txt 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mkxds - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MKXDS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../libmatroska" /I "../libebml" /I "../libebml/src/platform/win32" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MKXDS_EXPORTS" /D "_QUEUE_STATS_" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 strmbasdu.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Registering DirectX Media Filters...
TargetDir=.\Debug
TargetPath=.\Debug\mkxds.dll
InputPath=.\Debug\mkxds.dll
SOURCE="$(InputPath)"

"$(TargetDir)\null.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32.exe /s /c "$(TargetPath)" 
	echo " " > $(TargetDir)\null.txt 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mkxds - Win32 Release W98"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mkxds___Win32_Release_W98"
# PROP BASE Intermediate_Dir "mkxds___Win32_Release_W98"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_W98"
# PROP Intermediate_Dir "Release_W98"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "../../../src" /I "../../../../libebml/src" /I "../../../../libebml/src/platform/win32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MKXDS_EXPORTS" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../libmatroska" /I "../libebml" /I "../libebml/src/platform/win32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MKXDS_EXPORTS" /D "_QUEUE_STATS_" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 strmbase.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 strmbase.lib comctl32.lib libmatroska.lib libebml.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /libpath:"../libmatroska/make/vc6/lib/static/Release" /libpath:"../libebml/make/vc6/lib/static/Release"
# Begin Custom Build - Registering DirectX Media Filters...
TargetDir=.\Release_W98
TargetPath=.\Release_W98\mkxds.dll
InputPath=.\Release_W98\mkxds.dll
SOURCE="$(InputPath)"

"$(TargetDir)\null.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32.exe /s /c "$(TargetPath)" 
	echo " " > $(TargetDir)\null.txt 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "mkxds - Win32 Release"
# Name "mkxds - Win32 Debug"
# Name "mkxds - Win32 Release W98"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=MatroskaReader.cpp
# End Source File
# Begin Source File

SOURCE=mkx_opin.cpp
# End Source File
# Begin Source File

SOURCE=mkxds.cpp
# End Source File
# Begin Source File

SOURCE=mkxds.def
# End Source File
# Begin Source File

SOURCE=.\mkxds.rc
# End Source File
# Begin Source File

SOURCE=mkxdsProperty.cpp
# End Source File
# Begin Source File

SOURCE=mkxPrioFrame.cpp
# End Source File
# Begin Source File

SOURCE=mkxread.cpp
# End Source File
# Begin Source File

SOURCE=..\libebml\src\platform\win32\WinIOCallback.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=codecs.h
# End Source File
# Begin Source File

SOURCE=.\CoreVorbisGUID.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\IChapterInfo.h
# End Source File
# Begin Source File

SOURCE=.\MatroskaReader.h
# End Source File
# Begin Source File

SOURCE=.\mkx_opin.h
# End Source File
# Begin Source File

SOURCE=.\mkxds.h
# End Source File
# Begin Source File

SOURCE=.\mkxdsProperty.h
# End Source File
# Begin Source File

SOURCE=.\mkxPrioFrame.h
# End Source File
# Begin Source File

SOURCE=.\mkxread.h
# End Source File
# Begin Source File

SOURCE=.\OggDS.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE="C:\Program Files\Program\DXSDK\Samples\C++\DirectShow\BaseClasses\source.h"
# End Source File
# Begin Source File

SOURCE=.\Subtitles.h
# End Source File
# Begin Source File

SOURCE=..\libebml\src\platform\win32\WinIOCallback.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
