#!/usr/bin/env bash
set -euo pipefail

version="${1:-dev}"
package="dist/NovaPad-linux-x64"
qtlibs="$(qmake -query QT_INSTALL_LIBS)"
qtplugins="$(qmake -query QT_INSTALL_PLUGINS)"
mkdir -p "$package/bin" "$package/lib" "$package/plugins"
cp build/bin/novapad "$package/bin/"
# TLS is mandatory, not a best-effort copy. The OpenSSL backend loads its
# libraries dynamically, so copying only the executable's ldd output is not enough.
for plugin in platforms imageformats tls; do
    cp -a "$qtplugins/$plugin" "$package/plugins/"
done
mapfile -t plugins < <(find "$package/plugins" -type f -name '*.so*')
dependencies="$(ldd build/bin/novapad "${plugins[@]}")"
while IFS= read -r library; do
    case "$library" in
        "$qtlibs"/*) cp -L "$library" "$package/lib/" ;;
    esac
done < <(printf '%s\n' "$dependencies" | awk '$2 == "=>" && $3 ~ /^\// {print $3}' | sort -u)
for library in libssl.so.3 libcrypto.so.3; do
    source="$(ldconfig -p | awk -v name="$library" '$1 == name && /x86-64/ {if (!found) print $NF; found=1}')"
    test -n "$source"
    cp -L "$source" "$package/lib/$library"
done
test -f "$package/lib/libQt6Network.so.6"
test -f "$package/plugins/tls/libqopensslbackend.so"
printf '[Paths]\nPrefix = ..\nPlugins = plugins\nLibraries = lib\n' > "$package/bin/qt.conf"
cat > "$package/novapad" <<'LAUNCHER'
#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$HERE/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$HERE/plugins"
exec "$HERE/bin/novapad" "$@"
LAUNCHER
chmod +x "$package/novapad"
# Resolve against the bundle rather than the development Qt installation.
for binary in "$package/bin/novapad" "$package/plugins/tls/"*.so; do
    resolved="$(LD_LIBRARY_PATH="$PWD/$package/lib" ldd "$binary")"
    if grep -q 'not found' <<< "$resolved"; then
        printf '%s\n' "$resolved" >&2
        exit 1
    fi
done
for test in --tls-check --selftest --stress; do
    env -u QT_PLUGIN_PATH -u QT_QPA_PLATFORM_PLUGIN_PATH -u LD_LIBRARY_PATH \
        QT_QPA_PLATFORM=offscreen "$package/novapad" "$test"
done
cp LICENSE "$package/"
mkdir -p "$package/licenses"
cp resources/fonts/LICENSE-*.txt resources/licenses/* "$package/licenses/"
# License texts for the bundled Qt / OpenSSL / ICU libraries are included from
# the installed Qt tree / system package when available; upstream notices are
# linked from THIRD_PARTY.md as well.
cp THIRD_PARTY.md "$package/"
for license in /usr/share/doc/libssl3t64/copyright "$qtlibs/../LICENSES/"*; do
    if [[ -f "$license" ]]; then
        cp "$license" "$package/licenses/$(basename "$(dirname "$license")")-$(basename "$license")"
    fi
done
tar -czf "NovaPad-$version-Linux-x64.tar.gz" -C dist NovaPad-linux-x64
