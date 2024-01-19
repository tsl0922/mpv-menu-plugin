# mpv-menu-plugin

Customizable context menu for mpv on Windows, based on the [C PLUGINS](https://mpv.io/manual/master/#c-plugins) API.

![screenshot](screenshot/menu.jpg)

See also [mpv-debug-plugin](https://github.com/tsl0922/mpv-debug-plugin).

## Installation

[mpv](https://mpv.io) >= `0.37.0` is required, and the `cplugins` feature should be enabled.

Download the plugin from Releases, place `menu.dll` and `dyn_menu.lua` in your mpv `scripts` folder.

## Configuration

> [!TIP]
> - Want a usable full config example? [Check this gist](https://gist.github.com/tsl0922/8989aa32994b0448a2652ee260348a35).
> - You may define custom keywords and update menu with [message](#messages).

### input.conf

The menu syntax is similar to [mpv.net](https://github.com/mpvnet-player/mpv.net), with some enhancements:

- define menu title after `#menu:`
  - define separator with `-`
  - split title with `>` to define submenus
  - use `#@keyword` to display selection menu for:
    - `#@tracks`: track list (video/audio/subtitle)
      - `#@tracks/video`: video track list
      - `#@tracks/audio`: audio track list
      - `#@tracks/sub`: subtitle list
      - `#@tracks/sub-secondary`: subtitle list (secondary)
    - `#@chapters`: chapter list
    - `#@editions`: edition list
    - `#@audio-devices`: audio device list
    - `#@playlist`: playlist
    - `#@profiles`: profile list
  - use `#@prop:check` to check menu item if property type and value is:
    - `boolean`: `true`
    - `string`: not empty
    - `number`: not zero
    - `table`: not empty
    - none of above: not `nil`
  - `#@prop:check!` is the reverse form of `#@prop:check`
- use `_` if no keybinding
- use `ignore` if no command

```
Ctrl+a  show-text foobar    #menu: Foo > Bar
_       ignore              #menu: -
_       ignore              #menu: Tracks #@tracks
_       ignore              #menu: Chapters #@chapters
_       ignore              #menu: Editions #@editions
_       ignore              #menu: -
_       cycle mute          #menu: Audio > Mute #@mute:check
_       ignore              #menu: Audio > Devices #@audio-devices
```

Add a keybinding to trigger the menu (required):

```
MBTN_RIGHT script-message-to menu show
```

> [!NOTE]
> - If you changed the dll name, `script-message-to` target and conf file name should apply too.
> - If the menu doesn't always show on mouse click, Rename other scripts that used the `menu` name.
>
> If both `menu.dll` and `menu.lua` exists in scripts folder, one of it may be named with `menu2` by mpv,
> `script-message-to menu show` will break when it happens on `menu.dll`.

### ~~/script-opts/menu.conf

- `load=no`: Disable menu load on startup.
- `uosc=yes`: Enable [uosc](https://github.com/tomasklaen/uosc#syntax) menu syntax support.

### ~~/script-opts/dyn_menu.conf

- `max_title_length=80`: Limits the title length in dynamic submenu, set to 0 to disable.
- `max_playlist_items=20`: Limit the playlist items in submenu, set to 0 to disable.

## Scripting

### Properties

#### `user-data/menu/items`

> [!TIP]
> To reduce update frequency, it's recommended to update this property in [mp.register_idle(fn)](https://mpv.io/manual/master/#lua-scripting-mp-register-idle(fn)).

```
MPV_FORMAT_NODE_ARRAY
  MPV_FORMAT_NODE_MAP (menu item)
     "type"           MPV_FORMAT_STRING     (supported type: separator, submenu)
     "title"          MPV_FORMAT_STRING     (required if type is not separator)
     "cmd"            MPV_FORMAT_STRING     (optional)
     "state"          MPV_FORMAT_NODE_ARRAY[MPV_FORMAT_STRING] (supported state: checked, disabled)
     "submenu"        MPV_FORMAT_NODE_ARRAY[menu item]         (required if type is submenu)
```

The menu data of the C plugin is stored in this property, updating it will trigger an update of the menu UI.

> [!NOTE]
> Be aware that `dyn_menu.lua` is conflict with other scripts that also update the `user-data/menu/items` property,
> you may use the messages below if you only want to update part of the menu.

#### `user-data/menu/dialog/filters`

```
MPV_FORMAT_NODE_ARRAY
  MPV_FORMAT_NODE_MAP
    "name"            MPV_FORMAT_STRING
    "spec"            MPV_FORMAT_STRING
```

Custom file type filters used for open dialog, the first one will be selected by default.

Example:

```lua
local file_types = {
    { name = 'All Files (*.*)', spec = '*.*' },
    { name = 'Video Files',     spec = '*mp4;*.mkv' },
    { name = 'Audio Files',     spec = '*.mp3;*.m4a' },
    { name = 'Subtitle Files',  spec = '*.srt;*.ass' },
    { name = 'Playlist Files',  spec = '*.m3u;*.m3u8' },
}
```

#### `user-data/menu/dialog/default-path`

Default path for open and save dialog.

#### `user-data/menu/dialog/default-name`

Default file name for save dialog.

### Messages

> [!TIP]
> Want a usage example? Check [Scripting example](https://github.com/tsl0922/mpv-menu-plugin/wiki/Scripting-example) in the wiki.

#### Script Messages supported by `menu.dll`:

##### `clipboard/get <src>`

Retrieves data from the clipboard (text only).

The result is replied via: `srcript-message-to <src> clipboard-get-reply <text>`.

##### `clipboard/set <text>`

Places data on the clipboard (text only).

##### `dialog/open <src>`

Show an open dialog.

The result is replied via: `srcript-message-to <src> dialog-open-reply <path>`.

##### `dialog/open-multi <src>`

Show an open dialog that can select multiple files.

The result is replied via: `srcript-message-to <src> dialog-open-multi-reply <path1> <path2> ...`.

##### `dialog/open-folder <src>`

Show an open dialog that can select folder only.

The result is replied via: `srcript-message-to <src> dialog-open-folder-reply <path>`.

##### `dialog/save <src>`

Show a save dialog.

The result is replied via: `srcript-message-to <src> dialog-save-reply <path>`.

#### Script Messages supported by `dyn_menu.lua`:

##### `menu-ready`

Broadcasted when `dyn_menu.lua` has initialized itself.

##### `get <keyword> <src>`

Get the menu item structure of `keyword`.

The result is replied via: `srcript-message-to <src> menu-get-reply <json>`.

```json
{
  "keyword": "chapters"
  "item": {
    "title": "Chapters",
    "type": "submenu",
    "submenu": []
  }
}
```

If `keyword` is not found, the result json will contain an additional `error` field, and no `item` field.

##### `update <keyword> <json>`

Update the menu item structure of `keyword` with `json`.

As a convenience, if you don't want to override menu title and type, omit the corresponding field in `json`.

## Credits

This project contains code copied from [mpv](https://github.com/mpv-player/mpv).

# License

[GPLv2](LICENSE.txt).
