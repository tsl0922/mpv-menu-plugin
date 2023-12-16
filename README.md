# mpv-plugin

mpv plugin for Windows based on the [C PLUGINS](https://mpv.io/manual/master/#c-plugins) API.

## Features

### Context Menu

![screenshot](screenshot/menu.jpg)

Customizable native context menu for mpv window.

## Installation

> This plugin is still in development, no Releases are provided for now.
> You can download development build from the Actions Artifacts.

Place `menu.dll` in your mpv `scripts` folder.

## Configuration

### input.conf

The syntax is similar to [mpv.net](https://github.com/mpvnet-player/mpv.net):

```
Ctrl+a  show-text hello  #menu: Foo > Bar
```

### ~~/script-opts/menu.conf

> **NOTE:** If you changed the dll name, replace `menu` with it.

- `uosc=yes`: Enalbe [uosc](https://github.com/tomasklaen/uosc) menu syntax support.

## Credits

This project contains code copied from [mpv](https://github.com/mpv-player/mpv).
