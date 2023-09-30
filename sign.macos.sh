#!/usr/bin/env bash -e

set -euo pipefail

for dir in NeuralNote.{app,component,vst3}; do
	if ! test -d "$dir"; then
		>&2 echo "Could not find $dir"
		exit 1
	fi
done

signingID=$(security find-identity -v -p codesigning | head -1 | cut -d'"' -f2)
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

chmod +x NeuralNote.{app,component,vst3}/Contents/MacOS/NeuralNote
codesign --remove-signature NeuralNote.{app,component,vst3} || true
codesign --entitlements entitlements.plist --options=runtime -s "$signingID" NeuralNote.{app,component,vst3}

for dir in NeuralNote.{app,component,vst3}; do
	plist="$dir/Contents/Info.plist"
	bundleID=$(/usr/libexec/PlistBuddy -c "Print CFBundleIdentifier" "$plist")
	if test -z "$bundleID"; then
		>&2 echo "Could not find CFBundleIdentifier in $plist"
		exit 1
	fi
	ditto -c -k --sequesterRsrc --keepParent "$dir" "$dir.zip"
	xcrun notarytool submit --apple-id "$APPLE_USERNAME" --team-id "$APPLE_TEAMID" --password "$APPLE_PASSWORD" --wait "$dir.zip"
	xcrun stapler staple "$dir"
	rm "$dir.zip"
done
