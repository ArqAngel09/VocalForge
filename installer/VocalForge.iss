#define MyAppName "AMR Vocal Mix"
#define MyAppVersion "2.1.0"
#define MyAppPublisher "Angel Music Records"

[Setup]
AppId={{A1F0C8D0-5D8B-4C20-A3C2-7E0B4E6A1F31}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Common Files\VST3\AMR Vocal Mix
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=AMR-Vocal-Mix-Windows-Installer
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin

[Files]
Source: "..\build\VocalForge_artefacts\Release\VST3\AMR Vocal Mix.vst3\*"; DestDir: "{commoncf}\VST3\AMR Vocal Mix.vst3"; Flags: recursesubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\AMR Vocal Mix.vst3"
