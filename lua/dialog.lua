-- Copyright (c) 2023 tsl0922. All rights reserved.
-- SPDX-License-Identifier: GPL-2.0-only

local utils = require('mp.utils')

-- pre-defined file types
local file_types = {
    video = table.concat({ '*.mpeg4', '*.m4v', '*.mp4', '*.mp4v', '*.mpg4', '*.h264', '*.avc', '*.x264', '*.264',
        '*.hevc', '*.h265', '*.x265', '*.265', '*.m2ts', '*.m2t', '*.mts', '*.mtv', '*.ts', '*.tsv', '*.tsa', '*.tts',
        '*.mpeg', '*.mpg', '*.mpe', '*.mpeg2', '*.m1v', '*.m2v', '*.mp2v', '*.mkv', '*.mk3d', '*.wm', '*.wmv', '*.asf',
        '*.vob', '*.vro', '*.evob', '*.evo', '*.ogv', '*.ogm', '*.ogx', '*.webm', '*.avi', '*.vfw', '*.divx', '*.3iv',
        '*.xvid', '*.nut', '*.flic', '*.fli', '*.flc', '*.nsv', '*.gxf', '*.mxf', '*.dvr-ms', '*.dvr', '*.wtv', '*.dv',
        '*.hdv', '*.flv', '*.f4v', '*.qt', '*.mov', '*.hdmov', '*.rm', '*.rmvb', '*.3gpp', '*.3gp', '*.3gp2', '*.3g2',
        '*.yuv', '*.y4m' }, ';'),
    audio = table.concat({ '*.mp3', '*.m4a', '*.aac', '*.flac', '*.ac3', '*.a52', '*.eac3', '*.mpa', '*.m1a', '*.m2a',
        '*.mp1', '*.mp2', '*.oga', '*.ogg', '*.wav', '*.mlp', '*.dts', '*.dts-hd', '*.dtshd', '*.true-hd', '*.thd',
        '*.truehd', '*.thd+ac3', '*.tta', '*.pcm', '*.aiff', '*.aif', '*.aifc', '*.amr', '*.awb', '*.au', '*.snd',
        '*.lpcm', '*.ape', '*.wv', '*.shn', '*.adts', '*.adt', '*.opus', '*.spx', '*.mka', '*.weba', '*.wma', '*.f4a',
        '*.ra', '*.ram', '*.3ga', '*.3ga2', '*.ay', '*.gbs', '*.gym', '*.hes', '*.kss', '*.nsf', '*.nsfe', '*.sap',
        '*.spc', '*.vgm', '*.vgz', '*.m3u', '*.m3u8', '*.pls', '*.cue' }, ';'),
    image = table.concat({ '*.jpg', '*.bmp', '*.png', '*.gif', '*.webp' }, ';'),
    iso = table.concat({ '*.iso' }, ';'),
    subtitle = table.concat(
        { '*.srt', '*.ass', '*.idx', '*.sub', '*.sup', '*.ttxt', '*.txt', '*.ssa', '*.smi', '*.mks' }, ';'),
    playlist = table.concat({ '*.m3u', '*.m3u8', '*.pls', '*.cue' }, ';'),
}

-- set file type filters
local function update_default_filters()
    local dialog_filters = {
        { name = 'All Files (*.*)', spec = '*.*' },
        { name = 'Video Files',     spec = file_types['video'] },
        { name = 'Audio Files',     spec = file_types['audio'] },
        { name = 'Image Files',     spec = file_types['image'] },
        { name = 'ISO Image Files', spec = file_types['iso'] },
        { name = 'Subtitle Files',  spec = file_types['subtitle'] },
        { name = 'Playlist Files',  spec = file_types['playlist'] },
    }
    mp.set_property_native('user-data/menu/dialog/filters', dialog_filters)
end

-- open Bluray iso or dir
local function open_bluray(path)
    mp.commandv('set', 'bluray-device', path)
    mp.commandv('loadfile', 'bd://')
end

-- open DVD iso or dir
local function open_dvd(path)
    mp.commandv('set', 'dvd-device', path)
    mp.commandv('loadfile', 'dvd://')
end

-- open callback with support for Bluray/DVD iso and subtitle file
local function open_cb(...)
    local function check_file_type(ext, type)
        return ext ~= '' and file_types[type]:find(ext)
    end

    for i, v in ipairs({ ... }) do
        local path = tostring(v)
        local ext = path:match('^.+(%..+)$') or ''

        if check_file_type(ext, 'iso') then
            local info = utils.file_info(path)
            if info and info.is_file then
                if info.size > 4.7 * 1000 * 1000 * 1000 then
                    open_bluray(path)
                else
                    open_dvd(path)
                end
                break -- do not load other files
            end
        end

        if check_file_type(ext, 'subtitle') then
            mp.commandv('sub-add', path, 'auto')
        else
            mp.commandv('loadfile', path, i > 1 and 'append' or 'replace')
        end
    end
end

-- open folder callback with support for Bluray/DVD Folder
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
    mp.commandv('loadfile', clipboard, 'append-play')
    mp.osd_message('clipboard: ' .. clipboard)
end

-- message replies
mp.register_script_message('dialog-open-multi-reply', open_cb)
mp.register_script_message('dialog-open-folder-reply', open_folder_cb)
mp.register_script_message('clipboard-get-reply', clipboard_cb)


-- init default file types
update_default_filters()

-- open dialog
mp.register_script_message('open', function()
    mp.commandv('script-message-to', 'menu', 'dialog/open-multi', mp.get_script_name())
end)

-- open folder dialog
mp.register_script_message('open-folder', function()
    mp.commandv('script-message-to', 'menu', 'dialog/open-folder', mp.get_script_name())
end)

-- open clipboard
mp.register_script_message('open-clipboard', function()
    mp.commandv('script-message-to', 'menu', 'clipboard/get', mp.get_script_name())
end)
