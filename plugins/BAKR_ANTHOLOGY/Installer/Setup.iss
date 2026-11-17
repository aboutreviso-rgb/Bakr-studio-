; Installateur Windows 10/11 (64 bits) : BAKR ANTHOLOGY (6 plugins VST3)
#define AppName "BAKR ANTHOLOGY"
#define AppVersion "1.0"
#define Art "..\..\..\build_anth"

[Setup]
AppId={{3F7A1C2E-8B4D-4C6A-9E21-0B4E0A7A0101}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=BAKR Audio Plugins
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName} (6 plugins VST3)
OutputDir=..\..\..\
OutputBaseFilename=BAKR_ANTHOLOGY_Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Types]
Name: "full"; Description: "Les 6 plugins"
Name: "custom"; Description: "Choisir les plugins"; Flags: iscustom

[Components]
Name: "opto";  Description: "BAKR AMBER OPTO  - compresseur opto a lampes";            Types: full custom
Name: "karat"; Description: "BAKR KARAT STRIP - tranche console (pre + EQ + comp)";    Types: full custom
Name: "varimu";Description: "BAKR VARIMU ST   - compresseur stereo variable-mu";       Types: full custom
Name: "bus4";  Description: "BAKR BUS4        - compresseur de bus";                   Types: full custom
Name: "line";  Description: "BAKR LINE PRE    - preampli classe A + EQ";               Types: full custom
Name: "blue";  Description: "BAKR BLUE COMP   - compresseur VCA";                      Types: full custom

[Files]
Source: "{#Art}\BAKR_AMBER_OPTO_artefacts\Release\VST3\BAKR AMBER OPTO.vst3\*";   DestDir: "{commoncf64}\VST3\BAKR AMBER OPTO.vst3";  Components: opto;  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Art}\BAKR_KARAT_STRIP_artefacts\Release\VST3\BAKR KARAT STRIP.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR KARAT STRIP.vst3"; Components: karat; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Art}\BAKR_VARIMU_ST_artefacts\Release\VST3\BAKR VARIMU ST.vst3\*";     DestDir: "{commoncf64}\VST3\BAKR VARIMU ST.vst3";   Components: varimu; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Art}\BAKR_BUS4_artefacts\Release\VST3\BAKR BUS4.vst3\*";               DestDir: "{commoncf64}\VST3\BAKR BUS4.vst3";        Components: bus4;  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Art}\BAKR_LINE_PRE_artefacts\Release\VST3\BAKR LINE PRE.vst3\*";       DestDir: "{commoncf64}\VST3\BAKR LINE PRE.vst3";    Components: line;  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Art}\BAKR_BLUE_COMP_artefacts\Release\VST3\BAKR BLUE COMP.vst3\*";     DestDir: "{commoncf64}\VST3\BAKR BLUE COMP.vst3";   Components: blue;  Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR AMBER OPTO.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR KARAT STRIP.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR VARIMU ST.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR BUS4.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR LINE PRE.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR BLUE COMP.vst3"

[Messages]
fr.FinishedLabel=Les plugins BAKR sont installes.%n%nDans FL Studio : Options > Manage plugins > Find installed plugins.
