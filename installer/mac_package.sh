#!/bin/bash
# Signs (Developer ID), notarizes and packages WADAHEX for macOS.
# Without the signing secrets it falls back to an ad-hoc signed, un-notarized build.
set -euo pipefail
VERSION="${VERSION:-1.0.0}"
APP="build/HEX1_artefacts/Release/Standalone/WADAHEX.app"
VST="build/HEX1_artefacts/Release/VST3/WADAHEX.vst3"
OUT="$PWD/out"; mkdir -p "$OUT" pkgroot/vst3 pkgroot/app dmg

SIGNED=0
if [ -n "${MACOS_CERT_APP_P12:-}" ] && [ -n "${MACOS_CERT_INSTALLER_P12:-}" ]; then
  SIGNED=1
  KC="$RUNNER_TEMP/wadahex.keychain-db"; KCP="$(uuidgen)"
  security create-keychain -p "$KCP" "$KC"
  security set-keychain-settings -lut 21600 "$KC"
  security unlock-keychain -p "$KCP" "$KC"
  echo "$MACOS_CERT_APP_P12"       | base64 --decode > "$RUNNER_TEMP/app.p12"
  echo "$MACOS_CERT_INSTALLER_P12" | base64 --decode > "$RUNNER_TEMP/inst.p12"
  for p in app inst; do
    security import "$RUNNER_TEMP/$p.p12" -k "$KC" -P "$MACOS_CERT_PASSWORD" -T /usr/bin/codesign -T /usr/bin/productbuild -T /usr/bin/productsign
  done
  security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KCP" "$KC" >/dev/null
  security list-keychains -d user -s "$KC" $(security list-keychains -d user | tr -d '"')
  APP_ID=$(security find-identity -v -p codesigning "$KC" | grep "Developer ID Application" | head -1 | sed -E 's/.*"(.*)"/\1/')
  INST_ID=$(security find-identity -v "$KC" | grep "Developer ID Installer" | head -1 | sed -E 's/.*"(.*)"/\1/')
  echo "Signing as: $APP_ID / $INST_ID"
  NOTARY=(--apple-id "$APPLE_ID" --password "$APPLE_APP_PASSWORD" --team-id "$APPLE_TEAM_ID" --wait)

  codesign --force --options runtime --timestamp --sign "$APP_ID" "$VST"
  codesign --force --options runtime --timestamp --entitlements installer/entitlements.plist --sign "$APP_ID" "$APP"
  codesign --verify --strict --verbose=2 "$VST" "$APP"

  # notarize the bare VST3 (for the manual-install zip) and staple the ticket to the bundle
  ditto -c -k --keepParent "$VST" "$RUNNER_TEMP/vst3.zip"
  xcrun notarytool submit "$RUNNER_TEMP/vst3.zip" "${NOTARY[@]}"
  xcrun stapler staple "$VST"
else
  echo "No signing secrets: ad-hoc signing (users will see a Gatekeeper warning)"
  codesign --force --deep -s - "$VST"
  codesign --force --deep -s - "$APP"
fi

(cd "$(dirname "$VST")" && ditto -c -k --keepParent WADAHEX.vst3 "$OUT/WADAHEX_mac_VST3.zip")

cp -R "$VST" pkgroot/vst3/
cp -R "$APP" pkgroot/app/
pkgbuild --root pkgroot/vst3 --identifier com.yraziel.wadahex.vst3 --version "$VERSION" --install-location "/Library/Audio/Plug-Ins/VST3" vst3.pkg
pkgbuild --root pkgroot/app  --identifier com.yraziel.wadahex.app  --version "$VERSION" --install-location "/Applications" app.pkg
if [ $SIGNED = 1 ]; then
  productbuild --sign "$INST_ID" --timestamp --package vst3.pkg --package app.pkg "dmg/WADAHEX Installer.pkg"
  xcrun notarytool submit "dmg/WADAHEX Installer.pkg" "${NOTARY[@]}"
  xcrun stapler staple "dmg/WADAHEX Installer.pkg"
else
  productbuild --package vst3.pkg --package app.pkg "dmg/WADAHEX Installer.pkg"
fi
cp docs/WADAHEX_Manual.pdf dmg/ || true
hdiutil create -volname "WADAHEX" -srcfolder dmg -ov -format UDZO "$OUT/WADAHEX_mac.dmg"
if [ $SIGNED = 1 ]; then
  codesign --force --timestamp --sign "$APP_ID" "$OUT/WADAHEX_mac.dmg"
  xcrun notarytool submit "$OUT/WADAHEX_mac.dmg" "${NOTARY[@]}"
  xcrun stapler staple "$OUT/WADAHEX_mac.dmg"
  spctl -a -vvv -t install "dmg/WADAHEX Installer.pkg" || true
fi
ls -la "$OUT"
