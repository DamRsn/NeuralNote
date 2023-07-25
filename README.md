# NeuralNote <img style="float: right;" src="NeuralNote/Assets/logo.png" width="100" />

NeuralNote is the audio plugin that brings **state-of-the-art Audio to MIDI conversion** into
your favorite Digital Audio Workstation.

- Works with any tonal instrument (voice included)
- Supports polyphonic transcription
- Supports pitch bends
- Lightweight and very fast transcription
- Can scale and time quantize transcribed MIDI directly in the plugin

## Install NeuralNote

Download the latest release for your platform [here](https://github.com/DamRsn/NeuralNote/releases) (Windows and macOS (
Universal) supported)!

Currently, only the raw `.vst3`, `.component` (Audio Unit), `.app` and `.exe` (Standalone) files are provided.
Installers will be created soon. In the meantime, you can manually copy the plugin/app file in the appropriate
directory. The code is signed on macOS, but not on Windows, so you might have to perform few extra steps in order to be
able to use NeuralNote on Windows (to be documented soon).

## Usage

![UI](NeuralNote_UI.png)

NeuralNote comes as a simple AudioFX plugin (VST3/AU/Standalone app) to be applied on the track to transcribe.

The workflow is very simple:

- Gather some audio
    - Click record. Works when recording for real or when playing the track in a DAW.
    - Or drop an audio file on the plugin. (.wav, .aiff and .flac supported)
- The midi transcription instantly appears in the piano roll section. Play with the different settings to adjust it.
- Export the MIDI transcription with a simple drag and drop from the plugin to a MIDI track.

