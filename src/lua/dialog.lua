-- Copyright (c) 2023-2024 tsl0922. All rights reserved.
-- SPDX-License-Identifier: GPL-2.0-only

local opts = require('mp.options')
local utils = require('mp.utils')
local msg = require('mp.msg')

-- user options
local o = {
    video_exts = '*.mp4;*.m4v;*.mkv;*.h264;*.h265;*.m2ts;*.mpeg;*.wmv;*.webm;*.avi;*.flv;*.mov;*.rm;*.rmvb;*.3gp',
    audio_exts = '*.mp3;*.m4a;*.aac;*.flac;*.ac3;*.ogg;*.wav;*.dts;*.tta;*.amr;*.ape;*.wv;*.mka;*.weba;*.wma;*.f4a',
    image_exts = '*.jpg;*.jpeg;*.bmp;*.png;*.apng;*.gif;*.tiff;*.webp',
    subtitle_exts = '*.srt;*.ass;*.idx;*.sub;*.sup;*.txt;*.ssa;*.smi;*.mks',
    playlist_exts = '*.m3u;*.m3u8;*.pls;*.cue',
}
opts.read_options(o)

local menu_native = 'menu'
local open_action = ''
local save_action = ''
local save_arg1 = nil

-- show error message on screen and log
local function show_error(message)
    msg.error(message)
    mp.osd_message('error: ' .. message)
end

-- mp.commandv with error check
local function mp_commandv(...)
    local args = { ... } -- remove trailing nil
    local res, err = mp.commandv(unpack(args))
    if not res then
        local cmd = table.concat(args, ' ')
        show_error(cmd .. ': ' .. err)
    end
end

-- open bluray iso or dir
local function open_bluray(path)
    mp.commandv('set', 'bluray-device', path)
    mp.commandv('loadfile', 'bd://')
end

-- open dvd iso or dir
local function open_dvd(path)
    mp.commandv('set', 'dvd-device', path)
    mp.commandv('loadfile', 'dvd://')
end

-- check if path is url
local function check_url(path)
    return type(path) == 'string' and (path:find("^%a[%w.+-]-://") or path:find("^%a[%w.+-]-:%?"))
end

-- write m3u8 playlist
local function write_playlist(path)
    local playlist = mp.get_property_native('playlist')

    local file, err = io.open(path, 'w')
    if not file then
        show_error('write playlist failed: ' .. err)
        return
    end
    file:write('#EXTM3U\n')
    local pwd = mp.get_property("working-directory")
    for _, item in ipairs(playlist) do
        local fullpath = item.filename
        if not check_url(fullpath) then
            fullpath = utils.join_path(pwd, fullpath)
        end
        if item.title and item.title ~= '' then
            file:write('#EXTINF:-1, ', item.title, '\n')
        end
        file:write(fullpath, '\n')
    end
    file:close()
end

-- open a single file
local function open_file(i, path, action)
    if action == 'add-sub' then
        mp.commandv('sub-add', path, 'auto')
    elseif action == 'add-video' then
        mp.commandv('video-add', path, 'auto')
    elseif action == 'add-audio' then
        mp.commandv('audio-add', path, 'auto')
    elseif action == 'bd-iso' then
        open_bluray(path)
    elseif action == 'dvd-iso' then
        open_dvd(path)
    elseif action == 'append' then
        mp.commandv('loadfile', path, 'append')
    else
        mp.commandv('loadfile', path, i > 1 and 'append-play' or 'replace')
    end
end

-- open callback
local function open_cb(...)
    for i, v in ipairs({ ... }) do
        local path = tostring(v)
        open_file(i, path, open_action)
    end
end

-- open folder callback
local function open_folder_cb(path)
    if utils.file_info(utils.join_path(path, 'BDMV')) then
        open_bluray(path)
    elseif utils.file_info(utils.join_path(path, 'VIDEO_TS')) then
        open_dvd(path)
    else
        mp.commandv('loadfile', path)
    end
end

