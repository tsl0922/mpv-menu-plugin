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

The menu syntax is similar to [mpv.net](https://github.com/mpvnet-player/mpv.net), with some enhancements:

- define menu title after `#menu:`
  - define separator with `-`
  - split title with `>` to define submenus
  - use `#@keyword` to display selection menu for:
    - `#@tracks/video`: video tracks
    - `#@tracks/audio`: audio tracks
    - `#@tracks/sub`: subtitles
    - `#@tracks/sub-secondary`: secondary subtitle
    - `#@chapters`: chapters
    - `#@editions`: editions
    - `#@audio-devices`: audio devices
- use `_` if no keybinding
- use `ignore` if no command

```
Ctrl+a  show-text foobar    #menu: Foo > Bar
_       ignore              #menu: -

_       ignore    #menu: Tracks > Video #@tracks/video
_       ignore    #menu: Tracks > Audio #@tracks/audio
_       ignore    #menu: -
_       ignore    #menu: Subtitle #@tracks/sub
_       ignore    #menu: Second Subtitle #@tracks/sub-secondary
_       ignore    #menu: -
_       ignore    #menu: Chapters #@chapters
_       ignore    #menu: Editions #@editions
_       ignore    #menu: -
_       ignore    #menu: Audio Devices #@audio-devices
```

Add a keybinding to trigger the menu (required):

```
MBTN_RIGHT script-message-to menu show
```

### ~~/script-opts/menu.conf

- `uosc=yes`: Enalbe [uosc](https://github.com/tomasklaen/uosc#syntax) menu syntax support.

## Credits

This project contains code copied from [mpv](https://github.com/mpv-player/mpv).
