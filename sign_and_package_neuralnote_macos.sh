#!/usr/bin/env bash -e

set -euo pipefail

# First argument gives the path to the dir containing the Standalone, AU and VST3 subdirectories.
# Typically cmake-build-release/NeuralNote_artefacts/Release or build/NeuralNote_artefacts/Release
# The optional second one is the installer's file name, NeuralNote_Installer_Mac_<arm64|x64>.pkg by default.
PLUG_DIR=${1:-}
PKG_NAME=${2:-}

if [[ $# -eq 0 ]]; then
	>&2 echo "usage: $0 <release_dir> [installer_name.pkg]"; exit 1
fi

for dir in "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3}; do
	if ! test -d "$dir"; then
		>&2 echo "Could not find $dir"
		exit 1
	fi

	# Without it, Metal fails to initialise and Auto runs on the CPU.
	if ! test -f "$dir/Contents/Resources/default.metallib"; then
		>&2 echo "Missing $dir/Contents/Resources/default.metallib: build with MUSCRIPTOR_METAL_PRECOMPILED=ON"
		exit 1
	fi
done

ARCH=$(lipo -archs "$PLUG_DIR"/Standalone/NeuralNote.app/Contents/MacOS/NeuralNote)
for dir in "$PLUG_DIR"/{AU/NeuralNote.component,VST3/NeuralNote.vst3}; do
	if [[ "$(lipo -archs "$dir/Contents/MacOS/NeuralNote")" != "$ARCH" ]]; then
		>&2 echo "$dir is not built for $ARCH like the Standalone"
		exit 1
	fi
done

# x64, as the release assets spell it.
PKG_NAME=${PKG_NAME:-NeuralNote_Installer_Mac_${ARCH/x86_64/x64}.pkg}
if [[ "$PKG_NAME" != *.pkg || "$PKG_NAME" == */* ]]; then
	>&2 echo "The installer name must be a file name ending in .pkg: $PKG_NAME"
	exit 1
fi

# Refuse a name that advertises the other architecture.
shopt -s nocasematch
if [[ ( "$ARCH" == arm64 && "$PKG_NAME" =~ x64|x86_64|intel ) || ( "$ARCH" == x86_64 && "$PKG_NAME" =~ arm64 ) ]]; then
	>&2 echo "$PKG_NAME names another architecture than the binaries' ($ARCH)"
	exit 1
fi
shopt -u nocasematch

PKG=Installers/Mac/build/$PKG_NAME
echo "Packaging the $ARCH build as $PKG"

signingID=$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 | cut -d'"' -f2)
if test -z "$signingID"; then
	>&2 echo "No signing certificates found in keychain. You need to import the Apple generated .p12 certificates into Keychain"
	exit 1
fi

read -p "Enter your Apple ID: " APPLE_USERNAME
APPLE_TEAMID=$(echo "$signingID" | cut -d'(' -f2 | cut -d')' -f1)
if test -z "$APPLE_TEAMID"; then
	read -p "Enter your Apple Team ID: " APPLE_TEAMID
fi
read -s -p "Enter your Apple ID password (App specific): " APPLE_PASSWORD
echo

chmod +x "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3}/Contents/MacOS/NeuralNote

echo "Signing Standalone, AU and VST3"
codesign --remove-signature "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3} || true
codesign --entitlements entitlements.plist --options=runtime -s "$signingID" "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3}

# Check signature
printf "\nVerifying signature app\n"
codesign -dv --verbose=4 "$PLUG_DIR"/Standalone/NeuralNote.app

printf "\nVerifying signature VST3\n"
codesign -dv --verbose=4 "$PLUG_DIR"/VST3/NeuralNote.vst3

printf "\nVerifying signature AU\n"
codesign -dv --verbose=4 "$PLUG_DIR"/AU/NeuralNote.component

# Build installer
echo "Building installer"
# Per-architecture build folder, so packaging both architectures at once doesn't collide.
UNSIGNED_DIR=$PWD/Installers/Mac/build/unsigned_$ARCH
packagesbuild -F "$PLUG_DIR" --build-folder "$UNSIGNED_DIR" Installers/Mac/NeuralNote.pkgproj

# Sign installer
echo "Signing installer"
product_sign_ID=$(security find-identity -v -p basic | grep "Developer ID Installer" | head -1 | cut -d'"' -f2)
productsign --sign "$product_sign_ID" "$UNSIGNED_DIR/NeuralNote.pkg" "$PKG"
rm -r "$UNSIGNED_DIR"

# Notarize the pkg and staple it
echo "Notarize and staple installer"
xcrun notarytool submit --apple-id "$APPLE_USERNAME" --team-id "$APPLE_TEAMID" --password "$APPLE_PASSWORD" --wait "$PKG"
xcrun stapler staple "$PKG"
