# hyperion-obs

An [OBS Studio][obs] plugin that provides output capabilities to a
[Hyperion.ng][hyperion] Server.

The idea for this plugin originated from a fork of [Murat Seker][m-seker].

[obs]: https://obsproject.com/
[hyperion]: https://github.com/hyperion-project/hyperion.ng
[m-seker]: https://github.com/m-seker/hyperion.ng/tree/feature/obs

## Usage with hyperion-obs

- Open OBS and select the menu entry `Tools > Hyperion Output`.
- Enter the flatbuffer destination IP and select the appropriate port.
- Click the `Start` button.

![hyperion-obs](screenshot/hyperion-obs.png)

## Build

- Install QT and libobs

```
sudo apt install qtbase5-dev libobs-dev
```

- Build plugin

```
git clone --recursive https://github.com/hyperion-project/hyperion-obs-plugin.git
cd hyperion-obs-plugin
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j $(nproc)
sudo make install
```
