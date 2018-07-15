# Microsoft Developer Studio Project File - Name="q1mv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=q1mv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "q1mv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "q1mv.mak" CFG="q1mv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "q1mv - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "q1mv - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "q1mv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\temp\q1mv\!release"
# PROP Intermediate_Dir "..\..\temp\q1mv\!release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\mxtk" /I "..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x807 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 mxtk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib opengl32.lib glu32.lib comctl32.lib winmm.lib /nologo /entry:"mainCRTStartup" /subsystem:windows /pdb:none /machine:I386 /nodefaultlib:"libcmt" /release /opt:nowin98
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build
TargetDir=\Quake\ID1\src_main\temp\q1mv\!release
InputPath=\Quake\ID1\src_main\temp\q1mv\!release\q1mv.exe
SOURCE="$(InputPath)"

"C:\Program Files\ModelViewer\q1mv.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetDir)\q1mv.exe "C:\Program Files\ModelViewer\q1mv.exe"

# End Custom Build

!ELSEIF  "$(CFG)" == "q1mv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\temp\q1mv\!debug"
# PROP Intermediate_Dir "..\..\temp\q1mv\!debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\mxtk" /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FAs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x807 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mxtk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib opengl32.lib glu32.lib winmm.lib /nologo /entry:"mainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd" /pdbtype:sept
# Begin Custom Build
TargetDir=\Quake\ID1\src_main\temp\q1mv\!debug
InputPath=\Quake\ID1\src_main\temp\q1mv\!debug\q1mv.exe
SOURCE="$(InputPath)"

"C:\Program Files\ModelViewer\q1mv.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetDir)\q1mv.exe "C:\Program Files\ModelViewer\q1mv.exe"

# End Custom Build

!ENDIF 

# Begin Target

# Name "q1mv - Win32 Release"
# Name "q1mv - Win32 Debug"
# Begin Source File

SOURCE=.\alias_render.cpp
# End Source File
# Begin Source File

SOURCE=.\alias_utils.cpp
# End Source File
# Begin Source File

SOURCE=.\anorms.h
# End Source File
# Begin Source File

SOURCE=..\common\common.cpp
# End Source File
# Begin Source File

SOURCE=.\FileAssociation.cpp
# End Source File
# Begin Source File

SOURCE=.\FileAssociation.h
# End Source File
# Begin Source File

SOURCE=.\GlWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\GlWindow.h
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=..\common\mathlib.cpp
# End Source File
# Begin Source File

SOURCE=.\mdlviewer.cpp
# End Source File
# Begin Source File

SOURCE=.\mdlviewer.h
# End Source File
# Begin Source File

SOURCE=.\pakviewer.cpp
# End Source File
# Begin Source File

SOURCE=.\pakviewer.h
# End Source File
# Begin Source File

SOURCE=.\q1mv.rc
# End Source File
# Begin Source File

SOURCE=..\common\stringlib.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewerSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewerSettings.h
# End Source File
# End Target
# End Project
