# mpv-menu-plugin

Configurable context menu for [mpv](https://mpv.io) on Windows, with additional support for native file dialog and clipboard.

![screenshot](screenshot/menu.jpg)

See also [mpv-debug-plugin](https://github.com/tsl0922/mpv-debug-plugin).

## Features

- Configurable Native Context Menu
- Open from File Dialog / Clipboard
- Scripting API for developers

## Installation

[mpv](https://mpv.io) >= `0.37.0` is required, and the `cplugins` feature should be enabled.

Download the plugin from [Releases](https://github.com/tsl0922/mpv-menu-plugin/releases/latest), place the `.dll` and `.lua` files in your mpv [scripts](https://mpv.io/manual/master/#script-location) folder.

> [!IMPORTANT]
> **THIS PLUGIN DOES NOT HAVE A BUILTIN DEFAULT MENU CONFIG.**
> You must define some menu items in [input.conf](https://mpv.io/manual/master/#command-interface), or you won't see a menu, check the **Configuration** section below.
>
> See also [Frequently Asked Questions (FAQ)](https://github.com/tsl0922/mpv-menu-plugin/wiki/FAQ).

> [!TIP]
> To find mpv config location on Windows, run `echo %APPDATA%\mpv` in `cmd.exe`.
>
> You can also use `portable_config` next to `mpv.exe`, read [FILES ON WINDOWS](https://mpv.io/manual/master/#files-on-windows).
>
> If the `scripts` folder doesn't exist in mpv config dir, you may create it yourself.

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
