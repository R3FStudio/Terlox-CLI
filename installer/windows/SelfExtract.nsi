; The released EXE contains the complete Qt/runtime payload. Nothing beside it
; is required. NSIS owns the temporary directory and removes it after the wizard.
Unicode true
!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinMessages.nsh"
Name "Terlox Setup"
OutFile "${OUTPUT_FILE}"
Icon "${PROJECT_ROOT}\branding\icons\terlox.ico"
RequestExecutionLevel user
ManifestDPIAware true
SetCompressor /SOLID zlib
VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "Terlox"
VIAddVersionKey /LANG=1033 "CompanyName" "R3F Studio"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "FileDescription" "Terlox Installer"
VIAddVersionKey /LANG=1033 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright 2026 R3F Studio"
BrandingText " "
AutoCloseWindow true
ShowInstDetails nevershow
Page instfiles
Var VerifyOnly

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "This Terlox release requires 64-bit Windows."
    Abort
  ${EndIf}
  StrCpy $VerifyOnly "0"
  ClearErrors
  ${GetOptions} "$CMDLINE" "/VERIFY" $0
  IfErrors normal_start
    StrCpy $VerifyOnly "1"
    SetSilent silent
  normal_start:
FunctionEnd

Section
  InitPluginsDir
  SetOutPath "$PLUGINSDIR\payload"
  SetDetailsPrint none
  ClearErrors
  File /r "${PAYLOAD_DIR}\*"
  IfErrors extraction_failed
  StrCmp $VerifyOnly "1" verify
    ShowWindow $HWNDPARENT ${SW_HIDE}
    ClearErrors
    ExecWait '"$PLUGINSDIR\payload\terlox-setup-wizard.exe" --package "$PLUGINSDIR\payload"' $0
    Goto finished
  verify:
    ClearErrors
    ExecWait '"$PLUGINSDIR\payload\terlox-setup-wizard.exe" --package "$PLUGINSDIR\payload" --check-package --destination "$PLUGINSDIR\verification destination"' $0
  finished:
    IfErrors launch_failed
    SetErrorLevel $0
    Goto done
  extraction_failed:
    StrCmp $VerifyOnly "1" extraction_exit
    MessageBox MB_OK|MB_ICONSTOP "Setup could not unpack its files. Check free space in your temporary folder and try again."
  extraction_exit:
    SetErrorLevel 2
    Goto done
  launch_failed:
    StrCmp $VerifyOnly "1" launch_exit
    MessageBox MB_OK|MB_ICONSTOP "Terlox Setup could not start. Try downloading the installer again."
  launch_exit:
    SetErrorLevel 3
  done:
SectionEnd
