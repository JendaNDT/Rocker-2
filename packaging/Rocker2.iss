#ifndef TargetArch
  #error TargetArch must identify the native Windows build
#endif

[Setup]
AppId={{E16900AC-A2E7-458B-BD35-3EFA90DE39FC}
AppName=Rocker 2
AppVersion=1.0.0
AppPublisher=Rocker 2
DefaultDirName={localappdata}\Programs\Rocker 2
DefaultGroupName=Rocker 2
PrivilegesRequired=lowest
DisableProgramGroupPage=yes
DisableWelcomePage=no
UninstallDisplayIcon={app}\Rocker 2.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\installers
OutputBaseFilename=Rocker2-Setup-{#TargetArch}
#if TargetArch == "ARM64"
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
#else
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#endif

[Languages]
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"

[Files]
Source: "..\pack\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\Rocker 2"; Filename: "{app}\Rocker 2.exe"

[Run]
Filename: "{app}\Rocker 2.exe"; Description: "Spustit Rocker 2"; Flags: postinstall nowait skipifsilent