-- save callback
local function save_cb(path)
    if save_action == 'screenshot' then
        mp_commandv('screenshot-to-file', path, save_arg1)
    elseif save_action == 'playlist' then
        write_playlist(path)
    end
end

-- clipboard callback
local function clipboard_cb(clipboard)
    mp.osd_message('clipboard: ' .. clipboard)
    local i = 1
    for line in string.gmatch(clipboard, '[^\r\n]+') do
        open_file(i, line, open_action)
        i = i + 1
    end
end

-- handle message replies
mp.register_script_message('dialog-open-multi-reply', open_cb)
mp.register_script_message('dialog-open-folder-reply', open_folder_cb)
mp.register_script_message('dialog-save-reply', save_cb)
mp.register_script_message('clipboard-get-reply', clipboard_cb)

-- detect dll client name
mp.register_script_message('menu-init', function(name) menu_native = name end)

-- open dialog
mp.register_script_message('open', function(action)
    local function append_raw(filters, name, spec)
        filters[#filters + 1] = { name = name, spec = spec }
    end
    local function append(filters, name, type)
        append_raw(filters, name, o[type])
    end

    open_action = action or ''
    local filters = {}

    if open_action == '' or open_action == 'append' then
        append(filters, 'Video Files', 'video_exts')
        append(filters, 'Audio Files', 'audio_exts')
        append(filters, 'Image Files', 'image_exts')
        append(filters, 'Playlist Files', 'playlist_exts')
    elseif open_action == 'add-sub' then
        append(filters, 'Subtitle Files', 'subtitle_exts')
    elseif open_action == 'add-video' then
        append(filters, 'Video Files', 'video_exts')
    elseif open_action == 'add-audio' then
        append(filters, 'Audio Files', 'audio_exts')
    elseif open_action == 'bd-iso' or open_action == 'dvd-iso' then
        append_raw(filters, 'ISO Files', '*.iso')
    else
        mp.osd_message('unknown open action: ' .. open_action)
        return
    end

    if open_action ~= 'bd-iso' and open_action ~= 'dvd-iso' then
        append_raw(filters, 'All Files', '*.*')
    end

    mp.set_property_native('user-data/menu/dialog/filters', filters)
    mp.commandv('script-message-to', menu_native, 'dialog/open-multi', mp.get_script_name())
end)

-- open folder dialog
mp.register_script_message('open-folder', function()
    mp.commandv('script-message-to', menu_native, 'dialog/open-folder', mp.get_script_name())
end)

-- save dialog
mp.register_script_message('save', function(action, arg1)
    save_action = action or ''
    save_arg1 = arg1
    if save_action == 'screenshot' then
        if not mp.get_property_number('vid') then
            mp.osd_message('no video track')
            return
        end

        mp.set_property_native('user-data/menu/dialog/filters', {
            { name = 'JPEG Image', spec = '*.jpg' },
            { name = 'PNG Image',  spec = '*.png' },
            { name = 'WebP Image', spec = '*.webp' },
        })
        local filename = mp.get_property('filename/no-ext') or ('screenshot-' .. os.time())
        mp.set_property('user-data/menu/dialog/default-name', filename)
    elseif save_action == 'playlist' then
        if mp.get_property_number('playlist-count', 0) == 0 then
            mp.osd_message('playlist is empty')
            return
        end

        mp.set_property_native('user-data/menu/dialog/filters', {
            { name = 'M3U8 Playlist', spec = '*.m3u8' },
        })
        mp.set_property('user-data/menu/dialog/default-name', 'playlist-' .. os.time())
    else
        mp.osd_message('unknown save action: ' .. save_action)
        return
    end
    mp.commandv('script-message-to', menu_native, 'dialog/save', mp.get_script_name())
end)

-- open clipboard
mp.register_script_message('open-clipboard', function(action)
    open_action = action
    mp.commandv('script-message-to', menu_native, 'clipboard/get', mp.get_script_name())
end)

-- set clipboard
mp.register_script_message('set-clipboard', function(text)
    if not text then return end
    local value = text:gsub('\xFD.-\xFE', '')
    mp.commandv('script-message-to', menu_native, 'clipboard/set', value)
end)
