; Installateur unique BAKR Suite (Windows 10 / 11, 64 bits)
#define AppVer GetEnv("SUITE_VERSION")
#if AppVer == ""
  #define AppVer "1.0"
#endif

[Setup]
AppId={{6B1E2C4A-8F3D-4B7A-9C11-BAKR0SUITE01}
AppName=BAKR Suite
AppVersion={#AppVer}
AppPublisher=BAKR - Aboubacar Coulibaly
DefaultDirName={autopf}\BAKR Suite
DefaultGroupName=BAKR Suite
DisableProgramGroupPage=yes
OutputDir=..\
OutputBaseFilename=BAKR_Suite_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=BAKR Suite (plugins VST3)

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Types]
Name: "full"; Description: "Tous les plugins BAKR"
Name: "custom"; Description: "Choisir les plugins"; Flags: iscustom

[Components]
Name: "guitar";  Description: "BAKR GUITAR - guitares acoustiques et electriques (instrument)"; Types: full
Name: "inflate"; Description: "BAKR INFLATE - volume et densite sans compression"; Types: full
Name: "tubecomp";Description: "BAKR TUBECOMP - compresseur a lampes"; Types: full
Name: "b81";     Description: "BAKR 81 - preampli + EQ console classe A"; Types: full
Name: "ha81a";   Description: "HA-81A EQ - Coulibaly Edition"; Types: full
Name: "repair";  Description: "BAKR REPAIR - reparation vocale"; Types: full
Name: "voxmidi"; Description: "BAKR VOXMIDI - voix vers MIDI"; Types: full

[Files]
Source: "..\pkg_suite\VST3\BAKR GUITAR.vst3\*";  DestDir: "{commoncf64}\VST3\BAKR GUITAR.vst3";  Components: guitar;  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR INFLATE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR INFLATE.vst3"; Components: inflate; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR TUBECOMP.vst3\*";DestDir: "{commoncf64}\VST3\BAKR TUBECOMP.vst3";Components: tubecomp;Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR 81.vst3\*";      DestDir: "{commoncf64}\VST3\BAKR 81.vst3";      Components: b81;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\HA-81A EQ - Coulibaly Edition.vst3\*"; DestDir: "{commoncf64}\VST3\HA-81A EQ - Coulibaly Edition.vst3"; Components: ha81a; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR REPAIR.vst3\*";  DestDir: "{commoncf64}\VST3\BAKR REPAIR.vst3";  Components: repair;  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR VOXMIDI.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR VOXMIDI.vst3"; Components: voxmidi; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\Notices\*"; DestDir: "{app}\Notices"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Notices des plugins BAKR"; Filename: "{app}\Notices"
Name: "{group}\Desinstaller BAKR Suite"; Filename: "{uninstallexe}"

[Messages]
fr.FinishedLabel=Les plugins BAKR sont installes dans C:\Program Files\Common Files\VST3.%n%nDans FL Studio : Options > Manage plugins > Find installed plugins.
