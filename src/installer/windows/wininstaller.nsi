;--------------------------------
;Include Modern UI
!include "MUI2.nsh"

;--------------------------------
;General
  ;Name and file
  Name "Quentier"
  OutFile "Install.exe"

  ;Default installation folder
  InstallDir "$LOCALAPPDATA\Quentier"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Quentier" ""

  ;Request application privileges for Windows >= Vista
  RequestExecutionLevel user

;--------------------------------
;Variables

  Var StartMenuFolder

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE ..\..\..\COPYING
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Quentier"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Quentier"
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections
 
Section "Default Section" SecDefault

  SetOutPath "$INSTDIR"
  
  ;ADD YOUR OWN FILES HERE...
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Quentier" "" $INSTDIR
                   
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
      
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortcut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecDefault ${LANG_ENGLISH} "Installing..."
  
  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDefault} $(DESC_SecDefault)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
  
;--------------------------------
;Uninstaller Section

Section "uninstall"

  ; TODO: add custom uninstallations steps here 
  Delete "$INSTDIR\Uninstall.exe"
  RmDir "$INSTDIR"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
    
  DeleteRegKey /ifempty HKCU "Software\Quentier"

SectionEnd
