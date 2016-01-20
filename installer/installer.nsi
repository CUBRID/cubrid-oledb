;
; CUBRID OLEDB installer
;   Installer design key point
;	1. Install 32bit driver only in 32bit Windows
;	2. Install both 32bit and 64bit driver in 64bit Windows
;	3. Do not ask user to choose 32bit or 64bit. to prevent confusion for beginner user


;--------------------------------
;Include Modern UI

	!include "MUI2.nsh"
	!include "x64.nsh"
	!include "Library.nsh"


;--------------------------------
;General

	;Name and file
	Name "CUBRID OLEDB Driver (32/64bit)"
	OutFile "cubrid-oledb.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\cubrid-oledb"
	
	;Get installation folder from registry if available
	InstallDirRegKey HKLM "SOFTWARE\cubrid-oledb" ""

	;Request application privileges for Windows Vista
	RequestExecutionLevel admin

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_LICENSE "license.txt"
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
	
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES
	
;--------------------------------
;Languages
 
	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "OLEDB Driver" SecOLEDB

	SetOutPath "$INSTDIR"
	
	;Store installation folder
	WriteRegStr HKLM "SOFTWARE\cubrid-oledb" "" $INSTDIR
	
	;Create uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"

	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\cubrid-oledb" "DisplayName" "CUBRID OLEDB Driver 32/64bit"
	WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\cubrid-oledb" "UninstallString" "$INSTDIR\uninstall.exe"

	; add 32bit driver file and register to normal registry
	File "CUBRIDProvider32.dll"
	File "README.txt"
  RegDLL "$INSTDIR\CUBRIDProvider32.dll"
	${If} ${RunningX64}
		; add 64bit driver file and register to normal registry
		File "CUBRIDProvider64.dll"
		ExecWait `regsvr32 /s "$INSTDIR\CUBRIDProvider64.dll"`
  ${EndIf}
SectionEnd

;--------------------------------
;Descriptions

	;Language strings
	LangString DESC_SecOLEDB ${LANG_ENGLISH} "OLEDB Driver files, include 32bit and 64bit driver"

	;Assign language strings to sections
	!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
		!insertmacro MUI_DESCRIPTION_TEXT ${SecOLEDB} $(DESC_SecOLEDB)
	!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	${If} ${RunningX64}
    ExecWait `regsvr32 /s /u "$INSTDIR\CUBRIDProvider64.dll"`
		Delete "$INSTDIR\CUBRIDProvider64.dll"
	${EndIf}
	UnRegDLL "$INSTDIR\CUBRIDProvider32.dll"
	Delete "$INSTDIR\CUBRIDProvider32.dll"
	Delete "$INSTDIR\uninstall.exe"
	Delete "$INSTDIR\README.txt"
	RMDir "$INSTDIR"

	DeleteRegKey /ifempty HKLM "SOFTWARE\cubrid-oledb"
	DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\cubrid-oledb"
	
  SetRegView 32


	${If} ${RunningX64}
		SetRegView 64

		SetRegView 32
	${EndIf}

SectionEnd


