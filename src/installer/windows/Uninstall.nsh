;--------------------------------
;Macros

!define ERROR_ALREADY_EXISTS 0x000000b7
!define ERROR_ACCESS_DENIED 0x5

!macro CheckSingleInstance SINGLE_INSTANCE_ID
  System::Call 'kernel32::CreateMutex(i 0, i 0, t "Global\${SINGLE_INSTANCE_ID}") i .r0 ?e'
  Pop $1 ; the stack contains the result of GetLastError
  ${If} $1 = "${ERROR_ALREADY_EXISTS}"
  ;ERROR_ACCESS_DENIED means the mutex was created by another user and we don't have access to open it, so application is running
  ${OrIf} $1 = "${ERROR_ACCESS_DENIED}"
    ;If the user closes the already running instance, allow to start a new one without closing the MessageBox
    System::Call 'kernel32::CloseHandle(i $0)'
    ;Will display NSIS taskbar button, no way to hide it before GUIInit, $HWNDPARENT is 0
    MessageBox MB_ICONSTOP "The setup of ${PRODUCT_NAME} is already running." /SD IDOK
    ;Will SetErrorLevel 2 - Installation aborted by script
    Quit
  ${EndIf}
!macroend

!macro un.DeleteRetryAbort filename
  StrCpy $0 "${filename}"
  Call un.DeleteRetryAbort
!macroend

;--------------------------------
;Uninstaller attributes

ShowUninstDetails show

;--------------------------------
;Variables

;Installer started uninstaller in semi-silent mode using /SS parameter
Var SemiSilentMode

;Installer started uninstaller using /uninstall parameter
Var RunningFromInstaller

;--------------------------------
;Uninstaller pages

;Ask for confirmation when cancelling the uninstallation
!define MUI_UNABORTWARNING

!define MULTIUSER_INSTALLMODE_CHANGE_MODE_UNFUNCTION un.PageInstallModeChangeMode
!insertmacro MULTIUSER_UNPAGE_INSTALLMODE

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Sections

Section "un.Program Files" SectionUninstallProgram

  ; Try to delete the EXE as the first step - if it's in use, don't remove anything else
  !insertmacro un.DeleteRetryAbort "$INSTDIR\${PROGEXE}"

  ${If} "$StartMenuFolder" != ""
    nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Attempting to remove the start menu folder: SMPROGRAMS = $SMPROGRAMS, StartMenuFolder = $StartMenuFolder"
    RMDir /r "$SMPROGRAMS\$StartMenuFolder"
  ${EndIf}

  nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Attempting to remove the desktop shortcut: DESKTOP = $DESKTOP, PRODUCT_NAME = ${PRODUCT_NAME}"
  !insertmacro un.DeleteRetryAbort "$DESKTOP\${PRODUCT_NAME}.lnk"

  nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Attempting to remove the start menu shortcut: STARTMENU = $STARTMENU, PRODUCT_NAME = ${PRODUCT_NAME}"
  !insertmacro un.DeleteRetryAbort "$STARTMENU\${PRODUCT_NAME}.lnk"

  nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Attempting to remove the quick launch shortcut: QUICKLAUNCH = $QUICKLAUNCH, PRODUCT_NAME = ${PRODUCT_NAME}"
  !insertmacro un.DeleteRetryAbort "$QUICKLAUNCH\${PRODUCT_NAME}.lnk"

  ;Remove registry keys
  DeleteRegKey HKCU "Software\${PRODUCT_NAME}"
SectionEnd

Section "-Uninstall"
  !insertmacro MULTIUSER_RegistryRemoveInstallInfo
  nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Attempting to remove the installation dir with its whole content: INSTDIR = $INSTDIR"
  RMDir /r /REBOOTOK "$INSTDIR"
SectionEnd

;--------------------------------
;Functions

Function un.onInit
  ${GetParameters} $R0

  ${GetOptions} $R0 "/uninstall" $R1
  ${IfNot} ${Errors}
    StrCpy $RunningFromInstaller 1
    nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Running from installer"
  ${Else}
    StrCpy $RunningFromInstaller 0
    nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Not running from installer"
  ${EndIf}

  ${GetOptions} $R0 "/SS" $R1
  ${IfNot} ${Errors}
    StrCpy $SemiSilentMode 1
    nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Running in semi-silent mode"
    ;Auto close (if no errors) if we are called from the installer; if there are errors, will be automatically set to false
    SetAutoClose true
  ${Else}
    StrCpy $SemiSilentMode 0
    nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Not running in semi-silent mode"
  ${EndIf}

  ${IfNot} ${UAC_IsInnerInstance}
  ${AndIf} $RunningFromInstaller$SemiSilentMode == "00"
    nsislog::log "$TEMP\${MUI_PRODUCT}_uninstall.log" "Running inside UAC forked uninstaller instance + not running from installer or in semi-silent mode"
    !insertmacro CheckSingleInstance "${SINGLE_INSTANCE_ID}"
  ${Endif}

  !insertmacro MULTIUSER_UNINIT
FunctionEnd

Function un.onUninstFailed
  ${If} $SemiSilentMode == 0
    MessageBox MB_ICONSTOP "${PRODUCT_NAME} ${VERSION} could not be fully uninstalled.$\r$\nPlease, restart Windows and run the uninstaller again." /SD IDOK
  ${Else}
    MessageBox MB_ICONSTOP "${PRODUCT_NAME} could not be fully installed.$\r$\nPlease, restart Windows and run the setup program again." /SD IDOK
  ${EndIf}
FunctionEnd

Function un.PageInstallModeChangeMode
  !insertmacro MUI_STARTMENU_GETFOLDER "" $StartMenuFolder
FunctionEnd

Function un.DeleteRetryAbort
  ;Unlike the File instruction, Delete doesn't abort (un)installation on error - it just sets the error flag and skips the file as if nothing happened
  try:
    ClearErrors
    Delete "$0"
    ${If} ${Errors}
      MessageBox MB_RETRYCANCEL|MB_ICONSTOP "Error deleting file:$\r$\n$\r$\n$0$\r$\n$\r$\nClick Retry to try again, or$\r$\nCancel to stop the uninstall." /SD IDCANCEL IDRETRY try
      ;When called from section, will SetErrorLevel 2 - Installation aborted by script
      Abort "Error deleting file $0"
    ${EndIf}
FunctionEnd