**Watch our presentation video for the Neural Audio Plugin
competition [here](https://www.youtube.com/watch?v=6_MC0_aG_DQ)**.

NeuralNote uses internally the model from Spotify's [basic-pitch](https://github.com/spotify/basic-pitch). See
their [blogpost](https://engineering.atspotify.com/2022/06/meet-basic-pitch/)
and [paper](https://arxiv.org/abs/2203.09893) for more information. In NeuralNote, basic-pitch is run
using [RTNeural](https://github.com/jatinchowdhury18/RTNeural) for the CNN part
and [ONNXRuntime](https://github.com/microsoft/onnxruntime) for the feature part (Constant-Q transform calculation +
Harmonic Stacking).
As part of this project, [we contributed to RTNeural](https://github.com/jatinchowdhury18/RTNeural/pull/89) to add 2D
convolution support.

## Build from source

Requirements are: `git`, `cmake`, and your OS's preferred compiler suite.

Use this when cloning:

```
git clone --recurse-submodules --shallow-submodules https://github.com/DamRsn/NeuralNote
 ```

The following OS-specific build scripts have to be executed at least once before being able to use the project as a
normal CMake project. The script downloads onnxruntime static library (that we created
with [ort-builder](https://github.com/olilarkin/ort-builder)) before calling CMake.

#### macOS

```
$ ./build.sh
```

#### Windows

Due to [a known issue](https://github.com/DamRsn/NeuralNote/issues/21), if you're not using Visual Studio 2022 (MSVC
version: 19.35.x, check `cl` output), then you'll need to manually build onnxruntime.lib like so:

1. Ensure you have Python installed; if not, download at https://www.python.org/downloads/windows/

2. Execute each of the following lines in a command prompt:

```
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/tiborvass/libonnxruntime-neuralnote ThirdParty\onnxruntime
cd ThirdParty\onnxruntime
python3 -m venv venv
.\venv\Scripts\activate.bat
pip install -r requirements.txt
.\convert-model-to-ort.bat model.onnx
.\build-win.bat model.required_operators_and_types.with_runtime_opt.config
copy model.with_runtime_opt.ort ..\..\Lib\ModelData\features_model.ort
cd ..\..
```

Now you can get back to building NeuralNote as follows:

```
> .\build.bat
```

#### Linux
##### Dependencies
Use you distro's package manager to install the following:
```
libXinerama-devel
libXcursor-devel
freetype2-devel
alsa-devel
libcurl-devel
webkit2gtk4-devel
```
You need a custom build of [libonnxruntime-neuralnote](https://github.com/tiborvass/libonnxruntime-neuralnote/)
The Build-linux script in this repo works, except it doesn't create the shared libraries in the lib directory as used by the make-archive script. Adding the following lines to the end of build-linux.sh in this repo will allow make archive to generate the appropriate archive
```
mkdir -p "lib"

# using find to avoid cp stat errors with wildcards
find ./onnxruntime/build/Linux_x86_64/MinSizeRel -name "libonnxruntime.so*" -exec cp {} -a lib/ \;
```
Then run `./make-archive.sh v1.14.1-neuralnote.0`.  When it is finished, manually copy the resultant linux archive (eg onnxruntime-v1.14.1-neuralnote.0-linux-x86_64.tar.gz) to the root of the NeuralNote repo.
Now, running build.sh should find and extract the compiled for linux archive.
Alternatively, manually extract the archive.  This is usedfull if you used the "dirty" build of libonnxruntime. For example
```
rm -rf ThirdParty/onnxruntime
tar -C ThirdParty/ -xvf onnxruntime-neuralnote.git2067da8.dirty-linux-x86_64.tar.gz
mv ThirdParty/onnxruntime-neuralnote.git2067da8.dirty-linux-x86_64/ ThirdParty/onnxruntime
cp -a ThirdParty/onnxruntime/model.with_runtime_opt.ort Lib/ModelData/features_model.ort
```
Then run the `build-linux.sh` script.

At the moment, this will fail while linking with the following error:
```
/usr/lib64/gcc/x86_64-suse-linux/12/../../../../x86_64-suse-linux/bin/ld: libBasicPitchCNN.a(BasicPitchCNN.cpp.o): relocation R_X86_64_32 against `.rodata.str1.1' can not be used when making a shared object; recompile with -fPIC
/usr/lib64/gcc/x86_64-suse-linux/12/../../../../x86_64-suse-linux/bin/ld: failed to set dynamic section sizes: bad value
collect2: error: ld returned 1 exit status
```
#### IDEs

Once the build script has been executed at least once, you can load this project in your favorite IDE
(CLion/Visual Studio/VSCode/etc) and click 'build' for one of the targets.

## Reuse code from NeuralNote’s transcription engine

All the code to perform the transcription is in `Lib/Model` and all the model weights are in `Lib/ModelData/`. Feel free
to use only this part of the code in your own project! We'll try to isolate it more from the rest of the repo in the
future and make it a library.

The code to generate the files in `Lib/ModelData/` is not currently available as it required a lot of manual operations.
But here's a description of the process we followed to create those files:

- `features_model.onnx` was generated by converting a keras model containing only the CQT + Harmonic Stacking part of
  the full basic-pitch graph using `tf2onnx` (with manually added weights for batch normalization).
- the `.json` files containing the weights of the basic-pitch cnn were generated from the tensorflow-js model available
  in the [basic-pitch-ts repository](https://github.com/spotify/basic-pitch-ts), then converted to onnx with `tf2onnx`.
  Finally, the weights were gathered manually to `.npy` thanks to [Netron](https://netron.app/) and finally applied to a
  split keras model created with [basic-pitch](https://github.com/spotify/basic-pitch) code.

The original basic-pitch CNN was split in 4 sequential models wired together, so they can be run with RTNeural.

## Roadmap

- Improve stability
- Save plugin internal state properly, so it can be loaded back when reentering a session
- Add tooltips
- Build a simple synth in the plugin so that one can listen to the transcription while playing with the settings, before
  export
- Allow pitch bends on non-overlapping parts of overlapping notes
- Support transcription of mp3 files

## Bug reports and feature requests

If you have any request/suggestion concerning the plugin or encounter a bug, please file a GitHub issue.

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

## Could NeuralNote transcribe audio in real-time?

Unfortunately no and this for a few reasons:

- Basic Pitch uses the Constant-Q transform (CQT) as input feature. The CQT requires really long audio chunks (> 1s) to
  get amplitudes for the lowest frequency bins. This makes the latency too high to have real-time transcription.
- The basic pitch CNN has an additional latency of approximately 120ms.
- Very few DAWs support audio input/MIDI output plugins as far as I know. This is partially why NeuralNote is an
  Audio FX plugin (audio-to-audio) and that MIDI is exported via drag and drop.

But if you have ideas please share!

## Credits

NeuralNote was developed by [Damien Ronssin](https://github.com/DamRsn) and [Tibor Vass](https://github.com/tiborvass).
The plugin user interface was designed by Perrine Morel.