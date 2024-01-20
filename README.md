# mpv-menu-plugin

Configurable context menu for mpv on Windows, based on the [C PLUGINS](https://mpv.io/manual/master/#c-plugins) API.

![screenshot](screenshot/menu.jpg)

See also [mpv-debug-plugin](https://github.com/tsl0922/mpv-debug-plugin).

## Features

- Configurable Native Context Menu
- Get / Set Clipboard data from script
- Show Native File Dialog from script

## Installation

[mpv](https://mpv.io) >= `0.37.0` is required, and the `cplugins` feature should be enabled.

Download the plugin from Releases, place `menu.dll` and `dyn_menu.lua` in your mpv `scripts` folder.

## Configuration

- [Documentation](https://github.com/tsl0922/mpv-menu-plugin/wiki/Configuration)
- [Example](https://gist.github.com/tsl0922/8989aa32994b0448a2652ee260348a35)

## Scripting

- [Documentation](https://github.com/tsl0922/mpv-menu-plugin/wiki/Scripting)
- [Example](https://github.com/tsl0922/mpv-menu-plugin/wiki/Scripting-example)

## Credits

This project contains code copied from [mpv](https://github.com/mpv-player/mpv).

# License

[GPLv2](LICENSE.txt).
