# NeuralNote UI parameters:

### Audio management

- Gain (to boost or lower volume of audio sample)
    - Rotary Knob
    - Range: -20 dB to 12 dB
- Mute (plugin stops passing audio through)
    - On/Off button or checkbox
- Record
    - On/off button
    - Start recording for further transcription to midi
    - Could use a simple rec logo that changes color when
- Clear
    - One shot click button
    - Delete recorded audio and generated midi
    - Could be like a bin icon or something like that

### Transcription settings:

This section is to adjust the settings of the model that generates the midi from the audio.

- Sensibility
    - if high, model will produce more notes, if low less notes
    - Rotary knob
    - (1 - Model_Confidence_Threshold)
    - Range: 0.05 - 0.95
- Note Split (Note Segmentation ... )
    - How easily a note should be split into two.
    - Range: 0.05 - 0.95
    - Rotary knob
- Minimum note duration
    - All note shorter than this will be filtered out
    - in ms
    - From 30 to 550
- Pitch bend
    - Dropdown with these choices: (NoPitchBend, SinglePitchBend, MultiPitchBend)

### Note options

This section is to filter generated notes

- Checkbox/small button to enable/disable this section (Note sure if needed)
- Min and max note (A1 to C5 for example) (removes notes below min and above max)
    - Could be two horizontal sliders or two dropdowns
    - Might be possible to do a range slider: one slider with two positions for min and max (don't know how to do this
      in code)
- Key
    - 2 dropdowns:
        - Root note (C, C#, D, D# ...)
        - (Chromatic, Major, Minor)
        - Chromatic choice makes Root note dropdown disabled
- Snap mode:
    - Dropdown
        - Filter: Remove notes out of the selected key
        - Adjust: Change note to the closest note in key

### Rhythm settings (if we have time to implement it)

This section is for rhythmic quantization: to snap generated notes' start time on the time grid according to tempo.

- Checkbox/small button to enable/disable this section (Note sure if needed)
- Time division
    - Dropdown
        - [1, 1/2, 1/3, 1/4, 1/6 ,1/8, 1/12, 1/16, 1/24, 1/32]
- Quantization force
    - Range: 0 to 100
        - 0: don't move any note at all
        - 100 align completely all the notes on the grid
    - Horizontal slider or rotary slider
- Whole section disabled if no tempo information available (can happen)

# UX

Description général de l'UX

- Ouverture plugin:
    - Aucun audio n'est chargé. Les regions waveform et piano roll sont vides.
        - Afficher "Click record or drop an audio file here." là ou la waveform est affichée quand un truc est chargé
        - "Waiting for audio to transcribe" dans la région piano roll
    - Les sections Transcription, Notes et Rhythm sont désactivées (mais
      visible)
- Obtenir de l'audio à transcrire
    - Possible de Drag and Drop un fichier audio (.mp3, .wav, .aiff, .flac ...) sur la waveform et le fichier se charge,
      la waveform se déssine
        - Durée max: 3 minutes
    - Record: l'audio qui vient du plugin s'enregistre au fur et à mesure (et se dessine au fur et à mesure si
      possible).
        - Ca s'arrète quand record est re-click. Record button change de couleur / forme quand ça enregistre.
        - Durée max: 3 minutes
- Une fois que l'audio est chargé, le tout est transcrit automatiquemnt et les notes se déssinent dans la section piano
  roll.
- Dès qu'un parametre de Transcription, Notes ou Rhythm change, les notes sont update automatiquement dans le piano
  roll.
- On peut drag le fichier MIDI crée depuis la region piano roll et le drop dans le DAW ou ailleurs à n'importe quel
  moment.
- Dès que le bouton Clear est clické, la waveform et le piano roll sont effacés et on repart au début