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
Name: "massive"; Description: "BAKR MASSIVE - egaliseur passif stereo a lampes 4 bandes"; Types: full
Name: "casque"; Description: "BAKR CASQUE PRO - calibration casque DT 880, enceintes et pieces virtuelles, controle du mix"; Types: full
Name: "instinct"; Description: "BAKR INSTINCT - assistant vocal qui ecoute ta voix et regle la chaine"; Types: full
Name: "tak35pro"; Description: "BAKR TAK35 PRO - rend le micro Takstar TAK35 plus professionnel"; Types: full
Name: "calibre"; Description: "BAKR CALIBRE - calibration automatique des enceintes (Yamaha HS8)"; Types: full
Name: "fatbox"; Description: "BAKR FATBOX - compresseur / saturateur 2 canaux (Warmth, Tranny, Buss/Track/Spank)"; Types: full
Name: "nleveler"; Description: "BAKR LEVELER (nouveau look) - compresseur opto a lampe"; Types: full
Name: "nfet"; Description: "BAKR FET (nouveau look) - limiteur FET (4/8/12/20/ALL)"; Types: full
Name: "nchannel"; Description: "BAKR CHANNEL (nouveau look) - tranche de console complete"; Types: full
Name: "ntube"; Description: "BAKR TUBE (nouveau look) - saturation a lampes Mid/Side"; Types: full
Name: "nmaster"; Description: "BAKR MASTER (nouveau look) - chaine analogique pour le Master"; Types: full
Name: "amberopto";Description: "BAKR AMBER OPTO - compresseur opto a lampes"; Types: full
Name: "karat";    Description: "BAKR KARAT STRIP - tranche console (pre + EQ + comp)"; Types: full
Name: "varimu";   Description: "BAKR VARIMU ST - compresseur stereo variable-mu"; Types: full
Name: "bus4";     Description: "BAKR BUS4 - compresseur de bus"; Types: full
Name: "linepre";  Description: "BAKR LINE PRE - preampli classe A + EQ"; Types: full
Name: "bluecomp"; Description: "BAKR BLUE COMP - compresseur VCA"; Types: full
Name: "voxchain";Description: "BAKR VOXCHAIN - chaine vocale complete (5 modules)"; Types: full
Name: "inflate"; Description: "BAKR INFLATE - volume et densite sans compression"; Types: full
Name: "tubecomp";Description: "BAKR TUBECOMP - compresseur a lampes"; Types: full
Name: "b81";     Description: "BAKR 81 - preampli + EQ console classe A"; Types: full
Name: "ha81a";   Description: "HA-81A EQ - Coulibaly Edition"; Types: full
Name: "voxmidi"; Description: "BAKR VOXMIDI - voix vers MIDI"; Types: full

[Files]
Source: "..\pkg_suite\VST3\BAKR MASSIVE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR MASSIVE.vst3"; Components: massive; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR CASQUE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR CASQUE.vst3"; Components: casque; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR INSTINCT.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR INSTINCT.vst3"; Components: instinct; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR TAK35 PRO.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR TAK35 PRO.vst3"; Components: tak35pro; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR CALIBRE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR CALIBRE.vst3"; Components: calibre; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR FATBOX.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR FATBOX.vst3"; Components: fatbox; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR LEVELER.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR\BAKR LEVELER.vst3"; Components: nleveler; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR FET.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR\BAKR FET.vst3"; Components: nfet; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR CHANNEL.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR\BAKR CHANNEL.vst3"; Components: nchannel; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR TUBE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR\BAKR TUBE.vst3"; Components: ntube; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR MASTER.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR\BAKR MASTER.vst3"; Components: nmaster; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR AMBER OPTO.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR AMBER OPTO.vst3"; Components: amberopto; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR KARAT STRIP.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR KARAT STRIP.vst3"; Components: karat; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR VARIMU ST.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR VARIMU ST.vst3"; Components: varimu; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR BUS4.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR BUS4.vst3"; Components: bus4; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR LINE PRE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR LINE PRE.vst3"; Components: linepre; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR BLUE COMP.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR BLUE COMP.vst3"; Components: bluecomp; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR VOXCHAIN.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR VOXCHAIN.vst3"; Components: voxchain; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR INFLATE.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR INFLATE.vst3"; Components: inflate; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR TUBECOMP.vst3\*";DestDir: "{commoncf64}\VST3\BAKR TUBECOMP.vst3";Components: tubecomp;Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR 81.vst3\*";      DestDir: "{commoncf64}\VST3\BAKR 81.vst3";      Components: b81;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\HA-81A EQ - Coulibaly Edition.vst3\*"; DestDir: "{commoncf64}\VST3\HA-81A EQ - Coulibaly Edition.vst3"; Components: ha81a; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\VST3\BAKR VOXMIDI.vst3\*"; DestDir: "{commoncf64}\VST3\BAKR VOXMIDI.vst3"; Components: voxmidi; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\pkg_suite\Notices\*"; DestDir: "{app}\Notices"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR\BAKR LEVELER.vst3"; Components: nleveler
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR\BAKR FET.vst3"; Components: nfet
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR\BAKR CHANNEL.vst3"; Components: nchannel
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR\BAKR TUBE.vst3"; Components: ntube
Type: filesandordirs; Name: "{commoncf64}\VST3\BAKR\BAKR MASTER.vst3"; Components: nmaster

[Icons]
Name: "{group}\Notices des plugins BAKR"; Filename: "{app}\Notices"
Name: "{group}\Desinstaller BAKR Suite"; Filename: "{uninstallexe}"

[Messages]
fr.FinishedLabel=Les plugins BAKR sont installes dans C:\Program Files\Common Files\VST3.%n%nDans FL Studio : Options > Manage plugins > Find installed plugins.
