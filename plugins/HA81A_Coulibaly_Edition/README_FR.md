# HA-81A EQ – Coulibaly Edition (projet VST3)

Ce dossier contient le **code source** d'un égaliseur inspiré de l'appareil montré en référence. Il ne contient pas encore de fichier `.vst3` compilé.

## Fonctions incluses
- VST3 64 bits et application Standalone via JUCE
- 4 bandes : LF shelf, LMF bell, HMF bell, HF shelf
- Réglages de fréquence et de gain pour chaque bande
- Input / Output, Bypass, inversion de phase et Power
- Sauvegarde/rappel des paramètres par l'hôte
- Interface graphique personnalisée sombre avec commandes rotatives

## Compilation sous Windows 10

### Logiciels nécessaires
1. **Visual Studio 2022 Community** avec la charge de travail « Développement Desktop en C++ ».
2. **CMake** 3.22 ou plus récent.
3. **Git for Windows**.
4. Une connexion Internet lors de la première configuration : CMake télécharge JUCE 8.0.6.

### Construire le VST3
Ouvrir « x64 Native Tools Command Prompt for VS 2022 », aller dans ce dossier puis exécuter :

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
```

Le plugin compilé sera normalement dans :

```text
build\HA81A_Coulibaly_Edition_artefacts\Release\VST3\HA-81A EQ - Coulibaly Edition.vst3
```

Copier le dossier `.vst3` dans :

```text
C:\Program Files\Common Files\VST3\
```

Puis redémarrer Cubase et lancer un scan des plugins si nécessaire.

## Important
- Il s'agit d'une émulation logicielle originale inspirée de l'apparence et de l'organisation de commandes de la photo, pas d'un produit officiel Heritage Audio.
- Le graphe affiché est actuellement une décoration statique : il ne suit pas encore la courbe réelle de l'égaliseur.
- Le code est une première version de développement. Il doit être compilé et testé sur ton PC avant utilisation en session importante.
- Aucun installateur ni binaire VST3 précompilé n'est fourni dans cette archive.
