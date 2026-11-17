; Installateur Windows 10/11 (64 bits) pour BAKR TUBECOMP
#define AppName "BAKR TUBECOMP"
#define AppVersion "1.0"

[Setup]
AppId={{6C1E2B7A-4D9F-4E8B-9A31-0B4E00710E01}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=BAKR Audio Plugins
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName} (VST3)
OutputDir=..\..\
OutputBaseFilename=BAKR_TUBECOMP_Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Files]
Source: "..\..\build_tc\BAKR_TUBECOMP_artefacts\Release\VST3\BAKR TUBECOMP.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR TUBECOMP.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR TUBECOMP.vst3"

[Messages]
fr.FinishedLabel=BAKR TUBECOMP est installé.%n%nDans FL Studio : Options > Manage plugins > Find installed plugins.
