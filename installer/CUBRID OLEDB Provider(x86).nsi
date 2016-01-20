;--------------------------------
; CUBRID OLE DB Data Provider Installer/Uninstaller
; Ver 9.0.0
; Last update: January 2013
;--------------------------------

SetCompressor /solid lzma

;--------------------------------
;Include Modern UI
;!include "MUI.nsh"
!include "MUI2.nsh"

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!define MUI_ICON CUBRID.ico
!define MUI_UNICON CUBRID.ico

;--------------------------------

!include WordFunc.nsh
!insertmacro VersionCompare

!include LogicLib.nsh

; Include functions and plugin
!addplugindir "." 

; The name of the installer
Name "CUBRID OLE DB Data Provider 9.0.0"

; The file to write
OutFile "CUBRID OLE DB Data Provider 9.0.0 (x86).exe"
;OutFile "CUBRID OLE DB Data Provider 9.0.0 (x64).exe"

;required on Windows Vista and Windows 7
RequestExecutionLevel admin

; The default installation directory
InstallDir "$PROGRAMFILES\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)"

; Registry key to check for installation directory
InstallDirRegKey HKLM "Software\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)" "Install_Dir"

ShowInstDetails show
ShowUnInstDetails show

;--------------------------------

; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "BSD License.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
Page InstFiles
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME  
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

!insertmacro MUI_LANGUAGE "English"

;--------------------------------

; Components to install
Section "CUBRID OLE DB Data Provider Files" DataProviderFiles

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  CreateDirectory "$OUTDIR"

  File "/oname=$OUTDIR\BSD License.txt" "..\Source\Documents\BSD License.txt"
  File "/oname=$OUTDIR\Release notes.txt" "..\Source\Documents\Release notes.txt"
  File "/oname=$OUTDIR\CUBRIDProvider.dll" "..\Source\Win32\Release\CUBRIDProvider.dll"

  ;Register provider
  WriteRegStr HKCR "AppID\{2B22247F-E9F7-47e8-A9B0-79E8039DCFC8}" "" "CUBRID.Provider"
  
  WriteRegStr HKCR "AppID\CUBRIDProvider.DLL" "AppID" "{2B22247F-E9F7-47E8-A9B0-79E8039DCFC8}"
  
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}" "" "CUBRID.Provider"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}" "AppID" "{2B22247F-E9F7-47E8-A9B0-79E8039DCFC8}"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\ExtendedErrors" "" "Extended Error Service"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\ExtendedErrors\{3165D76D-CB91-482F-9378-00C216FD5F32}" "" "CUBRID OLE DB Provider Error Lookup Service"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\InprocServer32" "" "$INSTDIR\CUBRIDProvider.dll"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\InprocServer32" "ThreadingModel" "Apartment"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\OLE DB Provider" "" "CUBRID OLE DB Provider"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\ProgID" "" "CUBRID.Provider.9.0"
  WriteRegStr HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}\VersionIndependentProgID" "" "CUBRID.Provider"

  WriteRegStr HKCR "CLSID\{3165D76D-CB91-482F-9378-00C216FD5F32}" "" "CUBRID OLE DB Provider Error Lookup Service"
  WriteRegStr HKCR "CLSID\{3165D76D-CB91-482F-9378-00C216FD5F32}\InprocServer32" "" "$INSTDIR\CUBRIDProvider.dll"
  WriteRegStr HKCR "CLSID\{3165D76D-CB91-482F-9378-00C216FD5F32}\InprocServer32" "ThreadingModel" "Apartment"
  WriteRegStr HKCR "CLSID\{3165D76D-CB91-482F-9378-00C216FD5F32}\ProgID" "" "CUBRID.ErrorLookup.9.0"
  WriteRegStr HKCR "CLSID\{3165D76D-CB91-482F-9378-00C216FD5F32}\VersionIndependentProgID" "" "CUBRID.ErrorLookup"

  WriteRegStr HKCR "CUBRID.ErrorLookup" "" "CUBRID OLE DB Provider Error Lookup Service"
  WriteRegStr HKCR "CUBRID.ErrorLookup\CLSID" "" "{3165D76D-CB91-482F-9378-00C216FD5F32}"
  WriteRegStr HKCR "CUBRID.ErrorLookup\CurVer" "" "CUBRID.ErrorLookup.9.0"

  WriteRegStr HKCR "CUBRID.ErrorLookup.9.0" "" "CUBRID OLE DB Provider Error Lookup Service"
  WriteRegStr HKCR "CUBRID.ErrorLookup.9.0\CLSID" "" "{3165D76D-CB91-482F-9378-00C216FD5F32}"

  WriteRegStr HKCR "CUBRID.Provider" "" "CUBRID OLE DB Provider"
  WriteRegStr HKCR "CUBRID.Provider\CLSID" "" "{15A12058-4353-4C9A-8421-23D80F25EE4E}"
  WriteRegStr HKCR "CUBRID.Provider\CurVer" "" "CUBRID.Provider.9.0"

  WriteRegStr HKCR "CUBRID.Provider.9.0" "" "CUBRID OLE DB Provider"
  WriteRegStr HKCR "CUBRID.Provider.9.0\CLSID" "" "{15A12058-4353-4C9A-8421-23D80F25EE4E}"

  ; Write the installation path
  WriteRegStr HKLM "SOFTWARE\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)" "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall information
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CUBRID OLE DB Data Provider 9.0.0 (x86)" "DisplayName" "CUBRID OLE DB Data Provider 9.0.0 (x86)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CUBRID OLE DB Data Provider 9.0.0 (x86)" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CUBRID OLE DB Data Provider 9.0.0 (x86)" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CUBRID OLE DB Data Provider 9.0.0 (x86)" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd


