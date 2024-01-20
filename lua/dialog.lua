-- Copyright (c) 2023 tsl0922. All rights reserved.
-- SPDX-License-Identifier: GPL-2.0-only

local utils = require('mp.utils')

-- common file extensions
local file_types = {
    video = '*.mp4;*.m4v;*.mkv;*.h264;*.h265;*.m2ts;*.ts;*.mpeg;*.wmv;*.webm;*.avi;*.flv;*.f4v;*.mov;*.rm;*.rmvb;*.3gp',
    audio = '*.mp3;*.m4a;*.aac;*.flac;*.ac3;*.ogg;*.wav;*.dts;*.tta;*.amr;*.ape;*.wv;*.mka;*.weba;*.wma;*.f4a',
    image = '*.jpg;*.jpeg;*.bmp;*.png;*.apng;*.gif;*.tiff;*.webp',
    subtitle = '*.srt;*.ass;*.idx;*.sub;*.sup;*.txt;*.ssa;*.smi;*.mks',
    playlist = '*.m3u;*.m3u8;*.pls;*.cue',
    iso = '*.iso',
}
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
    for line in string.gmatch(clipboard, '[^\n]+') do
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
    local function append(filters, name, type)
        filters[#filters + 1] = { name = name, spec = file_types[type] }
    end

    open_action = action
    local filters = {}

    if action == 'add-sub' then
        append(filters, 'Subtitle Files', 'subtitle')
    elseif action == 'add-video' then
        append(filters, 'Video Files', 'video')
    elseif action == 'add-audio' then
        append(filters, 'Audio Files', 'audio')
    elseif action == 'bd-iso' or action == 'dvd-iso' then
        append(filters, 'ISO Files', 'iso')
    else
        append(filters, 'Video Files', 'video')
        append(filters, 'Audio Files', 'audio')
        append(filters, 'Image Files', 'image')
        append(filters, 'Subtitle Files', 'subtitle')
    end

    if action ~= 'bd-iso' and action ~= 'dvd-iso' then
        filters[#filters + 1] = { name = 'All Files', spec = '*.*' }
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

--set clipboard
mp.register_script_message('set-clipboard', function(text)
    if not text then return end
    local res, err = mp.command_native({ 'expand-text', text })
    local value = res and res or 'error: ' .. err
    mp.commandv('script-message-to', 'menu', 'clipboard/set', value)
end)
