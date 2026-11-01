; Inno Setup script for WADAHEX (Windows, 64-bit)
#define AppName "WADAHEX"
#ifndef AppVersion
#define AppVersion "1.3.1"
#endif

[Setup]
AppId={{6B1E2D4A-91C3-4F7E-A8B2-5D3E1A9C7F21}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=YRAZIEL
AppPublisherURL=https://yraziel.com
DefaultDirName={autopf}\YRAZIEL\WADAHEX
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=WADAHEX_win_setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full compact custom; Flags: fixed
Name: "standalone"; Description: "Standalone app"; Types: full

[Files]
Source: "..\build\HEX1_artefacts\Release\VST3\WADAHEX.vst3\*"; DestDir: "{commoncf64}\VST3\WADAHEX.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
Source: "..\build\HEX1_artefacts\Release\Standalone\WADAHEX.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\docs\WADAHEX_Manual.pdf"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{autoprograms}\WADAHEX"; Filename: "{app}\WADAHEX.exe"; Components: standalone
Name: "{autoprograms}\WADAHEX Manual"; Filename: "{app}\WADAHEX_Manual.pdf"
