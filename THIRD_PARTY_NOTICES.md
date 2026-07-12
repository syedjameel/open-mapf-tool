# Third-party notices

OpenMAPF itself is MIT-licensed (see [LICENSE](LICENSE)). It incorporates or
depends on the following third-party components.

## Vendored in this repository

| Component | Location | Upstream | License |
|---|---|---|---|
| Dear ImGui (docking branch) | `vendor/imgui/` (git submodule) | https://github.com/ocornut/imgui | MIT (LICENSE.txt in the submodule) |
| imgui-filebrowser | `vendor/imgui-filebrowser/imfilebrowser.h` | https://github.com/AirGuanZ/imgui-filebrowser | MIT (full text below) |
| subprocess.h | `vendor/subprocess/subprocess.h` | https://github.com/sheredom/subprocess.h | Unlicense / public domain (full text embedded at the top of the file) |

## Fetched at build time (not distributed with the sources)

| Component | How | License |
|---|---|---|
| Catch2 v3 | CMake FetchContent, test builds only (`OPENMAPF_BUILD_TESTS`) | BSL-1.0 |
| yaml-cpp 0.8 | CMake FetchContent, only with `-DOPENMAPF_FETCH_DEPS=ON` (otherwise a system package is used) | MIT |

## System dependencies (linked, not distributed)

| Component | License |
|---|---|
| GLFW 3 | zlib/libpng |
| yaml-cpp (system package) | MIT |
| OpenGL | n/a (system implementation) |

## Bundled benchmark data

`maps/movingai/` contains maps and scenario files from the MovingAI
benchmark collections (Sturtevant 2012; Stern et al. 2019), redistributed
unmodified for research convenience — see `maps/movingai/README.md` for
citations and source URLs.

## External solvers (executed, never distributed)

OpenMAPF drives external solver executables that users obtain and build
themselves (`scripts/get-solvers.sh` clones them from upstream so users
accept upstream terms directly). They are not part of this repository or of
any OpenMAPF release artifact, and they run as separate processes (no
linking), so their licenses do not affect OpenMAPF's MIT terms; they matter
to anyone redistributing the solvers themselves:

- LaCAM / LaCAM2 / LaCAM3 (Keisuke Okumura) — MIT.
- CBSH2-RTC (Jiaoyang Li et al.) — University of Southern California
  research license (© 2021 USC, all rights reserved): free for
  educational, research, and non-profit purposes with the notice retained;
  commercial use requires permission from the USC Stevens Center for
  Innovation. **Not an OSI-approved open-source license.**
- CBSH2-RTC-CHBP (Bojie Shen et al.) — GPL-2.0-or-later. The hexagonal
  fork (CBSH2-RTC-CHBP-HEX), being derived from it, must be published
  under GPL-2.0-or-later with a modification notice (see
  docs/RELEASING.md).

---

## MIT License — imgui-filebrowser

```
MIT License

Copyright (c) 2019-2025 Zhuang Guan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
