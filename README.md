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

> **NOTE:** If you changed the dll name, `script-message-to` target and conf file name should apply too.

### input.conf

The menu syntax is similar to [mpv.net](https://github.com/mpvnet-player/mpv.net):

- define menu title after `#menu:`
  - define separator with `-`
  - split title with `>` to define submenus
  - everything after a `#` is considered a comment
- use `_` if no keybinding
- use `ignore` if no command

```
Ctrl+a  show-text foobar    #menu: Foo > Bar
_       ignore              #menu: -
```

Add a keybinding that trigger the menu:

```
MBTN_RIGHT script-message-to menu show
```

### ~~/script-opts/menu.conf

- `uosc=yes`: Enalbe [uosc](https://github.com/tomasklaen/uosc#syntax) menu syntax support.

## Credits

This project contains code copied from [mpv](https://github.com/mpv-player/mpv).
