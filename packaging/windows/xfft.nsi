# SPDX-License-Identifier: GPL-2.0
# Based on subsurface's version
# To create installer: makensis xfft.nsi

#--------------------------------
# Include Modern UI

    !include "MUI2.nsh"

#--------------------------------
# General

    # Program version
    !define XFFT_VERSION "1.0"
	!define XFFT_VERSION_FULL "1.0.0.0"

    # Installer name and filename
    Name "xfft"
    Caption "xfft ${XFFT_VERSION} Setup"
    OutFile "..\xfft-${XFFT_VERSION}.exe"

    # Icon to use for the installer
    !define MUI_ICON "xfft.ico"

    # Default installation folder
    InstallDir "$PROGRAMFILES64\xfft"

    # Request application privileges
    RequestExecutionLevel admin

#--------------------------------
# Version information

    VIProductVersion "${XFFT_VERSION_FULL}"
    VIAddVersionKey "ProductName" "xfft"
    VIAddVersionKey "FileDescription" "xfft - an interactive program to play with the two-dimensional Fourier transform"
    VIAddVersionKey "FileVersion" "${XFFT_VERSION}"
    VIAddVersionKey "LegalCopyright" "GPL v.2"
    VIAddVersionKey "ProductVersion" "${XFFT_VERSION}"

#--------------------------------
# Settings

    # Show a warn on aborting installation
    !define MUI_ABORTWARNING

    # Defines the target start menu folder
    !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
    !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\xfft"
    !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

#--------------------------------
# Variables

    Var StartMenuFolder

#--------------------------------
# Custom pages

# Maintain some checkboxes in the uninstall dialog
Var UninstallDialog

Function un.UninstallOptions
    nsDialogs::Create 1018
    Pop $UninstallDialog
    ${If} $UninstallDialog == error
        Abort
    ${EndIf}

    nsDialogs::Show
FunctionEnd

#--------------------------------
# Pages

    # Installer pages
    !insertmacro MUI_PAGE_LICENSE "LICENSE"
    !insertmacro MUI_PAGE_DIRECTORY
    !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
    !insertmacro MUI_PAGE_INSTFILES

    # Uninstaller pages
    !insertmacro MUI_UNPAGE_CONFIRM
    UninstPage custom un.UninstallOptions
    !insertmacro MUI_UNPAGE_INSTFILES

#--------------------------------
# Languages

    !insertmacro MUI_LANGUAGE "English"

Section
    SetShellVarContext all

    # Installation path
    SetOutPath "$INSTDIR"

    # Delete any already installed DLLs to avoid buildup of various
    # versions of the same library when upgrading
    Delete "$INSTDIR\*.dll"

    # Files to include in installer
    # now that we install into the staging directory and try to only have
    # the DLLs there that we depend on, this is much easier
    File xfft.exe
    File /r iconengines
    File /r imageformats
    File /r platforms
    File /r styles
    File /r translations
    File *.dll
    File xfft.ico

    # Create shortcuts
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\xfft.lnk" "$INSTDIR\xfft.exe" "" "$INSTDIR\xfft.ico" 0
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall xfft.lnk" "$INSTDIR\Uninstall.exe"
        CreateShortCut "$DESKTOP\xfft.lnk" "$INSTDIR\xfft.exe" "" "$INSTDIR\xfft.ico" 0
    !insertmacro MUI_STARTMENU_WRITE_END

    # Create the uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\xfft" \
        "DisplayName" "xfft"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\xfft" \
        "DisplayIcon" "$INSTDIR\xfft.ico"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\xfft" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\xfft" \
        "DisplayVersion" ${XFFT_VERSION}

SectionEnd

#--------------------------------
# Uninstaller section

Section "Uninstall"
    SetShellVarContext all

    # Delete installed files
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\xfft.exe"
    Delete "$INSTDIR\xfft.ico"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir /r "$INSTDIR\iconengines"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\translations"
    RMDir "$INSTDIR"

    # Remove shortcuts
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\xfft.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall xfft.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"
    Delete "$DESKTOP\xfft.lnk"

    # remove the uninstaller entry
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\xfft"

SectionEnd
