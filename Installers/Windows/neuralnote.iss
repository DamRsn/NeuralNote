#ifndef ReleaseDir
#define ReleaseDir "cmake-build-release-visual-studio/NeuralNote_artefacts/Release"
#endif

[Setup]
AppName=NeuralNote
AppVersion=1.0.0
OutputBaseFilename=NeuralNoteInstaller
DefaultDirName={pf}\NeuralNote
DefaultGroupName=NeuralNote
InfoBeforeFile=..\readme.txt
LicenseFile=..\license.txt
AppPublisher=Dr. Audio
AppPublisherURL=https://github.com/DamRsn/NeuralNote
AppSupportURL=https://github.com/DamRsn/NeuralNote
AppUpdatesURL=https://github.com/DamRsn/NeuralNote
AlwaysShowComponentsList=yes
Compression=lzma
SolidCompression=yes
Uninstallable=no
DisableDirPage=yes
AppCopyright=Copyright (c) 2024 Damien Ronssin

[Types]
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "mainapp"; Description: "NeuralNote Standalone"; Types: custom; Flags: disablenouninstallwarning
Name: "plugin"; Description: "NeuralNote VST3"; Types: custom; Flags: disablenouninstallwarning

[Files]
Source: "..\..\{#ReleaseDir}\Standalone\NeuralNote.exe"; DestDir: "{pf}\NeuralNote"; Components:mainapp; Flags: ignoreversion recursesubdirs;
Source: "..\..\{#ReleaseDir}\VST3\NeuralNote.vst3\*"; DestDir: "C:\Program Files\Common Files\VST3\NeuralNote.vst3"; Components:plugin; Flags: ignoreversion recursesubdirs;
