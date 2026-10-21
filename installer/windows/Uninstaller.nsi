; Build-time writer emits the native uninstaller into the release payload.
; The uninstaller runs its inventory backend from a private temporary runtime.
Unicode true
!include "LogicLib.nsh"
Name "Uninstall Terlox"
OutFile "${WRITER_FILE}"
Icon "${PROJECT_ROOT}\branding\icons\terlox.ico"
UninstallIcon "${PROJECT_ROOT}\branding\icons\terlox.ico"
RequestExecutionLevel user
SetCompressor /SOLID zlib
VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName" "Terlox"
VIAddVersionKey /LANG=1033 "CompanyName" "R3F Studio"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "FileDescription" "Terlox Uninstaller"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright 2026 R3F Studio"
BrandingText " "
ShowUninstDetails show
UninstallText "Remove Terlox from this account? Personal settings, shell history and files you added to the installation folder are kept. Close Terlox and its terminal sessions first."
UninstPage uninstConfirm
UninstPage instfiles
Function .onInit
  SetSilent silent
FunctionEnd
Section
  WriteUninstaller "${PAYLOAD_DIR}\Uninstall.exe"
  IfErrors 0 +2
    SetErrorLevel 2
SectionEnd
Section "Uninstall"
  InitPluginsDir
  SetOutPath "$PLUGINSDIR\runtime"
  File "${PAYLOAD_DIR}\terlox-maintenance.exe"
  File "${PAYLOAD_DIR}\Qt6Core.dll"
  File "${PAYLOAD_DIR}\*140*.dll"
  DetailPrint "Removing Terlox from $INSTDIR"
  nsExec::ExecToStack '"$PLUGINSDIR\runtime\terlox-maintenance.exe" --uninstall --directory "$INSTDIR"'
  Pop $0
  Pop $1
  DetailPrint "$1"
  ${If} $0 != 0
    IfSilent failed
      MessageBox MB_OK|MB_ICONSTOP "$1$\r$\n$\r$\nTerlox could not be completely removed. Resolve the problem and run Uninstall again."
    failed:
    SetErrorLevel 2
    Abort
  ${EndIf}
  SetErrorLevel 0
SectionEnd
