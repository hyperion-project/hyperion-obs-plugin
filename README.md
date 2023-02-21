# hyperion-obs

[![Latest-Release](https://img.shields.io/github/v/release/hyperion-project/hyperion-obs-plugin)](https://github.com/hyperion-project/hyperion-obs-plugin/releases)
[![GitHub Actions](https://github.com/hyperion-project/hyperion-obs-plugin/workflows/hyperion-obs/badge.svg?branch=main)](https://github.com/hyperion-project/hyperion-obs-plugin/actions)

An [OBS Studio][obs] plugin that provides output capabilities to a [Hyperion.ng][hyperion] Server. \
The idea for this plugin originated from a [Hyperion.ng][hyperion] fork of [Murat Seker][m-seker].

## Usage with hyperion-obs

- Open OBS and select the menu entry `Tools > Hyperion Streaming`.
- Enter the flatbuffer destination IP and select the appropriate port.
- Optionally you can change the `Priority` or the image `Output Decimation` factor.
- Click the `Start` button.

![hyperion-obs](screenshot/hyperion-obs.png)

## Contributing

Contributions are welcome! Feel free to join us! We are looking always for people who wants to participate.<br>
[![Contributors](https://img.shields.io/github/contributors/hyperion-project/hyperion-obs-plugin.svg?label=Contributors)](https://github.com/hyperion-project/hyperion-obs-plugin/graphs/contributors)

For an example, you can participate in the translation.<br>
[![Join Translation](https://img.shields.io/badge/POEditor-translate-green.svg)](https://poeditor.com/join/project?hash=0diZuCpLVX)

## Download

See [Release Page](https://github.com/hyperion-project/hyperion-obs-plugin/releases)

## Build (Windows/macOS/Linux)
1. In-tree build:
   - First follow the build instructions for OBS-Studio: [https://obsproject.com/wiki/Install-Instructions][obs_build]
   - Check out this repository to plugins/hyperion-obs
   - Append `add_subdirectory(hyperion-obs)` to plugins/CMakeLists.txt
   - Rebuild OBS Studio

   - Optional: Install [Flatbuffers][flatbuffers] as system library and use it with the CMake switch `-DUSE_SYSTEM_FLATBUFFERS_LIBS=ON`

2. Stand-alone build (Linux only):
    - Verify that you have package with development files for OBS
    - Check out this repository and run `cmake -S . -B build -DBUILD_OUT_OF_TREE=On && cmake --build build`

## License
The source is released under MIT-License (see https://opensource.org/licenses/MIT).<br>
[![GitHub license](https://img.shields.io/badge/License-MIT-yellow.svg)](https://raw.githubusercontent.com/hyperion-project/hyperion-obs-plugin/main/LICENSE)

[obs]: https://obsproject.com/
[obs_build]: https://obsproject.com/wiki/Install-Instructions
[flatbuffers]: https://google.github.io/flatbuffers/flatbuffers_guide_building.html
[hyperion]: https://github.com/hyperion-project/hyperion.ng
[m-seker]: https://github.com/m-seker
