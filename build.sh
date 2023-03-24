#!/usr/bin/env bash
set -euf -o pipefail
set -x
os=windows
arch="$(uname -m)"
file=onnxruntime.lib
version=v1.14.1-neuralnote.1
if test "$(uname -s)" = "Darwin"; then
	os=macOS
	arch=universal
	file=libonnxruntime.a
	# To remove warnings about minimum macOS target versions
	version=v1.14.1-neuralnote.2
fi
dir="onnxruntime-${version}-${os}-${arch}"
archive="$dir.tar.gz"

# If either the library or the ort model is missing or if an archive was found
# then fetch ort model and library again.
if test ! -f "Lib/ModelData/features_model.ort" -o ! -f "ThirdParty/onnxruntime/lib/$file" -o -f "$archive"; then
	if ! test -f "$archive"; then
		curl -fsSLO "https://github.com/tiborvass/libonnxruntime-neuralnote/releases/download/${version}/${archive}"
	fi
	rm -rf ThirdParty/$dir ThirdParty/onnxruntime
	tar -C ThirdParty/ -xvf "$archive"
	mv "ThirdParty/$dir" ThirdParty/onnxruntime
	mv ThirdParty/onnxruntime/model.with_runtime_opt.ort Lib/ModelData/features_model.ort
	rm "$archive"
fi

ncpus="$(getconf _NPROCESSORS_ONLN || echo 1)"

config=Release
cmake -S . -B build -DCMAKE_BUILD_TYPE="${config}" -DBUILD_UNIT_TESTS=ON && cmake --build build -j "${ncpus}"

./build/Tests/UnitTests_artefacts/Release/UnitTests
echo
echo "Run: ./build/NeuralNote_artefacts/Release/Standalone/NeuralNote.app/Contents/MacOS/NeuralNote"
