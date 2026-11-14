# Prophorcel (VST3)

Synthétiseur vectoriel inspiré du Prophet VS, en plugin VST3 (JUCE 8).

- 4 oscillateurs à tables d'ondes (13 formes), mélangés par un joystick vectoriel
- Enveloppe vectorielle à 4 points, avec boucle
- Filtre passe-bas 24 dB, deux enveloppes, LFO, écho
- Chorus / ensemble stéréo
- Modes polyphonique (8 voix), mono legato et unisson, portamento
- Vélocité sur le volume et sur le filtre, molette de pitch, molette 1 sur l'axe vertical du joystick
- 9 sons de départ (menu en haut à droite)

## Compiler avec GitHub (le plus simple)

1. Crée un dépôt GitHub et envoie-y le contenu de ce dossier (le dossier `.github` compris).
2. Onglet **Actions** : le workflow « Build Prophorcel » démarre tout seul. Tu peux aussi le lancer à la main avec « Run workflow ».
3. Quand il est vert, ouvre l'exécution et télécharge l'artefact de ta plateforme (en bas de la page) : `Prophorcel-VST3-Windows`, `-macOS` ou `-Linux`.
4. Copie `Prophorcel.vst3` dans le dossier VST3 de ton système :
   - Windows : `C:\Program Files\Common Files\VST3`
   - macOS : `~/Library/Audio/Plug-Ins/VST3`
   - Linux : `~/.vst3`
5. Relance l'analyse des plugins dans ta DAW.

Sur macOS, le plugin n'est pas signé : si le système le bloque, retire la quarantaine avec
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Prophorcel.vst3`.

## Compiler chez toi

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

JUCE est téléchargé automatiquement. Le VST3 se trouve ensuite dans `build/Prophorcel_artefacts/`.
Une version Standalone (application autonome) est compilée en même temps, pratique pour tester sans DAW.

## Test du moteur

    cmake -B build-test -DPROPH_BUILD_TEST=ON
    cmake --build build-test --target EngineTest
    ./build-test/EngineTest sortie.wav

Il rend plusieurs scénarios (poly, mono, unisson, rafale de notes, toutes les formes d'onde), vérifie qu'il n'y a ni NaN ni débordement, et écrit un WAV pour l'écoute.

## Structure

- `Source/Engine.h` : tout le son (sans dépendance à l'interface)
- `Source/PluginProcessor.*` : paramètres, MIDI, sons de départ
- `Source/PluginEditor.*` : interface
