#!/bin/bash
# WADAHEX: send your Mac signing credentials to GitHub as encrypted repository secrets,
# then start a signed + notarized build. Run this on your Mac, in Terminal:
#     bash setup_mac_signing.sh
# Nothing is sent anywhere except GitHub's encrypted secrets store for yishayraziel/wadahex.
set -euo pipefail
REPO="yishayraziel/wadahex"
APP_P12="${1:-$HOME/Desktop/DeveloperIDApplication.p12}"
INST_P12="${2:-$HOME/Desktop/DeveloperIDInstaller.p12}"

say() { printf "\n\033[1;31m▶\033[0m %s\n" "$*"; }

# 1. GitHub CLI (downloaded to a private folder if it isn't installed - no Homebrew needed)
if ! command -v gh >/dev/null 2>&1; then
  GHDIR="$HOME/.wadahex-gh"
  if [ ! -x "$GHDIR/bin/gh" ]; then
    say "Downloading GitHub CLI (one time)"
    ARCH=$(uname -m); [ "$ARCH" = "x86_64" ] && ARCH=amd64
    TAG=$(curl -fsSI https://github.com/cli/cli/releases/latest | tr -d '\r' | sed -nE 's#^[Ll]ocation: .*/tag/v(.*)$#\1#p')
    [ -n "$TAG" ] || { echo "Could not find the latest GitHub CLI version - check your internet connection."; exit 1; }
    TMPD=$(mktemp -d)
    curl -fsSL -o "$TMPD/gh.zip" "https://github.com/cli/cli/releases/download/v$TAG/gh_${TAG}_macOS_${ARCH}.zip"
    unzip -q "$TMPD/gh.zip" -d "$TMPD"
    mkdir -p "$GHDIR"; cp -R "$TMPD"/gh_*/bin "$GHDIR/"
    xattr -dr com.apple.quarantine "$GHDIR" 2>/dev/null || true
  fi
  export PATH="$GHDIR/bin:$PATH"
fi
gh auth status >/dev/null 2>&1 || { say "Log in to GitHub (a browser window will open)"; gh auth login -h github.com -p https -w; }

# 2. Certificates exported from Keychain Access
for f in "$APP_P12" "$INST_P12"; do
  [ -f "$f" ] || { echo "Missing $f - export it from Keychain Access (see steps), or pass the paths: bash setup_mac_signing.sh app.p12 installer.p12"; exit 1; }
done
say "Password you gave the .p12 files when exporting"
read -r -s -p "p12 password: " P12PW; echo

subject_of() { openssl pkcs12 -in "$1" -nokeys -passin pass:"$P12PW" 2>/dev/null | openssl x509 -noout -subject 2>/dev/null || true; }
APP_SUBJ=$(subject_of "$APP_P12"); INST_SUBJ=$(subject_of "$INST_P12")
[ -n "$APP_SUBJ" ] || { echo "Could not open $APP_P12 with that password."; exit 1; }
echo "$APP_SUBJ"  | grep -q "Developer ID Application" || { echo "$APP_P12 is not a 'Developer ID Application' certificate."; exit 1; }
echo "$INST_SUBJ" | grep -q "Developer ID Installer"   || { echo "$INST_P12 is not a 'Developer ID Installer' certificate."; exit 1; }
TEAM_ID=$(echo "$APP_SUBJ" | sed -nE 's/.*OU ?= ?([A-Z0-9]{10}).*/\1/p')
[ -n "$TEAM_ID" ] || read -r -p "Team ID (developer.apple.com > Membership): " TEAM_ID
echo "Team ID: $TEAM_ID"

# 3. Notarization login
say "Apple ID email of your developer account"
read -r -p "Apple ID: " APPLE_ID
say "App-specific password (appleid.apple.com > Sign-In and Security > App-Specific Passwords)"
read -r -s -p "App-specific password: " APPLE_APP_PASSWORD; echo

# 4. Upload as encrypted GitHub secrets
say "Saving secrets to $REPO"
base64 -i "$APP_P12"  | tr -d '\n' | gh secret set MACOS_CERT_APP_P12       --repo "$REPO"
base64 -i "$INST_P12" | tr -d '\n' | gh secret set MACOS_CERT_INSTALLER_P12 --repo "$REPO"
printf %s "$P12PW"              | gh secret set MACOS_CERT_PASSWORD --repo "$REPO"
printf %s "$APPLE_ID"           | gh secret set APPLE_ID            --repo "$REPO"
printf %s "$APPLE_APP_PASSWORD" | gh secret set APPLE_APP_PASSWORD  --repo "$REPO"
printf %s "$TEAM_ID"            | gh secret set APPLE_TEAM_ID       --repo "$REPO"
gh secret list --repo "$REPO"

# 5. Kick off a signed build
say "Starting a signed + notarized build"
gh workflow run build.yml --repo "$REPO" --ref main
echo
echo "Done. Follow it at https://github.com/$REPO/actions"
echo "You can now delete the two .p12 files from your Desktop."
