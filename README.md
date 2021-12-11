# hyperion-obs

[![Latest-Release](https://img.shields.io/github/v/release/hyperion-project/hyperion-obs-plugin)](https://github.com/hyperion-project/hyperion-obs-plugin/releases)
[![GitHub Actions](https://github.com/hyperion-project/hyperion-obs-plugin/workflows/hyperion-obs/badge.svg?branch=main)](https://github.com/hyperion-project/hyperion-obs-plugin/actions)

An [OBS Studio][obs] plugin that provides output capabilities to a
[Hyperion.ng][hyperion] Server.

The idea for this plugin originated from a [Hyperion.ng][hyperion] fork of [Murat Seker][m-seker].

## Usage with hyperion-obs

- Open OBS and select the menu entry `Tools > Hyperion Streaming`.
- Enter the flatbuffer destination IP and select the appropriate port.
- Click the `Start` button.

![hyperion-obs](screenshot/hyperion-obs.png)

## Download

See [Release Page](https://github.com/hyperion-project/hyperion-obs-plugin/releases)

## Build
### Windows
First follow the build guide for [OBS Studio][obs_build].
- Add the following entries before the first configuration:

| Entry name         | Type     | Value (e.g.)            |
|--------------------|----------|-------------------------|
| OBS_SOURCE         | PATH     | /obs-studio             |
| OBS_BUILD          | PATH     | /obs-studio/build       |

- This should cause the plugin DLL file to be created in the desired development environment.

### Linux
- Install QT and libobs

```
sudo apt install qtbase5-dev libobs-dev
```

- Get OBS Studio source code

```
git clone --recursive https://github.com/obsproject/obs-studio.git
```

- Build plugin

```
git clone --recursive https://github.com/hyperion-project/hyperion-obs-plugin.git
cd hyperion-obs-plugin
mkdir build && cd build
cmake -DOBS_SOURCE=../../obs-studio -DCMAKE_INSTALL_PREFIX=/usr ..
make -j $(nproc)
sudo make install
```

[obs]: https://obsproject.com/
[obs_build]: https://github.com/obsproject/obs-studio/wiki/install-instructions#windows-build-directions
[hyperion]: https://github.com/hyperion-project/hyperion.ng
[m-seker]: https://github.com/m-seker/hyperion.ng/tree/feature/obs