# mpv-menu-plugin

Customizable context menu for mpv on Windows, based on the [C PLUGINS](https://mpv.io/manual/master/#c-plugins) API.

![screenshot](screenshot/menu.jpg)

See also [mpv-debug-plugin](https://github.com/tsl0922/mpv-debug-plugin).

## Installation

[mpv](https://mpv.io) >= `0.37.0` is required, and the `cplugins` feature should be enabled.

Download the plugin from Releases, and place `menu.dll` in your mpv `scripts` folder.

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

# License

[GPLv2](LICENSE.txt).