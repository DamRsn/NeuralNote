#!/usr/bin/env bash -e

set -euo pipefail

# First argument gives the path to the dir containing the .app, .component and .vst3
PLUG_DIR=$1

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
read -s -p "Enter your Apple ID password: " APPLE_PASSWORD
echo

chmod +x "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3}/Contents/MacOS/NeuralNote
codesign --remove-signature "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3} || true
codesign --entitlements entitlements.plist --options=runtime -s "$signingID" "$PLUG_DIR"/{Standalone/NeuralNote.app,AU/NeuralNote.component,VST3/NeuralNote.vst3}

# Build installer
packagesbuild --project Installers/Mac/NeuralNote.pkgproj
mv Installers/Mac/build/NeuralNote.pkg Installers/Mac/build/NeuralNote_unsigned.pkg

# Sign installer
product_sign_ID=$(echo "$signingID" | sed 's/Application/Installer/')
#productsignID=$(security find-identity -v -p basic | grep "Developer ID Installer" | head -1 | cut -d'"' -f2)
productsign --sign "$product_sign_ID" Installers/Mac/build/NeuralNote_unsigned.pkg Installers/Mac/build/NeuralNote.pkg
rm Installers/Mac/build/NeuralNote_unsigned.pkg

# Notarize the pkg and staple it
xcrun notarytool submit --apple-id "$APPLE_USERNAME" --team-id "$APPLE_TEAMID" --password "$APPLE_PASSWORD" --wait Installers/Mac/build/NeuralNote.pkg
xcrun stapler staple Installers/Mac/NeuralNote.pkg
