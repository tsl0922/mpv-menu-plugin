-- Copyright (c) 2023 tsl0922. All rights reserved.
-- SPDX-License-Identifier: GPL-2.0-only
--
-- #@keyword support for dynamic menu
--
-- supported keywords:
--   #@tracks:   video/audio/sub tracks
--   #@tracks/video:         video track list
--   #@tracks/audio:         audio track list
--   #@tracks/sub:           subtitle list
--   #@tracks/sub-secondary: subtitle list (secondary)
--   #@chapters:             chapter list
--   #@editions:             edition list
--   #@audio-devices:        audio device list

local menu_prop = 'user-data/menu/items'
local menu_items = mp.get_property_native(menu_prop, {})

function build_track_title(track, prefix, filename)
    local title = track.title or ''
    local codec = track.codec or ''
    local type = track.type

    if track.external and title ~= '' then
        if filename ~= '' then title = title:gsub(filename .. '%.?', '') end
        if title:lower() == codec:lower() then title = '' end
    end
    if title == '' then
        local name = type:sub(1, 1):upper() .. type:sub(2, #type)
        title = string.format('%s %d', name, track.id)
    end

    local hints = {}
    local function h(value) hints[#hints + 1] = value end
    if codec ~= '' then h(codec) end
    if track['demux-h'] then
        h(track['demux-w'] and (track['demux-w'] .. 'x' .. track['demux-h'] or track['demux-h'] .. 'p'))
    end
    if track['demux-fps'] then h(string.format('%.5gfps', track['demux-fps'])) end
    if track['audio-channels'] then h(track['audio-channels'] .. 'ch') end
    if track['demux-samplerate'] then h(string.format('%.3gkHz', track['demux-samplerate'] / 1000)) end
    if track.forced then h('forced') end
    if track.default then h('default') end
    if track.external then h('external') end
    if #hints > 0 then title = string.format('%s [%s]', title, table.concat(hints, ', ')) end

    if track.lang then title = string.format('%s\t%s', title, track.lang:upper()) end
    if prefix then title = string.format('%s: %s', type:sub(1, 1):upper(), title) end
    return title
end

function build_track_items(list, type, prop, prefix)
    local items = {}

    local filename = mp.get_property('filename/no-ext', ''):gsub("[%(%)%.%%%+%-%*%?%[%]%^%$]", "%%%0")
    local pos = mp.get_property_number(prop, -1)
    for _, track in ipairs(list) do
        if track.type == type then
            local state = {}
            if track.selected then
                table.insert(state, 'checked')
                if track.id ~= pos then table.insert(state, 'disabled') end
            end

            items[#items + 1] = {
                title = build_track_title(track, prefix, filename),
                cmd = string.format('set %s %d', prop, track.id),
                state = state,
            }
        end
    end

    if #items > 0 then
        local title = pos > 0 and 'Off' or 'Auto'
        local value = pos > 0 and 'no' or 'auto'
        if prefix then title = string.format('%s: %s', type:sub(1, 1):upper(), title) end

        items[#items + 1] = {
            title = title,
            cmd = string.format('set %s %s', prop, value),
        }
    end

    return items
end

function update_tracks_menu(submenu)
    mp.observe_property('track-list', 'native', function(_, track_list)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end
        if not track_list then return end

        local items_v = build_track_items(track_list, 'video', 'vid', true)
        local items_a = build_track_items(track_list, 'audio', 'aid', true)
        local items_s = build_track_items(track_list, 'sub', 'sid', true)

        for _, item in ipairs(items_v) do table.insert(submenu, item) end
        if #submenu > 0 and #items_a > 0 then table.insert(submenu, { type = 'separator' }) end
        for _, item in ipairs(items_a) do table.insert(submenu, item) end
        if #submenu > 0 and #items_s > 0 then table.insert(submenu, { type = 'separator' }) end
        for _, item in ipairs(items_s) do table.insert(submenu, item) end

        mp.set_property_native(menu_prop, menu_items)
    end)
end

function update_track_menu(submenu, type, prop)
    mp.observe_property('track-list', 'native', function(_, track_list)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end
        if not track_list then return end

        local items = build_track_items(track_list, type, prop, false)
        for _, item in ipairs(items) do table.insert(submenu, item) end

        mp.set_property_native(menu_prop, menu_items)
    end)
end

function update_chapters_menu(submenu)
    mp.observe_property('chapter-list', 'native', function(_, chapter_list)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end
        if not chapter_list then return end

        local pos = mp.get_property_number('chapter', -1)
        for id, chapter in ipairs(chapter_list) do
            local title = chapter.title or ''
            if title == '' then title = 'Chapter ' .. id end
            local time = string.format('%02d:%02d:%02d', chapter.time / 3600, chapter.time / 60 % 60, chapter.time % 60)

            submenu[#submenu + 1] = {
                title = string.format('%s\t[%s]', title, time),
                cmd = string.format('seek %f absolute', chapter.time),
                state = id == pos + 1 and { 'checked' } or {},
            }
        end

        mp.set_property_native(menu_prop, menu_items)
    end)

    mp.observe_property('chapter', 'number', function(_, pos)
        if not pos then return end
        for id, item in ipairs(submenu) do
            item.state = id == pos + 1 and { 'checked' } or {}
        end
        mp.set_property_native(menu_prop, menu_items)
    end)
end

function update_editions_menu(submenu)
    mp.observe_property('edition-list', 'native', function(_, editions)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end
        if not editions then return end

        local current = mp.get_property_number('current-edition', -1)
        for id, edition in ipairs(editions) do
            local title = edition.title or ''
            if title == '' then title = 'Edition ' .. id end
            if edition.default then title = title .. ' [default]' end
            submenu[#submenu + 1] = {
                title = title,
                cmd = string.format('set edition %d', id - 1),
                state = id == current + 1 and { 'checked' } or {},
            }
        end

        mp.set_property_native(menu_prop, menu_items)
    end)

    mp.observe_property('current-edition', 'number', function(_, pos)
        if not pos then return end
        for id, item in ipairs(submenu) do
            item.state = id == pos + 1 and { 'checked' } or {}
        end
        mp.set_property_native(menu_prop, menu_items)
    end)
end

function update_audio_devices_menu(submenu)
    mp.observe_property('audio-device-list', 'native', function(_, devices)
        for i = #submenu, 1, -1 do table.remove(submenu, i) end
        if not devices then return end

        local current = mp.get_property('audio-device', '')
        for _, device in ipairs(devices) do
            submenu[#submenu + 1] = {
                title = device.description or device.name,
                cmd = string.format('set audio-device %s', device.name),
                state = device.name == current and { 'checked' } or {},
            }
        end

        mp.set_property_native(menu_prop, menu_items)
    end)

    mp.observe_property('audio-device', 'string', function(_, name)
        if not name then return end
        for _, item in ipairs(submenu) do
            item.state = item.cmd:match('%s*set audio%-device%s+(%S+)%s*$') == name and { 'checked' } or {}
        end
        mp.set_property_native(menu_prop, menu_items)
    end)
end

function update_dyn_menu(submenu, keyword)
    if keyword == 'tracks' then
        update_tracks_menu(submenu)
    elseif keyword == 'tracks/video' then
        update_track_menu(submenu, "video", "vid")
    elseif keyword == 'tracks/audio' then
        update_track_menu(submenu, "audio", "aid")
    elseif keyword == 'tracks/sub' then
        update_track_menu(submenu, "sub", "sid")
    elseif keyword == 'tracks/sub-secondary' then
        update_track_menu(submenu, "sub", "secondary-sid")
    elseif keyword == 'chapters' then
        update_chapters_menu(submenu)
    elseif keyword == 'editions' then
        update_editions_menu(submenu)
    elseif keyword == 'audio-devices' then
        update_audio_devices_menu(submenu)
    end
end

function check_keyword(items)
    if not items then return end
    for _, item in ipairs(items) do
        if item.type == 'submenu' then
            check_keyword(item.submenu)
        else
            if item.type ~= 'separator' and item.cmd then
                local keyword = item.cmd:match('%s*#@([%S]+).-%s*$') or ''
                if keyword ~= '' then
                    local submenu = {}

                    item.type = 'submenu'
                    item.submenu = submenu
                    item.cmd = nil

                    mp.set_property_native(menu_prop, menu_items)
                    update_dyn_menu(submenu, keyword)
                end
            end
        end
    end
end

function update_menu(name, items)
    if items and #items > 0 then
        mp.unobserve_property(update_menu)

        menu_items = items
        check_keyword(items)
    end
end

if #menu_items > 0 then
    check_keyword(menu_items)
else
    mp.observe_property(menu_prop, 'native', update_menu)
end
