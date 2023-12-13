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

The syntax is similar to [mpv.net](https://github.com/mpvnet-player/mpv.net):

**input.conf**
```
Ctrl+a  show-text hello  #menu: Foo > Bar
```

## Credits

This project contains code copied from [mpv](https://github.com/mpv-player/mpv).
