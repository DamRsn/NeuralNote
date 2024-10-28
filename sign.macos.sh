#!/usr/bin/env bash -e

set -euo pipefail

# First argument gives the path to the dir containing the Standalone, AU and VST3 subdirectories.
# Typically cmake-build-release/NeuralNote_artefacts/Release or build/NeuralNote_artefacts/Release
PLUG_DIR=${1:-}

if [[ $# -eq 0 ]]; then
	>&2 echo "usage: $0 <release_dir>"; exit 1
fi

for dir in "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3}; do
	if ! test -d "$dir"; then
		>&2 echo "Could not find $dir"
		exit 1
	fi
done

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
packagesbuild -F "$PLUG_DIR" Installers/Mac/NeuralNote.pkgproj
mv Installers/Mac/build/NeuralNote.pkg Installers/Mac/build/NeuralNote_unsigned.pkg

# Sign installer
echo "Signing installer"
product_sign_ID=$(security find-identity -v -p basic | grep "Developer ID Installer" | head -1 | cut -d'"' -f2)
productsign --sign "$product_sign_ID" Installers/Mac/build/NeuralNote_unsigned.pkg Installers/Mac/build/NeuralNote.pkg
rm Installers/Mac/build/NeuralNote_unsigned.pkg

# Notarize the pkg and staple it
echo "Notarize and staple installer"
xcrun notarytool submit --apple-id "$APPLE_USERNAME" --team-id "$APPLE_TEAMID" --password "$APPLE_PASSWORD" --wait Installers/Mac/build/NeuralNote.pkg
xcrun stapler staple Installers/Mac/build/NeuralNote.pkg
