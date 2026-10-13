#!/usr/bin/env bash

# Puts the Developer ID certificates in a temporary keychain for codesign and productsign.
# Needs MACOS_CERTIFICATES_P12 (base64) and MACOS_CERTIFICATES_PASSWORD.

set -euo pipefail

KEYCHAIN=$RUNNER_TEMP/signing.keychain-db

keychain_password=$(openssl rand -base64 24)

security create-keychain -p "$keychain_password" "$KEYCHAIN"
security set-keychain-settings -lut 21600 "$KEYCHAIN"
security unlock-keychain -p "$keychain_password" "$KEYCHAIN"

p12=$RUNNER_TEMP/certificates.p12
echo "$MACOS_CERTIFICATES_P12" | base64 --decode > "$p12"
security import "$p12" -k "$KEYCHAIN" -P "$MACOS_CERTIFICATES_PASSWORD" -T /usr/bin/codesign -T /usr/bin/productsign
rm "$p12"

# Without the intermediate, find-identity -v lists the Developer ID certificates as invalid.
intermediate=$RUNNER_TEMP/DeveloperIDG2CA.cer
curl -fsSL https://www.apple.com/certificateauthority/DeveloperIDG2CA.cer -o "$intermediate"
security import "$intermediate" -k "$KEYCHAIN"

# Lets codesign and productsign use the keys without a UI prompt.
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$keychain_password" "$KEYCHAIN" > /dev/null
security list-keychains -d user -s "$KEYCHAIN" $(security list-keychains -d user | tr -d '"')
