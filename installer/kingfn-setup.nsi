; ============================================================
;  KINGFN Browser Installer — NSIS Modern UI 2
;
;  Local build:
;    makensis installer\kingfn-setup.nsi
;
;  CI / custom build dir:
;    makensis /DBUILD_OUT_DIR="C:\abs\path\Release" \
;             /DREPO_ROOT="C:\abs\repo" \
;             /DPRODUCT_VERSION="0.3.1" \
;             installer\kingfn-setup.nsi
; ============================================================

; ── Configurable paths (override with /D on the command line) ─────────────────
!ifndef BUILD_OUT_DIR
  !define BUILD_OUT_DIR "..\out\cef\Release"
!endif
!ifndef REPO_ROOT
  !define REPO_ROOT ".."
!endif
!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "0.3.0"
!endif

!define PRODUCT_NAME      "KINGFN Browser"
!define PRODUCT_PUBLISHER "Farhan0629"
!define PRODUCT_URL       "https://github.com/Farhan0629/Bravelike"
!define PRODUCT_EXE       "kingfn_browser.exe"
!define REGKEY_UNINSTALL  "Software\Microsoft\Windows\CurrentVersion\Uninstall\KINGFN"
!define REGKEY_APP        "Software\KINGFN"

; NSIS MUI2
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"

; ── Installer metadata ────────────────────────────────────────────────────────
Name              "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile           "kingfn-browser-setup-${PRODUCT_VERSION}.exe"
InstallDir        "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey  HKLM "${REGKEY_APP}" "InstallDir"
RequestExecutionLevel admin
SetCompressor     /SOLID lzma
SetCompressorDictSize 32

; ── MUI2 settings ─────────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN          "$INSTDIR\${PRODUCT_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT     "Launch KINGFN Browser"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE      "${REPO_ROOT}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Embedded version info ─────────────────────────────────────────────────────
VIProductVersion  "${PRODUCT_VERSION}.0"
VIAddVersionKey   "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey   "ProductVersion"  "${PRODUCT_VERSION}"
VIAddVersionKey   "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey   "FileVersion"     "${PRODUCT_VERSION}"
VIAddVersionKey   "FileDescription" "KINGFN Browser Installer"
VIAddVersionKey   "LegalCopyright"  "MIT License"

