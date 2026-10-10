#!/bin/bash
# Installs Basement (VST3 + AU) from the downloaded GitHub artifact.
# Usage:  bash ~/Downloads/Basement-macOS/install-mac.sh
set -e

here="$(cd "$(dirname "$0")" && pwd)"

# Find the folder holding the plugins: next to this script, or in Downloads.
src=""
for d in "$here" "$HOME/Downloads/Basement-macOS" "$HOME/Downloads"; do
    if [ -d "$d/Basement.vst3" ]; then src="$d"; break; fi
done
if [ -z "$src" ] && [ -f "$HOME/Downloads/Basement-macOS.zip" ]; then
    unzip -oq "$HOME/Downloads/Basement-macOS.zip" -d "$HOME/Downloads/Basement-macOS"
    src="$HOME/Downloads/Basement-macOS"
fi
if [ -z "$src" ]; then
    echo "Basement.vst3 bulunamadi. Once GitHub'dan Basement-macOS'u Downloads klasorune indir."
    exit 1
fi

vst3="$HOME/Library/Audio/Plug-Ins/VST3"
au="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$vst3" "$au"

install_bundle() {
    local bundle="$1" dest="$2"
    [ -d "$src/$bundle" ] || return 0
    rm -rf "$dest/$bundle"
    cp -R "$src/$bundle" "$dest/"
    chmod +x "$dest/$bundle"/Contents/MacOS/* 2>/dev/null || true   # zip downloads drop the executable bit
    xattr -cr "$dest/$bundle"                                       # remove the "downloaded from internet" quarantine
    codesign --force --deep -s - "$dest/$bundle" 2>/dev/null || true
    echo "kuruldu: $dest/$bundle"
}

install_bundle Basement.vst3 "$vst3"
install_bundle Basement.component "$au"

if [ -d "$src/Basement.app" ]; then
    chmod +x "$src/Basement.app"/Contents/MacOS/* 2>/dev/null || true
    xattr -cr "$src/Basement.app"
    codesign --force --deep -s - "$src/Basement.app" 2>/dev/null || true
fi

echo "Tamam. FL Studio'yu ac: Options > Manage plugins > Start scan."
