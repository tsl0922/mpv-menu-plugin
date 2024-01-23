-- Copyright (c) 2023 tsl0922. All rights reserved.
-- SPDX-License-Identifier: GPL-2.0-only

local opts = require('mp.options')
local utils = require('mp.utils')

-- user options
local o = {
    video_exts = '*.mp4;*.m4v;*.mkv;*.h264;*.h265;*.m2ts;*.mpeg;*.wmv;*.webm;*.avi;*.flv;*.mov;*.rm;*.rmvb;*.3gp',
    audio_exts = '*.mp3;*.m4a;*.aac;*.flac;*.ac3;*.ogg;*.wav;*.dts;*.tta;*.amr;*.ape;*.wv;*.mka;*.weba;*.wma;*.f4a',
    image_exts = '*.jpg;*.jpeg;*.bmp;*.png;*.apng;*.gif;*.tiff;*.webp',
    subtitle_exts = '*.srt;*.ass;*.idx;*.sub;*.sup;*.txt;*.ssa;*.smi;*.mks',
    playlist_exts = '*.m3u;*.m3u8;*.pls;*.cue',
}
opts.read_options(o)

local open_action = ''

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
mp.register_script_message('clipboard-get-reply', clipboard_cb)

-- open dialog
mp.register_script_message('open', function(action)
    local function append_raw(filters, name, spec)
        filters[#filters + 1] = { name = name, spec = spec }
    end
    local function append(filters, name, type)
        append_raw(filters, name, o[type])
    end

    open_action = action
    local filters = {}

    if action == 'add-sub' then
        append(filters, 'Subtitle Files', 'subtitle_exts')
    elseif action == 'add-video' then
        append(filters, 'Video Files', 'video_exts')
    elseif action == 'add-audio' then
        append(filters, 'Audio Files', 'audio_exts')
    elseif action == 'bd-iso' or action == 'dvd-iso' then
        append_raw(filters, 'ISO Files', '*.iso')
    else
        append(filters, 'Video Files', 'video_exts')
        append(filters, 'Audio Files', 'audio_exts')
        append(filters, 'Image Files', 'image_exts')
        append(filters, 'Playlist Files', 'playlist_exts')
    end

    if action ~= 'bd-iso' and action ~= 'dvd-iso' then
        append_raw(filters, 'All Files', '*.*')
    end

    mp.set_property_native('user-data/menu/dialog/filters', filters)
    mp.commandv('script-message-to', 'menu', 'dialog/open-multi', mp.get_script_name())
end)

-- open folder dialog
mp.register_script_message('open-folder', function()
    mp.commandv('script-message-to', 'menu', 'dialog/open-folder', mp.get_script_name())
end)

-- open clipboard
mp.register_script_message('open-clipboard', function(action)
    open_action = action
    mp.commandv('script-message-to', 'menu', 'clipboard/get', mp.get_script_name())
end)

-- set clipboard
mp.register_script_message('set-clipboard', function(text)
    if not text then return end
    local value = text:gsub('\xFD.-\xFE', '')
    mp.commandv('script-message-to', 'menu', 'clipboard/set', value)
end)