; ── Install section ───────────────────────────────────────────────────────────
Section "KINGFN Browser" SecMain
  SectionIn RO

  SetOutPath "$INSTDIR"

  ; Main executable
  File "${BUILD_OUT_DIR}\${PRODUCT_EXE}"

  ; CEF runtime DLLs
  File /nonfatal "${BUILD_OUT_DIR}\libcef.dll"
  File /nonfatal "${BUILD_OUT_DIR}\chrome_elf.dll"
  File /nonfatal "${BUILD_OUT_DIR}\d3dcompiler_47.dll"
  File /nonfatal "${BUILD_OUT_DIR}\libEGL.dll"
  File /nonfatal "${BUILD_OUT_DIR}\libGLESv2.dll"
  File /nonfatal "${BUILD_OUT_DIR}\vk_swiftshader.dll"
  File /nonfatal "${BUILD_OUT_DIR}\vulkan-1.dll"
  File /nonfatal "${BUILD_OUT_DIR}\vk_swiftshader_icd.json"
  File /nonfatal "${BUILD_OUT_DIR}\chrome_crashpad_handler.exe"

  ; CEF data paks
  File /nonfatal "${BUILD_OUT_DIR}\cef.pak"
  File /nonfatal "${BUILD_OUT_DIR}\cef_100_percent.pak"
  File /nonfatal "${BUILD_OUT_DIR}\cef_200_percent.pak"
  File /nonfatal "${BUILD_OUT_DIR}\cef_extensions.pak"
  File /nonfatal "${BUILD_OUT_DIR}\devtools_resources.pak"
  File /nonfatal "${BUILD_OUT_DIR}\snapshot_blob.bin"
  File /nonfatal "${BUILD_OUT_DIR}\v8_context_snapshot.bin"
  File /nonfatal "${BUILD_OUT_DIR}\icudtl.dat"

  ; Locales
  SetOutPath "$INSTDIR\locales"
  File /nonfatal "${BUILD_OUT_DIR}\locales\en-US.pak"
  File /nonfatal "${BUILD_OUT_DIR}\locales\*.pak"

  ; KINGFN resources
  SetOutPath "$INSTDIR\resources"
  File "${REPO_ROOT}\resources\home.html"
  File "${REPO_ROOT}\resources\shields.js"
  File "${REPO_ROOT}\resources\history.html"
  File "${REPO_ROOT}\resources\downloads.html"

  SetOutPath "$INSTDIR\resources\icons"
  File /nonfatal "${REPO_ROOT}\resources\icons\*.*"

  ; Blocklist config
  SetOutPath "$INSTDIR\config"
  File "${REPO_ROOT}\config\kingfn-blocklist.txt"
  IfFileExists "$INSTDIR\config\custom-blocklist.txt" skip_sample
    File /oname=custom-blocklist.txt "${REPO_ROOT}\config\sample-blocklist.txt"
  skip_sample:

  ; ── Registry ─────────────────────────────────────────────────────────────
  SetOutPath "$INSTDIR"
  WriteRegStr HKLM "${REGKEY_APP}" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "${REGKEY_APP}" "Version"    "${PRODUCT_VERSION}"

  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "DisplayName"          "${PRODUCT_NAME}"
  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "UninstallString"      '"$INSTDIR\uninstall.exe"'
  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "InstallLocation"      "$INSTDIR"
  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "Publisher"            "${PRODUCT_PUBLISHER}"
  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "URLInfoAbout"         "${PRODUCT_URL}"
  WriteRegStr   HKLM "${REGKEY_UNINSTALL}" "DisplayVersion"       "${PRODUCT_VERSION}"
  WriteRegDWORD HKLM "${REGKEY_UNINSTALL}" "NoModify"             1
  WriteRegDWORD HKLM "${REGKEY_UNINSTALL}" "NoRepair"             1

  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${REGKEY_UNINSTALL}" "EstimatedSize" "$0"

  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Default browser registration
  WriteRegStr HKLM "Software\Clients\StartMenuInternet\KINGFN" "" "${PRODUCT_NAME}"
  WriteRegStr HKLM "Software\Clients\StartMenuInternet\KINGFN\Capabilities" \
              "ApplicationName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "Software\Clients\StartMenuInternet\KINGFN\Capabilities" \
              "ApplicationDescription" "KINGFN — Private & Fast Windows Browser"
  WriteRegStr HKLM "Software\Clients\StartMenuInternet\KINGFN\shell\open\command" \
              "" '"$INSTDIR\${PRODUCT_EXE}" "%1"'

  ; ── Shortcuts ─────────────────────────────────────────────────────────────
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\KINGFN Browser.lnk" \
                 "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\${PRODUCT_EXE}" 0
  CreateShortcut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall KINGFN.lnk" \
                 "$INSTDIR\uninstall.exe"
  CreateShortcut "$DESKTOP\KINGFN Browser.lnk" \
                 "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\${PRODUCT_EXE}" 0
SectionEnd

; ── Uninstall section ─────────────────────────────────────────────────────────
Section "Uninstall"
  ExecWait 'taskkill /IM "${PRODUCT_EXE}" /F' $0

  RMDir /r "$INSTDIR\locales"
  RMDir /r "$INSTDIR\resources"
  RMDir /r "$INSTDIR\config"
  ; Keep KINGFNProfile — preserve user data (history, bookmarks, shields prefs)

  Delete "$INSTDIR\${PRODUCT_EXE}"
  Delete "$INSTDIR\libcef.dll"
  Delete "$INSTDIR\chrome_elf.dll"
  Delete "$INSTDIR\d3dcompiler_47.dll"
  Delete "$INSTDIR\libEGL.dll"
  Delete "$INSTDIR\libGLESv2.dll"
  Delete "$INSTDIR\vk_swiftshader.dll"
  Delete "$INSTDIR\vulkan-1.dll"
  Delete "$INSTDIR\vk_swiftshader_icd.json"
  Delete "$INSTDIR\chrome_crashpad_handler.exe"
  Delete "$INSTDIR\cef.pak"
  Delete "$INSTDIR\cef_100_percent.pak"
  Delete "$INSTDIR\cef_200_percent.pak"
  Delete "$INSTDIR\cef_extensions.pak"
  Delete "$INSTDIR\devtools_resources.pak"
  Delete "$INSTDIR\snapshot_blob.bin"
  Delete "$INSTDIR\v8_context_snapshot.bin"
  Delete "$INSTDIR\icudtl.dat"
  Delete "$INSTDIR\uninstall.exe"
  RMDir  "$INSTDIR"

  Delete "$SMPROGRAMS\${PRODUCT_NAME}\KINGFN Browser.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall KINGFN.lnk"
  RMDir  "$SMPROGRAMS\${PRODUCT_NAME}"
  Delete "$DESKTOP\KINGFN Browser.lnk"

  DeleteRegKey HKLM "${REGKEY_UNINSTALL}"
  DeleteRegKey HKLM "${REGKEY_APP}"
  DeleteRegKey HKLM "Software\Clients\StartMenuInternet\KINGFN"
SectionEnd
