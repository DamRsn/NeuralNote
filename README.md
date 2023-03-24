# NeuralNote <img style="float: right;" src="NeuralNote/Assets/logo.png" width="100" />

NeuralNote is the audio plugin that brings **state-of-the-art Audio to MIDI conversion** into
your favorite Digital Audio Workstation.

- Works with any tonal instrument (voice included)
- Supports polyphonic transcription
- Supports pitch bends
- Lightweight and very fast transcription
- Can scale and time quantize transcribed MIDI directly in the plugin

NeuralNote comes as a simple AudioFX plugin (VST3/AU/Standalone app) to be applied on the track to transcribe.

The workflow is very simple:

- Gather some audio
    - Click record. Works when recording for real or when playing the track in a DAW.
    - Or drop an audio file on the plugin. (.wav, .aiff and .flac supported)
- The midi transcription instantly appears in the piano roll section. Play with the different settings to adjust it.
- Export the MIDI transcription with a simple drag and drop from the plugin to a MIDI track.

NeuralNote uses internally the model from Spotify's [basic-pitch](https://github.com/spotify/basic-pitch). See
their [blogpost](https://engineering.atspotify.com/2022/06/meet-basic-pitch/)
and [paper](https://arxiv.org/abs/2203.09893) for more information.

In NeuralNote, basic-pitch is run
using [RTNeural](https://github.com/jatinchowdhury18/RTNeural) for the CNN part
and [ONNXRuntime](https://github.com/microsoft/onnxruntime) for the feature part (Constant-Q transform calculation +
Harmonic Stacking).
As part of this project, [we contributed to RTNeural](https://github.com/jatinchowdhury18/RTNeural/pull/89) to add 2D
convolution support.

## Install and use the plugin

To simply install and start to use the plugin right away, download the latest release for your platform! (Windows and
Mac (Universal) supported)

Currently, only the .vst3, .component (Audio Unit), .app and .exe files are provided. Installers will be created soon.
Also, the code is not yet signed (will be soon), so you might have to authorize the plugin in your security settings, as
it currently comes from an unidentified developer.

## Build from source

Use this when cloning:

```
git clone --recurse-submodules --shallow-submodules https://github.com/DamRsn/NeuralNote
 ```

#### Mac

```
$ ./build.sh
```

#### Windows

```
> build.bat
```

The build script has to be executed at least once before being able to use the project as a normal CMake project.
The script downloads onnxruntime static library (that we created
with [ort-builder](https://github.com/olilarkin/ort-builder)) before calling CMake.

#### IDEs

Once the build script corresponding as been executed at least once, you can load this project in your favorite IDE
(CLion/Visual Studio/VSCode/etc) and click 'build' for one of the targets.

## Roadmap

- Improve stability.
- Save plugin internal state properly, so it can be loaded back when reentering a session.
- Add tooltips
- Build a simple synth in the plugin so that one can listen to the transcription while playing with the settings, before
  export.
- Allow pitch bends on non-overlapping parts of overlapping notes.
- Support transcription of mp3 files

## Bug reports and feature requests

If you have any request/suggestion concerning the plugin or encounter a bug, please fill a Github issue, we'll
do our best to address it.

## Contributing

Contributions are most welcome! If you want to add some features to the plugin or simply improve the documentation,
please open a PR!

## License

NeuralNote software and code is published under the Apache-2.0 license. See the [license file](LICENSE).

#### Third Party libraries used and license

Here's a list of all the third party libraries used in NeuralNote and the license under which they are used.

- [JUCE](https://juce.com/) (JUCE Personal)
- [RTNeural](https://github.com/jatinchowdhury18/RTNeural) (BSD-3-Clause license)
- [ONNXRuntime](https://github.com/microsoft/onnxruntime) (MIT License)
- [ort-builder](https://github.com/olilarkin/ort-builder) (MIT License)
- [basic-pitch](https://github.com/spotify/basic-pitch) (Apache-2.0 license)
- [basic-pitch-ts](https://github.com/spotify/basic-pitch-ts) (Apache-2.0 license)

## Credits

NeuralNote was developed by [Damien Ronssin](https://github.com/DamRsn) and [Tibor Vass](https://github.com/tiborvass).
The plugin user interface was designed by Perrine Morel.