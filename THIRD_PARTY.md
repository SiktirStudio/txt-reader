# Third-party software notices

NovaPad itself is MIT-licensed (see `LICENSE`). Release packages dynamically
link Qt; the Linux package also distributes OpenSSL and, when required by Qt,
ICU libraries. They retain their respective licenses and copyrights. This does
not change NovaPad's own license.

- **Qt 6**: Copyright The Qt Company Ltd. and other contributors. The modules
  used here (Core, Gui, Widgets, PrintSupport, Network and deployed plugins) are
  distributed under the applicable open-source licenses, including LGPL v3.
  LGPL v3 and its incorporated GPL v3 text, together with Qt's third-party
  license texts, are under `licenses/Qt-*` in the binary packages and
  `resources/licenses/Qt-*` in source. Corresponding Qt sources, third-party
  attributions and build instructions are available at
  <https://github.com/qt/qtbase/tree/v6.8.3> and <https://code.qt.io/cgit/qt/>.
  The exact Qt runtime version is printed by `novapad --version`.
- **OpenSSL 3** (Linux): Copyright The OpenSSL Project Authors and contributors;
  Apache License 2.0. See `licenses/OpenSSL.txt`. Source and build instructions:
  <https://github.com/openssl/openssl>. The Ubuntu package's additional notices
  are included when supplied by the build image; corresponding distribution
  source packages are available from Ubuntu.
- **ICU** (Linux, if required by the installed Qt): Copyright Unicode, Inc. and
  others. See `licenses/ICU.txt`; source and notices:
  <https://github.com/unicode-org/icu/tree/release-73-2>.
- **JetBrains Mono**: SIL Open Font License 1.1; see `licenses/LICENSE-JetBrainsMono.txt`.
- **Fira Sans**: SIL Open Font License 1.1; see `licenses/LICENSE-Fira.txt`.

The bundled shared libraries are not modified by NovaPad. Users may replace
compatible library builds in the package (or rebuild NovaPad from source),
including for debugging modifications to LGPL components; NovaPad imposes no
additional restriction on that use. No application signing mechanism is used
to prevent replacement. Operating-system libraries are supplied by the OS and
are not included except where explicitly stated above.
