#define MyAppName "VocalForge"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "VocalForge"

[Setup]
AppId={{A1F0C8D0-5D8B-4C20-A3C2-7E0B4E6A1F31}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Common Files\VST3\VocalForge
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=VocalForge-Windows-Installer
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin

[Files]
Source: "..\build\VocalForge_artefacts\Release\VST3\VocalForge.vst3\*"; DestDir: "{commoncf}\VST3\VocalForge.vst3"; Flags: recursesubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf}\VST3\VocalForge.vst3"