; Optional section (can be disabled by the user)
Section "Start Menu shortcuts" DataProviderShortcuts

  CreateDirectory "$SMPROGRAMS\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)"
  CreateShortCut "$SMPROGRAMS\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${DataProviderFiles} "CUBRID OLE DB Data Provider library"
  !insertmacro MUI_DESCRIPTION_TEXT ${DataProviderShortcuts} "Create shortcuts to CUBRID OLE DB Data Provider"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Installer Functions

Function .onInit
UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights on NT4+
	MessageBox mb_iconstop "Administrator rights required!"
	SetErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
	Quit
${EndIf}
FunctionEnd


Function un.onUninstSuccess

  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "The CUBRID OLE DB Data Provider 9.0.0 was successfully removed from your computer."

FunctionEnd


Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove the CUBRID OLE DB Data Provider 9.0.0 and all of its components?" IDYES +2
  Abort
FunctionEnd


;--------------------------------

; Uninstaller

Section "Uninstall"

  SetOutPath $TEMP

  ;Un-Register provider
  DeleteRegKey HKCR "AppID\{2B22247F-E9F7-47e8-A9B0-79E8039DCFC8}"
  DeleteRegKey HKCR "AppID\CUBRIDProvider.DLL"
  DeleteRegKey HKCR "CLSID\{15A12058-4353-4C9A-8421-23D80F25EE4E}"
  DeleteRegKey HKCR "CLSID\{3165D76D-CB91-482F-9378-00C216FD5F32}"
  DeleteRegKey HKCR "CUBRID.ErrorLookup"
  DeleteRegKey HKCR "CUBRID.ErrorLookup.9.0"
  DeleteRegKey HKCR "CUBRID.Provider"
  DeleteRegKey HKCR "CUBRID.Provider.9.0"

  Delete "$INSTDIR\BSD License.txt"
  Delete "$INSTDIR\Release notes.txt"
  Delete "$INSTDIR\CUBRIDProvider.dll"

  RMDir "$INSTDIR"

  Delete "$SMPROGRAMS\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)\Uninstall.lnk"

  RMDir /r "$SMPROGRAMS\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)"
  RMDir "$SMPROGRAMS\CUBRID"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\CUBRID OLE DB Data Provider 9.0.0 (x86)"
  DeleteRegKey HKLM "SOFTWARE\CUBRID\CUBRID OLE DB Data Provider 9.0.0 (x86)"

SectionEnd
