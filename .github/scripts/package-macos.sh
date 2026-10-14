#!/usr/bin/env bash
set -euo pipefail
version="${1:-dev}"
app="build/bin/novapad.app"
mkdir -p "$app/Contents/Resources/licenses" "$app/Contents/PlugIns/platforms"
cp LICENSE THIRD_PARTY.md "$app/Contents/Resources/"
cp resources/licenses/* resources/fonts/LICENSE-*.txt "$app/Contents/Resources/licenses/"
# macdeployqt intentionally deploys only Cocoa, not the offscreen platform.
# Include offscreen explicitly and let macdeployqt relocate its Qt dependencies
# so the deployed bundle can run the same headless tests as the development app.
offscreen="$app/Contents/PlugIns/platforms/libqoffscreen.dylib"
cp "$(qmake -query QT_INSTALL_PLUGINS)/platforms/libqoffscreen.dylib" "$offscreen"
# Ad-hoc signing is required for relocated Mach-O libraries on Apple Silicon;
# this is NOT Developer ID signing or notarization.
macdeployqt "$app" "-executable=$offscreen" -codesign=- -dmg
test -d "$app/Contents/PlugIns/tls"
test -f "$offscreen"
for test in --tls-check --selftest --stress; do
    env -u QT_PLUGIN_PATH -u QT_QPA_PLATFORM_PLUGIN_PATH QT_QPA_PLATFORM=offscreen \
        "$app/Contents/MacOS/novapad" "$test"
done
mv build/bin/novapad.dmg "NovaPad-$version-macOS-arm64.dmg"
