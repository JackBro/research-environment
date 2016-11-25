# Microsoft Developer Studio Generated NMAKE File, Based on ipsecgui.dsp
!IF "$(CFG)" == ""
CFG=ipsecgui - Win32 Release
!MESSAGE No configuration specified. Defaulting to ipsecgui - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "ipsecgui - Win32 Release" && "$(CFG)" != "ipsecgui - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ipsecgui.mak" CFG="ipsecgui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ipsecgui - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ipsecgui - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ipsecgui - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\ipsecgui.exe"


CLEAN :
	-@erase "$(INTDIR)\ipsecdlg.obj"
	-@erase "$(INTDIR)\ipsecgui.obj"
	-@erase "$(INTDIR)\ipsecgui.res"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ipsecgui.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /Fp"$(INTDIR)\ipsecgui.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ipsecgui.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ipsecgui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\ipsecgui.pdb" /machine:I386 /out:"$(OUTDIR)\ipsecgui.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ipsecdlg.obj" \
	"$(INTDIR)\ipsecgui.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\ipsecgui.res"

"$(OUTDIR)\ipsecgui.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ipsecgui - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\ipsecgui.exe"


CLEAN :
	-@erase "$(INTDIR)\ipsecdlg.obj"
	-@erase "$(INTDIR)\ipsecgui.obj"
	-@erase "$(INTDIR)\ipsecgui.res"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ipsecgui.exe"
	-@erase "$(OUTDIR)\ipsecgui.ilk"
	-@erase "$(OUTDIR)\ipsecgui.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /Fp"$(INTDIR)\ipsecgui.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ipsecgui.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ipsecgui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\ipsecgui.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ipsecgui.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ipsecdlg.obj" \
	"$(INTDIR)\ipsecgui.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\ipsecgui.res"

"$(OUTDIR)\ipsecgui.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("ipsecgui.dep")
!INCLUDE "ipsecgui.dep"
!ELSE 
!MESSAGE Warning: cannot find "ipsecgui.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ipsecgui - Win32 Release" || "$(CFG)" == "ipsecgui - Win32 Debug"
SOURCE=.\ipsecdlg.cpp

"$(INTDIR)\ipsecdlg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ipsecgui.cpp

"$(INTDIR)\ipsecgui.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ipsecgui.rc

"$(INTDIR)\ipsecgui.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\stdafx.cpp

"$(INTDIR)\stdafx.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

