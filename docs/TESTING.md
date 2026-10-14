# Verification and release checklist

## Automated (Linux, Windows, macOS)

Requirements: CMake >= 3.21, C++17, Qt >= 6.4 with Widgets, PrintSupport and
Network. CI uses Qt 6.8 and Linux x64 / Windows x64 / macOS Apple Silicon.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir build -C Release --output-on-failure
```

On PowerShell, set `$env:QT_QPA_PLATFORM = 'offscreen'` before running CTest.
The executable is `build/bin/novapad` on Linux, `build/bin/novapad.exe` on Windows,
and `build/bin/novapad.app/Contents/MacOS/novapad` on macOS.

CTest runs:

- `novapad --selftest`: existing file I/O, search, syntax, editing and pairing
  regressions; four-column/tab-aware indentation; View preference propagation
  and restoration; full-screen restoration from normal/maximized states; native
  state/menu synchronization; embedded icon; and offline AI regression tests.
- `novapad --stress`: 120 cycles of tab creation, editing, switching and closing,
  then multi-tab teardown. The dock/client also participate in window teardown.
- `novapad --tls-check`: require an available Qt TLS backend.
- `novapad --version`: require the CMake project version in the output.

Self-test and stress use temporary INI settings, never the user's session or
credentials. AI tests use only disposable mock keys and local TCP servers;
there are no real provider calls, keys in CI, or paid requests. Tests cover
payloads, provider headers, secure URL rules, TLS request configuration,
redirect refusal, HTTP errors, malformed/oversized responses, cancellation,
selection normalization and the actual QSettings persistence path.

CI smoke-tests **the deployed application** with `--tls-check`, `--selftest` and
`--stress` before archiving each platform's package. Linux packages include
Qt Network, TLS plugins, OpenSSL 3 libraries, a launcher and `qt.conf`.
The launcher resolves the bundled libraries/plugins rather than relying on an
installed development Qt. Desktop/X11/Wayland libraries and CA certificates
remain operating-system dependencies; Linux binaries target Ubuntu 24.04 or a
compatible newer distribution. Windows/macOS use their deployed Qt TLS backends.

## Manual desktop checks

Automated offscreen state checks do not replace testing a real window manager.
Before distributing widely, verify on each target desktop:

1. Launch into maximized mode, both with and without saved window geometry.
2. Toggle F11 twice from maximized; repeat from a resized normal window.
   Confirm the original mode is restored; also try the OS-native full-screen
   controls and verify the View checkmark.
3. Toggle View → Indentation Guides, create a tab, restart, and verify the
   choice is retained. Check four/eight spaces, tabs, mixed indentation,
   horizontal scrolling, wrapping, zoom and both themes.
4. Open the AI dock; select code and add it. Confirm nothing is sent until Send.
   Against a trusted test account, verify one real request with each provider.
   Confirm Cancel works and invalid certificates / non-loopback HTTP fail.
5. Change provider/base URL and confirm the masked key is cleared. Close and
   reopen the application and confirm the key and prompt are absent.
6. Inspect the new white-background icon at small and large sizes. On macOS,
   check the application bundle icon as well as the running application's icon.
7. Test extracted packages on machines without a development Qt installation.
   Windows/macOS downloads are unsigned/unnotarized; OS warnings are expected.

Real desktop checks, live-provider interoperability, signing/notarization and
clean-machine GUI tests are not automated in this repository. Do not describe
them as completed merely because CI is green.

## Publishing

1. Use an unused version and update CMake, the CLI fallback and bilingual release
   notes under `.github/RELEASE_NOTES_v<version>.md`.
2. Open a PR from the working branch. **Do not merge before Linux, Windows and
   macOS jobs all succeed on the latest PR commit.**
3. After merge, tag the merge commit `v<version>` (never replace a published tag).
4. The tag workflow builds/tests/packages again. Its publication job depends on
   the entire matrix, downloads all three artifacts, and creates/updates the
   release using the matching notes file.
5. Verify the release contains all three correctly versioned packages in the
   `uploaded` state. A pushed tag or a partial release is not completion.
